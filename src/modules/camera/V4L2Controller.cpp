// V4L2Controller.cpp – Implementierung des V4L2-Hardware-Adapters

#include "V4L2Controller.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>
#include <linux/videodev2.h>

#include <QDebug>

V4L2Controller::V4L2Controller(QObject *parent)
    : QObject(parent)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(200); // 5x pro Sekunde (200 ms)
    connect(m_pollTimer, &QTimer::timeout, this, &V4L2Controller::doPoll);
}

V4L2Controller::~V4L2Controller()
{
    closeDevice();
}

// ---------------------------------------------------------------------------
// Öffentliche API
// ---------------------------------------------------------------------------

bool V4L2Controller::openDevice(const QString &path)
{
    // Laufenden Reconnect-Versuch abbrechen
    if (m_reconnectTimer && m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }
    m_reconnectAttempts = 0;

    // Altes Gerät schließen
    closeDevice();

    if (!tryOpen(path)) {
        // Log the initial failure once; reconnect retries are silent
        qDebug() << "V4L2Controller: failed to open" << path << strerror(errno);
        m_pendingDevicePath = path;

        // Create reconnect timer (lazy)
        if (!m_reconnectTimer) {
            m_reconnectTimer = new QTimer(this);
            m_reconnectTimer->setInterval(2000);
            connect(m_reconnectTimer, &QTimer::timeout,
                    this, &V4L2Controller::tryReconnect);
        }
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start();
        }

        emit deviceOpenFailed(path);
        return false;
    }

    m_pollTimer->start();
    emit deviceOpened(path);
    return true;
}

void V4L2Controller::closeDevice()
{
    if (m_pollTimer) m_pollTimer->stop();
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool V4L2Controller::isOpen() const
{
    return m_fd >= 0;
}

// ---------------------------------------------------------------------------
// Private Hilfsmethoden
// ---------------------------------------------------------------------------

bool V4L2Controller::tryOpen(const QString &path)
{
    // O_RDWR bevorzugt (manche Geräte benötigen Schreibzugriff für VIDIOC_S_CTRL)
    int fd = ::open(path.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    }
    if (fd < 0) {
        return false;
    }
    m_fd = fd;
    qDebug() << "V4L2Controller: opened" << path;
    return true;
}

// ---------------------------------------------------------------------------
// Private Slots
// ---------------------------------------------------------------------------

void V4L2Controller::doPoll()
{
    if (m_fd < 0) return;

    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
    if (ioctl(m_fd, VIDIOC_G_CTRL, &ctrl) == 0) {
        emit focusPositionChanged(ctrl.value);
    }

    ctrl.id = V4L2_CID_ZOOM_ABSOLUTE;
    if (ioctl(m_fd, VIDIOC_G_CTRL, &ctrl) == 0) {
        emit zoomPositionChanged(ctrl.value);
    }
}

void V4L2Controller::tryReconnect()
{
    if (tryOpen(m_pendingDevicePath)) {
        if (m_reconnectTimer) m_reconnectTimer->stop();
        m_reconnectAttempts = 0;
        m_pollTimer->start();
        qDebug() << "V4L2Controller: reconnected to" << m_pendingDevicePath;
        emit deviceOpened(m_pendingDevicePath);
    } else {
        ++m_reconnectAttempts;
        if (m_reconnectAttempts > 10) {
            if (m_reconnectTimer) m_reconnectTimer->stop();
            qDebug() << "V4L2Controller: giving up reconnect to"
                     << m_pendingDevicePath << "after" << m_reconnectAttempts
                     << "attempts – device not available";
        }
    }
}
