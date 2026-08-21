#pragma once

#include <QObject>
#include <QWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QTextEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QList>
#include <QDateTime>
#include <functional>

#include "../odr/DetectionActionTypes.h"

class CollapsibleHelper;
class CheckableComboBox;

// CameraInterface — Abhängigkeiten von MainWindow die ActionsModule benötigt.
// Wird per setup() injiziert; alle Felder sind optional (nullptr = Feature deaktiviert).
struct CameraInterface {
    // true wenn rpicam-vid-Prozess gerade läuft
    std::function<bool()>    isVidRunning;
    // Prozess-PID des laufenden Kamera-Prozesses (0 = kein Prozess)
    std::function<qint64()>  getProcessId;
    // true wenn Signal-Recording (SIGUSR1 Split) aktiviert ist
    std::function<bool()>    isSignalRecordingEnabled;
    // Aktuell gewählte App (z.B. "rpicam-vid")
    std::function<QString()> getCurrentApp;
    // Preview-Fenster-Koordinaten (Format: "x,y,w,h") für Screenshot-Capture
    std::function<QString()> getBoxCoords;
    // Konfigurierter Output-Dateiname (z.B. "/path/to/video.mjpeg") für Telegram-Video-Suche
    std::function<QString()> getOutputFileName;
};

// ActionsModule — Kapselt den "Actions"-Tab mit Erkennnungs-Aktionen.
//
// Enthält:
//   - Object-Filter (CheckableComboBox)
//   - Detektions-Aktionen: Sound, Bild, Benachrichtigung, Skript, Aufnahme, Telegram
//   - Cooldown und Min-Confidence-Einstellungen
//   - executeDetection() Slot (verbunden mit OdrModule::detectionToExecute)
//
// Für kamerabezogene Operationen (Prozess-Signal, Frame-Capture, Aufnahmesteuerung)
// werden Signale emittiert die MainWindow empfängt.
class ActionsModule : public QObject
{
    Q_OBJECT
public:
    explicit ActionsModule(const QString &tabGroup, QObject *parent = nullptr);

    // Muss einmalig aufgerufen werden.
    // cameraIface: optionale Callbacks für Kamera-Zustand
    // onStateChanged: wird aufgerufen wenn sich Checkbox-Zustände ändern
    //                 (MainWindow aktualisiert seinen globalen Reset-Button)
    void setup(QList<CollapsibleHelper*> &helpers,
               std::function<void()> adjustWindowCallback,
               const CameraInterface &cameraIface,
               std::function<void()> onStateChanged = nullptr);

    QWidget *tab() const { return m_tab; }

    // Zugriff auf DetectionAction für MainWindowCmdHelpers / MainWindowHelpers
    const DetectionAction &detectionAction() const { return m_action; }

    // Kamera-Interface nachtraeglich setzen (nach setup() z.B. wenn Prozess erst spaeter bekannt)
    void setCameraInterface(const CameraInterface &ci) { m_camera = ci; }

    // Wird von MainWindow beim Start aufgerufen um Checkboxen zurückzusetzen
    // (verhindert versehentliche Aktionen beim Start)
    void resetActionCheckboxes();
    void resetFilterSettings(int cooldown, int confidence);

    // Settings
    void saveSettings(QSettings &settings);
    void loadSettings(QSettings &settings);

signals:
    // Kamera-kritische Signale — werden in MainWindow verarbeitet
    void startTimedRecordingRequested(int seconds);
    void captureFrameRequested(const QString &filename);

    // Auto-Konfiguration bei "Telegram Video" aktivieren
    void setCodecMjpegRequested();
    void enableSignalRecordingRequested();
    void enableSplitFilesRequested();
    void enableAutoNamingRequested();
    void enableSegmentPatternRequested();
    void setOutputModeFileRequested();

    // UI-Zustand hat sich geändert (für globalen Reset-Button in MainWindow)
    void actionsStateChanged();

public slots:
    // Primärer Slot — verbinden mit OdrModule::detectionToExecute
    void executeDetection(const QString &object, int confidence, const QString &fullDetection);

private slots:
    void browseActionSoundFile();
    void browseActionImageFolder();
    void browseActionScriptPath();
    void addCustomObject();
    void updateConfig();

    void updateActionsResetButtonColor();
    void updateFilterResetButtonColor();
    void updateFilterSettingsResetButtonColor();

private:
    void captureCurrentFrame(const QString &filename);
    void sendTelegramMessage(const QString &message);
    void sendTelegramPhoto(const QString &filePath, const QString &caption);
    void sendTelegramVideo(const QString &filePath, const QString &caption);

    QString m_tabGroup;
    QWidget *m_tab = nullptr;
    bool m_isInitializing = true;

    CameraInterface m_camera;
    std::function<void()> m_onStateChanged;

    DetectionAction m_action;
    QDateTime m_lastActionTime;   // Cooldown-Tracking

    // Netzwerk
    QNetworkAccessManager *m_networkManager = nullptr;

    // ---- Object Filter ----
    CheckableComboBox *m_filterComboBox     = nullptr;
    QLineEdit         *m_customObjectInput  = nullptr;
    QPushButton       *m_addCustomObjButton = nullptr;
    QPushButton       *m_filterResetButton  = nullptr;

    // ---- Detection Actions ----
    QPushButton *m_actionsResetButton = nullptr;

    QCheckBox   *m_playSoundCheckbox    = nullptr;
    QLineEdit   *m_soundFileInput       = nullptr;

    QCheckBox   *m_saveImageCheckbox    = nullptr;
    QLineEdit   *m_imageFolderInput     = nullptr;

    QCheckBox   *m_showNotificationCheckbox = nullptr;

    QCheckBox   *m_runScriptCheckbox    = nullptr;
    QLineEdit   *m_scriptPathInput      = nullptr;

    QCheckBox   *m_startRecordingCheckbox   = nullptr;
    QLineEdit   *m_recordingDurationInput   = nullptr;

    QCheckBox   *m_sendTelegramCheckbox        = nullptr;
    QLineEdit   *m_telegramBotTokenInput       = nullptr;
    QLineEdit   *m_telegramChatIdInput         = nullptr;
    QCheckBox   *m_sendTelegramImageCheckbox   = nullptr;
    QCheckBox   *m_sendTelegramVideoCheckbox   = nullptr;

    // ---- Filter Settings ----
    QSlider     *m_cooldownSlider       = nullptr;
    QLineEdit   *m_cooldownInput        = nullptr;
    QSlider     *m_confidenceSlider     = nullptr;
    QLineEdit   *m_confidenceInput      = nullptr;
    QPushButton *m_filterSettingsResetButton = nullptr;
};
