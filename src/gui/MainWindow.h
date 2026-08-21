#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QWidget>
#include <QProcess>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QMap>
#include <QResizeEvent>
#include <QDebug>
#include <QSlider>
#include "SelectionOverlay.h"
#include "ROIOverlay.h"
#include "CustomLineEdit.h"
#include "CollapsibleGroupBox.h"
#include "CollapsibleHelper.h"
#include "ToggleSwitch.h"
#include "CheckableComboBox.h"
#include "LoresComboBox.h"
#include "RefreshableComboBox.h"
#include <QShowEvent>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QTime>
#include "../Version.h"
#include "../utils/AppPaths.h"
#include "../app/TabRegistryService.h"
#include "../app/TabVisibilityService.h"
#include "../modules/camera/V4L2Controller.h"
#include "../modules/control/ControlSocketClient.h"
#include "../tabs/ActionsTab.h"
#include "../tabs/GStreamerTab.h"
#include "../tabs/GstLaunchTab.h"
#include "../tabs/InferenceTab.h"
#include "../tabs/ToolsTab.h"
// V4L2EventWatcher removed - using simple polling instead

// FpsComboBox: QComboBox, dessen Popup ein vertikaler Slider ist
// (Ganzzahlen 0..max Sensor-Framerate, 0 = Auto).
class FpsComboBox : public QComboBox {
    Q_OBJECT

public:
    explicit FpsComboBox(QWidget *parent = nullptr) : QComboBox(parent) {}

    void setPopupSlider(QSlider *slider) { m_slider = slider; }

protected:
    void showPopup() override;

private:
    QSlider *m_slider = nullptr;
};

// FpsSliderPopup: vertikaler Slider, der sich nach jeder echten Interaktion
// schließt (auch nach Klick auf die Rille). QSlider::sliderReleased feuert
// nur beim Loslassen des Handles, daher eigenes Signal.
// Wichtig: Beim Öffnen des Popups (Qt::Popup) fängt das Fenster die Maus –
// das Loslassen des Öffnungsklicks wird als mouseReleaseEvent an den Slider
// geliefert. Deshalb wird nur nach einem vorherigen mousePressEvent auf dem
// Slider geschlossen.
class FpsSliderPopup : public QSlider {
    Q_OBJECT

public:
    explicit FpsSliderPopup(QWidget *parent = nullptr)
        : QSlider(Qt::Vertical, parent) {}

signals:
    void interactionFinished();

protected:
    void mousePressEvent(QMouseEvent *event) override {
        m_pressed = true;
        QSlider::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        bool wasPressed = m_pressed;
        m_pressed = false;
        QSlider::mouseReleaseEvent(event);
        if (wasPressed) {
            emit interactionFinished();
        }
    }

private:
    bool m_pressed = false;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(int cameraIndex = 0, QWidget *parent = nullptr);
    ~MainWindow() override;
    // MCIM: Kameraindex fest setzen – versteckt cameraSelector,
    // separate Config-Datei, kein Code dupliziert.
    void fixCameraIndex(int idx);
    void startRpiCamApp();
    void saveConfigurationToFile(const QString &filePath);
    void loadConfigurationFromFile(const QString &filePath);
    void saveStartupDefaults();
    void loadStartupDefaults();
    // Core snapshot helpers (shared by "Set Defaults" and the profile manager).
    // includePreviewBox=false: do not persist/apply the preview position
    // (used by "Set Defaults" — the position is window-relative and stale after
    // the window moved; profiles may still pin it).
    void saveWidgetValuesToGroup(const QString &group, bool includePreviewBox = true);
    void loadWidgetValuesFromGroup(const QString &group, bool includePreviewBox = true);
    void saveProfileSnapshot(const QString &profileName);
    void loadProfileValues(const QString &profileName);
    int cameraIndex() const { return m_fixedCameraIdx; }
    // Directory used by the File menu's Save/Load Config dialogs.
    // configFilePath can hold a directory loaded from a config file
    // (ConfigFilePath= key); otherwise fall back to the configured
    // rpicam configs path (Paths/GuiRpicamConfigPath).
    QString rpicamConfigFilePath() const { return configFilePath.isEmpty() ? rpicamConfigPath : configFilePath; }
    void resetStartupDefaults();
    void parseListCamerasOutput(const QString &output);
    void stopRpiCamApp();
    void positionAppAndPreview(QWidget* previewWindow);
    void adjustWindowToOptimalSize();
    void toggleAllGroups();
    void collapseGroupsOnly(bool collapse);  // MCIM: collapse ohne Resize-Trigger
    bool firstGroupCollapsed() const { return !allCollapsibleHelpers.isEmpty() && allCollapsibleHelpers.first()->isCollapsed(); }
    TabVisibilityService *tabVisibilityService;

    // MCIM dual-camera: link the sibling MainWindow to avoid preview overlap
    void setSiblingWindow(MainWindow *sibling) {
        m_sibling = sibling;
        // SYNC only makes sense with a second camera present
        if (m_syncIndicator) m_syncIndicator->setVisible(sibling != nullptr);
    }
    void setSharedLogWidget(QTextEdit *widget) { m_sharedLogWidget = widget; }
    void appendLog(const QString &html);
    bool isPreviewRunning() const { return process.state() == QProcess::Running; }
    // Like isPreviewRunning() but also true while the process is starting.
    // Needed for sync start: when the second camera starts, the first one's
    // QProcess is still in Starting state, but its preview position is
    // already fixed — sibling placement must treat it as running.
    bool isPreviewActive() const { return process.state() == QProcess::Running || process.state() == QProcess::Starting; }
    QString getBoxInputText() const { return BoxInput ? BoxInput->text() : QString(); }

public slots:
    void showHelp();
    void showAboutDialog(int tabIndex = 0);
    void openGuiSetupDialog();
    void openGlobalSetupDialog();
    void showSupportDialog();
    void showSystemInfo();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    // SOP-style: Mouse/Keyboard events for selection
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateParameterFields();
    void showDonationDialog();
    void updateBoxInputFromSelection(const QRect &selection);
    void calculateLoresWidth(int height);
    void calculateLoresHeight(int width);
    void loadGuiConfiguration();
    void loadCustomResolutions();
    void updateStepSizeRadioButtons();  // Aktualisiert Focus/Zoom Radio Button Tooltips
    void updateControlSocketVisibility();  // Show/hide CTRL overlay based on settings
    void onStartStopClicked();             // Start/Stop button click (handles sync mode)
    void setSyncStartStop(bool enabled);   // Toggle sync mode (mirrored to sibling)
    void updateSyncBadgeStyle();           // Refresh the SYNC badge color/tooltip
    void repositionStartButtonBadges();    // Position RT + SYNC badges on the button
    void setupFocusTab();
    void setupZoomTab();
    void onAudioToggled(bool checked);
    void resetAudioToDefaults();
    void updateAudioControlsState();
    void updateAudioResetButtonColor();
    void loadAudioSettings();
    void saveAudioSettings();

private:
    TabRegistryService *tabRegistryService;

    QList<CollapsibleHelper*> allCollapsibleHelpers;  // Liste aller CollapsibleHelper-Instanzen

    // SOP-style: Selection grabbing state
    bool m_grabbing = false;
    bool m_suppressResize = false;  // MCIM: verhindert Resize-Trigger bei inaktiven Tabs
    bool m_selecting_window = false;  // Window vs rectangle selection
    bool m_selection_started = false; // True after first mouse press
    QRect m_rubber_band_rect;
    QPoint m_selection_start;
    void StartGrabbing();
    void StopGrabbing();
    void UpdateRubberBand();
    QComboBox *appSelector;
    QComboBox *cameraSelector;
    QTextEdit *cameraInfo;
    QFormLayout *parameterLayout;
    QWidget *parameterWidget;
    QLineEdit *parameterInput;
    QTextEdit *outputLog;
    QPushButton *startButton;

    // Debug-Optionen Checkboxen
    QCheckBox *debugToFileCheckbox;
    QCheckBox *debugToLogWindowCheckbox;
    QCheckBox *standardLoggingCheckbox;
    QCheckBox *processOutputCheckbox;
    QMap<QString, QString> cameraDetails;
    QComboBox *resolutionSelector;
    FpsComboBox *framerateSelector;
    FpsSliderPopup *m_fpsSliderPopup; // Vertikaler Slider als Popup-View des fps-Dropdowns
    QComboBox *previewSelector;
    QString m_previewLibsPath;              // custom path for --preview-libs
    QString m_postProcessLibsPath;          // custom path for --post-process-libs
    QString m_encoderLibsPath;              // custom path for --encoder-libs
    QLineEdit *outputFileName;
    QPushButton *browseButton;
    QLineEdit *timeoutInput;
    QComboBox *timeoutSelector;
    QComboBox *timelapseInput;
    QCheckBox *segmentationCheckbox = nullptr;
    QProcess process;
    QPushButton *stopButton;
    QPushButton *startStopButton;
    RefreshableComboBox *postProcessFileSelector;
    QPushButton *postProcessFileBrowseButton;
    RefreshableComboBox *tuningFileSelector;
    QPushButton *tuningFileBrowseButton;
    QLineEdit *customPreviewInput;
    QComboBox *codecSelector;
    QLabel *codecLabel;
    QLabel *profileLabel = nullptr;
    QComboBox *profileSelector = nullptr;
    QLabel *levelLabel = nullptr;
    QComboBox *levelSelector = nullptr;
    QCheckBox *inlineHeadersCheckbox = nullptr;
    QLabel *libavFormatLabel = nullptr;
    QComboBox *libavFormatSelector = nullptr;
    QLabel *libavVideoCodecLabel = nullptr;
    QComboBox *libavVideoCodecSelector = nullptr;
    QLabel *libavCodecOptsLabel = nullptr;
    QComboBox *libavCodecOptsSelector = nullptr;
    QCheckBox *lowLatencyCheckbox = nullptr;
    QLabel *syncLabel = nullptr;
    QComboBox *syncSelector = nullptr;
    QPushButton *syncResetButton = nullptr;
    QSpinBox *bitrateSpinBox = nullptr;
    QSpinBox *qualitySpinBox = nullptr;
    QSpinBox *intraSpinBox = nullptr;
    QSpinBox *framesSpinBox = nullptr;
    QCheckBox *flushCheckbox = nullptr;
    QLineEdit *savePtsInput = nullptr;
    QComboBox *awbSelector;
    CustomLineEdit *BoxInput;
    QTextEdit *debugLog;
    QCheckBox *timestampCheckbox;
    QCheckBox *autoNamingCheckbox;
    QCheckBox *segmentPatternCheckbox;

    // Output Mode (File / GStreamer / TCP / UDP)
    QRadioButton *outputModeFile = nullptr;
    QRadioButton *outputModeGStreamer = nullptr;
    QRadioButton *outputModeTCP = nullptr;
    QRadioButton *outputModeUDP = nullptr;
    QButtonGroup *outputModeGroup = nullptr;  // Button group für exklusive Auswahl
    QWidget *streamingModesWidget = nullptr;  // Container für GStreamer/TCP/UDP

    // Still-Image Encoding (nur bei rpicam-still/jpeg)
    QComboBox *encodingSelector = nullptr;
    QWidget *encodingWidget = nullptr;  // Container für Encoding
    QPushButton *encodingResetButton = nullptr;

    SelectionOverlay *selectionOverlay = nullptr;
    QSlider *sharpnessSlider;
    QLineEdit *sharpnessInput;
    QSlider *evSlider;
    QLineEdit *evInput;
    QSlider *gainSlider;
    QLineEdit *gainInput;
    QSlider *awbGainRedSlider;
    QLineEdit *awbGainRedInput;
    QPushButton *awbGainRedResetButton;
    QSlider *awbGainBlueSlider;
    QLineEdit *awbGainBlueInput;
    QPushButton *awbGainBlueResetButton;
    QSlider *brightnessSlider;
    QLineEdit *brightnessInput;
    QSlider *contrastSlider;
    QLineEdit *contrastInput;
    QSlider *saturationSlider;
    QLineEdit *saturationInput;
    QSlider *shutterSlider;
    QLineEdit *shutterValueInput;
    QPushButton *shutterSliderResetButton;
    QLabel *shutterLabel;

    // Shutter display: set label unit + input value from µs
    void updateShutterDisplay(int us);
    // Shutter max depends on framerate (cannot exceed frame interval)
    int shutterMaxUs() const;
    void updateShutterMaxRange();
    // Parse input field value back to µs (interprets number in current label unit)
    int parseShutterInput() const;
    CheckableComboBox *geometryComboBox = nullptr;
    QPushButton *geometryResetButton = nullptr;

    // Info Text Checkboxes
    QCheckBox *infoTextFrameCheckbox = nullptr;
    QCheckBox *infoTextFpsCheckbox = nullptr;
    QCheckBox *infoTextExpCheckbox = nullptr;
    QCheckBox *infoTextAgCheckbox = nullptr;
    QCheckBox *infoTextDgCheckbox = nullptr;
    QCheckBox *infoTextRgCheckbox = nullptr;
    QCheckBox *infoTextBgCheckbox = nullptr;
    QCheckBox *infoTextFocusCheckbox = nullptr;
    QCheckBox *infoTextAelockCheckbox = nullptr;
    QCheckBox *infoTextLpCheckbox = nullptr;
    QCheckBox *infoTextAfstateCheckbox = nullptr;

    // Geometry Checkboxes
    QCheckBox *hflipCheckbox = nullptr;
    QCheckBox *vflipCheckbox = nullptr;
    QCheckBox *rotationCheckbox = nullptr;

    QComboBox *hdrSelector = nullptr;
    QPushButton *hdrResetButton = nullptr;
    QComboBox *denoiseSelector = nullptr;
    QPushButton *denoiseResetButton = nullptr;
    QComboBox *flickerPeriodSelector = nullptr;
    QPushButton *flickerPeriodResetButton = nullptr;

    // Metadata Parameters
    QLineEdit *metadataFileEdit = nullptr;
    QPushButton *metadataFileButton = nullptr;
    QComboBox *metadataFormatSelector = nullptr;
    QCheckBox *metadataAutoNamingCheckbox = nullptr;
    QPushButton *metadataResetButton = nullptr;
    QWidget *metadataWidget = nullptr;  // Container widget for metadata controls

    // Autofocus Parameters
    QComboBox *autofocusModeSelector = nullptr;
    QPushButton *resetAutofocusModeButton = nullptr;
    QComboBox *autofocusRangeSelector = nullptr;
    QPushButton *resetAutofocusRangeButton = nullptr;
    QComboBox *autofocusSpeedSelector = nullptr;
    QPushButton *resetAutofocusSpeedButton = nullptr;
    CustomLineEdit *autofocusWindowInput = nullptr;
    QPushButton *resetAutofocusWindowButton = nullptr;
    QSlider *lensPositionSlider = nullptr;
    QLineEdit *lensPositionInput = nullptr;
    QPushButton *resetLensPositionButton = nullptr;

    // Manual Focus Controls
    QLineEdit *v4l2DeviceInput = nullptr;
    QButtonGroup *focusStepButtonGroup = nullptr;
    int currentFocusStepSize = 300;  // Current selected step size
    QPushButton *focusNearButton = nullptr;
    QPushButton *focusFarButton = nullptr;
    QSlider *focusAbsoluteSlider = nullptr;
    QLineEdit *focusAbsoluteInput = nullptr;
    QPushButton *focusAbsoluteOkButton = nullptr;
    QPushButton *resetFocusAbsoluteButton = nullptr;
    QLabel *currentFocusPositionLabel = nullptr;
    QPushButton *refreshFocusPositionButton = nullptr;
    QPushButton *saveFocusFavoriteButton = nullptr;
    QListWidget *focusFavoritesList = nullptr;
    QPushButton *deleteFocusFavoriteButton = nullptr;
    QLineEdit *focusFavoriteNameInput = nullptr;

    // Focus Range Calibration
    ToggleSwitch *parfocalButton = nullptr;
    QPushButton *focusCalibrationButton = nullptr;

    // Manual Zoom Controls
    QButtonGroup *zoomStepButtonGroup = nullptr;
    int currentZoomStepSize = 300;
    QPushButton *zoomNearButton = nullptr;
    QPushButton *zoomFarButton = nullptr;
    QSlider *zoomAbsoluteSlider = nullptr;
    QLineEdit *zoomAbsoluteInput = nullptr;
    QPushButton *zoomAbsoluteOkButton = nullptr;
    QPushButton *resetZoomAbsoluteButton = nullptr;
    QLabel *currentZoomPositionLabel = nullptr;
    QPushButton *refreshZoomPositionButton = nullptr;

    // Coupled Favorites: Name -> {focus: int, zoom: int, hasFocus: bool, hasZoom: bool}
    QMap<QString, QVariantMap> coupledFavorites;
    void rebuildFavoritesLists();

    void updateCameraInfo(int index);
    // snapToMax: jump to the highest detected integer framerate instead of
    // keeping the current value (used on user resolution changes).
    void updateFramerateOptions(const QString &resolution, bool snapToMax = false);
    void applyFormatFilter(); // Filtert Auflösung + Framerate nach gewähltem Pixelformat
    void openSaveFileDialog();
    void updateTimelapseField();
    void updateButtonVisibility();
    void updateTabIndicator();
    void updateCodecVisibility(const QString &selectedApp);
    void setupLayout();
    void updateResetButtonColor(QPushButton *button, double currentValue, double defaultValue);
    QString calculateBoxInput(int additionalOffsetY = 0);
    QString getDefaultBoxInput(); // Returns custom or calculated preview geometry
    QString guiOutputFilePath;
    QString guiPostProcessFilePath;
    QString guiTuningFilePath;
    QString guiMetadataPath;
    QString configFilePath;
    void parseConfigurationFile(const QString &filePath);
    void initializeSelectionOverlay();
    void initializeROIOverlay();
    void initializeBoxInput();
    void updateSelectionOverlayGeometry();
    void setupLayouts();
    void setupInputLayout();
    void setupOutputLayout();
    void setupSliderLayout();
    void setupAdvancedOptionsLayout();
    void updatePostProcessFileDropdown();
    void updateTuningFileDropdown();
    void updateTimelapseVisibility();
    double getCurrentVideoAspectRatio() const; // Neue Methode für Seitenverhältnis

    // MCIM: fester Kameraindex und Tab-Gruppe für Mehrinstanz-Betrieb
    // m_tabGroup: QSettings-Sektionsname, z.B. "Camera0-Tab" / "Camera1-Tab"
    // Alle tab-spezifischen QSettings verwenden AppPaths::globalConf() + beginGroup(m_tabGroup)
    int     m_fixedCameraIdx = -1;
    QString m_tabGroup;          // set in constructor: AppPaths::tabGroup(cameraIndex)
    bool    m_showEventDone       = false;  // guards first-show-only code in showEvent
    MainWindow *m_sibling = nullptr;        // sibling camera window (for preview overlap avoidance)

    QTabWidget *tabWidget = nullptr;
    QWidget *generalTab = nullptr;
    QWidget *outputTab = nullptr;
    QVBoxLayout *outputTabLayout = nullptr;
    QWidget *stillTab = nullptr;
    QWidget *imageTab = nullptr;
    QWidget *autofocusTab = nullptr;
    QWidget *audioTab = nullptr;
    QWidget *gstreamerTab = nullptr;
    QVBoxLayout *gstreamerTabLayout = nullptr;
    QWidget *gstLaunchTab = nullptr;
    QWidget *inferenceTab = nullptr;
    QWidget *actionsTab = nullptr;
    QWidget *toolsTab = nullptr;
    QWidget *expertTab = nullptr;
    QWidget *zoomTab = nullptr;
    QWidget *debugTab = nullptr;
    QTextEdit *m_sharedLogWidget = nullptr;
    QVBoxLayout *mainLayout = nullptr;

    // Expert Tab Widgets
    QComboBox *viewfinderModeSelector = nullptr;
    QComboBox *formatSelector = nullptr; // Pixelformat-Filter (globale Zeile)
    QPushButton *viewfinderModeResetButton = nullptr;
    QSpinBox *viewfinderWidthSpinBox = nullptr;
    QPushButton *viewfinderWidthResetButton = nullptr;
    QSpinBox *viewfinderHeightSpinBox = nullptr;
    QPushButton *viewfinderHeightResetButton = nullptr;
    QSpinBox *bufferCountSpinBox = nullptr;
    QPushButton *bufferCountResetButton = nullptr;
    QSpinBox *viewfinderBufferCountSpinBox = nullptr;
    QPushButton *viewfinderBufferCountResetButton = nullptr;

    // GStreamer Widgets are now managed internally by GStreamerModule (P19)
    // GST-Launch Tab widgets are now managed internally by GstLaunchModule (P14/P16)
    QHBoxLayout *boxLayout = nullptr;
    QCheckBox *doubleSizeCheckbox; // Checkbox für die Verdopplung der Größe
    QString rpicamConfigPath; // Speichert den rpicam config file path
    QPushButton *resetPostProcessFileButton;
    QPushButton *resetTuningFileButton;

    // Runtime Timer für Button-Text (V2-Style)
    QTimer *runtimeTimer = nullptr;
    QTimer *parfocalPollTimer = nullptr;

    // Auto-refresh Timer für Focus Position (derzeit inaktiv – Polling übernimmt)
    QTimer *focusPositionRefreshTimer = nullptr;
    // Auto-refresh Timer für Zoom Position (derzeit inaktiv – Polling übernimmt)
    QTimer *zoomPositionRefreshTimer = nullptr;
    // V4L2 Hardware-Adapter (Polling + Geräteverwaltung)
    V4L2Controller *m_v4l2Controller = nullptr;

    // Control Socket (live parameter updates, PR #917)
    ControlSocketClient *m_controlSocket = nullptr;
    QLabel *m_controlSocketIndicator = nullptr;
    // Sync toggle: linked start/stop across both camera tabs
    QLabel *m_syncIndicator = nullptr;
    bool m_syncStartStop = false;
    bool m_hasRpicamRt = false;              // true if rpicam-apps --version shows rpicam_rt:1
    bool m_hasPreviewBackend = false;      // true if rpicam-apps >= 1.13 (supports --preview-backend)
    bool m_hasRoiSelection = false;        // true if rpicam-apps --version shows roi_selection
    QString m_rpicamAppsVersion;           // build version string from --version
    void checkRpicamRtCapability();         // runs rpicam-vid --version to detect rpicam-rt support
    void initControlSocket();
    void connectControlSocket();
    void sendSliderToSocket(const QString &key, const QString &value);
    bool isControlSocketActive() const;
    QTime startTime;
    QPushButton *resetCodecButton;
    QPushButton *resetAwbButton;
    QLineEdit *ccmInput = nullptr;
    QPushButton *ccmResetButton = nullptr;
    QPushButton *resetOutputFileButton = nullptr;
    QGroupBox *codecGroup = nullptr;
    QPushButton *codecResetButton = nullptr;
    QStringList customAppEntries; // Liste für benutzerdefinierte Apps
    QStringList customTimeoutEntries; // Liste für benutzerdefinierte Timeout-Werte
    QStringList customTimelapseEntries; // Liste für benutzerdefinierte Timelapse-Werte
    QStringList cameraResolutions; // Liste der von der Kamera erkannten Resolutions
    QMap<QString, QString> cameraModes; // Map: display -> mode string für viewfinder-mode
    QMap<QString, QStringList> m_resolutionFps; // Auflösung -> erkannte Sensor-Framerates (numerische Strings)
    QStringList m_cameraFormats; // Erkannte Pixelformate (z. B. "SRGGB10_CSI2P")
    QMap<QString, QMap<QString, QStringList>> m_formatData; // Format -> (Auflösung -> fps-Liste)
    void updateAppSelector();    // Methode, um das Dropdown zu aktualisieren
    void updateTimeoutSelector(); // Methode, um das Timeout-Dropdown zu aktualisieren
    void saveTimeoutEntries();   // Speichert Timeout-Einträge in Config
    void updateTimelapseSelector(); // Methode, um das Timelapse-Dropdown zu aktualisieren
    void saveTimelapseEntries();   // Speichert Timelapse-Einträge in Config
    void updateViewfinderModes(); // Update viewfinder mode dropdown nach Camera Detection
    QHBoxLayout *timelapseLayout = nullptr;
    QHBoxLayout *timeoutLayout = nullptr;
    void updateOutputFileNameForTimelapse();
    QWidget *timelapseRowWidget = nullptr;
    QLabel *timelapseLabel = nullptr;
    QPushButton *timelapseResetButton = nullptr;
    QPushButton *overlayResetButton = nullptr;
    QPushButton *globalResetButton = nullptr;
    bool isInitializing = true; // Flag to prevent global reset button updates during initialization
    void updateOverlayResetButtonColor(QPushButton *button);
    void updateGlobalResetButtonColor();
    void updateOutputFileResetButtonColor();
    void updateFilterResetButtonColor();
    void updateActionsResetButtonColor();
    void updateFilterSettingsResetButtonColor();
    void resetAllToDefaults();
    QString buildGStreamerPipeline();
    void updateROIResetButtonColor();
    QLineEdit *infoTextEdit;

    // ROI Selection State
    enum class ROISelectionTarget {
        ROI_INPUT,
        AUTOFOCUS_WINDOW
    };
    ROISelectionTarget currentROITarget = ROISelectionTarget::ROI_INPUT;

    // Info-Text Parameter
    CheckableComboBox *infoTextComboBox;
    QPushButton *infoTextResetButton;

    // ROI Parameter
    CustomLineEdit *roiInput;
    QPushButton *roiResetButton;
    ROIOverlay *roiOverlay;

    // Rolling buffer for parsing ROI output from the rpicam-apps fork (rt-roi)
    // (tolerant to line splits across read chunks, capped at 256 chars)
    QString m_procScanBuffer;

    // Metering Parameter
    QComboBox *meteringSelector;
    QLineEdit *meteringCustomInput;
    QPushButton *meteringResetButton;

    // Low Resolution Parameter
    LoresComboBox *loresComboBox;
    QPushButton *loresResetButton;

    // Recording Options (for signal-based recording)
    QCheckBox *signalRecordingCheckbox;
    QCheckBox *keypressRecordingCheckbox;
    QPushButton *sendSignalButton;
    QComboBox *initialStateComboBox;
    QCheckBox *splitFilesCheckbox;
    QLineEdit *segmentDurationInput;
    QLineEdit *circularBufferInput;
    bool isRecordingPaused = false;  // Tracks current recording state (pause/record)

    // ActionsTab — Actions-Tab
    ActionsTab *m_actionsTab = nullptr;

    // GStreamerTab — GStreamer-Streaming-Tab
    GStreamerTab *m_gstreamerTab = nullptr;

    // GstLaunchTab — GST-Tab / Stream Viewer
    GstLaunchTab *m_gstLaunchTab = nullptr;

    // InferenceTab — ODR-Tab
    InferenceTab *m_inferenceTab = nullptr;

    // ToolsTab — Tools-Tab
    ToolsTab *m_toolsTab = nullptr;

    void setupStillTab();
    void setupAudioTab();
    void setupGstreamerTab();
    void setupGstLaunchTab();
    void setupInferenceTab();
    void setupActionsTab();
    void setupToolsTab();
    void updateUIForApp(const QString &app);
    void updateOutputFileExtension();
    QString getExtensionForEncoding(const QString &encoding);

    // Reset Buttons
    QPushButton *timeoutResetButton = nullptr;
    QPushButton *filterResetButton = nullptr;
    QPushButton *actionsResetButton = nullptr;
    QPushButton *filterSettingsResetButton = nullptr;

    // Detection Action GUI Components are managed internally by ActionsModule (P18)

    // Tools Tab Components — managed internally by ToolsPlugin (P15)

    // Still Tab Components
    QCheckBox *autofocusOnCaptureCheckbox = nullptr;
    QCheckBox *zslCheckbox = nullptr;
    QCheckBox *immediateCheckbox = nullptr;
    QSpinBox *framestartSpinBox = nullptr;
    QPushButton *captureControlResetButton = nullptr;
    QLineEdit *thumbLineEdit = nullptr;
    QPushButton *thumbResetButton = nullptr;
    QSpinBox *restartSpinBox = nullptr;
    QPushButton *restartResetButton = nullptr;
    QLineEdit *exifLineEdit = nullptr;
    QPushButton *exifResetButton = nullptr;
    QLineEdit *latestLineEdit = nullptr;
    QCheckBox *rawCheckbox = nullptr;
    QPushButton *fileManagementResetButton = nullptr;

    // Audio Tab Components
    QCheckBox *enableAudioCheckBox = nullptr;
    QComboBox *audioCodecSelector = nullptr;
    QSpinBox *audioBitrateSpinBox = nullptr;
    QComboBox *audioSourceSelector = nullptr;
    QLineEdit *audioDeviceEdit = nullptr;
    QSpinBox *audioChannelsSpinBox = nullptr;
    QComboBox *audioSampleRateSelector = nullptr;
    QSpinBox *audioAvSyncSpinBox = nullptr;
    QPushButton *audioResetButton = nullptr;

    // Timed recording (migrated from DetectionActions — still used by ActionsPlugin callbacks)
    void startTimedRecording(int seconds);
};

#endif // MAINWINDOW_H
