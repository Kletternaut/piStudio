// CameraInstance.cpp
// MCIM Phase 1.1 / 1.2 – Skeleton + MWE General Tab

#include "CameraInstance.h"
#include "../utils/AppPaths.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QDebug>
#include <unistd.h>  // close()

// ---------------------------------------------------------------------------
// Konstruktor
// ---------------------------------------------------------------------------
CameraInstance::CameraInstance(int cameraIndex, ResourceBroker *broker, QWidget *parent)
    : QWidget(parent)
    , m_cameraIndex(cameraIndex)
    , m_broker(broker)
    , m_settingsPrefix(QStringLiteral("Camera%1/").arg(cameraIndex))
{
    // Tab-Widget erzeugen
    m_tabWidget = new QTabWidget(this);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_tabWidget);
    setLayout(mainLayout);

    // QGroupBox-Styling (identisch zum Hauptfenster)
    setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    padding: 0 3px;"
        "    background-color: #f8f9fa;"
        "}"
    );

    // Haupt-Prozess (rpicam-apps)
    m_cameraProcess = new QProcess(this);
    connect(m_cameraProcess, &QProcess::started,
            this, &CameraInstance::onProcessStarted);
    connect(m_cameraProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CameraInstance::onProcessFinished);
    connect(m_cameraProcess, &QProcess::errorOccurred,
            this, &CameraInstance::onProcessError);
    connect(m_cameraProcess, &QProcess::readyReadStandardOutput,
            this, &CameraInstance::onReadyReadStandardOutput);
    connect(m_cameraProcess, &QProcess::readyReadStandardError,
            this, &CameraInstance::onReadyReadStandardError);

    // Tabs aufbauen (Phase 1.2+)
    setupGeneralTab();
    setupOutputTab();
    setupImageTab();
    setupStillTab();
    setupAutofocusTab();

    qDebug() << "[CameraInstance] Kamera" << m_cameraIndex
             << "erstellt, Prefix:" << m_settingsPrefix;
}

// ---------------------------------------------------------------------------
// Destruktor
// ---------------------------------------------------------------------------
CameraInstance::~CameraInstance()
{
    stopCamera();

    if (v4l2_fd >= 0) {
        ::close(v4l2_fd);
        v4l2_fd = -1;
    }
}

// ---------------------------------------------------------------------------
// settingsKey
// ---------------------------------------------------------------------------
QString CameraInstance::settingsKey(const QString &key) const
{
    return m_settingsPrefix + key;
}

// ---------------------------------------------------------------------------
// isCameraRunning
// ---------------------------------------------------------------------------
bool CameraInstance::isCameraRunning() const
{
    return m_cameraProcess && m_cameraProcess->state() != QProcess::NotRunning;
}

// ---------------------------------------------------------------------------
// startCamera / stopCamera
// ---------------------------------------------------------------------------
void CameraInstance::startCamera()
{
    if (isCameraRunning()) {
        qDebug() << "[CameraInstance" << m_cameraIndex << "] Kamera läuft bereits";
        return;
    }

    // --- App auswählen -------------------------------------------------
    QString app = appSelector ? appSelector->currentText().trimmed() : "rpicam-vid";
    if (app.isEmpty()) app = "rpicam-vid";

    QStringList args;

    // --- Kamera-Index – kommt direkt aus m_cameraIndex, kein Selector-Widget nötig
    args << "--camera" << QString::number(m_cameraIndex);

    // --- Auflösung / Framerate ----------------------------------------
    if (resolutionSelector && !resolutionSelector->currentText().isEmpty()) {
        QString res = resolutionSelector->currentText().trimmed();
        QStringList parts = res.split('x');
        if (parts.size() == 2) {
            args << "--width"  << parts[0].trimmed();
            args << "--height" << parts[1].trimmed();
        }
    }
    if (framerateSelector && !framerateSelector->currentText().isEmpty()) {
        QString fps = framerateSelector->currentText().trimmed();
        if (!fps.isEmpty() && fps != "Auto") {
            args << "--framerate" << fps;
        }
    }

    // --- Preview -------------------------------------------------------
    if (previewSelector) {
        QString data = previewSelector->currentData().toString();
        if (!data.isEmpty()) {
            if (data == "wayland-egl" || data == "egl" || data == "drm" || data == "qt") {
                args << "--preview-backend" << data;
            } else {
                args << data; // --fullscreen, --qt-preview, --nopreview
            }
        }
    }
    // --preview-libs (custom .so path)
    if (!m_previewLibsPath.isEmpty()) {
        args << "--preview-libs" << m_previewLibsPath;
    }

    // --- Timeout --------------------------------------------------------
    if (timeoutInput && !timeoutInput->text().isEmpty()) {
        bool ok = false;
        int ms = timeoutInput->text().trimmed().toInt(&ok);
        if (ok && ms >= 0) {
            args << "--timeout" << QString::number(ms);
        }
    }

    // --- Ausgabedatei ---------------------------------------------------
    if (app != "rpicam-vid" || splitFilesCheckbox == nullptr || !splitFilesCheckbox->isChecked()) {
        if (outputFileName && !outputFileName->text().isEmpty()) {
            args << "--output" << outputFileName->text().trimmed();
        }
    }

    // --- Post-Prozess-Datei -------------------------------------------
    if (postProcessFileSelector &&
        !postProcessFileSelector->currentText().isEmpty() &&
        postProcessFileSelector->currentText() != tr("(none)")) {
        args << "--post-process-file" << postProcessFileSelector->currentText().trimmed();
    }

    // --- Tuning-Datei --------------------------------------------------
    if (tuningFileSelector &&
        !tuningFileSelector->currentText().isEmpty() &&
        tuningFileSelector->currentText() != tr("(none)")) {
        args << "--tuning-file" << tuningFileSelector->currentText().trimmed();
    }

    // --- Auto-Naming ---------------------------------------------------
    if (autoNamingCheckbox && autoNamingCheckbox->isChecked()) {
        args << "--datetime";
    }
    if (timestampCheckbox && timestampCheckbox->isChecked()) {
        args << "--timestamp";
    }

    // --- Codec (nur rpicam-vid) ----------------------------------------
    if (app.contains("vid") && codecSelector) {
        QString codec = codecSelector->currentText().trimmed();
        if (!codec.isEmpty()) {
            args << "--codec" << codec;
        }
    }

    // --- AWB Gains ----------------------------------------------------
    static const double DEFAULT_RED  = 1.5;
    static const double DEFAULT_BLUE = 1.2;
    bool hasCcm = (ccmInput && !ccmInput->text().isEmpty());
    if (awbGainRedInput && awbGainBlueInput) {
        double red  = awbGainRedInput->text().toDouble();
        double blue = awbGainBlueInput->text().toDouble();
        // Always send explicit gains when CCM is set (rpicam-apps ignores --ccm without --awbgains)
        if (hasCcm || red != DEFAULT_RED || blue != DEFAULT_BLUE) {
            args << "--awbgains" << QString("%1,%2").arg(red, 0, 'f', 1).arg(blue, 0, 'f', 1);
        }
    }

    // --- CCM (Colour Correction Matrix) --------------------------------
    if (hasCcm) {
        QString ccm = ccmInput->text().trimmed();
        QStringList parts = ccm.split(',');
        if (parts.size() == 9) {
            args << "--ccm" << ccm;
        }
    }

    qDebug() << "[CameraInstance" << m_cameraIndex << "] Starte:" << app << args;
    if (outputLog) {
        outputLog->append(QDateTime::currentDateTime().toString("hh:mm:ss ")
                          + app + " " + args.join(" "));
    }

    m_cameraProcess->start(app, args);
}

void CameraInstance::stopCamera()
{
    if (m_cameraProcess && m_cameraProcess->state() != QProcess::NotRunning) {
        m_cameraProcess->terminate();
        if (!m_cameraProcess->waitForFinished(3000)) {
            m_cameraProcess->kill();
        }
    }
}

// ---------------------------------------------------------------------------
// loadSettings / saveSettings (Stub)
// ---------------------------------------------------------------------------
void CameraInstance::loadSettings()
{
    // TODO (Phase 1.2+): Einstellungen aus QSettings(settingsKey(...)) laden
}

void CameraInstance::saveSettings()
{
    // TODO (Phase 1.2+): Einstellungen in QSettings(settingsKey(...)) speichern
}

// ---------------------------------------------------------------------------
// Tab-Setup-Methoden (Stubs – werden in Phase 1.2-1.9 befüllt)
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Phase 1.2 – setupGeneralTab(): MWE-Implementierung
// ---------------------------------------------------------------------------
void CameraInstance::setupGeneralTab()
{
    // Scroll-Container
    auto *scrollArea  = new QScrollArea(this);
    auto *container   = new QWidget(scrollArea);
    auto *vbox        = new QVBoxLayout(container);
    scrollArea->setWidget(container);
    scrollArea->setWidgetResizable(true);

    // -----------------------------------------------------------------------
    // Gruppe: Anwendung & Kamera
    // -----------------------------------------------------------------------
    auto *appGroup       = new QGroupBox(tr("Application"), container);
    auto *appGroupLayout = new QGridLayout(appGroup);

    appSelector = new QComboBox(this);
    appSelector->addItems({"rpicam-vid", "rpicam-jpeg", "rpicam-still",
                           "rpicam-raw", "rpicam-hello"});
    appSelector->setToolTip(tr("Select the rpicam application to run."));

    // Kamera-Index ist fest (m_cameraIndex), kein Selector nötig
    cameraSelector = nullptr;

    appGroupLayout->addWidget(new QLabel(tr("App:")), 0, 0);
    appGroupLayout->addWidget(appSelector, 0, 1);

    // Kamera-Info
    cameraInfo = new QTextEdit(this);
    cameraInfo->setReadOnly(true);
    cameraInfo->setMaximumHeight(60);
    cameraInfo->setPlaceholderText(
        tr("Camera %1 – details will appear here after detection.").arg(m_cameraIndex));
    appGroupLayout->addWidget(cameraInfo, 1, 0, 1, 2);

    vbox->addWidget(appGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Aufnahme-Parameter
    // -----------------------------------------------------------------------
    auto *recGroup       = new QGroupBox(tr("Recording Parameters"), container);
    auto *recGroupLayout = new QGridLayout(recGroup);

    resolutionSelector = new QComboBox(this);
    resolutionSelector->addItems({"1332x990", "2028x1080", "2028x1520",
                                   "4056x3040"});
    resolutionSelector->setCurrentText("2028x1080");
    resolutionSelector->setToolTip(tr("Recording resolution."));

    framerateSelector = new QComboBox(this);
    framerateSelector->setEditable(true);
    framerateSelector->addItems({"", "15", "24", "25", "30", "50", "60"});
    framerateSelector->setToolTip(tr("Framerate (fps). Right-click to add/delete values."));

    // Kontextmenü für Framerate
    framerateSelector->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(framerateSelector, &QComboBox::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QMenu menu(tr("Framerate"), this);
        QAction *addAct = menu.addAction(tr("Add framerate…"));
        connect(addAct, &QAction::triggered, this, [this]() {
            bool ok;
            QString val = QInputDialog::getText(this, tr("Add Framerate"),
                                               tr("Enter framerate (fps):"),
                                               QLineEdit::Normal, {}, &ok);
            if (ok && !val.isEmpty()) {
                bool isNum;
                val.toDouble(&isNum);
                if (isNum && framerateSelector->findText(val) < 0) {
                    framerateSelector->addItem(val);
                }
            }
        });
        menu.addSeparator();
        QString cur = framerateSelector->currentText();
        QAction *delAct = menu.addAction(tr("Delete '%1'").arg(cur));
        delAct->setEnabled(!cur.isEmpty());
        connect(delAct, &QAction::triggered, this, [this, cur]() {
            int idx = framerateSelector->findText(cur);
            if (idx >= 0) framerateSelector->removeItem(idx);
        });
        menu.exec(framerateSelector->mapToGlobal(pos));
    });

    previewSelector = new QComboBox(this);
    previewSelector->addItem("", "");
    previewSelector->addItem(tr("Fullscreen"),  "--fullscreen");
    previewSelector->addItem(tr("Qt-Preview"),  "--qt-preview");
    previewSelector->addItem(tr("No Preview"),  "--nopreview");
    // Default: On remote sessions (XRDP) we MUST use --qt-preview;
    //          on local sessions leave empty (auto-detect from compositor/GPU).
    {
        bool remote = !qgetenv("XRDP_SESSION").isEmpty();
        if (!remote) {
            QString sessionId = qgetenv("XDG_SESSION_ID");
            if (!sessionId.isEmpty()) {
                QProcess proc;
                proc.start("loginctl", {"show-session", sessionId, "-p", "Remote"});
                if (proc.waitForFinished(2000) && proc.exitCode() == 0) {
                    QString output = proc.readAllStandardOutput();
                    if (output.contains("Remote=yes")) remote = true;
                }
            }
        }
        if (remote) {
            previewSelector->setCurrentIndex(
                previewSelector->findData("--qt-preview"));
        } else {
            previewSelector->setCurrentIndex(0); // empty = auto-detect
        }
    }
    previewSelector->setToolTip(tr("Preview mode."));

    previewLibsBrowseButton = new QPushButton("…", this);
    previewLibsBrowseButton->setFixedWidth(20);
    previewLibsBrowseButton->setToolTip(tr("Select a custom preview library path (--preview-libs)"));
    connect(previewLibsBrowseButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = m_previewLibsPath.isEmpty() ? "/usr/local/lib" : m_previewLibsPath;
        QString dirName = QFileDialog::getExistingDirectory(this, tr("Select Preview Libs Directory"), initialPath);
        if (!dirName.isEmpty()) {
            m_previewLibsPath = dirName;
        }
    });

    recGroupLayout->addWidget(new QLabel(tr("Resolution:")), 0, 0);
    recGroupLayout->addWidget(resolutionSelector, 0, 1);
    recGroupLayout->addWidget(new QLabel(tr("Framerate:")), 1, 0);
    recGroupLayout->addWidget(framerateSelector, 1, 1);

    // Preview row: label | dropdown + browse button
    recGroupLayout->addWidget(new QLabel(tr("Preview:"), this), 2, 0);
    auto *previewRow = new QHBoxLayout;
    previewRow->addWidget(previewSelector);
    previewRow->addWidget(previewLibsBrowseButton);
    recGroupLayout->addLayout(previewRow, 2, 1);

    // TODO: Make preview dropdown adaptive (--preview-backend for rpicam-apps >= 1.13)
    // Currently always shows legacy entries; upgrade path available via QProcess version check

    vbox->addWidget(recGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Ausgabe
    // -----------------------------------------------------------------------
    auto *outGroup       = new QGroupBox(tr("Output"), container);
    auto *outGroupLayout = new QGridLayout(outGroup);

    outputFileName = new QLineEdit(this);
    outputFileName->setPlaceholderText(
        tr("output_cam%1.mp4").arg(m_cameraIndex));
    outputFileName->setToolTip(tr("Output file path."));

    browseButton = new QPushButton(tr("Browse"), this);
    browseButton->setFixedWidth(80);
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, tr("Output File"), outputFileName->text());
        if (!path.isEmpty()) {
            outputFileName->setText(path);
        }
    });

    timestampCheckbox = new QCheckBox(tr("Timestamp"), this);
    timestampCheckbox->setToolTip(tr("Add timestamp to filename."));

    autoNamingCheckbox = new QCheckBox(tr("Auto-Name"), this);
    autoNamingCheckbox->setToolTip(tr("Automatic file naming."));

    timeoutInput = new QLineEdit(this);
    timeoutInput->setPlaceholderText("0");
    timeoutInput->setFixedWidth(80);
    timeoutInput->setToolTip(tr("Timeout in ms (0 = infinite)."));

    timeoutSelector = new QComboBox(this);
    timeoutSelector->setEditable(true);
    timeoutSelector->addItems({"0", "5000", "10000", "15000", "20000",
                                "30000", "60000"});
    timeoutSelector->setCurrentText("0");
    timeoutSelector->setToolTip(tr("Capture timeout in milliseconds."));

    auto *filenameRow = new QHBoxLayout;
    filenameRow->addWidget(outputFileName);
    filenameRow->addWidget(browseButton);

    auto *checkboxRow = new QHBoxLayout;
    checkboxRow->addWidget(timestampCheckbox);
    checkboxRow->addWidget(autoNamingCheckbox);
    checkboxRow->addStretch();

    outGroupLayout->addWidget(new QLabel(tr("File:")), 0, 0);
    outGroupLayout->addLayout(filenameRow, 0, 1);
    outGroupLayout->addWidget(new QLabel(tr("Timeout:")), 1, 0);
    outGroupLayout->addWidget(timeoutSelector, 1, 1);
    outGroupLayout->addLayout(checkboxRow, 2, 0, 1, 2);

    vbox->addWidget(outGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Verarbeitungsdateien
    // -----------------------------------------------------------------------
    auto *procGroup       = new QGroupBox(tr("Processing Files"), container);
    auto *procGroupLayout = new QGridLayout(procGroup);

    postProcessFileSelector = new RefreshableComboBox(this);
    postProcessFileSelector->setEditable(true);
    postProcessFileSelector->setToolTip(tr("Post-process JSON file."));

    postProcessFileBrowseButton = new QPushButton(tr("Browse"), this);
    postProcessFileBrowseButton->setFixedWidth(60);
    connect(postProcessFileBrowseButton, &QPushButton::clicked,
            this, [this]() {
        QString f = QFileDialog::getOpenFileName(
            this, tr("Post-Process File"), {},
            tr("JSON Files (*.json);;All Files (*)"));
        if (!f.isEmpty()) postProcessFileSelector->setCurrentText(f);
    });

    tuningFileSelector = new RefreshableComboBox(this);
    tuningFileSelector->setEditable(true);
    tuningFileSelector->setToolTip(tr("Camera tuning JSON file."));

    tuningFileBrowseButton = new QPushButton(tr("Browse"), this);
    tuningFileBrowseButton->setFixedWidth(60);
    connect(tuningFileBrowseButton, &QPushButton::clicked,
            this, [this]() {
        QString f = QFileDialog::getOpenFileName(
            this, tr("Tuning File"), {},
            tr("JSON Files (*.json);;All Files (*)"));
        if (!f.isEmpty()) tuningFileSelector->setCurrentText(f);
    });

    auto *ppRow = new QHBoxLayout;
    ppRow->addWidget(postProcessFileSelector);
    ppRow->addWidget(postProcessFileBrowseButton);

    auto *tuningRow = new QHBoxLayout;
    tuningRow->addWidget(tuningFileSelector);
    tuningRow->addWidget(tuningFileBrowseButton);

    procGroupLayout->addWidget(new QLabel(tr("Post-Process:")), 0, 0);
    procGroupLayout->addLayout(ppRow, 0, 1);
    procGroupLayout->addWidget(new QLabel(tr("Tuning File:")), 1, 0);
    procGroupLayout->addLayout(tuningRow, 1, 1);

    vbox->addWidget(procGroup);

    // -----------------------------------------------------------------------
    // Start/Stop-Button + Output-Log
    // -----------------------------------------------------------------------
    startStopButton = new QPushButton(tr("Start"), this);
    startStopButton->setMinimumHeight(36);
    connect(startStopButton, &QPushButton::clicked, this, [this]() {
        if (isCameraRunning()) {
            stopCamera();
        } else {
            startCamera();
        }
    });
    // Button-Text synchron mit Prozess-Status halten
    connect(this, &CameraInstance::cameraStarted, this, [this]() {
        if (startStopButton) startStopButton->setText(tr("Stop"));
    });
    connect(this, &CameraInstance::cameraStopped, this, [this]() {
        if (startStopButton) startStopButton->setText(tr("Start"));
    });
    vbox->addWidget(startStopButton);

    outputLog = new QTextEdit(this);
    outputLog->setReadOnly(true);
    outputLog->setMinimumHeight(80);
    outputLog->setMaximumHeight(120);
    outputLog->setPlaceholderText(tr("Process output appears here…"));
    // Prozess-Output direkt ins Log schreiben
    connect(this, &CameraInstance::logMessage, this, [this](int /*idx*/, const QString &msg) {
        if (outputLog) {
            outputLog->append(msg.trimmed());
        }
    });
    connect(outputLog, &QTextEdit::textChanged, this, [this]() {
        QTextCursor c = outputLog->textCursor();
        c.movePosition(QTextCursor::End);
        outputLog->setTextCursor(c);
        outputLog->ensureCursorVisible();
    });
    vbox->addWidget(outputLog);

    vbox->addStretch();

    // -----------------------------------------------------------------------
    // Tab einhängen
    // -----------------------------------------------------------------------
    m_generalTab = scrollArea;
    m_tabWidget->addTab(m_generalTab, tr("General"));

    // einmalig Einstellungen laden
    QSettings cfg(AppPaths::globalConf(), QSettings::IniFormat);
    cfg.beginGroup(AppPaths::tabGroup(m_cameraIndex));
    appSelector->setCurrentText(
        cfg.value(settingsKey("General/App"), "rpicam-vid").toString());
    outputFileName->setText(
        cfg.value(settingsKey("General/OutputFile"),
                  tr("output_cam%1.mp4").arg(m_cameraIndex)).toString());
    resolutionSelector->setCurrentText(
        cfg.value(settingsKey("General/Resolution"), "2028x1080").toString());
    framerateSelector->setCurrentText(
        cfg.value(settingsKey("General/Framerate"), "").toString());
    timeoutSelector->setCurrentText(
        cfg.value(settingsKey("General/Timeout"), "0").toString());
    cfg.endGroup();

    // Verbindungen fürs Auto-Speichern
    connect(appSelector, &QComboBox::currentTextChanged,
            this, [this](const QString &v) {
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        s.beginGroup(AppPaths::tabGroup(m_cameraIndex));
        s.setValue(settingsKey("General/App"), v);
        s.endGroup();
    });
    connect(outputFileName, &QLineEdit::textChanged,
            this, [this](const QString &v) {
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        s.beginGroup(AppPaths::tabGroup(m_cameraIndex));
        s.setValue(settingsKey("General/OutputFile"), v);
        s.endGroup();
    });
}

void CameraInstance::setupOutputTab()
{
    auto *scrollArea = new QScrollArea(this);
    auto *container  = new QWidget(scrollArea);
    m_outputTabLayout = new QVBoxLayout(container);
    scrollArea->setWidget(container);
    scrollArea->setWidgetResizable(true);

    // -----------------------------------------------------------------------
    // Gruppe: Codec-Einstellungen
    // -----------------------------------------------------------------------
    codecGroup = new QGroupBox(tr("Codec Settings"), container);
    auto *cgLayout = new QGridLayout(codecGroup);
    cgLayout->setColumnMinimumWidth(0, 140);

    codecSelector = new QComboBox(this);
    codecSelector->addItem("");
    codecSelector->addItems({"h264", "mjpeg", "yuv420", "libav"});
    codecSelector->setCurrentText("h264");
    codecSelector->setToolTip(tr("Codec for video encoding."));

    levelSelector = new QComboBox(this);
    levelSelector->addItems({"4.0", "4.1", "4.2"});
    levelSelector->setCurrentIndex(-1);
    levelSelector->setPlaceholderText(tr("None"));
    levelSelector->setToolTip(tr("H.264 level (--level)"));

    inlineHeadersCheckbox = new QCheckBox(tr("Inline"), this);
    inlineHeadersCheckbox->setToolTip(tr("Force PPS/SPS headers in every I-frame (--inline)"));

    lowLatencyCheckbox = new QCheckBox(tr("Low Latency"), this);
    lowLatencyCheckbox->setToolTip(tr("Enable low latency presets (--low-latency, libav only)"));

    profileSelector = new QComboBox(this);
    profileSelector->addItems({"baseline", "main", "high"});
    profileSelector->setCurrentIndex(-1);
    profileSelector->setPlaceholderText(tr("None"));
    profileSelector->setToolTip(tr("H.264/LibAV profile (--profile)"));

    libavFormatSelector = new QComboBox(this);
    libavFormatSelector->addItems({"mpegts", "mp4", "avi", "flv", "mkv", "mov"});
    libavFormatSelector->setCurrentText("mpegts");
    libavFormatSelector->setToolTip(tr("LibAV output format (--libav-format)"));

    libavVideoCodecSelector = new QComboBox(this);
    libavVideoCodecSelector->addItems({"h264_v4l2m2m", "libx264", "libx265", "vp8", "vp9"});
    libavVideoCodecSelector->setCurrentText("h264_v4l2m2m");
    libavVideoCodecSelector->setToolTip(tr("LibAV video codec (--libav-video-codec)"));

    libavCodecOptsSelector = new QComboBox(this);
    libavCodecOptsSelector->setEditable(true);
    libavCodecOptsSelector->addItems({"", "preset=ultrafast", "preset=fast",
                                       "tune=zerolatency",
                                       "preset=ultrafast;tune=zerolatency"});
    libavCodecOptsSelector->setCurrentText("");
    libavCodecOptsSelector->setToolTip(tr("LibAV codec options (--libav-video-codec-opts)"));

    codecLabel  = new QLabel(tr("Codec:"), this);
    levelLabel  = new QLabel(tr("Level:"), this);
    profileLabel = new QLabel(tr("Profile:"), this);
    libavFormatLabel       = new QLabel(tr("LibAV Format:"), this);
    libavVideoCodecLabel   = new QLabel(tr("LibAV Video Codec:"), this);
    libavCodecOptsLabel    = new QLabel(tr("LibAV Codec Options:"), this);
    syncLabel = nullptr;  // nicht in CameraInstance genutzt

    cgLayout->addWidget(codecLabel,          0, 0);
    cgLayout->addWidget(codecSelector,       0, 1);
    cgLayout->addWidget(levelLabel,          0, 2);
    cgLayout->addWidget(levelSelector,       0, 3);
    cgLayout->addWidget(inlineHeadersCheckbox, 0, 4);
    cgLayout->addWidget(lowLatencyCheckbox,  0, 5);
    cgLayout->addWidget(profileLabel,        1, 0);
    cgLayout->addWidget(profileSelector,     1, 1);
    cgLayout->addWidget(libavFormatLabel,    2, 0);
    cgLayout->addWidget(libavFormatSelector, 2, 1);
    cgLayout->addWidget(libavVideoCodecLabel,  3, 0);
    cgLayout->addWidget(libavVideoCodecSelector, 3, 1);
    cgLayout->addWidget(libavCodecOptsLabel, 4, 0);
    cgLayout->addWidget(libavCodecOptsSelector, 4, 1);

    // Initial Visibility: h264-Modus
    auto updateCodecVis = [this]() {
        bool isLibav = codecSelector && codecSelector->currentText() == "libav";
        bool isH264  = codecSelector &&
                       (codecSelector->currentText() == "h264" ||
                        codecSelector->currentText().isEmpty());
        if (profileLabel)          profileLabel->setVisible(isH264 || isLibav);
        if (profileSelector)       profileSelector->setVisible(isH264 || isLibav);
        if (levelLabel)            levelLabel->setVisible(isH264);
        if (levelSelector)         levelSelector->setVisible(isH264);
        if (inlineHeadersCheckbox) inlineHeadersCheckbox->setVisible(isH264);
        if (libavFormatLabel)      libavFormatLabel->setVisible(isLibav);
        if (libavFormatSelector)   libavFormatSelector->setVisible(isLibav);
        if (libavVideoCodecLabel)  libavVideoCodecLabel->setVisible(isLibav);
        if (libavVideoCodecSelector) libavVideoCodecSelector->setVisible(isLibav);
        if (libavCodecOptsLabel)   libavCodecOptsLabel->setVisible(isLibav);
        if (libavCodecOptsSelector) libavCodecOptsSelector->setVisible(isLibav);
        if (lowLatencyCheckbox)    lowLatencyCheckbox->setVisible(isLibav);
    };
    updateCodecVis();
    connect(codecSelector, &QComboBox::currentTextChanged,
            this, [updateCodecVis](const QString &) { updateCodecVis(); });

    codecResetButton = new QPushButton("✕", this);
    codecResetButton->setFixedWidth(20);
    codecResetButton->setToolTip(tr("Reset codec settings"));
    cgLayout->addWidget(codecResetButton, 1, 6);
    connect(codecResetButton, &QPushButton::clicked, this, [this, updateCodecVis]() {
        if (codecSelector)         codecSelector->setCurrentText("h264");
        if (profileSelector)       profileSelector->setCurrentIndex(-1);
        if (levelSelector)         levelSelector->setCurrentIndex(-1);
        if (inlineHeadersCheckbox) inlineHeadersCheckbox->setChecked(false);
        if (libavFormatSelector)   libavFormatSelector->setCurrentText("mpegts");
        if (libavVideoCodecSelector) libavVideoCodecSelector->setCurrentText("h264_v4l2m2m");
        if (libavCodecOptsSelector) libavCodecOptsSelector->setCurrentText("");
        if (lowLatencyCheckbox)    lowLatencyCheckbox->setChecked(false);
        codecResetButton->setStyleSheet("");
        updateCodecVis();
    });

    m_outputTabLayout->addWidget(codecGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Qualität & Bitrate
    // -----------------------------------------------------------------------
    auto *qualGroup  = new QGroupBox(tr("Quality & Bitrate"), container);
    auto *qlLayout   = new QGridLayout(qualGroup);
    qlLayout->setColumnMinimumWidth(0, 140);

    bitrateSpinBox = new QSpinBox(this);
    bitrateSpinBox->setRange(0, 100000);
    bitrateSpinBox->setValue(0);
    bitrateSpinBox->setSpecialValueText(tr("Default"));
    bitrateSpinBox->setSuffix(" kbps");
    bitrateSpinBox->setToolTip(tr("Video bitrate in kbps (--bitrate, 0=default)"));

    qualitySpinBox = new QSpinBox(this);
    qualitySpinBox->setRange(0, 100);
    qualitySpinBox->setValue(0);
    qualitySpinBox->setSpecialValueText(tr("Default"));
    qualitySpinBox->setToolTip(tr("MJPEG quality 1-100 (--quality, 0=default)"));

    intraSpinBox = new QSpinBox(this);
    intraSpinBox->setRange(0, 300);
    intraSpinBox->setValue(0);
    intraSpinBox->setSpecialValueText(tr("Default"));
    intraSpinBox->setToolTip(tr("Keyframe interval (--intra, 0=default)"));

    framesSpinBox = new QSpinBox(this);
    framesSpinBox->setRange(0, 999999);
    framesSpinBox->setValue(0);
    framesSpinBox->setSpecialValueText(tr("Unlimited"));
    framesSpinBox->setToolTip(tr("Max frames to record (--frames, 0=unlimited)"));

    qlLayout->addWidget(new QLabel(tr("Bitrate:"), this),    0, 0);
    qlLayout->addWidget(bitrateSpinBox,                      0, 1);
    qlLayout->addWidget(new QLabel(tr("Quality:"), this),    0, 2);
    qlLayout->addWidget(qualitySpinBox,                      0, 3);
    qlLayout->addWidget(new QLabel(tr("Intra Period:"), this), 1, 0);
    qlLayout->addWidget(intraSpinBox,                        1, 1);
    qlLayout->addWidget(new QLabel(tr("Max Frames:"), this), 1, 2);
    qlLayout->addWidget(framesSpinBox,                       1, 3);

    auto *qualResetBtn = new QPushButton("✕", this);
    qualResetBtn->setFixedWidth(20);
    qualResetBtn->setToolTip(tr("Reset quality & bitrate"));
    qlLayout->addWidget(qualResetBtn, 1, 4, Qt::AlignRight);
    connect(qualResetBtn, &QPushButton::clicked, this, [this, qualResetBtn]() {
        if (bitrateSpinBox) bitrateSpinBox->setValue(0);
        if (qualitySpinBox) qualitySpinBox->setValue(0);
        if (intraSpinBox)   intraSpinBox->setValue(0);
        if (framesSpinBox)  framesSpinBox->setValue(0);
        qualResetBtn->setStyleSheet("");
    });

    m_outputTabLayout->addWidget(qualGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Aufnahme-Optionen (Signal / Keypress / Split)
    // -----------------------------------------------------------------------
    auto *recOptGroup  = new QGroupBox(tr("Recording Options"), container);
    auto *recOptLayout = new QVBoxLayout(recOptGroup);

    signalRecordingCheckbox   = new QCheckBox(tr("Signal-based recording (--signal)"), this);
    keypressRecordingCheckbox = new QCheckBox(tr("Keypress-based recording (--keypress)"), this);
    splitFilesCheckbox        = new QCheckBox(tr("Split files (--split)"), this);

    auto *segRow    = new QHBoxLayout;
    segmentDurationInput = new QLineEdit(this);
    segmentDurationInput->setPlaceholderText(tr("Segment duration (ms)"));
    segmentDurationInput->setFixedWidth(160);
    segmentDurationInput->setToolTip(tr("Duration of each segment in ms (--segment)"));
    sendSignalButton = new QPushButton(tr("Send Signal"), this);
    sendSignalButton->setEnabled(false);
    sendSignalButton->setToolTip(tr("Send SIGUSR1 to toggle recording"));
    segRow->addWidget(new QLabel(tr("Segment (ms):"), this));
    segRow->addWidget(segmentDurationInput);
    segRow->addStretch();
    segRow->addWidget(sendSignalButton);

    auto *circRow = new QHBoxLayout;
    circularBufferInput = new QLineEdit(this);
    circularBufferInput->setPlaceholderText(tr("Buffer size (ms)"));
    circularBufferInput->setFixedWidth(160);
    circularBufferInput->setToolTip(tr("Circular buffer size in ms (--circular)"));
    circRow->addWidget(new QLabel(tr("Circular Buffer (ms):"), this));
    circRow->addWidget(circularBufferInput);
    circRow->addStretch();

    recOptLayout->addWidget(signalRecordingCheckbox);
    recOptLayout->addWidget(keypressRecordingCheckbox);
    recOptLayout->addWidget(splitFilesCheckbox);
    recOptLayout->addLayout(segRow);
    recOptLayout->addLayout(circRow);

    // Signal-Button erst aktiv wenn Signal-Checkbox angehakt
    connect(signalRecordingCheckbox, &QCheckBox::toggled,
            sendSignalButton, &QPushButton::setEnabled);

    m_outputTabLayout->addWidget(recOptGroup);

    m_outputTabLayout->addStretch();

    // -----------------------------------------------------------------------
    // Tab einhängen
    // -----------------------------------------------------------------------
    m_outputTab = scrollArea;
    m_tabWidget->addTab(m_outputTab, tr("Video"));

    // Einstellungen laden
    QSettings cfg(AppPaths::globalConf(), QSettings::IniFormat);
    cfg.beginGroup(AppPaths::tabGroup(m_cameraIndex));
    codecSelector->setCurrentText(
        cfg.value(settingsKey("Video/Codec"), "h264").toString());
    bitrateSpinBox->setValue(
        cfg.value(settingsKey("Video/Bitrate"), 0).toInt());
    qualitySpinBox->setValue(
        cfg.value(settingsKey("Video/Quality"), 0).toInt());
    cfg.endGroup();
    updateCodecVis();
}

void CameraInstance::setupImageTab()
{
    auto *scrollArea = new QScrollArea(this);
    auto *container  = new QWidget(scrollArea);
    auto *vbox       = new QVBoxLayout(container);
    scrollArea->setWidget(container);
    scrollArea->setWidgetResizable(true);

    // Hilfs-Lambda: Slider-Zeile (Label, Slider, LineEdit, Reset)
    auto addSliderRow = [&](QGridLayout *grid, int row,
                             const QString &labelText,
                             QSlider *slider, QLineEdit *input,
                             int sMin, int sMax, int sDef,
                             const QString &textDef, double vMin, double vMax,
                             double scale) -> QPushButton *
    {
        auto *label = new QLabel(labelText, container);
        label->setMinimumWidth(150);
        slider->setRange(sMin, sMax);
        slider->setSingleStep(1);
        slider->setValue(sDef);
        input->setValidator(new QDoubleValidator(vMin, vMax, 1, this));
        input->setText(textDef);
        input->setFixedWidth(44);
        auto *btn = new QPushButton("✕", container);
        btn->setFixedWidth(20);
        // Slider ↔ TextEdit synchronisieren
        connect(slider, &QSlider::valueChanged, input,
                [input, scale](int v) { input->setText(QString::number(v / scale, 'f', 1)); });
        connect(input, &QLineEdit::editingFinished, slider,
                [slider, input, scale]() {
            bool ok;
            double v = input->text().toDouble(&ok);
            if (ok) slider->setValue(qRound(v * scale));
        });
        // Reset
        connect(btn, &QPushButton::clicked, slider,
                [slider, sDef]() { slider->setValue(sDef); });
        grid->addWidget(label, row, 0);
        grid->addWidget(slider, row, 1);
        grid->addWidget(input,  row, 2);
        grid->addWidget(btn,    row, 3);
        return btn;
    };

    // -----------------------------------------------------------------------
    // Gruppe: Bildqualität
    // -----------------------------------------------------------------------
    auto *qualGroup  = new QGroupBox(tr("Image Quality Settings"), container);
    auto *qualLayout = new QGridLayout(qualGroup);
    qualLayout->setColumnMinimumWidth(0, 150);
    qualLayout->setColumnStretch(1, 1);

    sharpnessSlider = new QSlider(Qt::Horizontal, this);
    sharpnessInput  = new QLineEdit(this);
    addSliderRow(qualLayout, 0, tr("Sharpness:"),
                 sharpnessSlider, sharpnessInput,
                 0, 50, 10, "1.0", 0.0, 5.0, 10.0);

    brightnessSlider = new QSlider(Qt::Horizontal, this);
    brightnessInput  = new QLineEdit(this);
    addSliderRow(qualLayout, 1, tr("Brightness:"),
                 brightnessSlider, brightnessInput,
                 -10, 10, 0, "0.0", -1.0, 1.0, 10.0);

    contrastSlider = new QSlider(Qt::Horizontal, this);
    contrastInput  = new QLineEdit(this);
    addSliderRow(qualLayout, 2, tr("Contrast:"),
                 contrastSlider, contrastInput,
                 0, 50, 10, "1.0", 0.0, 5.0, 10.0);

    saturationSlider = new QSlider(Qt::Horizontal, this);
    saturationInput  = new QLineEdit(this);
    addSliderRow(qualLayout, 3, tr("Saturation:"),
                 saturationSlider, saturationInput,
                 0, 10, 10, "1.0", 0.0, 1.0, 10.0);

    vbox->addWidget(qualGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Belichtung & Gain
    // -----------------------------------------------------------------------
    auto *expGroup  = new QGroupBox(tr("Exposure & Gain"), container);
    auto *expLayout = new QGridLayout(expGroup);
    expLayout->setColumnMinimumWidth(0, 150);
    expLayout->setColumnStretch(1, 1);

    evSlider  = new QSlider(Qt::Horizontal, this);
    evInput   = new QLineEdit(this);
    addSliderRow(expLayout, 0, tr("EV:"),
                 evSlider, evInput,
                 -99, 99, 0, "0.0", -9.9, 9.9, 10.0);

    gainSlider = new QSlider(Qt::Horizontal, this);
    gainInput  = new QLineEdit(this);
    addSliderRow(expLayout, 1, tr("Analogue Gain:"),
                 gainSlider, gainInput,
                 0, 200, 0, "0.0", 0.0, 20.0, 10.0);

    awbGainRedSlider = new QSlider(Qt::Horizontal, this);
    awbGainRedInput  = new QLineEdit(this);
    awbGainRedResetButton = addSliderRow(expLayout, 2, tr("AWB Gain R:"),
                 awbGainRedSlider, awbGainRedInput,
                 0, 80, 15, "1.5", 0.0, 8.0, 10.0);

    awbGainBlueSlider = new QSlider(Qt::Horizontal, this);
    awbGainBlueInput  = new QLineEdit(this);
    awbGainBlueResetButton = addSliderRow(expLayout, 3, tr("AWB Gain B:"),
                 awbGainBlueSlider, awbGainBlueInput,
                 0, 80, 12, "1.2", 0.0, 8.0, 10.0);

    vbox->addWidget(expGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Weißabgleich (AWB) & Belichtungsmessung
    // -----------------------------------------------------------------------
    auto *awbGroup  = new QGroupBox(tr("White Balance (AWB) & Metering"), container);
    auto *awbLayout = new QGridLayout(awbGroup);
    awbLayout->setColumnMinimumWidth(0, 150);

    awbSelector = new QComboBox(this);
    awbSelector->addItems({"auto", "incandescent", "tungsten", "fluorescent",
                            "indoor", "daylight", "cloudy", "custom"});
    awbSelector->setToolTip(tr("Auto white balance mode (--awb)"));
    resetAwbButton = new QPushButton("✕", this);
    resetAwbButton->setFixedWidth(20);
    connect(resetAwbButton, &QPushButton::clicked,
            awbSelector, [this]() { awbSelector->setCurrentText("auto"); });

    meteringSelector = new QComboBox(this);
    meteringSelector->addItems({"centre", "spot", "average", "custom"});
    meteringSelector->setToolTip(tr("Exposure metering mode (--metering)"));
    meteringCustomInput = new QLineEdit(this);
    meteringCustomInput->setPlaceholderText(tr("Custom metering value"));
    meteringCustomInput->setFixedWidth(120);
    meteringCustomInput->setVisible(false);
    connect(meteringSelector, &QComboBox::currentTextChanged,
            meteringCustomInput, [this](const QString &v) {
        meteringCustomInput->setVisible(v == "custom");
    });
    meteringResetButton = new QPushButton("✕", this);
    meteringResetButton->setFixedWidth(20);
    connect(meteringResetButton, &QPushButton::clicked,
            meteringSelector, [this]() { meteringSelector->setCurrentText("centre"); });

    auto *meteringRow = new QHBoxLayout;
    meteringRow->addWidget(meteringSelector);
    meteringRow->addWidget(meteringCustomInput);
    meteringRow->addStretch();

    awbLayout->addWidget(new QLabel(tr("AWB Mode:"), container),    0, 0);

    // AWB Mode + CCM combined row
    auto *awbCcmRow = new QHBoxLayout;
    awbCcmRow->addWidget(awbSelector);
    awbCcmRow->addWidget(resetAwbButton);
    awbCcmRow->addSpacing(20);
    QLabel *ccmLabel = new QLabel(tr("CCM:"), container);
    ccmLabel->setToolTip(tr("Colour correction matrix (9 comma-separated values). Requires explicit AWB gains."));
    ccmInput = new QLineEdit(this);
    ccmInput->setFixedWidth(220);
    ccmInput->setPlaceholderText("1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0");
    ccmInput->setToolTip(tr("3x3 colour correction matrix. Example: 1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0"));
    ccmResetButton = new QPushButton("✕", this);
    ccmResetButton->setFixedWidth(20);
    ccmResetButton->setToolTip(tr("Reset CCM"));
    connect(ccmResetButton, &QPushButton::clicked, ccmInput, [this]() { ccmInput->clear(); });
    awbCcmRow->addWidget(ccmLabel);
    awbCcmRow->addWidget(ccmInput);
    awbCcmRow->addWidget(ccmResetButton);
    awbCcmRow->addStretch();  // Fill empty space when CCM is hidden or unavailable
    awbLayout->addLayout(awbCcmRow, 0, 1, 1, 2);

    awbLayout->addWidget(new QLabel(tr("Metering:"), container),    1, 0);
    awbLayout->addLayout(meteringRow,                               1, 1);
    awbLayout->addWidget(meteringResetButton,                       1, 2);

    vbox->addWidget(awbGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Verarbeitung (HDR / Denoise / Flicker / Shutter)
    // -----------------------------------------------------------------------
    auto *procGroup  = new QGroupBox(tr("Processing"), container);
    auto *procLayout = new QGridLayout(procGroup);
    procLayout->setColumnMinimumWidth(0, 150);

    hdrSelector = new QComboBox(this);
    hdrSelector->addItem("");
    hdrSelector->addItems({"off", "auto", "sensor", "single-exp"});
    hdrSelector->setToolTip(tr("HDR mode (--hdr)"));
    hdrResetButton = new QPushButton("✕", this);
    hdrResetButton->setFixedWidth(20);
    connect(hdrResetButton, &QPushButton::clicked,
            hdrSelector, [this]() { hdrSelector->setCurrentIndex(0); });

    denoiseSelector = new QComboBox(this);
    denoiseSelector->addItem("");
    denoiseSelector->addItems({"auto", "off", "cdn_off", "cdn_fast", "cdn_hq"});
    denoiseSelector->setToolTip(tr("Denoise mode (--denoise)"));
    denoiseResetButton = new QPushButton("✕", this);
    denoiseResetButton->setFixedWidth(20);
    connect(denoiseResetButton, &QPushButton::clicked,
            denoiseSelector, [this]() { denoiseSelector->setCurrentIndex(0); });

    flickerPeriodSelector = new QComboBox(this);
    flickerPeriodSelector->addItem("");
    flickerPeriodSelector->addItems({"off", "50hz", "60hz", "auto"});
    flickerPeriodSelector->setToolTip(tr("Flicker avoidance period (--flicker-period)"));
    flickerPeriodResetButton = new QPushButton("✕", this);
    flickerPeriodResetButton->setFixedWidth(20);
    connect(flickerPeriodResetButton, &QPushButton::clicked,
            flickerPeriodSelector, [this]() { flickerPeriodSelector->setCurrentIndex(0); });

    shutterInput = new QComboBox(this);
    shutterInput->setEditable(true);
    shutterInput->addItems({"", "0", "10000", "20000", "30000", "40000", "50000",
                             "100000", "200000"});
    shutterInput->setToolTip(tr("Shutter speed in µs (--shutter, 0=auto)"));
    shutterResetButton = new QPushButton("✕", this);
    shutterResetButton->setFixedWidth(20);
    connect(shutterResetButton, &QPushButton::clicked,
            shutterInput, [this]() { shutterInput->setCurrentIndex(0); });

    procLayout->addWidget(new QLabel(tr("HDR:"), container),              0, 0);
    procLayout->addWidget(hdrSelector,                                    0, 1);
    procLayout->addWidget(hdrResetButton,                                 0, 2);
    procLayout->addWidget(new QLabel(tr("Denoise:"), container),          1, 0);
    procLayout->addWidget(denoiseSelector,                                1, 1);
    procLayout->addWidget(denoiseResetButton,                             1, 2);
    procLayout->addWidget(new QLabel(tr("Flicker Period:"), container),   2, 0);
    procLayout->addWidget(flickerPeriodSelector,                          2, 1);
    procLayout->addWidget(flickerPeriodResetButton,                       2, 2);
    procLayout->addWidget(new QLabel(tr("Shutter (µs):"), container),     3, 0);
    procLayout->addWidget(shutterInput,                                   3, 1);
    procLayout->addWidget(shutterResetButton,                             3, 2);

    vbox->addWidget(procGroup);
    vbox->addStretch();

    // -----------------------------------------------------------------------
    // Tab einhängen
    // -----------------------------------------------------------------------
    m_imageTab = scrollArea;
    m_tabWidget->addTab(m_imageTab, tr("Adjust"));

    // Einstellungen laden
    QSettings cfg(AppPaths::globalConf(), QSettings::IniFormat);
    cfg.beginGroup(AppPaths::tabGroup(m_cameraIndex));
    awbSelector->setCurrentText(
        cfg.value(settingsKey("Image/AWB"), "auto").toString());
    meteringSelector->setCurrentText(
        cfg.value(settingsKey("Image/Metering"), "centre").toString());
    sharpnessSlider->setValue(
        cfg.value(settingsKey("Image/Sharpness"), 10).toInt());
    brightnessSlider->setValue(
        cfg.value(settingsKey("Image/Brightness"), 0).toInt());
    contrastSlider->setValue(
        cfg.value(settingsKey("Image/Contrast"), 10).toInt());
    saturationSlider->setValue(
        cfg.value(settingsKey("Image/Saturation"), 10).toInt());
    if (ccmInput) {
        ccmInput->setText(cfg.value(settingsKey("Image/CCM"), "").toString());
    }
    if (!m_previewLibsPath.isEmpty()) {
        // already set, may be overridden by config
    }
    cfg.endGroup();
}

void CameraInstance::setupStillTab()
{
    auto *scrollArea = new QScrollArea(this);
    auto *container  = new QWidget(scrollArea);
    auto *vbox       = new QVBoxLayout(container);
    scrollArea->setWidget(container);
    scrollArea->setWidgetResizable(true);

    // -----------------------------------------------------------------------
    // Gruppe: Aufnahme-Steuerung
    // -----------------------------------------------------------------------
    auto *capGroup  = new QGroupBox(tr("Capture Control"), container);
    auto *capLayout = new QGridLayout(capGroup);

    autofocusOnCaptureCheckbox = new QCheckBox(tr("Autofocus on Capture"), this);
    autofocusOnCaptureCheckbox->setToolTip(tr("Run AF cycle when capture is requested (--autofocus-on-capture)"));

    zslCheckbox = new QCheckBox(tr("Zero Shutter Lag"), this);
    zslCheckbox->setToolTip(tr("Enable zero shutter lag mode (--zsl)"));

    immediateCheckbox = new QCheckBox(tr("Immediate Capture"), this);
    immediateCheckbox->setToolTip(tr("Capture without waiting for preview (--immediate)"));

    framestartSpinBox = new QSpinBox(this);
    framestartSpinBox->setRange(0, 1000000);
    framestartSpinBox->setValue(0);
    framestartSpinBox->setToolTip(tr("Start frame number for numbered files (--framestart)"));

    captureControlResetButton = new QPushButton("✕", this);
    captureControlResetButton->setFixedWidth(20);
    captureControlResetButton->setToolTip(tr("Reset Capture Control to defaults"));
    connect(captureControlResetButton, &QPushButton::clicked, this, [this]() {
        autofocusOnCaptureCheckbox->setChecked(false);
        zslCheckbox->setChecked(false);
        immediateCheckbox->setChecked(false);
        framestartSpinBox->setValue(0);
        captureControlResetButton->setStyleSheet("");
    });

    capLayout->addWidget(autofocusOnCaptureCheckbox, 0, 0);
    capLayout->addWidget(zslCheckbox,                0, 1);
    capLayout->addWidget(immediateCheckbox,          1, 0);
    capLayout->addWidget(new QLabel(tr("Frame Start:"), container), 1, 1);
    capLayout->addWidget(framestartSpinBox,          1, 2);
    capLayout->addWidget(captureControlResetButton,  1, 3, Qt::AlignRight);

    vbox->addWidget(capGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Ausgabeformat
    // -----------------------------------------------------------------------
    auto *fmtGroup  = new QGroupBox(tr("Output Format"), container);
    auto *fmtLayout = new QGridLayout(fmtGroup);

    thumbLineEdit = new QLineEdit(this);
    thumbLineEdit->setPlaceholderText(tr("width:height:quality  e.g. 320:240:70"));
    thumbLineEdit->setToolTip(tr("Thumbnail in JPEG (--thumb width:height:quality)"));

    thumbResetButton = new QPushButton("✕", this);
    thumbResetButton->setFixedWidth(20);
    thumbResetButton->setToolTip(tr("Clear thumbnail spec"));
    connect(thumbResetButton, &QPushButton::clicked,
            thumbLineEdit, &QLineEdit::clear);

    restartSpinBox = new QSpinBox(this);
    restartSpinBox->setRange(0, 1000);
    restartSpinBox->setValue(0);
    restartSpinBox->setSpecialValueText(tr("None"));
    restartSpinBox->setToolTip(tr("JPEG restart interval (--restart, 0=none)"));

    restartResetButton = new QPushButton("✕", this);
    restartResetButton->setFixedWidth(20);
    connect(restartResetButton, &QPushButton::clicked,
            restartSpinBox, [this]() { restartSpinBox->setValue(0); });

    exifLineEdit = new QLineEdit(this);
    exifLineEdit->setPlaceholderText(tr("tag1=value1,tag2=value2"));
    exifLineEdit->setToolTip(tr("Custom EXIF tags (--exif)"));

    exifResetButton = new QPushButton("✕", this);
    exifResetButton->setFixedWidth(20);
    connect(exifResetButton, &QPushButton::clicked,
            exifLineEdit, &QLineEdit::clear);

    fmtLayout->addWidget(new QLabel(tr("Thumbnail:"), container),       0, 0);
    fmtLayout->addWidget(thumbLineEdit,                                  0, 1, 1, 2);
    fmtLayout->addWidget(thumbResetButton,                               0, 3);
    fmtLayout->addWidget(new QLabel(tr("Restart Interval:"), container), 1, 0);
    fmtLayout->addWidget(restartSpinBox,                                 1, 1, 1, 2);
    fmtLayout->addWidget(restartResetButton,                             1, 3);
    fmtLayout->addWidget(new QLabel(tr("EXIF Tags:"), container),        2, 0);
    fmtLayout->addWidget(exifLineEdit,                                   2, 1, 1, 2);
    fmtLayout->addWidget(exifResetButton,                                2, 3);

    vbox->addWidget(fmtGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Dateiverwaltung
    // -----------------------------------------------------------------------
    auto *fileGroup  = new QGroupBox(tr("File Management"), container);
    auto *fileLayout = new QGridLayout(fileGroup);

    latestLineEdit = new QLineEdit(this);
    latestLineEdit->setPlaceholderText(tr("Path for symlink to latest image"));
    latestLineEdit->setToolTip(tr("Create/update symlink to latest captured image (--latest)"));

    rawCheckbox = new QCheckBox(tr("Save RAW (DNG)"), this);
    rawCheckbox->setToolTip(tr("Save raw Bayer data as DNG (--raw)"));

    fileManagementResetButton = new QPushButton("✕", this);
    fileManagementResetButton->setFixedWidth(20);
    fileManagementResetButton->setToolTip(tr("Reset File Management settings"));
    connect(fileManagementResetButton, &QPushButton::clicked, this, [this]() {
        latestLineEdit->clear();
        rawCheckbox->setChecked(false);
        fileManagementResetButton->setStyleSheet("");
    });

    fileLayout->addWidget(new QLabel(tr("Latest Symlink:"), container), 0, 0);
    fileLayout->addWidget(latestLineEdit,          0, 1, 1, 2);
    fileLayout->addWidget(rawCheckbox,             1, 0);
    fileLayout->addWidget(fileManagementResetButton, 1, 3, Qt::AlignRight);

    vbox->addWidget(fileGroup);
    vbox->addStretch();

    // -----------------------------------------------------------------------
    // Tab einhängen
    // -----------------------------------------------------------------------
    m_stillTab = scrollArea;
    m_tabWidget->addTab(m_stillTab, tr("Still"));

    // Einstellungen laden
    QSettings cfg(AppPaths::globalConf(), QSettings::IniFormat);
    cfg.beginGroup(AppPaths::tabGroup(m_cameraIndex));
    autofocusOnCaptureCheckbox->setChecked(
        cfg.value(settingsKey("Still/AutofocusOnCapture"), false).toBool());
    zslCheckbox->setChecked(
        cfg.value(settingsKey("Still/ZSL"), false).toBool());
    rawCheckbox->setChecked(
        cfg.value(settingsKey("Still/Raw"), false).toBool());
    thumbLineEdit->setText(
        cfg.value(settingsKey("Still/Thumb"), "").toString());
    exifLineEdit->setText(
        cfg.value(settingsKey("Still/Exif"), "").toString());
    cfg.endGroup();
}

void CameraInstance::setupAutofocusTab()
{
    auto *scrollArea = new QScrollArea(this);
    auto *container  = new QWidget(scrollArea);
    auto *vbox       = new QVBoxLayout(container);
    scrollArea->setWidget(container);
    scrollArea->setWidgetResizable(true);

    QSettings cfg(AppPaths::globalConf(), QSettings::IniFormat);
    cfg.beginGroup(AppPaths::tabGroup(m_cameraIndex));

    // -----------------------------------------------------------------------
    // Gruppe: Autofokus-Parameter (rpicam-apps)
    // -----------------------------------------------------------------------
    auto *afGroup  = new QGroupBox(tr("Autofocus Parameters"), container);
    auto *afLayout = new QGridLayout(afGroup);
    afLayout->setColumnMinimumWidth(0, 80);

    autofocusModeSelector = new QComboBox(this);
    autofocusModeSelector->addItem("");
    autofocusModeSelector->addItems({"default", "manual", "auto",
                                      "normal", "macro", "continuous"});
    autofocusModeSelector->setToolTip(tr("Autofocus mode (--autofocus-mode)"));
    resetAutofocusModeButton = new QPushButton("✕", this);
    resetAutofocusModeButton->setFixedWidth(20);
    connect(resetAutofocusModeButton, &QPushButton::clicked,
            autofocusModeSelector, [this]() { autofocusModeSelector->setCurrentIndex(0); });

    autofocusRangeSelector = new QComboBox(this);
    autofocusRangeSelector->addItem("");
    autofocusRangeSelector->addItems({"normal", "macro", "full"});
    autofocusRangeSelector->setToolTip(tr("Autofocus range (--autofocus-range)"));
    resetAutofocusRangeButton = new QPushButton("✕", this);
    resetAutofocusRangeButton->setFixedWidth(20);
    connect(resetAutofocusRangeButton, &QPushButton::clicked,
            autofocusRangeSelector, [this]() { autofocusRangeSelector->setCurrentIndex(0); });

    autofocusSpeedSelector = new QComboBox(this);
    autofocusSpeedSelector->addItem("");
    autofocusSpeedSelector->addItems({"normal", "fast"});
    autofocusSpeedSelector->setToolTip(tr("Autofocus speed (--autofocus-speed)"));
    resetAutofocusSpeedButton = new QPushButton("✕", this);
    resetAutofocusSpeedButton->setFixedWidth(20);
    connect(resetAutofocusSpeedButton, &QPushButton::clicked,
            autofocusSpeedSelector, [this]() { autofocusSpeedSelector->setCurrentIndex(0); });

    autofocusWindowInput = new CustomLineEdit(this);
    autofocusWindowInput->setPlaceholderText("x,y,w,h  (0.0–1.0)");
    autofocusWindowInput->setFixedWidth(170);
    autofocusWindowInput->setToolTip(tr("Autofocus window region (--autofocus-window x,y,w,h)"));
    resetAutofocusWindowButton = new QPushButton("✕", this);
    resetAutofocusWindowButton->setFixedWidth(20);
    connect(resetAutofocusWindowButton, &QPushButton::clicked,
            autofocusWindowInput, &QLineEdit::clear);

    lensPositionSlider = new QSlider(Qt::Horizontal, this);
    lensPositionSlider->setRange(0, 320);
    lensPositionSlider->setValue(0);
    lensPositionSlider->setToolTip(tr("Lens position for manual focus (--lens-position)"));
    lensPositionInput = new QLineEdit(this);
    lensPositionInput->setFixedWidth(50);
    lensPositionInput->setText("0.0");
    lensPositionInput->setValidator(new QDoubleValidator(0.0, 32.0, 1, this));
    connect(lensPositionSlider, &QSlider::valueChanged, this, [this](int v) {
        lensPositionInput->setText(QString::number(v / 10.0, 'f', 1));
    });
    connect(lensPositionInput, &QLineEdit::editingFinished, this, [this]() {
        bool ok;
        double v = lensPositionInput->text().toDouble(&ok);
        if (ok) lensPositionSlider->setValue(qRound(v * 10));
    });
    resetLensPositionButton = new QPushButton("✕", this);
    resetLensPositionButton->setFixedWidth(20);
    connect(resetLensPositionButton, &QPushButton::clicked,
            lensPositionSlider, [this]() { lensPositionSlider->setValue(0); });

    afLayout->addWidget(new QLabel(tr("Mode:"), container),    0, 0);
    afLayout->addWidget(autofocusModeSelector,                 0, 1);
    afLayout->addWidget(resetAutofocusModeButton,              0, 2);
    afLayout->addWidget(new QLabel(tr("Range:"), container),   0, 3);
    afLayout->addWidget(autofocusRangeSelector,                0, 4);
    afLayout->addWidget(resetAutofocusRangeButton,             0, 5);
    afLayout->addWidget(new QLabel(tr("Speed:"), container),   1, 0);
    afLayout->addWidget(autofocusSpeedSelector,                1, 1);
    afLayout->addWidget(resetAutofocusSpeedButton,             1, 2);
    afLayout->addWidget(new QLabel(tr("Window:"), container),  1, 3);
    afLayout->addWidget(autofocusWindowInput,                  1, 4);
    afLayout->addWidget(resetAutofocusWindowButton,            1, 5);
    afLayout->addWidget(new QLabel(tr("Lens Position:"), container), 2, 0);
    afLayout->addWidget(lensPositionInput,                     2, 1);
    afLayout->addWidget(lensPositionSlider,                    2, 2, 1, 3);
    afLayout->addWidget(resetLensPositionButton,               2, 5);

    vbox->addWidget(afGroup);

    // -----------------------------------------------------------------------
    // Gruppe: V4L2-Gerät & aktuelle Position
    // -----------------------------------------------------------------------
    auto *devGroup  = new QGroupBox(tr("V4L2 Device & Current Position"), container);
    auto *devLayout = new QHBoxLayout(devGroup);

    v4l2DeviceInput = new QLineEdit(this);
    // Standardgerät aus ResourceBroker
    v4l2DeviceInput->setText(m_broker->v4l2SubDevice(m_cameraIndex));
    v4l2DeviceInput->setFixedWidth(160);
    v4l2DeviceInput->setToolTip(tr("V4L2 subdevice for manual focus (e.g. /dev/v4l-subdev0)"));

    currentFocusPositionLabel = new QLabel(tr("—"), this);
    currentFocusPositionLabel->setMinimumWidth(60);

    refreshFocusPositionButton = new QPushButton(tr("Refresh"), this);
    refreshFocusPositionButton->setFixedWidth(80);
    connect(refreshFocusPositionButton, &QPushButton::clicked,
            this, &CameraInstance::pollV4L2Controls);

    devLayout->addWidget(new QLabel(tr("V4L2 Device:"), container));
    devLayout->addWidget(v4l2DeviceInput);
    devLayout->addWidget(new QLabel(tr("Current:"), container));
    devLayout->addWidget(currentFocusPositionLabel);
    devLayout->addStretch();
    devLayout->addWidget(refreshFocusPositionButton);

    // V4L2-Polling-Timer aufbauen (Stub – wird in Phase 1.7+ aktiviert)
    v4l2PollTimer = new QTimer(this);
    v4l2PollTimer->setInterval(200);
    connect(v4l2PollTimer, &QTimer::timeout,
            this, &CameraInstance::pollV4L2Controls);

    vbox->addWidget(devGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Relativer Fokus (Near / Far)
    // -----------------------------------------------------------------------
    auto *relGroup  = new QGroupBox(tr("Relative Focus Movement"), container);
    auto *relLayout = new QVBoxLayout(relGroup);

    // Schrittweiten-Radio-Buttons (6 Werte)
    const QList<int> defaultSteps = {100, 300, 1000, 3000, 10000, 32767};
    focusStepButtonGroup = new QButtonGroup(this);
    auto *stepsRow = new QHBoxLayout;
    for (int i = 0; i < defaultSteps.size(); ++i) {
        int step = cfg.value(
            settingsKey(QStringLiteral("Focus/StepSize%1").arg(i + 1)),
            defaultSteps[i]).toInt();
        auto *rb = new QRadioButton(this);
        rb->setToolTip(tr("Step Size: %1").arg(step));
        focusStepButtonGroup->addButton(rb, i);
        stepsRow->addWidget(rb);
        if (i == 1) { rb->setChecked(true); currentFocusStepSize = step; }
    }
    stepsRow->addStretch();
    relLayout->addLayout(stepsRow);

    connect(focusStepButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, [this](int id) {
        const QList<int> defaults = {100, 300, 1000, 3000, 10000, 32767};
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        s.beginGroup(AppPaths::tabGroup(m_cameraIndex));
        int step = s.value(
            settingsKey(QStringLiteral("Focus/StepSize%1").arg(id + 1)),
            defaults.value(id, 300)).toInt();
        s.endGroup();
        currentFocusStepSize = step;
    });

    auto *btnRow = new QHBoxLayout;
    focusFarButton  = new QPushButton(tr("Far"),  this);
    focusNearButton = new QPushButton(tr("Near"), this);
    focusFarButton->setFixedWidth(80);
    focusNearButton->setFixedWidth(80);
    btnRow->addWidget(focusFarButton);
    btnRow->addWidget(focusNearButton);
    btnRow->addStretch();
    relLayout->addLayout(btnRow);

    vbox->addWidget(relGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Absoluter Fokus
    // -----------------------------------------------------------------------
    auto *absGroup  = new QGroupBox(tr("Absolute Focus Position"), container);
    auto *absLayout = new QHBoxLayout(absGroup);

    focusAbsoluteSlider = new QSlider(Qt::Horizontal, this);
    focusAbsoluteSlider->setRange(0, 65535);
    focusAbsoluteSlider->setValue(0);
    focusAbsoluteInput = new QLineEdit(this);
    focusAbsoluteInput->setFixedWidth(60);
    focusAbsoluteInput->setText("0");
    focusAbsoluteInput->setValidator(new QIntValidator(0, 65535, this));
    connect(focusAbsoluteSlider, &QSlider::valueChanged, this, [this](int v) {
        focusAbsoluteInput->setText(QString::number(v));
    });
    connect(focusAbsoluteInput, &QLineEdit::editingFinished, this, [this]() {
        bool ok;
        int v = focusAbsoluteInput->text().toInt(&ok);
        if (ok) focusAbsoluteSlider->setValue(v);
    });
    resetFocusAbsoluteButton = new QPushButton("✕", this);
    resetFocusAbsoluteButton->setFixedWidth(20);
    connect(resetFocusAbsoluteButton, &QPushButton::clicked,
            focusAbsoluteSlider, [this]() { focusAbsoluteSlider->setValue(0); });

    absLayout->addWidget(focusAbsoluteSlider, 1);
    absLayout->addWidget(focusAbsoluteInput);
    absLayout->addWidget(resetFocusAbsoluteButton);

    vbox->addWidget(absGroup);

    // -----------------------------------------------------------------------
    // Gruppe: Favoriten
    // -----------------------------------------------------------------------
    auto *favGroup  = new QGroupBox(tr("Focus Favorites"), container);
    auto *favLayout = new QVBoxLayout(favGroup);

    focusFavoritesList    = new QListWidget(this);
    focusFavoritesList->setMaximumHeight(100);
    focusFavoriteNameInput = new QLineEdit(this);
    focusFavoriteNameInput->setPlaceholderText(tr("Favorite name…"));

    saveFocusFavoriteButton   = new QPushButton(tr("Save"),   this);
    deleteFocusFavoriteButton = new QPushButton(tr("Delete"), this);
    saveFocusFavoriteButton->setFixedWidth(70);
    deleteFocusFavoriteButton->setFixedWidth(70);

    auto *favBtnRow = new QHBoxLayout;
    favBtnRow->addWidget(focusFavoriteNameInput);
    favBtnRow->addWidget(saveFocusFavoriteButton);
    favBtnRow->addWidget(deleteFocusFavoriteButton);

    connect(saveFocusFavoriteButton, &QPushButton::clicked, this, [this]() {
        QString name = focusFavoriteNameInput->text().trimmed();
        if (name.isEmpty()) return;
        int pos = focusAbsoluteSlider->value();
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        s.beginGroup(AppPaths::tabGroup(m_cameraIndex));
        s.setValue(settingsKey(QStringLiteral("FocusFav/%1").arg(name)), pos);
        s.endGroup();
        focusFavoritesList->addItem(QStringLiteral("%1 (%2)").arg(name).arg(pos));
    });
    connect(deleteFocusFavoriteButton, &QPushButton::clicked, this, [this]() {
        auto *item = focusFavoritesList->currentItem();
        if (!item) return;
        // Name ist vor ' (' zu extrahieren
        QString name = item->text().section(" (", 0, 0);
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        s.beginGroup(AppPaths::tabGroup(m_cameraIndex));
        s.remove(settingsKey(QStringLiteral("FocusFav/%1").arg(name)));
        s.endGroup();
        delete item;
    });
    connect(focusFavoritesList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *item) {
        if (!item) return;
        QString name = item->text().section(" (", 0, 0);
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        s.beginGroup(AppPaths::tabGroup(m_cameraIndex));
        int pos = s.value(settingsKey(QStringLiteral("FocusFav/%1").arg(name)), 0).toInt();
        s.endGroup();
        focusAbsoluteSlider->setValue(pos);
    });

    favLayout->addWidget(focusFavoritesList);
    favLayout->addLayout(favBtnRow);

    vbox->addWidget(favGroup);
    vbox->addStretch();

    // -----------------------------------------------------------------------
    // Tab einhängen
    // -----------------------------------------------------------------------
    m_autofocusTab = scrollArea;
    m_tabWidget->addTab(m_autofocusTab, tr("Focus"));

    // Einstellungen laden
    autofocusModeSelector->setCurrentText(
        cfg.value(settingsKey("Focus/Mode"), "").toString());
    autofocusRangeSelector->setCurrentText(
        cfg.value(settingsKey("Focus/Range"), "").toString());
    autofocusSpeedSelector->setCurrentText(
        cfg.value(settingsKey("Focus/Speed"), "").toString());
    lensPositionSlider->setValue(
        cfg.value(settingsKey("Focus/LensPosition"), 0).toInt());

    // Gespeicherte Favoriten laden (alle Keys mit Prefix "Camera0/FocusFav/")
    {
        QString prefix = settingsKey("FocusFav/");
        const QStringList allKeys = cfg.allKeys();
        for (const QString &k : allKeys) {
            if (k.startsWith(prefix)) {
                QString name = k.mid(prefix.size());
                int pos = cfg.value(k, 0).toInt();
                focusFavoritesList->addItem(
                    QStringLiteral("%1 (%2)").arg(name).arg(pos));
            }
        }
    }
    cfg.endGroup();
}

void CameraInstance::setupAudioTab()      { /* Phase 1.7 */ }
void CameraInstance::setupGstreamerTab()  { /* Phase 1.8 */ }
void CameraInstance::setupGstLaunchTab()  { /* Phase 1.8 */ }
void CameraInstance::setupInferenceTab()  { /* Phase 1.9 */ }
void CameraInstance::setupActionsTab()    { /* Phase 1.9 */ }
void CameraInstance::setupZoomTab()       { /* Phase 1.6 */ }

// ---------------------------------------------------------------------------
// Prozess-Slots (Stubs)
// ---------------------------------------------------------------------------
void CameraInstance::onProcessStarted()
{
    emit cameraStarted(m_cameraIndex);
}

void CameraInstance::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    emit cameraStopped(m_cameraIndex);
}

void CameraInstance::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    emit cameraStopped(m_cameraIndex);
}

void CameraInstance::onReadyReadStandardOutput()
{
    if (!m_cameraProcess) return;
    const QString output = QString::fromLocal8Bit(m_cameraProcess->readAllStandardOutput());
    emit logMessage(m_cameraIndex, output);
}

void CameraInstance::onReadyReadStandardError()
{
    if (!m_cameraProcess) return;
    const QString output = QString::fromLocal8Bit(m_cameraProcess->readAllStandardError());
    emit logMessage(m_cameraIndex, output);
}

// ---------------------------------------------------------------------------
// V4L2-Polling (Stub)
// ---------------------------------------------------------------------------
void CameraInstance::pollV4L2Controls()
{
    // TODO (Phase 1.6): V4L2-ioctl-Polling für Fokus-/Zoom-Position
}
