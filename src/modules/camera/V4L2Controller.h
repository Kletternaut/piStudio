// V4L2Controller.h – Hardware-Adapter für V4L2-Gerätezugriff (Focus/Zoom Polling)
// Kapselt den persistenten File-Deskriptor, den Poll-Timer und die Reconnect-Logik.
// Emittiert Signale mit den gelesenen Positionswerten – die UI-Aktualisierung erfolgt
// im Empfänger (MainWindow), nicht hier.

#pragma once

#include <QObject>
#include <QTimer>
#include <QString>

class V4L2Controller : public QObject {
    Q_OBJECT

public:
    explicit V4L2Controller(QObject *parent = nullptr);
    ~V4L2Controller() override;

    // Öffnet das V4L2-Gerät und startet das Polling.
    // Bei Fehler wird ein interner Reconnect-Timer gestartet (max. 10 Versuche).
    bool openDevice(const QString &path);

    // Schließt den File-Deskriptor und stoppt alle Timer.
    void closeDevice();

    bool isOpen() const;

signals:
    void focusPositionChanged(int rawValue);   // V4L2_CID_FOCUS_ABSOLUTE
    void zoomPositionChanged(int rawValue);    // V4L2_CID_ZOOM_ABSOLUTE
    void deviceOpened(const QString &path);
    void deviceOpenFailed(const QString &path);

private slots:
    void doPoll();
    void tryReconnect();

private:
    int     m_fd               = -1;
    QTimer *m_pollTimer        = nullptr;
    QTimer *m_reconnectTimer   = nullptr;
    int     m_reconnectAttempts = 0;
    QString m_pendingDevicePath;

    bool tryOpen(const QString &path);  // Öffnet ohne Nebeneffekte (für Reconnect)
};
