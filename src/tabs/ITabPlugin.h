#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QVariantMap>
#include <QWidget>

class CollapsibleHelper;
class TabRegistryService;

// ITabPlugin — Interface fuer alle Tab-Implementierungen.
//
// Jeder Tab erbt QObject und ITabPlugin und verwaltet seine Widgets,
// Settings und Tab-Identifikation selbst.
class ITabPlugin {
public:
    virtual ~ITabPlugin() = default;

    // Angezeigter Tab-Name (tr()-faehig in der Implementierung).
    virtual QString tabName() const = 0;

    // Position im Tab-Streifen (niedrigere Zahl = weiter links).
    virtual int tabPriority() const = 0;

    // QSettings-Key der steuert ob der Tab sichtbar ist.
    // Leerer String = Tab ist immer sichtbar.
    virtual QString settingKey() const = 0;

    // Wird von MainWindow aufgerufen um Widgets zu erstellen und den Tab
    // beim TabRegistryService anzumelden.
    virtual void initialize(const QString &tabGroup,
                            TabRegistryService *registry,
                            QList<CollapsibleHelper*> &helpers,
                            QWidget *parent) = 0;

    // Gibt das Tab-Widget zurueck (nach initialize() gueltig).
    virtual QWidget* tab() const = 0;

    // Settings-Persistenz.
    virtual void saveSettings(QSettings &settings) = 0;
    virtual void loadSettings(QSettings &settings) = 0;

    // Kamera-Events (optionale Hooks).
    virtual void onCameraStarted() {}
    virtual void onCameraStopped() {}
};
