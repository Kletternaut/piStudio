#ifndef CAMERAINSTANCE_H
#define CAMERAINSTANCE_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFormLayout>
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
#include <QMap>
#include <QProgressBar>
#include <QSlider>
#include <QTimer>
#include <QTime>
#include <QDateTime>
#include <QSet>
#include <QSettings>
#include <QString>

#include "ResourceBroker.h"
#include "CollapsibleHelper.h"
#include "CollapsibleGroupBox.h"
#include "CheckableComboBox.h"
#include "LoresComboBox.h"
#include "RefreshableComboBox.h"
#include "CustomLineEdit.h"
#include "SelectionOverlay.h"
#include "ROIOverlay.h"

/**
 * @brief CameraInstance - Ein QWidget, das alle Widgets und Logik
 *        für eine einzelne Kamera kapselt.
 *
 * Jede Instanz besitzt:
 *   - eigene Tab-Widgets (General, Output, Image, Still, Autofocus, ...)
 *   - eigenen QProcess (rpicam-apps)
 *   - eigenen QSettings-Namespace ("Camera0/" bzw. "Camera1/")
 *   - eigene Ressourcen via ResourceBroker (Port, Hailo, Ausgabepfad)
 *
 * Phase 1.1: Nur Skeleton – alle Member deklariert, keine Tab-Implementierung.
 */
class CameraInstance : public QWidget {
    Q_OBJECT

public:
    explicit CameraInstance(int cameraIndex, ResourceBroker *broker, QWidget *parent = nullptr);
    ~CameraInstance() override;

    // Kamera-Index (0-basiert)
    int cameraIndex() const { return m_cameraIndex; }

    // QSettings-Schlüssel mit kameraindividuellem Namespace
    // Beispiel: settingsKey("Focus/LensPosition") → "Camera0/Focus/LensPosition"
    QString settingsKey(const QString &key) const;

    // -----------------------------------------------------------------------
    // Tab-Setup-Methoden (werden in Phase 1.2 – 1.9 implementiert)
    // -----------------------------------------------------------------------
    void setupGeneralTab();
    void setupOutputTab();
    void setupImageTab();
    void setupStillTab();
    void setupAutofocusTab();
    void setupAudioTab();
    void setupGstreamerTab();
    void setupGstLaunchTab();
    void setupInferenceTab();
    void setupActionsTab();
    void setupZoomTab();

    // Einstellungen laden/speichern
    void loadSettings();
    void saveSettings();

    // Kamera starten/stoppen
    void startCamera();
    void stopCamera();
    bool isCameraRunning() const;

signals:
    void cameraStarted(int cameraIndex);
    void cameraStopped(int cameraIndex);
    void logMessage(int cameraIndex, const QString &message);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void pollV4L2Controls();

private:
    // -----------------------------------------------------------------------
    // Kern-Metadaten
    // -----------------------------------------------------------------------
    int           m_cameraIndex;       // 0 oder 1
    ResourceBroker *m_broker;          // Geteilte Ressourcenverwaltung
    QString       m_settingsPrefix;    // z.B. "Camera0/"

    QList<CollapsibleHelper*> m_collapsibleHelpers;

    // -----------------------------------------------------------------------
    // Tab-Container
    // -----------------------------------------------------------------------
    QTabWidget *m_tabWidget       = nullptr;
    QWidget    *m_generalTab      = nullptr;
    QWidget    *m_outputTab       = nullptr;
    QVBoxLayout *m_outputTabLayout = nullptr;
    QWidget    *m_imageTab        = nullptr;
    QWidget    *m_stillTab        = nullptr;
    QWidget    *m_autofocusTab    = nullptr;
    QWidget    *m_audioTab        = nullptr;
    QWidget    *m_gstreamerTab    = nullptr;
    QVBoxLayout *m_gstreamerTabLayout = nullptr;
    QWidget    *m_gstLaunchTab    = nullptr;
    QWidget    *m_inferenceTab    = nullptr;
    QWidget    *m_actionsTab      = nullptr;
    QWidget    *m_zoomTab         = nullptr;

    // -----------------------------------------------------------------------
    // Prozess
    // -----------------------------------------------------------------------
    QProcess *m_cameraProcess = nullptr;

    // -----------------------------------------------------------------------
    // General Tab
    // -----------------------------------------------------------------------
    QComboBox  *appSelector           = nullptr;
    QComboBox  *cameraSelector        = nullptr;
    QTextEdit  *cameraInfo            = nullptr;
    QFormLayout *parameterLayout      = nullptr;
    QWidget    *parameterWidget       = nullptr;
    QLineEdit  *parameterInput        = nullptr;
    QTextEdit  *outputLog             = nullptr;
    QPushButton *startButton          = nullptr;
    QPushButton *stopButton           = nullptr;
    QPushButton *startStopButton      = nullptr;

    QMap<QString, QString> cameraDetails;
    QStringList cameraResolutions;
    QMap<QString, QString> cameraModes;

    // Debug-Optionen
    QCheckBox *debugToFileCheckbox      = nullptr;
    QCheckBox *debugToLogWindowCheckbox = nullptr;
    QCheckBox *standardLoggingCheckbox  = nullptr;
    QCheckBox *processOutputCheckbox    = nullptr;
    QTextEdit *debugLog                 = nullptr;

    QComboBox *resolutionSelector = nullptr;
    QComboBox *framerateSelector  = nullptr;
    QComboBox *previewSelector    = nullptr;
    QPushButton *previewLibsBrowseButton = nullptr;
    QString m_previewLibsPath;              // custom path for --preview-libs

    QLineEdit   *customPreviewInput = nullptr;
    QCheckBox   *doubleSizeCheckbox = nullptr;
    QString      rpicamConfigPath;

    // Post-process & Tuning
    RefreshableComboBox *postProcessFileSelector       = nullptr;
    QPushButton         *postProcessFileBrowseButton   = nullptr;
    RefreshableComboBox *postProcessLibsSelector       = nullptr;
    QPushButton         *postProcessLibsBrowseButton   = nullptr;
    QPushButton         *resetPostProcessLibsButton    = nullptr;
    RefreshableComboBox *tuningFileSelector            = nullptr;
    QPushButton         *tuningFileBrowseButton        = nullptr;
    QPushButton         *resetPostProcessFileButton    = nullptr;
    QPushButton         *resetTuningFileButton         = nullptr;

    // Config-Datei
    QString     configFilePath;
    QString     guiPostProcessFilePath;
    QString     guiPostProcessLibsPath;
    QString     guiTuningFilePath;

    // Benutzerdefinierte Eintrags-Listen
    QStringList customAppEntries;
    QStringList customTimeoutEntries;
    QStringList customTimelapseEntries;
    QStringList customShutterEntries;

    // -----------------------------------------------------------------------
    // Output Tab
    // -----------------------------------------------------------------------
    QLineEdit   *outputFileName       = nullptr;
    QPushButton *browseButton         = nullptr;
    QLineEdit   *timeoutInput         = nullptr;
    QComboBox   *timeoutSelector      = nullptr;
    QComboBox   *timelapseInput       = nullptr;
    QCheckBox   *segmentationCheckbox = nullptr;
    QString      guiOutputFilePath;
    QString      guiMetadataPath;

    QCheckBox   *timestampCheckbox    = nullptr;
    QCheckBox   *autoNamingCheckbox   = nullptr;
    QCheckBox   *segmentPatternCheckbox = nullptr;

    // Output-Modus (File / GStreamer / TCP / UDP)
    QRadioButton *outputModeFile      = nullptr;
    QRadioButton *outputModeGStreamer  = nullptr;
    QRadioButton *outputModeTCP       = nullptr;
    QRadioButton *outputModeUDP       = nullptr;
    QButtonGroup *outputModeGroup     = nullptr;
    QWidget      *streamingModesWidget = nullptr;

    // Still-Encoding
    QComboBox   *encodingSelector     = nullptr;
    QWidget     *encodingWidget       = nullptr;
    QPushButton *encodingResetButton  = nullptr;

    // Codec-Widgets
    QComboBox  *codecSelector           = nullptr;
    QLabel     *codecLabel              = nullptr;
    QLabel     *profileLabel            = nullptr;
    QComboBox  *profileSelector         = nullptr;
    QLabel     *levelLabel              = nullptr;
    QComboBox  *levelSelector           = nullptr;
    QCheckBox  *inlineHeadersCheckbox   = nullptr;
    QLabel     *libavFormatLabel        = nullptr;
    QComboBox  *libavFormatSelector     = nullptr;
    QLabel     *libavVideoCodecLabel    = nullptr;
    QComboBox  *libavVideoCodecSelector = nullptr;
    QLabel     *libavCodecOptsLabel     = nullptr;
    QComboBox  *libavCodecOptsSelector  = nullptr;
    QCheckBox  *lowLatencyCheckbox      = nullptr;
    QLabel     *syncLabel               = nullptr;
    QComboBox  *syncSelector            = nullptr;
    QPushButton *syncResetButton        = nullptr;
    QSpinBox   *bitrateSpinBox          = nullptr;
    QSpinBox   *qualitySpinBox          = nullptr;
    QSpinBox   *intraSpinBox            = nullptr;
    QSpinBox   *framesSpinBox           = nullptr;
    QCheckBox  *flushCheckbox           = nullptr;
    QLineEdit  *savePtsInput            = nullptr;
    QGroupBox  *codecGroup              = nullptr;
    QPushButton *codecResetButton       = nullptr;
    QPushButton *resetCodecButton       = nullptr;

    // Metadata
    QLineEdit   *metadataFileEdit            = nullptr;
    QPushButton *metadataFileButton          = nullptr;
    QComboBox   *metadataFormatSelector      = nullptr;
    QCheckBox   *metadataAutoNamingCheckbox  = nullptr;
    QPushButton *metadataResetButton         = nullptr;
    QWidget     *metadataWidget              = nullptr;

    // Recording-Optionen
    QCheckBox   *signalRecordingCheckbox  = nullptr;
    QCheckBox   *keypressRecordingCheckbox = nullptr;
    QPushButton *sendSignalButton         = nullptr;
    QComboBox   *initialStateComboBox     = nullptr;
    QCheckBox   *splitFilesCheckbox       = nullptr;
    QLineEdit   *segmentDurationInput     = nullptr;
    QLineEdit   *circularBufferInput      = nullptr;
    bool         isRecordingPaused        = false;

    // Layout-Helfer
    QHBoxLayout *boxLayout       = nullptr;
    QHBoxLayout *timelapseLayout = nullptr;
    QHBoxLayout *timeoutLayout   = nullptr;
    QWidget     *timelapseRowWidget = nullptr;
    QLabel      *timelapseLabel     = nullptr;

    // Reset-Buttons (Output/General)
    QPushButton *resetOutputFileButton  = nullptr;
    QPushButton *timeoutResetButton     = nullptr;
    QPushButton *timelapseResetButton   = nullptr;
    QPushButton *shutterResetButton     = nullptr;

    // -----------------------------------------------------------------------
    // Image Tab
    // -----------------------------------------------------------------------
    QComboBox   *awbSelector         = nullptr;
    QPushButton *resetAwbButton      = nullptr;
    QLineEdit   *ccmInput            = nullptr;
    QPushButton *ccmResetButton      = nullptr;
    CustomLineEdit *BoxInput         = nullptr;

    QSlider     *sharpnessSlider     = nullptr;
    QLineEdit   *sharpnessInput      = nullptr;
    QSlider     *evSlider            = nullptr;
    QLineEdit   *evInput             = nullptr;
    QSlider     *gainSlider          = nullptr;
    QLineEdit   *gainInput           = nullptr;
    QSlider     *awbGainRedSlider    = nullptr;
    QLineEdit   *awbGainRedInput     = nullptr;
    QPushButton *awbGainRedResetButton  = nullptr;
    QSlider     *awbGainBlueSlider   = nullptr;
    QLineEdit   *awbGainBlueInput    = nullptr;
    QPushButton *awbGainBlueResetButton = nullptr;
    QSlider     *brightnessSlider    = nullptr;
    QLineEdit   *brightnessInput     = nullptr;
    QSlider     *contrastSlider      = nullptr;
    QLineEdit   *contrastInput       = nullptr;
    QSlider     *saturationSlider    = nullptr;
    QLineEdit   *saturationInput     = nullptr;

    CheckableComboBox *geometryComboBox  = nullptr;
    QPushButton       *geometryResetButton = nullptr;

    // Info-Text Checkboxen
    QCheckBox *infoTextFrameCheckbox   = nullptr;
    QCheckBox *infoTextFpsCheckbox     = nullptr;
    QCheckBox *infoTextExpCheckbox     = nullptr;
    QCheckBox *infoTextAgCheckbox      = nullptr;
    QCheckBox *infoTextDgCheckbox      = nullptr;
    QCheckBox *infoTextRgCheckbox      = nullptr;
    QCheckBox *infoTextBgCheckbox      = nullptr;
    QCheckBox *infoTextFocusCheckbox   = nullptr;
    QCheckBox *infoTextAelockCheckbox  = nullptr;
    QCheckBox *infoTextLpCheckbox      = nullptr;
    QCheckBox *infoTextAfstateCheckbox = nullptr;
    QLineEdit *infoTextEdit            = nullptr;
    CheckableComboBox *infoTextComboBox  = nullptr;
    QPushButton       *infoTextResetButton = nullptr;

    // Geometry-Checkboxen
    QCheckBox *hflipCheckbox    = nullptr;
    QCheckBox *vflipCheckbox    = nullptr;
    QCheckBox *rotationCheckbox = nullptr;

    QComboBox   *shutterInput            = nullptr;
    QComboBox   *hdrSelector             = nullptr;
    QPushButton *hdrResetButton          = nullptr;
    QComboBox   *denoiseSelector         = nullptr;
    QPushButton *denoiseResetButton      = nullptr;
    QComboBox   *flickerPeriodSelector   = nullptr;
    QPushButton *flickerPeriodResetButton = nullptr;

    // ROI
    CustomLineEdit *roiInput    = nullptr;
    QPushButton    *roiResetButton = nullptr;
    ROIOverlay     *roiOverlay  = nullptr;

    // Metering
    QComboBox   *meteringSelector    = nullptr;
    QLineEdit   *meteringCustomInput = nullptr;
    QPushButton *meteringResetButton = nullptr;

    // Low-Resolution
    LoresComboBox *loresComboBox   = nullptr;
    QPushButton   *loresResetButton = nullptr;

    // Image Reset-Buttons
    QPushButton *overlayResetButton = nullptr;
    QPushButton *globalResetButton  = nullptr;
    QPushButton *filterResetButton  = nullptr;
    bool         isInitializing     = true;

    // ROI-Selection
    enum class ROISelectionTarget { ROI_INPUT, AUTOFOCUS_WINDOW };
    ROISelectionTarget currentROITarget = ROISelectionTarget::ROI_INPUT;
    SelectionOverlay  *selectionOverlay = nullptr;

    // -----------------------------------------------------------------------
    // Still Tab
    // -----------------------------------------------------------------------
    QCheckBox   *autofocusOnCaptureCheckbox = nullptr;
    QCheckBox   *zslCheckbox                = nullptr;
    QCheckBox   *immediateCheckbox          = nullptr;
    QSpinBox    *framestartSpinBox          = nullptr;
    QPushButton *captureControlResetButton  = nullptr;
    QLineEdit   *thumbLineEdit              = nullptr;
    QPushButton *thumbResetButton           = nullptr;
    QSpinBox    *restartSpinBox             = nullptr;
    QPushButton *restartResetButton         = nullptr;
    QLineEdit   *exifLineEdit               = nullptr;
    QPushButton *exifResetButton            = nullptr;
    QLineEdit   *latestLineEdit             = nullptr;
    QCheckBox   *rawCheckbox                = nullptr;
    QPushButton *fileManagementResetButton  = nullptr;

    // -----------------------------------------------------------------------
    // Autofocus Tab
    // -----------------------------------------------------------------------
    QComboBox      *autofocusModeSelector        = nullptr;
    QPushButton    *resetAutofocusModeButton      = nullptr;
    QComboBox      *autofocusRangeSelector        = nullptr;
    QPushButton    *resetAutofocusRangeButton     = nullptr;
    QComboBox      *autofocusSpeedSelector        = nullptr;
    QPushButton    *resetAutofocusSpeedButton     = nullptr;
    CustomLineEdit *autofocusWindowInput          = nullptr;
    QPushButton    *resetAutofocusWindowButton    = nullptr;
    QSlider        *lensPositionSlider            = nullptr;
    QLineEdit      *lensPositionInput             = nullptr;
    QPushButton    *resetLensPositionButton       = nullptr;

    // Manual Focus
    QLineEdit   *v4l2DeviceInput           = nullptr;
    QButtonGroup *focusStepButtonGroup     = nullptr;
    int           currentFocusStepSize     = 300;
    QPushButton  *focusNearButton          = nullptr;
    QPushButton  *focusFarButton           = nullptr;
    QSlider      *focusAbsoluteSlider      = nullptr;
    QLineEdit    *focusAbsoluteInput       = nullptr;
    QPushButton  *resetFocusAbsoluteButton = nullptr;
    QLabel       *currentFocusPositionLabel = nullptr;
    QPushButton  *refreshFocusPositionButton = nullptr;
    QPushButton  *saveFocusFavoriteButton  = nullptr;
    QListWidget  *focusFavoritesList       = nullptr;
    QPushButton  *deleteFocusFavoriteButton = nullptr;
    QLineEdit    *focusFavoriteNameInput   = nullptr;
    QPushButton  *focusCalibrationButton   = nullptr;

    // V4L2 Polling (Focus/Zoom)
    QTimer *focusPositionRefreshTimer  = nullptr;
    QTimer *zoomPositionRefreshTimer   = nullptr;
    QTimer *v4l2ReconnectTimer         = nullptr;
    QTimer *v4l2PollTimer              = nullptr;
    int     v4l2ReconnectAttempts      = 0;
    int     v4l2_fd                    = -1;

    // -----------------------------------------------------------------------
    // Zoom Tab
    // -----------------------------------------------------------------------
    QButtonGroup *zoomStepButtonGroup      = nullptr;
    int           currentZoomStepSize      = 300;
    QPushButton  *zoomNearButton           = nullptr;
    QPushButton  *zoomFarButton            = nullptr;
    QSlider      *zoomAbsoluteSlider       = nullptr;
    QLineEdit    *zoomAbsoluteInput        = nullptr;
    QPushButton  *resetZoomAbsoluteButton  = nullptr;
    QLabel       *currentZoomPositionLabel = nullptr;
    QPushButton  *refreshZoomPositionButton = nullptr;
    QPushButton  *saveZoomFavoriteButton   = nullptr;
    QListWidget  *zoomFavoritesList        = nullptr;
    QPushButton  *deleteZoomFavoriteButton = nullptr;
    QLineEdit    *zoomFavoriteNameInput    = nullptr;

    // Gekoppelte Fokus+Zoom-Favoriten
    QMap<QString, QVariantMap> coupledFavorites;

    // -----------------------------------------------------------------------
    // Audio Tab
    // -----------------------------------------------------------------------
    QCheckBox   *enableAudioCheckBox    = nullptr;
    QComboBox   *audioCodecSelector     = nullptr;
    QSpinBox    *audioBitrateSpinBox    = nullptr;
    QComboBox   *audioSourceSelector   = nullptr;
    QLineEdit   *audioDeviceEdit        = nullptr;
    QSpinBox    *audioChannelsSpinBox   = nullptr;
    QComboBox   *audioSampleRateSelector = nullptr;
    QSpinBox    *audioAvSyncSpinBox     = nullptr;
    QPushButton *audioResetButton       = nullptr;

    // -----------------------------------------------------------------------
    // GStreamer Tab
    // -----------------------------------------------------------------------
    QComboBox  *gstTargetSelector   = nullptr;
    QLineEdit  *gstHostEdit         = nullptr;
    QSpinBox   *gstPortSpinBox      = nullptr;
    QComboBox  *gstFormatSelector   = nullptr;
    QComboBox  *gstParserSelector   = nullptr;
    QComboBox  *gstPayloadSelector  = nullptr;
    QSpinBox   *gstConfigInterval   = nullptr;
    QSpinBox   *gstPayloadType      = nullptr;
    QCheckBox  *gstSyncCheckBox     = nullptr;
    QLineEdit  *gstStreamWidthInput  = nullptr;
    QLineEdit  *gstStreamHeightInput = nullptr;
    QCheckBox  *gstRecordCheckBox    = nullptr;
    QLineEdit  *gstRecordFileEdit    = nullptr;
    QPushButton *gstRecordBrowseButton = nullptr;
    QCheckBox  *gstEosCheckBox      = nullptr;
    QSpinBox   *gstDebugLevelSpinBox = nullptr;
    QPushButton *gstSavePipelineButton = nullptr;
    QPushButton *gstClearLogButton   = nullptr;
    QTextEdit  *gstDebugOutput       = nullptr;

    // GST-Launch / Stream-Viewer Tab
    QTabWidget *streamViewerTabWidget = nullptr;
    struct StreamViewerTab {
        QWidget     *widget         = nullptr;
        QLineEdit   *nameEdit       = nullptr;
        QSpinBox    *portSpinBox    = nullptr;
        QComboBox   *codecSelector  = nullptr;
        QSpinBox    *payloadSpinBox = nullptr;
        QCheckBox   *syncCheckBox   = nullptr;
        QComboBox   *previewSizeSelector = nullptr;
        QPushButton *startButton    = nullptr;
        QProcess    *process        = nullptr;
    };
    QMap<int, StreamViewerTab> streamViewerTabs;
    int streamViewerTabCounter = 0;

    // Legacy-Zeiger (zeigen auf Ersttab)
    QSpinBox    *streamViewerPortSpinBox    = nullptr;
    QComboBox   *streamViewerCodecSelector  = nullptr;
    QSpinBox    *streamViewerPayloadSpinBox = nullptr;
    QCheckBox   *streamViewerSyncCheckBox   = nullptr;
    QPushButton *streamViewerStartButton    = nullptr;
    QProcess    *streamViewerProcess        = nullptr;

    // Test-Quellen (Tabbed)
    QTabWidget *testSourceTabWidget = nullptr;
    struct TestSourceTab {
        QWidget     *widget         = nullptr;
        QComboBox   *sourceSelector = nullptr;
        QLineEdit   *hostEdit       = nullptr;
        QSpinBox    *portSpinBox    = nullptr;
        QPushButton *startButton    = nullptr;
        QProcess    *process        = nullptr;
    };
    QMap<int, TestSourceTab> testSourceTabs;
    int testSourceTabCounter = 0;

    // Legacy-Zeiger
    QComboBox   *gstTestSourceSelector = nullptr;
    QLineEdit   *gstTestHostEdit       = nullptr;
    QSpinBox    *gstTestPortSpinBox    = nullptr;
    QPushButton *gstTestStartButton    = nullptr;
    QProcess    *gstTestProcess        = nullptr;

    // -----------------------------------------------------------------------
    // Inference Tab
    // -----------------------------------------------------------------------
    QProcess    *inferenceProcess       = nullptr;
    QPushButton *startInferenceButton   = nullptr;
    QLineEdit   *inferencePortInput     = nullptr;
    QListWidget *detectionResultsList   = nullptr;
    QCheckBox   *reportOnlyChangesCheckbox = nullptr;
    QString      lastDetectedObject;

    // -----------------------------------------------------------------------
    // Expert Tab (wird in späterer Phase zu CameraInstance verschoben)
    // -----------------------------------------------------------------------
    QComboBox   *viewfinderModeSelector           = nullptr;
    QPushButton *viewfinderModeResetButton        = nullptr;
    QSpinBox    *viewfinderWidthSpinBox           = nullptr;
    QPushButton *viewfinderWidthResetButton       = nullptr;
    QSpinBox    *viewfinderHeightSpinBox          = nullptr;
    QPushButton *viewfinderHeightResetButton      = nullptr;
    QSpinBox    *bufferCountSpinBox               = nullptr;
    QPushButton *bufferCountResetButton           = nullptr;
    QSpinBox    *viewfinderBufferCountSpinBox     = nullptr;
    QPushButton *viewfinderBufferCountResetButton = nullptr;

    // -----------------------------------------------------------------------
    // Laufzeit-Timer
    // -----------------------------------------------------------------------
    QTimer *runtimeTimer = nullptr;
    QTime   startTime;
};

#endif // CAMERAINSTANCE_H
