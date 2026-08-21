#pragma once

#include <QObject>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>
#include <QList>
#include <functional>

class CollapsibleHelper;

// GStreamerModule — GStreamer Streaming-Tab-Modul.
//
// Kapselt den "GStreamer"-Tab mit Streaming-Ziel, Pipeline-Konfiguration
// und dem buildPipeline()-Mechanismus fuer rpicam-vid | gst-launch-1.0.
//
// resolutionSelector wird benoetigt um das Seitenverhaeltnis bei der
// automatischen Hoehen-/Breitenberechnung korrekt zu berechnen.
class GStreamerModule : public QObject
{
    Q_OBJECT
public:
    explicit GStreamerModule(const QString &tabGroup, QObject *parent = nullptr);

    // Muss einmalig nach Konstruktion aufgerufen werden.
    // resolutionSelector: Referenz auf den Kamera-Aufloeseungs-Selector
    // (fuer Aspect-Ratio-Berechnung bei Stream-Groesse).
    void setup(QList<CollapsibleHelper*> &helpers,
               std::function<void()> adjustWindowCallback,
               QComboBox *resolutionSelector);

    // Gibt das Tab-Widget zurueck (nach setup() gueltig).
    QWidget *tab() const { return m_tab; }

    // Generiert die GStreamer-Pipeline basierend auf den aktuellen UI-Einstellungen.
    // Wird von MainWindowCmdHelpers::startRpiCamApp() aufgerufen wenn GStreamer-Modus aktiv.
    QString buildPipeline() const;

    // Settings persistieren
    void saveSettings(QSettings &settings);
    void loadSettings(QSettings &settings);

    // resolutionSelector nachtraeglich setzen (fuer Aspect-Ratio-Berechnung)
    void setResolutionSelector(QComboBox *s) { m_resolutionSelector = s; }

private:
    QString     m_tabGroup;
    QWidget    *m_tab = nullptr;
    QComboBox  *m_resolutionSelector = nullptr;

    // Streaming Target
    QComboBox  *m_targetSelector = nullptr;
    QLineEdit  *m_hostEdit = nullptr;
    QSpinBox   *m_portSpinBox = nullptr;

    // Pipeline Configuration
    QComboBox  *m_formatSelector = nullptr;
    QComboBox  *m_parserSelector = nullptr;
    QComboBox  *m_payloadSelector = nullptr;
    QSpinBox   *m_configInterval = nullptr;
    QSpinBox   *m_payloadType = nullptr;
    QCheckBox  *m_syncCheckBox = nullptr;
    QLineEdit  *m_streamWidthInput = nullptr;
    QLineEdit  *m_streamHeightInput = nullptr;
};
