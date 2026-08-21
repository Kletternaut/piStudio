// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// ControlSocketClient.cpp - Client for rpicam-apps runtime control socket (rpicam-rt)

#include "ControlSocketClient.h"

#include <QDebug>
#include <QSocketNotifier>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
ControlSocketClient::ControlSocketClient(int cameraIndex, QObject *parent)
    : QObject(parent)
    , m_cameraIndex(cameraIndex)
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &ControlSocketClient::tryReconnect);
}

ControlSocketClient::~ControlSocketClient()
{
    disconnectFromServer();
}

// ---------------------------------------------------------------------------
// socketPath
// ---------------------------------------------------------------------------
QString ControlSocketClient::socketPath() const
{
    // Matches the naming convention from PR #917:
    //   /tmp/rpicam-vid{N}.sock
    return QStringLiteral("/tmp/rpicam-vid%1.sock").arg(m_cameraIndex);
}

// ---------------------------------------------------------------------------
// connectToServer
// ---------------------------------------------------------------------------
void ControlSocketClient::connectToServer()
{
    if (m_connected || m_socketFd >= 0) {
        return; // Already connected or connection in progress
    }

    QString path = socketPath();

    // Create Unix socket
    m_socketFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_socketFd < 0) {
        emit errorOccurred(tr("Failed to create socket: %1").arg(strerror(errno)));
        return;
    }

    // Set non-blocking for connect
    int flags = ::fcntl(m_socketFd, F_GETFL, 0);
    ::fcntl(m_socketFd, F_SETFL, flags | O_NONBLOCK);

    // Setup sockaddr_un
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    // Qt6-style: use the byte-array conversion to avoid overflow warnings
    QByteArray pathBytes = path.toUtf8();
    if (static_cast<size_t>(pathBytes.size()) >= sizeof(addr.sun_path)) {
        emit errorOccurred(tr("Socket path too long: %1").arg(path));
        closeSocket();
        return;
    }
    memcpy(addr.sun_path, pathBytes.constData(), pathBytes.size() + 1);

    // Non-blocking connect
    int ret = ::connect(m_socketFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        // Socket file doesn't exist or other error — this is normal for mainline rpicam-apps
        qDebug() << "[ControlSocket] No socket at" << path << "-" << strerror(errno);
        closeSocket();
        return;
    }

    // Set up QSocketNotifier for reading (caps line will arrive immediately)
    m_notifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated,
            this, &ControlSocketClient::onSocketReadable);

    m_connected = true;
    m_reconnectAttempts = 0;
    qDebug() << "[ControlSocket] Connected to" << path;
    emit connected();
}

// ---------------------------------------------------------------------------
// disconnectFromServer
// ---------------------------------------------------------------------------
void ControlSocketClient::disconnectFromServer()
{
    m_reconnectTimer->stop();
    closeSocket();
    m_connected = false;
    m_capsReceived = false;
    m_readBuffer.clear();
}

// ---------------------------------------------------------------------------
// closeSocket (internal helper)
// ---------------------------------------------------------------------------
void ControlSocketClient::closeSocket()
{
    if (m_notifier) {
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (m_socketFd >= 0) {
        ::close(m_socketFd);
        m_socketFd = -1;
    }
}

// ---------------------------------------------------------------------------
// onSocketReadable
// ---------------------------------------------------------------------------
void ControlSocketClient::onSocketReadable(int /*fd*/)
{
    char buf[4096];
    ssize_t n = ::read(m_socketFd, buf, sizeof(buf) - 1);

    if (n <= 0) {
        if (n == 0 || errno != EAGAIN) {
            // Server closed connection
            qDebug() << "[ControlSocket] Server disconnected";
            bool wasConnected = m_connected;
            closeSocket();
            m_connected = false;
            m_capsReceived = false;
            if (wasConnected) {
                emit disconnected();
            }
            // Schedule reconnect
            if (m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                m_reconnectTimer->start();
            }
        }
        return;
    }

    buf[n] = '\0';
    m_readBuffer.append(buf, static_cast<int>(n));

    // Process complete lines
    int newlineIdx;
    while ((newlineIdx = m_readBuffer.indexOf('\n')) >= 0) {
        QByteArray line = m_readBuffer.left(newlineIdx).trimmed();
        m_readBuffer.remove(0, newlineIdx + 1);

        if (!line.isEmpty()) {
            parseLine(line);
        }
    }
}

// ---------------------------------------------------------------------------
// parseLine
// ---------------------------------------------------------------------------
void ControlSocketClient::parseLine(const QByteArray &line)
{
    // Expected formats:
    //   caps:maxfps=40,hasaf=0
    //   (future caps fields are ignored gracefully)

    if (line.startsWith("caps:")) {
        QByteArray capsData = line.mid(5); // after "caps:"
        QList<QByteArray> fields = capsData.split(',');

        for (const QByteArray &field : fields) {
            int eqIdx = field.indexOf('=');
            if (eqIdx < 0) continue;

            QByteArray key = field.left(eqIdx).trimmed();
            QByteArray val = field.mid(eqIdx + 1).trimmed();

            if (key == "maxfps") {
                bool ok;
                int fps = val.toInt(&ok);
                if (ok && fps > 0) {
                    m_maxFps = fps;
                }
            } else if (key == "hasaf") {
                m_hasAf = (val == "1");
            }
            // Unknown caps fields are ignored (defensive parsing)
        }

        m_capsReceived = true;
        qDebug() << "[ControlSocket] Caps received: maxfps=" << m_maxFps
                 << "hasaf=" << m_hasAf;
        emit capsUpdated(m_maxFps, m_hasAf);
    }
}

// ---------------------------------------------------------------------------
// sendCommand
// ---------------------------------------------------------------------------
void ControlSocketClient::sendCommand(const QString &key, const QString &value)
{
    if (!m_connected || m_socketFd < 0) {
        return;
    }

    QByteArray data = key.toUtf8() + ":" + value.toUtf8() + "\n";
    writeToSocket(data);
}

// ---------------------------------------------------------------------------
// sendCommands
// ---------------------------------------------------------------------------
void ControlSocketClient::sendCommands(const QStringList &commands)
{
    if (!m_connected || m_socketFd < 0 || commands.isEmpty()) {
        return;
    }

    QByteArray data;
    for (const QString &cmd : commands) {
        data.append(cmd.toUtf8());
        if (!cmd.endsWith('\n')) {
            data.append('\n');
        }
    }
    writeToSocket(data);
}

// ---------------------------------------------------------------------------
// writeToSocket (internal helper)
// ---------------------------------------------------------------------------
bool ControlSocketClient::writeToSocket(const QByteArray &data)
{
    if (m_socketFd < 0) return false;

    ssize_t written = ::write(m_socketFd, data.constData(), data.size());
    if (written < 0) {
        qDebug() << "[ControlSocket] Write error:" << strerror(errno);
        if (errno == EPIPE || errno == ECONNRESET) {
            // Server closed — clean up and try reconnect
            bool wasConnected = m_connected;
            closeSocket();
            m_connected = false;
            if (wasConnected) {
                emit disconnected();
            }
            if (m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                m_reconnectTimer->start();
            }
        }
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// tryReconnect
// ---------------------------------------------------------------------------
void ControlSocketClient::tryReconnect()
{
    if (m_connected) return;

    m_reconnectAttempts++;
    qDebug() << "[ControlSocket] Reconnect attempt" << m_reconnectAttempts
             << "of" << MAX_RECONNECT_ATTEMPTS;

    connectToServer();
}
