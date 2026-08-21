// V4L2EventWatcher.cpp
#include "V4L2EventWatcher.h"

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <QDebug>

V4L2EventWatcher::V4L2EventWatcher(QObject *parent) : QObject(parent) {}

V4L2EventWatcher::~V4L2EventWatcher() {
    stopWatching();
}

bool V4L2EventWatcher::isWatching() const {
    return m_running.load();
}

bool V4L2EventWatcher::startWatching(const QString &devicePath) {
    if (m_running.load()) {
        return false; // already running
    }
    m_device = devicePath;
    // Try O_RDWR first (required for subdevice event subscription)
    int fd = open(devicePath.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        // Fallback to O_RDONLY if O_RDWR fails
        fd = open(devicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    }
    if (fd < 0) {
        QString err = QString("Failed to open device %1: %2").arg(devicePath).arg(strerror(errno));
        m_lastError = err;
        emit errorOccurred(err);
        return false;
    }
    // Subscribe to control events
    // For subdevices, subscribe to specific control IDs instead of id=0 (all)
    struct v4l2_event_subscription sub;
    memset(&sub, 0, sizeof(sub));
    sub.type = V4L2_EVENT_CTRL;
    
    // Try subscribing to FOCUS_ABSOLUTE first
    sub.id = V4L2_CID_FOCUS_ABSOLUTE;
    bool focus_subscribed = (ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub) == 0);
    
    // Try subscribing to ZOOM_ABSOLUTE
    sub.id = V4L2_CID_ZOOM_ABSOLUTE;
    bool zoom_subscribed = (ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub) == 0);
    
    // If neither subscription worked, try fallback with id=0 (all controls)
    if (!focus_subscribed && !zoom_subscribed) {
        sub.id = 0; // 0 - subscribe to all controls
        if (ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub) < 0) {
            // Save the immediate error and close the fd
            QString initialErr = QString("VIDIOC_SUBSCRIBE_EVENT failed on %1: %2").arg(devicePath).arg(strerror(errno));
            m_lastError = initialErr;
            close(fd);
        // Try fallback: iterate /dev/video* nodes and attempt to subscribe there instead
        DIR *d = opendir("/dev");
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != nullptr) {
                if (strncmp(entry->d_name, "video", 5) != 0) continue; // only /dev/video*
                QString candidate = QString("/dev/%1").arg(entry->d_name);
                // Try O_RDWR first, fallback to O_RDONLY
                int vfd = open(candidate.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
                if (vfd < 0) {
                    vfd = open(candidate.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
                }
                if (vfd < 0) continue;
                if (ioctl(vfd, VIDIOC_SUBSCRIBE_EVENT, &sub) == 0) {
                    qDebug() << "V4L2EventWatcher: fallback subscribed to" << candidate;
                    // success on a candidate video device
                    m_fd = vfd;
                    m_device = candidate;
                    m_running.store(true);
                    m_thread = std::make_unique<std::thread>([this]() { this->runLoop(); });
                    closedir(d);
                    return true;
                }
                // Don't log every fallback failure to avoid spam
                close(vfd);
            }
            closedir(d);
            }
            // fallback didn't find a video node we can subscribe to
            // Only emit error once, not for every failed attempt
            emit errorOccurred(initialErr);
            return false;
        }
    }

    // At least one subscription succeeded (focus and/or zoom or id=0)
    qDebug() << "V4L2EventWatcher: successfully subscribed to" << devicePath 
             << "(focus:" << focus_subscribed << "zoom:" << zoom_subscribed << ")";
    m_fd = fd;
    m_running.store(true);
    m_thread = std::make_unique<std::thread>([this]() { this->runLoop(); });
    return true;
}

void V4L2EventWatcher::stopWatching() {
    if (!m_running.load()) return;
    m_running.store(false);
    if (m_thread && m_thread->joinable()) {
        m_thread->join();
    }
    if (m_fd >= 0) {
        // There's no explicit unsubscribe ioctl in older kernels; closing FD removes subscription
        close(m_fd);
        m_fd = -1;
    }
}

void V4L2EventWatcher::runLoop() {
    if (m_fd < 0) return;
    while (m_running.load()) {
        struct pollfd pfd;
        pfd.fd = m_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int rc = poll(&pfd, 1, 2000); // 2s timeout
        if (rc < 0) {
            if (errno == EINTR) continue;
            QString err = QString("poll failed: %1").arg(strerror(errno));
            m_lastError = err;
            emit errorOccurred(err);
            break;
        }
        if (rc == 0) continue; // timeout, continue loop

        if (pfd.revents & (POLLERR | POLLNVAL)) {
            emit errorOccurred(QString("poll returned error revents=%1").arg(pfd.revents));
            break;
        }

        if (pfd.revents & POLLIN) {
            struct v4l2_event ev;
            memset(&ev, 0, sizeof(ev));
            if (ioctl(m_fd, VIDIOC_DQEVENT, &ev) < 0) {
                QString err = QString("VIDIOC_DQEVENT failed: %1").arg(strerror(errno));
                m_lastError = err;
                emit errorOccurred(err);
                continue;
            }
            if (ev.type == V4L2_EVENT_CTRL) {
                unsigned int control = ev.u.ctrl.type;
                int value = ev.u.ctrl.value;
                // Emit signal (thread-safe, queued between threads)
                emit controlChanged(control, value);
            }
        }
    }
}