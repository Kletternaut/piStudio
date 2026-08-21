#pragma once

// ResourceBroker.h
// MCIM Phase 0, Schritt 0.3
// Verwaltet geteilte Ressourcen zwischen CameraInstanzen:
// Ports, Hailo8L-Exklusivzugriff, Output-Pfade, V4L2-Subdevices

#include <QObject>
#include <QString>
#include <QMap>
#include <QMutex>

class ResourceBroker : public QObject
{
    Q_OBJECT

public:
    enum class PortRole {
        GStreamerRTSP,   // Basis: 8554 → Cam0: 8554, Cam1: 8654
        StreamViewer,    // Basis: 8555 → Cam0: 8555, Cam1: 8655
        UdpInference,    // Basis: 8567 → Cam0: 8567, Cam1: 8667
        TcpOutput        // Basis: 8888 → Cam0: 8888, Cam1: 8988
    };

    explicit ResourceBroker(QObject *parent = nullptr);

    // Port-Allokation: basePort(role) + cameraIndex * m_portOffset
    int allocatePort(int cameraIndex, PortRole role) const;

    // Hailo8L Exklusivzugriff (nur eine Instanz gleichzeitig)
    // Gibt true zurück wenn Zugriff gewährt, false wenn bereits belegt
    bool tryAcquireHailo(int cameraIndex);
    void releaseHailo(int cameraIndex);
    int hailoOwner() const;  // -1 = frei

    // Output-Pfad mit Kamera-Namespace: basePath/cam0/, basePath/cam1/
    QString outputPath(int cameraIndex) const;
    void setOutputBasePath(const QString &basePath);

    // V4L2 Subdevice pro Kamera (/dev/v4l-subdev0, /dev/v4l-subdev1)
    QString v4l2SubDevice(int cameraIndex) const;
    void setV4l2SubDevice(int cameraIndex, const QString &device);

signals:
    // Wird emittiert wenn Hailo-Besitzer wechselt (-1 = frei)
    void hailoOwnerChanged(int newOwnerIndex);

private:
    static int basePort(PortRole role);

    static constexpr int PORT_OFFSET = 100;

    int         m_hailoOwner = -1;
    QString     m_outputBasePath;
    QMap<int, QString> m_v4l2Devices;  // cameraIndex → /dev/v4l-subdevN
    mutable QMutex m_mutex;
};