// V4L2Helpers.h - Small helpers for reading/writing V4L2 controls without spawning v4l2-ctl
// Provides simple cross-device helpers returning boolean success and optional error messages

#ifndef V4L2_HELPERS_H
#define V4L2_HELPERS_H

#include <QString>
#include <QStringList>
#include <QList>

namespace V4L2 {

    // Information about a detected AF-capable V4L2 subdevice
    struct AfDeviceInfo {
        QString devicePath;   // e.g. "/dev/v4l-subdev3"
        int cameraIndex;      // 0-based camera index, or -1 if unknown
        QString driverName;   // kernel driver name, e.g. "hocusfocus"
        int i2cBus;           // I2C bus number, or -1 if unknown
    };

    // Check whether a device supports a given control (VIDIOC_QUERYCTRL).
    // Returns false if the device cannot be opened or the control is not supported.
    bool hasControl(const QString &devicePath, unsigned int controlId);

    // Read a control value (VIDIOC_G_CTRL). controlId uses V4L2_CID_* macros
    // Returns true on success and sets "value". On error, returns false and optionally sets *err.
    bool getControlValue(const QString &devicePath, unsigned int controlId, int &value, QString *err = nullptr);

    // Set a control value (VIDIOC_S_CTRL). controlId uses V4L2_CID_* macros
    // Returns true on success and optionally sets *err on failure.
    bool setControlValue(const QString &devicePath, unsigned int controlId, int value, QString *err = nullptr);

    // Scan /dev/v4l-subdev0 through /dev/v4l-subdev15 for devices that
    // support the given control (e.g. V4L2_CID_FOCUS_ABSOLUTE).
    // Returns all matching devices with camera index and driver name
    // resolved via sysfs.
    QList<AfDeviceInfo> detectAfDevices(unsigned int controlId);
}

#endif // V4L2_HELPERS_H
