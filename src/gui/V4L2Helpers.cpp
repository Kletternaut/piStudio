// V4L2Helpers.cpp
#include "V4L2Helpers.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>
#include <linux/videodev2.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QRegularExpression>

namespace V4L2 {

// ---------------------------------------------------------------------------
// Internal helpers for reading sysfs
// ---------------------------------------------------------------------------

static QString readSysfsLine(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QString line = QString::fromLocal8Bit(f.readLine()).trimmed();
    f.close();
    return line;
}

static QString readSymlink(const QString &path) {
    return QFile::symLinkTarget(path);
}

// Extract the DT node address from an of_node symlink target.
// E.g. "../../../platform/fe804000.i2c/i2c-0/i2c@88000" → "i2c@88000"
static QString nodeAddressFromOfNode(const QString &symlinkTarget) {
    QRegularExpression re(R"(i2c@[0-9a-f]+)");
    QRegularExpressionMatch m = re.match(symlinkTarget);
    return m.hasMatch() ? m.captured() : QString();
}

// Build a map: I2C bus number → camera index (0-based), or -1 if not a CSI bus.
// RPi5 CSI-0 = i2c@88000, CSI-1 = i2c@80000. Future boards may have other addresses.
static QMap<int, int> buildI2cToCameraMap() {
    QMap<int, int> result;
    // Known RPi5 CSI I2C device-tree addresses
    QMap<QString, int> dtToCamera;
    dtToCamera["i2c@88000"] = 0;  // CSI-0 → Camera 0
    dtToCamera["i2c@80000"] = 1;  // CSI-1 → Camera 1

    QDir i2cDir("/sys/bus/i2c/devices");
    for (const QString &entry : i2cDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!entry.startsWith("i2c-")) continue;
        bool ok = false;
        int busNum = entry.mid(4).toInt(&ok);  // "i2c-10" → 10
        if (!ok) continue;

        QString ofNodePath = QDir(i2cDir.absoluteFilePath(entry)).filePath("of_node");
        QString target = readSymlink(ofNodePath);
        if (target.isEmpty()) continue;

        QString addr = nodeAddressFromOfNode(target);
        if (dtToCamera.contains(addr)) {
            result[busNum] = dtToCamera[addr];
        }
    }
    return result;
}

bool hasControl(const QString &devicePath, unsigned int controlId) {
    const QByteArray path = devicePath.toLocal8Bit();
    int fd = open(path.constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return false;
    struct v4l2_queryctrl qc;
    memset(&qc, 0, sizeof(qc));
    qc.id = controlId;
    bool ok = (ioctl(fd, VIDIOC_QUERYCTRL, &qc) == 0);
    close(fd);
    return ok;
}

bool getControlValue(const QString &devicePath, unsigned int controlId, int &value, QString *err) {
    const QByteArray path = devicePath.toLocal8Bit();
    int fd = open(path.constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        if (err) *err = QString("open failed: %1").arg(strerror(errno));
        return false;
    }
    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = controlId;
    if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
        if (err) *err = QString("VIDIOC_G_CTRL failed: %1").arg(strerror(errno));
        close(fd);
        return false;
    }
    value = ctrl.value;
    close(fd);
    return true;
}

bool setControlValue(const QString &devicePath, unsigned int controlId, int value, QString *err) {
    const QByteArray path = devicePath.toLocal8Bit();
    int fd = open(path.constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        if (err) *err = QString("open failed: %1").arg(strerror(errno));
        return false;
    }
    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = controlId;
    ctrl.value = value;
    if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
        if (err) *err = QString("VIDIOC_S_CTRL failed: %1").arg(strerror(errno));
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

QList<AfDeviceInfo> detectAfDevices(unsigned int controlId) {
    QList<AfDeviceInfo> result;
    QMap<int, int> i2cToCamera = buildI2cToCameraMap();

    for (int i = 0; i < 16; ++i) {
        QString dev = QString("/dev/v4l-subdev%1").arg(i);
        if (!hasControl(dev, controlId)) continue;

        // Read driver name from sysfs
        QString sysName = QString("v4l-subdev%1").arg(i);
        QString driverName = readSysfsLine(
            QString("/sys/class/video4linux/%1/device/name").arg(sysName));

        // Extract I2C bus number from device symlink
        // e.g. "../../../11-000d" → bus 11
        QString devLink = readSymlink(
            QString("/sys/class/video4linux/%1/device").arg(sysName));
        int i2cBus = -1;
        if (!devLink.isEmpty()) {
            QRegularExpression busRe(R"(\b(\d+)-\w+)");
            QRegularExpressionMatch m = busRe.match(devLink);
            if (m.hasMatch()) {
                i2cBus = m.captured(1).toInt();
            }
        }

        // Map I2C bus to camera index
        int camIdx = (i2cBus >= 0 && i2cToCamera.contains(i2cBus))
                        ? i2cToCamera[i2cBus] : -1;

        AfDeviceInfo info;
        info.devicePath  = dev;
        info.cameraIndex = camIdx;
        info.driverName  = driverName;
        info.i2cBus      = i2cBus;
        result.append(info);
    }

    const char *ctrlName = (controlId == V4L2_CID_FOCUS_ABSOLUTE) ? "focus" :
                           (controlId == V4L2_CID_ZOOM_ABSOLUTE)  ? "zoom" : "?";
    if (result.isEmpty()) {
        qDebug() << "V4L2: no" << ctrlName << "-capable subdevice found";
    } else {
        QStringList parts;
        for (const auto &inf : result) {
            parts.append(QString("%1 (cam=%2, drv=%3)")
                .arg(inf.devicePath).arg(inf.cameraIndex).arg(inf.driverName));
        }
        qDebug() << "V4L2: detected" << result.size() << ctrlName << "-capable device(s):" << parts.join(", ");
    }

    return result;
}

} // namespace V4L2
