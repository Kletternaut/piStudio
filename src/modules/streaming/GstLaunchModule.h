#pragma once

#include <QObject>
#include <QWidget>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QProcess>
#include <QMap>
#include <QSettings>
#include <QList>
#include <functional>

class CollapsibleHelper;

// GstLaunchModule — Stream Viewer + Test Source Sender Tab-Modul.
//
// Kapselt den "GST"-Tab mit:
//   - Stream Viewer (mehrere UDP-Streams empfangen + darstellen)
//   - Test Source Sender (gst-launch-1.0 Test-Quellen per UDP senden)
//   - Recording-Optionen, EOS, Debug-Level
//
// Beinhaltet den Code aus MainWindowStreamViewer.cpp +
// den Setup-Code aus MainWindow::setupGstLaunchTab().
class GstLaunchModule : public QObject
{
    Q_OBJECT
public:
    // Per-Stream-Viewer Tab-Daten (frueheer in MainWindow.h)
    struct StreamViewerTab {
        QWidget     *widget           = nullptr;
        QLineEdit   *nameEdit         = nullptr;
        QSpinBox    *portSpinBox      = nullptr;
        QComboBox   *codecSelector    = nullptr;
        QSpinBox    *payloadSpinBox   = nullptr;
        QCheckBox   *syncCheckBox     = nullptr;
        QComboBox   *previewSizeSelector = nullptr;
        QPushButton *startButton      = nullptr;
        QProcess    *process          = nullptr;
    };

    // Per-Test-Source Tab-Daten (frueheer in MainWindow.h)
    struct TestSourceTab {
        QWidget     *widget       = nullptr;
        QComboBox   *sourceSelector = nullptr;
        QLineEdit   *hostEdit     = nullptr;
        QSpinBox    *portSpinBox  = nullptr;
        QPushButton *startButton  = nullptr;
        QProcess    *process      = nullptr;
    };

    explicit GstLaunchModule(const QString &tabGroup, QObject *parent = nullptr);
    ~GstLaunchModule() override;

    // Einmalig aufrufen um das Tab-Widget aufzubauen.
    void setup(QList<CollapsibleHelper *> &helpers,
               std::function<void()> adjustWindowCallback);

    // Gibt das fertig aufgebaute Tab-Widget zurueck.
    QWidget *tab() const { return m_tab; }

    // Settings persistieren (wird von savePipeline-Button intern aufgerufen,
    // kann aber auch von aussen aufgerufen werden).
    void saveSettings();
    void loadSettings();

signals:
    // Emitted when debug logging is active; connect to main log widget
    void debugLog(const QString &message);

private slots:
    void addStreamViewerTab();
    void startStreamViewer();
    void stopStreamViewer();
    void startStreamViewerForTab(int tabIndex);
    void stopStreamViewerForTab(int tabIndex);

    void addTestSourceTab();
    void startGstTest();
    void stopGstTest();
    void startGstTestForTab(int tabIndex);
    void stopGstTestForTab(int tabIndex);

    void browseRecordFile();

private:
    QString   m_tabGroup;
    QWidget  *m_tab = nullptr;

    // ---- Stream Viewer ----
    QTabWidget                  *m_streamViewerTabWidget   = nullptr;
    QMap<int, StreamViewerTab>   m_streamViewerTabs;
    int                          m_streamViewerTabCounter  = 0;

    // Legacy-Pointer auf den ersten Tab fuer Abwaertskompatibilitaet
    QSpinBox    *m_streamViewerPortSpinBox    = nullptr;
    QComboBox   *m_streamViewerCodecSelector  = nullptr;
    QSpinBox    *m_streamViewerPayloadSpinBox = nullptr;
    QCheckBox   *m_streamViewerSyncCheckBox   = nullptr;
    QPushButton *m_streamViewerStartButton    = nullptr;
    QProcess    *m_streamViewerProcess        = nullptr;

    // ---- Test Source ----
    QTabWidget                  *m_testSourceTabWidget     = nullptr;
    QMap<int, TestSourceTab>     m_testSourceTabs;
    int                          m_testSourceTabCounter    = 0;

    // Legacy-Pointer auf den ersten Tab fuer Abwaertskompatibilitaet
    QComboBox   *m_testSourceSelector = nullptr;
    QLineEdit   *m_testHostEdit       = nullptr;
    QSpinBox    *m_testPortSpinBox    = nullptr;
    QPushButton *m_testStartButton    = nullptr;
    QProcess    *m_testProcess        = nullptr;

    // ---- Recording & Options ----
    QCheckBox   *m_recordCheckBox      = nullptr;
    QLineEdit   *m_recordFileEdit      = nullptr;
    QPushButton *m_recordBrowseButton  = nullptr;
    QCheckBox   *m_eosCheckBox         = nullptr;
    QSpinBox    *m_debugLevelSpinBox   = nullptr;
    QCheckBox   *m_gstDebugLogCheckBox = nullptr;  // Log GST debug output to main log
    QPushButton *m_savePipelineButton  = nullptr;
};
