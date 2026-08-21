#pragma once
// CameraInstanceManager.h
// MCIM Phase 1.7
// Verwaltet alle CameraInstance-Objekte und deren QTabWidget-Tabs.
// Minimalversion: erkennt Kameras via rpicam-vid --list-cameras,
// erzeugt eine CameraInstance pro Kamera.

#include <QObject>
#include <QTabWidget>
#include <QList>
#include <QString>
#include "CameraInstance.h"
#include "ResourceBroker.h"

class CameraInstanceManager : public QObject
{
    Q_OBJECT

public:
    explicit CameraInstanceManager(ResourceBroker *broker,
                                   QWidget *parent = nullptr);
    ~CameraInstanceManager() override;

    // Startet Kamera-Erkennung via rpicam-vid --list-cameras,
    // legt dann pro erkannter Kamera eine CameraInstance an und
    // zeigt sie als Sub-Tab im zurückgegebenen QTabWidget.
    // Muss einmalig nach dem Konstruktor aufgerufen werden.
    void initialize();

    // Gibt das externe QTabWidget zurück, das in MainWindow eingebettet wird.
    QTabWidget *tabWidget() const { return m_tabWidget; }

    // Gibt die CameraInstance für den angegebenen Index zurück.
    CameraInstance *instance(int cameraIndex) const;

    // Gibt alle CameraInstances zurück.
    QList<CameraInstance *> allInstances() const { return m_instances; }

    // Anzahl der erkannten Kameras.
    int cameraCount() const { return m_instances.size(); }

    // Aktive CameraInstance (selektierter Tab).
    CameraInstance *activeInstance() const;

signals:
    void cameraDetectionFinished(int count);
    void logMessage(int cameraIndex, const QString &message);

private slots:
    void onDetectionFinished(int exitCode, QProcess::ExitStatus status);

private:
    void parseListCamerasOutput(const QString &output);
    void createInstance(int cameraIndex);

    ResourceBroker  *m_broker;
    QTabWidget      *m_tabWidget  = nullptr;
    QList<CameraInstance *> m_instances;
    QProcess        *m_detectionProcess = nullptr;
    QString          m_detectionOutput;
};
