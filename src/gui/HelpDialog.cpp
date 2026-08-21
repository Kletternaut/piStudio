#include "HelpDialog.h"
#include "../Version.h"
#include "../utils/AppPaths.h"
#include "../app/AppMeta.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QPalette>
#include <QColor>
#include <QScrollArea>
#include <QGroupBox>
#include <QPixmap>
#include <QTextEdit>
#include <QComboBox>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QSysInfo>
#include <QDateTime>

HelpDialog::HelpDialog(QWidget *parent, int initialTab)
    : QDialog(parent), m_initialTab(initialTab)
{
    setWindowTitle(tr("%1 Help").arg(QLatin1String(AppMeta::NAME)));
    resize(700, 700);
    setWindowFlags(Qt::Window); // Modeless window
    setAttribute(Qt::WA_DeleteOnClose);
    setupUI();
}

void HelpDialog::scrollToEnhancedMode()
{
    if (m_guiScrollArea && m_enhancedGroup) {
        m_guiScrollArea->ensureWidgetVisible(m_enhancedGroup);
    }
}

void HelpDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QTabWidget *tabWidget = new QTabWidget(this);
    m_tabWidget = tabWidget;
    layout->addWidget(tabWidget);

    createAppHelpTab(tabWidget);
    createRpicamAppsParametersTab(tabWidget);
    createSupportTab(tabWidget);

    if (m_initialTab > 0 && m_initialTab < tabWidget->count())
        tabWidget->setCurrentIndex(m_initialTab);

    // Close button
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    layout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
}

void HelpDialog::createAppHelpTab(QTabWidget *tabWidget)
{
    QWidget *guiTab = new QWidget();
    QVBoxLayout *guiLayout = new QVBoxLayout(guiTab);
    guiLayout->setSpacing(10);

    QScrollArea *guiScrollArea = new QScrollArea();
    m_guiScrollArea = guiScrollArea;
    guiScrollArea->setWidgetResizable(true);
    guiScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *guiScrollWidget = new QWidget();
    guiScrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *guiScrollLayout = new QVBoxLayout(guiScrollWidget);

    // Header with logo and info
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(15);

    // Logo on the left
    QLabel *logoLabel = new QLabel();
    QPixmap logoPixmap(QLatin1String(AppMeta::LOGO_RESOURCE));
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(65, 65, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    headerLayout->addWidget(logoLabel, 0, Qt::AlignTop);

    // Text on the right
    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(1);

    QLabel *titleLabel = new QLabel(QString("<h3 style='margin: 0; font-size: 15pt;'>%1</h3>")
                                        .arg(QLatin1String(AppMeta::NAME)));
    titleLabel->setAlignment(Qt::AlignLeft);
    headerTextLayout->addWidget(titleLabel);

    QLabel *versionLabel = new QLabel(QString("<p style='margin: 1px 0; font-size: 10.5pt;'><b>Version:</b> %1</p>").arg(VERSION_STRING));
    versionLabel->setAlignment(Qt::AlignLeft);
    headerTextLayout->addWidget(versionLabel);

    QLabel *copyrightLabel = new QLabel("<p style='margin: 1px 0; font-size: 10.5pt;'>Copyright © 2025-2026 <b>Kletternaut</b></p>");
    copyrightLabel->setAlignment(Qt::AlignLeft);
    headerTextLayout->addWidget(copyrightLabel);

    headerTextLayout->addStretch();
    headerLayout->addLayout(headerTextLayout, 1);

    guiScrollLayout->addLayout(headerLayout);
    guiScrollLayout->addSpacing(10);

    QString groupStyle =
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}";

    // === Quick Start Group ===
    QGroupBox *quickStartGroup = new QGroupBox(tr("Quick Start"));
    quickStartGroup->setStyleSheet(groupStyle);
    QVBoxLayout *quickStartLayout = new QVBoxLayout(quickStartGroup);

    QLabel *quickStartLabel = new QLabel(
        "<ul>"
        "<li><b>Select Application:</b> Choose between rpicam-vid, rpicam-still, rpicam-raw, rpicam-jpeg, rpicam-hello</li>"
        "<li><b>Configure Settings:</b> Use the tabs to configure camera and application parameters</li>"
        "<li><b>Preview:</b> Click \"Start Preview\" to see live camera output</li>"
        "<li><b>Record/Capture:</b> Set output filename and start recording or capturing</li>"
        "</ul>"
    );
    quickStartLabel->setWordWrap(true);
    quickStartLayout->addWidget(quickStartLabel);

    guiScrollLayout->addWidget(quickStartGroup);

    // === Tabs Overview Group ===
    QGroupBox *tabsGroup = new QGroupBox(tr("Tabs Overview"));
    tabsGroup->setStyleSheet(groupStyle);
    QVBoxLayout *tabsLayout = new QVBoxLayout(tabsGroup);

    QTextBrowser *tabsBrowser = new QTextBrowser();
    tabsBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tabsBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tabsBrowser->setHtml(R"(
        <table border="1" cellpadding="5" style="border-collapse: collapse; width: 100%;">
            <tr style="background-color: #1e3a5f; color: white;">
                <th>Tab</th><th>Description</th>
            </tr>
            <tr><td><b>General</b></td><td>Basic camera settings: application selection, resolution, framerate, codec, timeout</td></tr>
            <tr><td><b>Video</b></td><td>Video recording: output format, bitrate, quality, segmentation, circular buffer</td></tr>
            <tr><td><b>Adjust</b></td><td>Image adjustments: white balance, exposure, HDR, denoise, sharpness, contrast, saturation</td></tr>
            <tr><td><b>Still</b></td><td>Still image capture: quality, encoding options, raw format, zero-shutter-lag</td></tr>
            <tr><td><b>Expert</b></td><td>Advanced settings: viewfinder, camera modes, tuning files, metadata, low-level parameters</td></tr>
            <tr><td><b>Audio</b></td><td>Audio recording: device selection, sample rate, channels (requires rpicam-vid)</td></tr>
            <tr><td><b>Focus</b></td><td>Autofocus control: AF modes, manual lens position, focus window, continuous AF</td></tr>
            <tr><td><b>Zoom</b></td><td>Digital zoom and Region of Interest (ROI) configuration</td></tr>
            <tr><td><b>GStreamer</b></td><td>Network streaming: RTSP/UDP streaming setup, pipeline configuration</td></tr>
            <tr><td><b>GST</b></td><td>Stream viewer: Monitor network camera streams from other devices</td></tr>
            <tr><td><b>ODR</b></td><td>Object Detection: AI-based detection using Hailo or YOLO models</td></tr>
            <tr><td><b>Action</b></td><td>Automated actions: Trigger commands based on object detection events</td></tr>
            <tr><td><b>Log</b></td><td>Debug output: Application logs, command output, diagnostic information</td></tr>
        </table>
    )");

    // Adjust size to content and prevent scrollbars
    tabsBrowser->document()->adjustSize();
    int contentHeight = tabsBrowser->document()->size().height();
    tabsBrowser->setFixedHeight(contentHeight + 5);

    tabsLayout->addWidget(tabsBrowser);

    guiScrollLayout->addWidget(tabsGroup);

    // === Keyboard Shortcuts Group ===
    QGroupBox *shortcutsGroup = new QGroupBox(tr("Keyboard Shortcuts"));
    shortcutsGroup->setStyleSheet(groupStyle);
    QVBoxLayout *shortcutsLayout = new QVBoxLayout(shortcutsGroup);

    QLabel *shortcutsLabel = new QLabel(
        "<ul>"
        "<li><b>Ctrl+0:</b> Optimal window size - automatically adjust window to fit current tab</li>"
        "</ul>"
    );
    shortcutsLabel->setWordWrap(true);
    shortcutsLayout->addWidget(shortcutsLabel);

    guiScrollLayout->addWidget(shortcutsGroup);

    // === Configuration Management Group ===
    QGroupBox *configGroup = new QGroupBox(tr("Configuration Management"));
    configGroup->setStyleSheet(groupStyle);
    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);

    QLabel *configLabel = new QLabel(
        "<ul>"
        "<li><b>Save Configuration:</b> Save current settings to a .txt file for later reuse</li>"
        "<li><b>Load Configuration:</b> Load previously saved settings</li>"
        "<li><b>Reset:</b> Reset all parameters to default values</li>"
        "</ul>"
    );
    configLabel->setWordWrap(true);
    configLayout->addWidget(configLabel);

    guiScrollLayout->addWidget(configGroup);

    // === Preview Window Group ===
    QGroupBox *previewGroup = new QGroupBox(tr("Preview Window"));
    previewGroup->setStyleSheet(groupStyle);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    QLabel *previewLabel = new QLabel(
        "<p><b>Custom Preview Position:</b> Right-click on the BoxInput field to save current preview position as default. "
        "Enable \"Use Custom Preview Geometry\" in GUI Setup to automatically use your saved position.</p>"
    );
    previewLabel->setWordWrap(true);
    previewLayout->addWidget(previewLabel);

    guiScrollLayout->addWidget(previewGroup);

    // === Tab Visibility Group ===
    QGroupBox *visibilityGroup = new QGroupBox(tr("Tab Visibility"));
    visibilityGroup->setStyleSheet(groupStyle);
    QVBoxLayout *visibilityLayout = new QVBoxLayout(visibilityGroup);

    QLabel *visibilityLabel = new QLabel(
        "<p>Use <b>GUI Setup</b> (Menu → Settings → GUI Setup) to enable/disable optional tabs: "
        "Expert, Audio, Focus, Zoom, GStreamer, GST, ODR, Action, Log</p>"
    );
    visibilityLabel->setWordWrap(true);
    visibilityLayout->addWidget(visibilityLabel);

    guiScrollLayout->addWidget(visibilityGroup);

    // === Enhanced Mode (Runtime Control) Group ===
    QGroupBox *enhancedGroup = new QGroupBox(tr("Enhanced Mode (RT)"));
    m_enhancedGroup = enhancedGroup;
    enhancedGroup->setStyleSheet(groupStyle);
    QVBoxLayout *enhancedLayout = new QVBoxLayout(enhancedGroup);

    QLabel *enhancedLabel = new QLabel(QString(
        "<p><b>Enhanced Mode</b> provides live parameter updates while the camera is running. "
        "When rpicam-vid is started via %1, a runtime control socket (rpicam-rt) is "
        "established that allows real-time adjustment of camera settings without stopping the stream.</p>"
        "<p><b>Requirement:</b> This feature needs the rpicam-apps fork with runtime control "
        "(<a href='https://github.com/Kletternaut/rpicam-apps/tree/feature/rt-roi'>feature/rt-roi</a>), "
        "built with Meson option <code>-Denable_rpicam_rt=enabled</code>. Without it, "
        "the green RT badge will not appear "
        "and live parameter updates are not available.</p>"
        "<p><b>How it works:</b></p>"
        "<ul>"
        "<li>The green <b>RT</b> label on the Start button indicates availability</li>"
        "<li>Click <b>RT</b> to open this help dialog</li>"
        "<li>Change any slider or dropdown while streaming — the value is sent to the running camera instantly</li>"
        "<li>Supported: Sharpness, EV, Gain, AWB, Brightness, Contrast, Saturation, Metering, Shutter, HDR, Denoise, ROI, Framerate</li>"
        "</ul>"
        "<p><i>This is an experimental feature using <b>/tmp/rpicam-vid{N}.sock</b> Unix domain sockets.</i></p>"
    ).arg(QLatin1String(AppMeta::NAME)));
    enhancedLabel->setWordWrap(true);
    enhancedLabel->setTextFormat(Qt::RichText);
    enhancedLabel->setOpenExternalLinks(true);
    enhancedLayout->addWidget(enhancedLabel);

    guiScrollLayout->addWidget(enhancedGroup);
    guiScrollLayout->addStretch();

    guiScrollArea->setWidget(guiScrollWidget);
    guiLayout->addWidget(guiScrollArea);

    tabWidget->addTab(guiTab, tr("%1 Help").arg(QLatin1String(AppMeta::NAME)));
}

void HelpDialog::createRpicamAppsParametersTab(QTabWidget *tabWidget)
{
    QWidget *paramsTab = new QWidget();
    QVBoxLayout *paramsLayout = new QVBoxLayout(paramsTab);
    paramsLayout->setSpacing(10);

    QScrollArea *paramsScrollArea = new QScrollArea();
    paramsScrollArea->setWidgetResizable(true);
    paramsScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *paramsScrollWidget = new QWidget();
    paramsScrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *paramsScrollLayout = new QVBoxLayout(paramsScrollWidget);

    QString groupStyle =
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}";

    // === Parameters Group ===
    QGroupBox *paramsGroup = new QGroupBox(tr("rpicam-apps Parameters"));
    paramsGroup->setStyleSheet(groupStyle);
    QVBoxLayout *paramsGroupLayout = new QVBoxLayout(paramsGroup);

    QLabel *infoLabel = new QLabel(
        "<b>All rpicam-apps Parameters</b> (99 parameters from rpicam-vid, rpicam-still, rpicam-raw, rpicam-jpeg, rpicam-hello)<br>"
        "<i>Click parameter names to open official documentation</i>"
    );
    infoLabel->setWordWrap(true);
    paramsGroupLayout->addWidget(infoLabel);

    QLineEdit *searchField = new QLineEdit();
    searchField->setPlaceholderText(tr("Search parameters..."));
    paramsGroupLayout->addWidget(searchField);

    QTableWidget *table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({tr("Parameter"), tr("Description")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    // Force header background color by setting it on the header widget directly
    table->horizontalHeader()->setAutoFillBackground(true);
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "    background-color: #1e3a5f;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    border: 1px solid #0d1f2f;"
        "}"
    );

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Define ALL parameters
    struct ParamInfo {
        QString name;
        QString description;
        QString category;
    };

    QVector<ParamInfo> allParams = {
        // Common Options
        {"timeout", "Time for which the program runs (milliseconds)", "Common"},
        {"preview", "Preview window dimensions (x,y,w,h)", "Common"},
        {"fullscreen", "Force fullscreen preview mode", "Common"},
        {"qt-preview", "Use Qt-based preview window (legacy, use --preview-backend on rpicam-apps >= 1.13)", "Common"},
        {"preview-backend", "Force a specific preview backend: wayland-egl, egl, drm, or qt", "Common"},
        {"preview-libs", "Set a custom location for the preview library .so files", "Common"},
        {"encoder-libs", "Set a custom location for the encoder library .so files", "Common"},
        {"nopreview", "Disable preview window", "Common"},
        {"info-text", "Show info text overlay on preview", "Common"},
        {"width", "Capture image width in pixels", "Common"},
        {"height", "Capture image height in pixels", "Common"},
        {"viewfinder-width", "Viewfinder width in pixels", "Common"},
        {"viewfinder-height", "Viewfinder height in pixels", "Common"},
        {"tuning-file", "Camera tuning file (JSON format)", "Common"},
        {"lores-width", "Low resolution stream width", "Common"},
        {"lores-height", "Low resolution stream height", "Common"},
        {"lores-par", "Preserve pixel aspect ratio of low-res image", "Common"},
        {"mode", "Camera sensor mode (W:H:bit-depth:packing)", "Common"},
        {"viewfinder-mode", "Viewfinder sensor mode", "Common"},
        {"buffer-count", "Number of capture buffers", "Common"},
        {"viewfinder-buffer-count", "Number of viewfinder buffers", "Common"},
        {"no-raw", "Disable RAW stream request", "Common"},
        {"post-process-file", "JSON file for post-processing", "Common"},
        {"post-process-libs", "Custom location for post-processing .so files", "Common"},

        // Transform
        {"rotation", "Image rotation (0, 180)", "Transform"},
        {"hflip", "Horizontal flip", "Transform"},
        {"vflip", "Vertical flip", "Transform"},
        {"roi", "Region of interest - digital zoom (x,y,w,h normalized)", "Transform"},

        // Camera Controls
        {"shutter", "Fixed shutter speed (microseconds)", "Camera"},
        {"analoggain", "Fixed analogue gain (synonym for gain)", "Camera"},
        {"gain", "Fixed gain value", "Camera"},
        {"metering", "Metering mode (centre, spot, average, custom)", "Camera"},
        {"exposure", "Exposure mode (normal, sport)", "Camera"},
        {"ev", "Exposure compensation in stops", "Camera"},
        {"awb", "Auto white balance mode", "Camera"},
        {"awbgains", "Manual AWB gains (red,blue)", "Camera"},
        {"ccm", "Colour correction matrix (9 comma-separated values). NOTE: must also set explicit AWB gains", "Camera"},
        {"brightness", "Brightness adjustment (-1.0 to 1.0)", "Camera"},
        {"contrast", "Contrast adjustment (1.0 = normal)", "Camera"},
        {"saturation", "Saturation adjustment (1.0 = normal, 0.0 = greyscale)", "Camera"},
        {"sharpness", "Sharpness adjustment (1.0 = normal)", "Camera"},
        {"denoise", "Denoise mode (auto, off, cdn_off, cdn_fast, cdn_hq)", "Camera"},
        {"flicker-period", "Manual flicker correction period", "Camera"},

        // Autofocus
        {"autofocus-mode", "Autofocus mode (default, manual, auto, continuous)", "Autofocus"},
        {"autofocus-range", "Focus distance range (normal, macro, full)", "Autofocus"},
        {"autofocus-speed", "Focus movement speed (normal, fast)", "Autofocus"},
        {"autofocus-window", "AF metering window (x,y,w,h normalized)", "Autofocus"},
        {"lens-position", "Manual lens position (reciprocal distance)", "Autofocus"},
        {"autofocus-on-capture", "Trigger autofocus before still capture", "Autofocus"},

        // HDR
        {"hdr", "High Dynamic Range mode (off, auto, single-exp)", "HDR"},

        // Video Encoding
        {"codec", "Video codec (h264, mjpeg, yuv420, libav)", "Video"},
        {"bitrate", "Video bitrate (bits per second)", "Video"},
        {"intra", "Intra frame period for video encoding", "Video"},
        {"profile", "H.264 profile (baseline, main, high)", "Video"},
        {"level", "H.264 level (4, 4.1, 4.2)", "Video"},
        {"inline", "Insert PPS/SPS headers with every I-frame (h264)", "Video"},
        {"framerate", "Video framerate (fps)", "Video"},
        {"frames", "Run for exact number of frames", "Video"},
        {"segment", "Split video into segments (milliseconds)", "Video"},
        {"circular", "Circular buffer size (MB) - saved on exit", "Video"},
        {"sync", "Multi-camera sync (off, server, client)", "Video"},
        {"split", "Create new file when paused/resumed", "Video"},
        {"save-pts", "Save presentation timestamps to file", "Video"},
        {"listen", "Wait for socket connection before recording", "Video"},
        {"initial", "Initial recording state (record, pause)", "Video"},
        {"keypress", "Enable keyboard shortcuts during recording", "Video"},
        {"signal", "Enable signal control during recording", "Video"},

        // Still Capture
        {"encoding", "Image encoding (jpg, png, bmp, rgb, yuv420)", "Still"},
        {"quality", "JPEG quality (0-100)", "Still"},
        {"exif", "EXIF metadata tags", "Still"},
        {"timelapse", "Timelapse interval (milliseconds)", "Still"},
        {"framestart", "Frame number to start capture", "Still"},
        {"datetime", "Add datetime to filename", "Still"},
        {"timestamp", "Add timestamp to filename", "Still"},
        {"restart", "Restart time interval (seconds)", "Still"},
        {"thumb", "Thumbnail dimensions (w:h:quality)", "Still"},
        {"raw", "Save RAW image alongside encoded image", "Still"},
        {"latest", "Create symlink to latest file", "Still"},
        {"immediate", "Start capture immediately without preview", "Still"},
        {"zsl", "Zero Shutter Lag mode", "Still"},

        // LibAV / Audio
        {"libav-format", "LibAV output format (mp4, mkv, avi)", "LibAV"},
        {"libav-audio", "Enable audio recording", "LibAV"},
        {"audio-codec", "Audio codec (aac, mp3, opus)", "LibAV"},
        {"audio-source", "Audio source (pulse, alsa)", "LibAV"},
        {"audio-device", "Audio device name", "LibAV"},
        {"audio-channels", "Number of audio channels", "LibAV"},
        {"audio-bitrate", "Audio bitrate", "LibAV"},
        {"audio-samplerate", "Audio sample rate (Hz)", "LibAV"},
        {"av-sync", "Audio/video time offset (microseconds)", "LibAV"},
        {"libav-video-codec", "LibAV video codec", "LibAV"},
        {"libav-video-codec-opts", "LibAV video codec options", "LibAV"},
        {"low-latency", "Enable low-latency encoding presets", "LibAV"},

        // Output
        {"output", "Output filename (use '-' for stdout)", "Output"},
        {"wrap", "Wrap file counter at this number", "Output"},
        {"flush", "Flush output data immediately", "Output"},

        // Camera Selection
        {"camera", "Camera index for multi-camera setups", "Camera Selection"},
        {"list-cameras", "List available cameras", "Camera Selection"},

        // Advanced
        {"verbose", "Verbose output for debugging", "Advanced"},
        {"config", "Read options from config file", "Advanced"},
        {"help", "Display help information", "Advanced"},
        {"version", "Display version information", "Advanced"},
        {"metadata", "Save metadata to file or stdout", "Advanced"},
        {"metadata-format", "Metadata format (json, txt)", "Advanced"}
    };

    // Populate table
    table->setRowCount(allParams.size());
    for (int i = 0; i < allParams.size(); ++i) {
        const auto &param = allParams[i];

        // Map parameter names to correct documentation anchors
        QString anchor = param.name;
        if (param.name == "analoggain") {
            anchor = "gain";
        } else if (param.name == "viewfinder-width" || param.name == "viewfinder-height") {
            anchor = "viewfinder-width-and-viewfinder-height";
        } else if (param.name == "width" || param.name == "height") {
            anchor = "width-and-height";
        } else if (param.name == "lores-width" || param.name == "lores-height" || param.name == "lores-par") {
            anchor = "lores-width-and-lores-height";
        }

        // Parameter name as clickable label
        QLabel *nameLabel = new QLabel(QString(
            "<a href='https://www.raspberrypi.com/documentation/computers/camera_software.html#%1' "
            "style='color: #1e3a5f; text-decoration: none; font-weight: bold;'>--%2</a>"
        ).arg(anchor, param.name));
        nameLabel->setOpenExternalLinks(true);
        nameLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        table->setCellWidget(i, 0, nameLabel);

        // Description
        QTableWidgetItem *descItem = new QTableWidgetItem(param.description);
        table->setItem(i, 1, descItem);
    }

    paramsGroupLayout->addWidget(table);

    paramsScrollLayout->addWidget(paramsGroup);

    // === Source Citation Group ===
    QGroupBox *sourceGroup = new QGroupBox(tr("Documentation Source"));
    sourceGroup->setStyleSheet(groupStyle);
    QVBoxLayout *sourceLayout = new QVBoxLayout(sourceGroup);

    QLabel *sourceLabel = new QLabel();
    sourceLabel->setText(
        "<p>Parameter documentation from "
        "<a href='https://www.raspberrypi.com/documentation/computers/camera_software.html' "
        "style='color: #1e3a5f;'>Raspberry Pi Camera Software Documentation</a><br>"
        "© 2012-2025 Raspberry Pi Ltd - Licensed under CC BY-SA 4.0</p>"
    );
    sourceLabel->setOpenExternalLinks(true);
    sourceLabel->setWordWrap(true);
    sourceLabel->setTextFormat(Qt::RichText);
    sourceLayout->addWidget(sourceLabel);

    paramsScrollLayout->addWidget(sourceGroup);

    paramsScrollArea->setWidget(paramsScrollWidget);
    paramsLayout->addWidget(paramsScrollArea);

    tabWidget->addTab(paramsTab, tr("rpicam-apps Parameters"));

    // Search functionality
    connect(searchField, &QLineEdit::textChanged, [table](const QString &text) {
        for (int i = 0; i < table->rowCount(); ++i) {
            bool visible = false;
            if (text.isEmpty()) {
                visible = true;
            } else {
                // Search in both parameter name and description
                QWidget *nameWidget = table->cellWidget(i, 0);
                QString nameText = "";
                if (QLabel *label = qobject_cast<QLabel*>(nameWidget)) {
                    nameText = label->text();
                }
                QTableWidgetItem *descItem = table->item(i, 1);
                QString descText = descItem ? descItem->text() : "";

                visible = nameText.contains(text, Qt::CaseInsensitive) ||
                         descText.contains(text, Qt::CaseInsensitive);
            }
            table->setRowHidden(i, !visible);
        }
    });
}

// ============================================================
//  collectSystemInfo - collects system information automatically
// ============================================================
QString HelpDialog::collectSystemInfo()
{
    QString info;
    QTextStream s(&info);

    s << QString("=== %1 Bug-Report ===\n").arg(QLatin1String(AppMeta::NAME));
    s << "Datum: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    s << AppMeta::NAME << " Version: " << VERSION_STRING << "\n\n";

    // Helper for external commands (optional timeout in ms)
    auto runCmd = [](const QString &prog, const QStringList &args,
                     int timeoutMs = 3000) -> QString {
        QProcess p;
        p.start(prog, args);
        p.waitForFinished(timeoutMs);
        QString out = p.readAllStandardOutput().trimmed();
        if (out.isEmpty()) out = p.readAllStandardError().trimmed();
        return out.isEmpty() ? QObject::tr("(nicht verfügbar)") : out;
    };

    // Cameras with the complete mode list - the most important info, so it goes first.
    // (--list needs a sensor configuration per camera → generous timeout.)
    s << "--- Kameras (rpicam-hello --list) ---\n";
    s << runCmd("rpicam-hello", {"--list"}, 20000) << "\n\n";

    // OS / Kernel
    s << "--- System ---\n";
    s << "OS:       " << QSysInfo::prettyProductName() << "\n";
    s << "Kernel:   " << QSysInfo::kernelVersion() << "\n";
    s << "Arch:     " << QSysInfo::currentCpuArchitecture() << "\n";
    s << "Qt:       " << qVersion() << "\n\n";

    // rpicam-apps Version
    s << "--- rpicam-apps Version ---\n";
    s << "rpicam-vid:   " << runCmd("rpicam-vid",   {"--version"}) << "\n";
    s << "rpicam-still: " << runCmd("rpicam-still", {"--version"}) << "\n\n";

    // Kameras (v4l2-Geräteknoten – ergänzend zur Modusliste oben)
    s << "--- Kameras (v4l2-ctl) ---\n";
    s << runCmd("v4l2-ctl", {"--list-devices"}) << "\n\n";

    // Speicher
    s << "--- Speicher ---\n";
    s << runCmd("free", {"-h"}) << "\n\n";

    // Session-Typ (XRDP vs. lokal, Wayland vs. X11)
    s << "--- Session ---\n";
    s << "XDG_SESSION_TYPE:      " << qgetenv("XDG_SESSION_TYPE") << "\n";
    s << "XDG_SESSION_ID:        " << qgetenv("XDG_SESSION_ID") << "\n";
    s << "XRDP_SESSION:          " << (qgetenv("XRDP_SESSION").isEmpty() ? "(nicht gesetzt)" : qgetenv("XRDP_SESSION")) << "\n";
    s << "DISPLAY:               " << qgetenv("DISPLAY") << "\n";
    s << "WAYLAND_DISPLAY:       " << (qgetenv("WAYLAND_DISPLAY").isEmpty() ? "(nicht gesetzt)" : qgetenv("WAYLAND_DISPLAY")) << "\n";
    // loginctl details for current session
    {
        QString sessionId = qgetenv("XDG_SESSION_ID");
        if (!sessionId.isEmpty()) {
            s << "--- loginctl show-session " << sessionId << " ---\n";
            s << runCmd("loginctl", {"show-session", sessionId,
                                     "-p", "Type", "-p", "Remote", "-p", "Service",
                                     "-p", "Name", "-p", "Desktop", "-p", "Seat"}) << "\n";
        }
    }
    // Display server processes
    s << "--- Display-Server Prozesse ---\n";
    s << "Xorg:      " << runCmd("pgrep", {"-a", "Xorg"}) << "\n";
    s << "Xwayland:  " << runCmd("pgrep", {"-a", "Xwayland"}) << "\n";
    s << "labwc:     " << runCmd("pgrep", {"-a", "labwc"}) << "\n";
    s << "wayfire:   " << runCmd("pgrep", {"-a", "wayfire"}) << "\n\n";

    // GPU / DRM
    s << "--- GPU / DRM ---\n";
    s << "/dev/dri:  " << runCmd("ls", {"-1", "/dev/dri"}) << "\n\n";

    return info;
}

// ============================================================
//  createSupportTab – Support- und Bug-Report-Tab
// ============================================================
void HelpDialog::createSupportTab(QTabWidget *tabWidget)
{
    QWidget *supportTab = new QWidget();
    QVBoxLayout *tabLayout = new QVBoxLayout(supportTab);
    tabLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *outerLayout = new QVBoxLayout(scrollWidget);
    outerLayout->setSpacing(8);
    outerLayout->setContentsMargins(10, 10, 10, 10);

    QString groupStyle =
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}";

    // === Description ===
    QGroupBox *infoGroup = new QGroupBox(tr("Support & Bug Report"));
    infoGroup->setStyleSheet(groupStyle);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    QLabel *infoLabel = new QLabel(
        tr("<p>Have you found a bug or have a question? Please fill out the form – Thank you.</p>"
           "<p>Directly via GitHub: "
           "<a href='%1/issues' style='color:#1e3a5f;'>Bug</a>"
           "&nbsp;&nbsp;"
           "<a href='%2/discussions' style='color:#1e3a5f;'>Discussion</a></p>")
            .arg(AppMeta::repoUrl(), AppMeta::repoUrl())
    );
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);
    outerLayout->addWidget(infoGroup);

    // === Formular ===
    QGroupBox *formGroup = new QGroupBox(tr("Bug Report / Question"));
    formGroup->setStyleSheet(groupStyle);
    QVBoxLayout *formLayout = new QVBoxLayout(formGroup);
    formLayout->setSpacing(4);
    formLayout->setContentsMargins(8, 12, 8, 8);

    // Subject
    QHBoxLayout *subjectRow = new QHBoxLayout();
    QLabel *subjectLbl = new QLabel(tr("Subject:"));
    subjectLbl->setFixedWidth(110);
    auto *subjectEdit = new QLineEdit();
    subjectEdit->setPlaceholderText(tr("Brief summary of the problem..."));
    subjectRow->addWidget(subjectLbl);
    subjectRow->addWidget(subjectEdit);
    formLayout->addLayout(subjectRow);

    // Category
    QHBoxLayout *categoryRow = new QHBoxLayout();
    QLabel *categoryLbl = new QLabel(tr("Category:"));
    categoryLbl->setFixedWidth(110);
    auto *categoryBox = new QComboBox();
    categoryBox->addItems({
        tr("Bug / Error"),
        tr("Crash"),
        tr("Feature Request"),
        tr("Question / General"),
        tr("Documentation")
    });
    categoryRow->addWidget(categoryLbl);
    categoryRow->addWidget(categoryBox);
    categoryRow->addStretch();
    formLayout->addLayout(categoryRow);

    // Problem Description
    QLabel *descLbl = new QLabel(tr("Problem Description:"));
    descLbl->setContentsMargins(0, 6, 0, 2);
    formLayout->addWidget(descLbl);
    auto *descEdit = new QTextEdit();
    descEdit->setPlaceholderText(tr("What happened? What did you expect?"));
    descEdit->setMinimumHeight(90);
    descEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    formLayout->addWidget(descEdit);

    // Steps to Reproduce
    QLabel *stepsLbl = new QLabel(tr("Steps to Reproduce:"));
    stepsLbl->setContentsMargins(0, 10, 0, 2);
    formLayout->addWidget(stepsLbl);
    auto *stepsEdit = new QTextEdit();
    stepsEdit->setPlaceholderText(tr("1. Start application\n2. Open tab X\n3. ..."));
    stepsEdit->setMinimumHeight(75);
    stepsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    formLayout->addWidget(stepsEdit);

    outerLayout->addWidget(formGroup);

    // === System Info ===
    QGroupBox *sysGroup = new QGroupBox(tr("Automatic System Information"));
    sysGroup->setStyleSheet(groupStyle);
    QVBoxLayout *sysLayout = new QVBoxLayout(sysGroup);

    auto *sysInfoEdit = new QTextEdit();
    sysInfoEdit->setReadOnly(true);
    sysInfoEdit->setFixedHeight(110);
    sysInfoEdit->setFont(QFont("Monospace", 8));
    sysInfoEdit->setPlaceholderText(tr("Will be added automatically when sending/saving..."));
    sysLayout->addWidget(sysInfoEdit);

    QPushButton *refreshSysBtn = new QPushButton(tr("Collect System Information Now"));
    refreshSysBtn->setFixedWidth(260);
    sysLayout->addWidget(refreshSysBtn, 0, Qt::AlignLeft);

    connect(refreshSysBtn, &QPushButton::clicked, this, [this, sysInfoEdit]() {
        sysInfoEdit->setPlainText(collectSystemInfo());
    });

    outerLayout->addWidget(sysGroup);

    // === Send Options ===
    QGroupBox *sendGroup = new QGroupBox(tr("Send / Save Report"));
    sendGroup->setStyleSheet(groupStyle);
    QVBoxLayout *sendLayout = new QVBoxLayout(sendGroup);

    QLabel *sendHint = new QLabel(tr(
        "<small><i>System information will be attached automatically when sending, "
        "if not already collected.</i></small>"
    ));
    sendHint->setWordWrap(true);
    sendLayout->addWidget(sendHint);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    QPushButton *emailBtn  = new QPushButton(tr("Send by E-Mail"));
    QPushButton *fileBtn   = new QPushButton(tr("Save to File"));
    QPushButton *clipBtn   = new QPushButton(tr("Copy"));

    QString btnStyle =
        "QPushButton {"
        "    background-color: #1e3a5f;"
        "    color: white;"
        "    border-radius: 4px;"
        "    padding: 0px 12px;"
        "    font-weight: bold;"
        "    min-height: 32px;"
        "    max-height: 32px;"
        "}"
        "QPushButton:hover { background-color: #2a5080; }"
        "QPushButton:pressed { background-color: #15293f; }";
    emailBtn->setStyleSheet(btnStyle);
    fileBtn->setStyleSheet(btnStyle);
    clipBtn->setStyleSheet(btnStyle);
    emailBtn->setFixedHeight(32);
    fileBtn->setFixedHeight(32);
    clipBtn->setFixedHeight(32);
    clipBtn->setToolTip(tr("Copy bug report to clipboard"));

    btnRow->addWidget(emailBtn);
    btnRow->addWidget(fileBtn);
    btnRow->addWidget(clipBtn);
    btnRow->addStretch();
    sendLayout->addLayout(btnRow);
    outerLayout->addWidget(sendGroup);

    outerLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    tabLayout->addWidget(scrollArea);

    // === Lambda: build report ===
    auto buildReport = [=]() -> QString {
        QString sysInfo = sysInfoEdit->toPlainText();
        if (sysInfo.trimmed().isEmpty())
            sysInfo = collectSystemInfo();

        QString report;
        QTextStream s(&report);
        s << "=== " << AppMeta::NAME << " " << tr("Bug Report") << " ===\n";
        s << tr("Category") << ": " << categoryBox->currentText() << "\n";
        s << tr("Subject")  << ": " << subjectEdit->text() << "\n\n";
        s << "--- " << tr("Problem Description") << " ---\n";
        s << descEdit->toPlainText() << "\n\n";
        s << "--- " << tr("Steps to Reproduce") << " ---\n";
        s << stepsEdit->toPlainText() << "\n\n";
        s << sysInfo;
        return report;
    };

    // E-Mail
    connect(emailBtn, &QPushButton::clicked, this, [=]() {
        QString report = buildReport();
        QString subject = QUrl::toPercentEncoding(
            QString("[%1] ").arg(QLatin1String(AppMeta::NAME))
            + categoryBox->currentText()
            + (subjectEdit->text().isEmpty() ? "" : ": " + subjectEdit->text()));
        QString body = QUrl::toPercentEncoding(report);
        QDesktopServices::openUrl(QUrl(
            "mailto:tomge68@gmail.com?subject=" + subject + "&body=" + body
        ));
    });

    // Save to file
    connect(fileBtn, &QPushButton::clicked, this, [=]() {
        QString defaultName = QString("%1-bugreport_")
            .arg(QLatin1String(AppMeta::BINARY))
            + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";
        QString path = QFileDialog::getSaveFileName(
            this, tr("Save Bug Report"), defaultName,
            tr("Text files (*.txt);;All files (*)"));
        if (path.isEmpty()) return;
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << buildReport();
            f.close();
            QMessageBox::information(this, tr("Saved"),
                tr("Bug report saved:\n") + path);
        } else {
            QMessageBox::warning(this, tr("Error"),
                tr("Could not save file:\n") + path);
        }
    });

    // Clipboard
    connect(clipBtn, &QPushButton::clicked, this, [=]() {
        QApplication::clipboard()->setText(buildReport());
        QMessageBox::information(this, tr("Copied"),
            tr("Bug report copied to clipboard.\n"
               "You can now paste it into an email, forum or chat."));
    });

    tabWidget->addTab(supportTab, tr("Support"));
}
