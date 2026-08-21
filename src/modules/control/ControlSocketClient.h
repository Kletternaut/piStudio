// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// ControlSocketClient.h - Client for rpicam-apps runtime control socket (rpicam-rt)
//
// Connects to /tmp/rpicam-vid{N}.sock to send live parameter updates
// without restarting the rpicam process. Falls back gracefully when
// the socket is not available (mainline rpicam-apps).

#ifndef CONTROLSOCKETCLIENT_H
#define CONTROLSOCKETCLIENT_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QTimer>
#include <QMap>

class QSocketNotifier;

class ControlSocketClient : public QObject
{
    Q_OBJECT

public:
    explicit ControlSocketClient(int cameraIndex, QObject *parent = nullptr);
    ~ControlSocketClient() override;

    // Connection state
    bool isConnected() const { return m_connected; }
    bool capsReceived() const { return m_capsReceived; }
    int maxFps() const { return m_maxFps; }
    bool hasAutoFocus() const { return m_hasAf; }

    // Connect/disconnect to/from the Unix domain socket
    void connectToServer();
    void disconnectFromServer();

    // Send a single key:value command (queued if not connected)
    void sendCommand(const QString &key, const QString &value);

    // Send multiple commands at once (one key:value per line)
    void sendCommands(const QStringList &commands);

    // Get the socket path for diagnostics
    QString socketPath() const;

signals:
    void connected();
    void disconnected();
    void capsUpdated(int maxFps, bool hasAf);
    void errorOccurred(const QString &message);

private slots:
    void onSocketReadable(int fd);
    void tryReconnect();

private:
    void parseLine(const QByteArray &line);
    bool writeToSocket(const QByteArray &data);
    void closeSocket();

    int m_cameraIndex;
    int m_socketFd = -1;
    QSocketNotifier *m_notifier = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QByteArray m_readBuffer;

    // Caps from server
    int m_maxFps = 30;
    bool m_hasAf = false;
    bool m_capsReceived = false;

    // Connection state
    bool m_connected = false;
    int m_reconnectAttempts = 0;
    static const int MAX_RECONNECT_ATTEMPTS = 10;
    static const int RECONNECT_INTERVAL_MS = 2000;
};

#endif // CONTROLSOCKETCLIENT_H
