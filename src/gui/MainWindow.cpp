// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
//
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// MainWindow.cpp - Central UI hub for piStudio.

#include "MainWindow.h"
#include "DonationDialog.h"
#include "HelpDialog.h"
#include "ROIOverlay.h"
#include "SelectionOverlay.h"
#include "GuiSetupDialog.h"
#include "../utils/DebugLogger.h"
#include "../utils/AppPaths.h"
#include "../app/AppMeta.h"
#include "../app/TabRegistryService.h"
#include <cmath>
#include "../app/TabVisibilityService.h"
#include "../Version.h"
#include "Defaults.h"
#include "../modules/streaming/GstLaunchModule.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QTabWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QScrollArea>
#include <QFrame>
#include <QRegularExpression>
#include <QFileDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QStyle>
#include <QGuiApplication>
#include <QApplication>
#include <QDesktopServices>
#include <QScreen>
#include <QMenuBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QPainter>
#include <QPropertyAnimation>
#include <QGuiApplication>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include <QThread>
#include <QDateTime>
#include <QtGlobal>
#include <QtMath>
#include <QGroupBox>
#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include "CollapsibleHelper.h"
#include <QSystemTrayIcon>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QToolTip>
#include <QEvent>
#include <algorithm>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>
#include <linux/videodev2.h>
#include "V4L2Helpers.h"

// X11 includes for SOP-style mouse coordinate detection
// MUST be after Qt includes to avoid macro conflicts!
#include <QtX11Extras/QX11Info>
#include <X11/Xlib.h>
// Undef X11 macros that conflict with Qt
#undef None
#undef Bool
#undef Status
#undef True
#undef False

// Forward declarations: session-type detection (defined later in this file)
bool isNativeWaylandSession();
bool isRemoteSession();

// Utility-Funktion für Reset-Buttons
void connectResetButton(QPushButton* button, std::function<void()> resetAction, std::function<bool()> isDefault, std::vector<QPushButton*> syncButtons = {}) {
    QObject::connect(button, &QPushButton::clicked, button->parent(), [resetAction, isDefault, button, syncButtons]() {
        resetAction();
        auto updateColor = [isDefault](QPushButton* btn) {
            if (isDefault()) {
                btn->setStyleSheet("color: black;");
            } else {
                btn->setStyleSheet("color: red;");
            }
        };
        updateColor(button);
        for (auto* btn : syncButtons) {
            updateColor(btn);
        }
    });
}

// ---------------------------------------------------------------------------
// FpsComboBox::showPopup – öffnet den vertikalen Slider statt einer Item-Liste
// ---------------------------------------------------------------------------
void FpsComboBox::showPopup()
{
    if (m_slider) {
        // Unterhalb des Combos, horizontal zentriert
        QPoint globalPos = mapToGlobal(QPoint(0, height() + 2));
        globalPos.setX(globalPos.x() + (width() - m_slider->width()) / 2);
        m_slider->move(globalPos);
        m_slider->show();
        return;
    }
    QComboBox::showPopup();
}

// ---------------------------------------------------------------------------
// Slider <-> fps-Text: oberhalb von 1 ganze Zahlen (Einheit wie bisher),
// unterhalb von 1 Zehntel-Schritte (0.1 .. 0.9), 0 = Auto.
// ---------------------------------------------------------------------------
static QString fpsSliderToText(int sliderPos)
{
    if (sliderPos <= 0)
        return "0";
    if (sliderPos < 10)
        return QString::number(sliderPos / 10.0, 'f', 1); // 0.1 .. 0.9
    return QString::number(sliderPos - 9);                 // 1, 2, 3, ...
}

static int fpsTextToSlider(const QString &text)
{
    bool ok = false;
    double fps = text.trimmed().toDouble(&ok);
    if (!ok || fps <= 0.0)
        return 0;
    if (fps < 1.0)
        return qRound(fps * 10.0); // 0.1..0.9 -> 1..9
    return qRound(fps) + 9;        // 1..n -> 10..n+9
}

MainWindow::MainWindow(int cameraIndex, QWidget *parent)
    : QMainWindow(parent),
      signalRecordingCheckbox(nullptr),
      initialStateComboBox(nullptr),
      splitFilesCheckbox(nullptr),
      segmentDurationInput(nullptr),
      circularBufferInput(nullptr) {
    // MCIM: m_tabGroup VOR loadGuiConfiguration() setzen!
    m_fixedCameraIdx = cameraIndex;
    m_tabGroup = AppPaths::tabGroup(cameraIndex);

    DebugLogger::initialize("debug.log");

    // Set the application icon
    QIcon appIcon = QIcon::fromTheme(QLatin1String(AppMeta::ICON_THEME));
    setWindowIcon(appIcon);

    postProcessFileSelector = new RefreshableComboBox(this);
    postProcessFileSelector->setRefreshCallback([this]() {
        updatePostProcessFileDropdown();
    });
    resetPostProcessFileButton = new QPushButton("✕", this);
    resetPostProcessFileButton->setFixedWidth(20); // gleiche Größe wie bei den Slidern
    resetPostProcessFileButton->setToolTip(tr("Reset Auswahl"));
    qDebug() << "postProcessFileSelector:" << postProcessFileSelector;

    tuningFileSelector = new RefreshableComboBox(this);
    tuningFileSelector->setRefreshCallback([this]() {
        updateTuningFileDropdown();
    });
    resetTuningFileButton = new QPushButton("✕", this);
    resetTuningFileButton->setFixedWidth(20);
    resetTuningFileButton->setToolTip(tr("Reset Tuning File"));
    qDebug() << "tuningFileSelector:" << tuningFileSelector;
    loadGuiConfiguration();
    appSelector = new QComboBox(this);
    cameraSelector = new QComboBox(this);
    resolutionSelector = new QComboBox(this);
    framerateSelector = new FpsComboBox(this);
    formatSelector = new QComboBox(this);

    // Vertikaler Slider als Popup des fps-Dropdowns:
    // bildet Ganzzahlen von 0 bis zur maximalen Sensor-Framerate ab
    // (0 = Auto, kein --framerate-Argument).
    m_fpsSliderPopup = new FpsSliderPopup(framerateSelector);
    m_fpsSliderPopup->setWindowFlags(Qt::Popup);
    m_fpsSliderPopup->setRange(0, 39); // 0 + 9 Zehntel-Schritte + 30 fps
    m_fpsSliderPopup->setValue(39);
    m_fpsSliderPopup->setFixedSize(56, 240);
    m_fpsSliderPopup->setToolTip(tr("Framerate (fps).\n0 = Auto (no --framerate argument)."));
    framerateSelector->setPopupSlider(m_fpsSliderPopup);
    connect(m_fpsSliderPopup, &QSlider::valueChanged, this, [this](int value) {
        framerateSelector->setCurrentText(fpsSliderToText(value));
    });
    // Popup bei jedem Loslassen der Maus schließen (auch nach Klick auf
    // die Rille – sliderReleased feuert nur beim Loslassen des Handles).
    connect(m_fpsSliderPopup, &FpsSliderPopup::interactionFinished,
            m_fpsSliderPopup, &QWidget::hide);
    // Sync: manuelle Eingabe / Config-Werte auf den Slider übertragen
    connect(framerateSelector, &QComboBox::currentTextChanged, this,
            [this](const QString &text) {
        int sliderPos = fpsTextToSlider(text);
        if (sliderPos >= m_fpsSliderPopup->minimum() &&
            sliderPos <= m_fpsSliderPopup->maximum()) {
            m_fpsSliderPopup->blockSignals(true);
            m_fpsSliderPopup->setValue(sliderPos);
            m_fpsSliderPopup->blockSignals(false);
        }
    });
    previewSelector = new QComboBox(this);
    cameraInfo = new QTextEdit(this);
    parameterLayout = new QFormLayout;
    parameterWidget = new QWidget(this);
    outputLog = new QTextEdit(this);

    // Auto-Scroll Konfiguration für das Log-Fenster
    outputLog->setReadOnly(true);
    outputLog->hide(); // Hidden: shared Log tab in main.cpp handles all output

    // Log-Widget an DebugLogger übergeben (shared widget set from main.cpp takes priority)
    connect(outputLog, &QTextEdit::textChanged, this, [this]() {
        // Automatisch zur letzten Zeile scrollen
        QTextCursor cursor = outputLog->textCursor();
        cursor.movePosition(QTextCursor::End);
        outputLog->setTextCursor(cursor);
        outputLog->ensureCursorVisible();
    });

    startStopButton = new QPushButton(tr("Start"), this);

    // "RT" overlay label on the Start/Stop button (bottom-right), clickable → Help dialog
    m_controlSocketIndicator = new QLabel(startStopButton);
    m_controlSocketIndicator->setCursor(Qt::PointingHandCursor);
    m_controlSocketIndicator->setStyleSheet(
        "QLabel { color: #00e676; font-weight: bold; font-size: 7pt;"
        "background: transparent; border: none; padding: 0px 2px; }");
    m_controlSocketIndicator->setText(tr("<a style='color:#00e676; text-decoration:none;' href='#rt'>RT</a>"));
    m_controlSocketIndicator->setToolTip(tr("Enhanced Mode: Live parameter control via rpicam-rt runtime control socket. Click for details."));
    connect(m_controlSocketIndicator, &QLabel::linkActivated, this, [this](const QString &) {
        HelpDialog *d = new HelpDialog(this, 0);
        d->show();
        d->raise();
        d->activateWindow();
        QTimer::singleShot(0, d, [d]() { d->scrollToEnhancedMode(); });
    });

    // "SYNC" overlay label on the Start/Stop button (left of the RT badge).
    // Clicking toggles linked start/stop across both camera tabs.
    m_syncIndicator = new QLabel(startStopButton);
    m_syncIndicator->setCursor(Qt::PointingHandCursor);
    m_syncIndicator->setStyleSheet(
        "QLabel { font-weight: bold; font-size: 7pt;"
        "background: transparent; border: none; padding: 0px 2px; }");
    connect(m_syncIndicator, &QLabel::linkActivated, this, [this](const QString &) {
        setSyncStartStop(!m_syncStartStop);
    });
    updateSyncBadgeStyle();
    // Hidden until setSiblingWindow() links a second camera instance
    m_syncIndicator->setVisible(false);

    outputFileName = new QLineEdit(this);
    outputFileName->setFixedWidth(250);  // Same width as processing files
    browseButton = new QPushButton(tr("Browse"), this);
    browseButton->setFixedWidth(80);
    timeoutSelector = new QComboBox(this);
    timelapseInput = new QComboBox(this);
    // segmentationCheckbox entfernt – war Orphan-Widget (nicht im Layout)
    postProcessFileBrowseButton = new QPushButton(tr("Browse"), this);
    postProcessFileBrowseButton->setFixedWidth(80);

    tuningFileBrowseButton = new QPushButton(tr("Browse"), this);
    tuningFileBrowseButton->setFixedWidth(80);
    // customPreviewInput entfernt – war Orphan-Widget (nicht im Layout)
    BoxInput = new CustomLineEdit(this);
    selectionOverlay = new SelectionOverlay(nullptr, true); // SOP: true = outside mode (frame around selection!)

    awbSelector = new QComboBox(this);
    meteringSelector = new QComboBox(this);
    meteringCustomInput = new QLineEdit(this);
    loresComboBox = new LoresComboBox(this);
    hdrSelector = new QComboBox(this);
    denoiseSelector = new QComboBox(this);
    flickerPeriodSelector = new QComboBox(this);
    flickerPeriodResetButton = new QPushButton("✕", this);

    // Metadata Parameters
    metadataFileEdit = new QLineEdit(this);
    metadataFileButton = new QPushButton("📁", this);
    metadataFormatSelector = new QComboBox(this);
    metadataAutoNamingCheckbox = new QCheckBox(tr("AN"), this);
    metadataResetButton = new QPushButton("✕", this);

    // Autofocus Parameters
    autofocusModeSelector = new QComboBox(this);
    resetAutofocusModeButton = new QPushButton("✕", this);
    autofocusRangeSelector = new QComboBox(this);
    resetAutofocusRangeButton = new QPushButton("✕", this);
    autofocusSpeedSelector = new QComboBox(this);
    resetAutofocusSpeedButton = new QPushButton("✕", this);
    autofocusWindowInput = new CustomLineEdit(this);
    resetAutofocusWindowButton = new QPushButton("✕", this);
    lensPositionSlider = new QSlider(Qt::Horizontal, this);
    lensPositionInput = new QLineEdit(this);
    resetLensPositionButton = new QPushButton("✕", this);

    // Runtime Timer für Button-Text (V2-Style)
    runtimeTimer = new QTimer(this);
    connect(runtimeTimer, &QTimer::timeout, this, [this]() {
        QTime currentTime = QTime::currentTime();
        int elapsed = startTime.msecsTo(currentTime);
        QTime displayTime = QTime(0, 0).addMSecs(elapsed);
        startStopButton->setText(tr("Stop") + " " + displayTime.toString("hh:mm:ss"));
    });

    sharpnessSlider = new QSlider(Qt::Horizontal, this);
    sharpnessInput = new QLineEdit(this);
    evSlider = new QSlider(Qt::Horizontal, this);
    evInput = new QLineEdit(this);
    gainSlider = new QSlider(Qt::Horizontal, this);
    gainInput = new QLineEdit(this);
    awbGainRedSlider = new QSlider(Qt::Horizontal, this);
    awbGainRedInput = new QLineEdit(this);
    awbGainBlueSlider = new QSlider(Qt::Horizontal, this);
    awbGainBlueInput = new QLineEdit(this);
    brightnessSlider = new QSlider(Qt::Horizontal, this);
    brightnessInput = new QLineEdit(this);
    contrastSlider = new QSlider(Qt::Horizontal, this);
    contrastInput = new QLineEdit(this);
    saturationSlider = new QSlider(Qt::Horizontal, this);
    saturationInput = new QLineEdit(this);
    timestampCheckbox = new QCheckBox(tr("TS"), this);
    autoNamingCheckbox = new QCheckBox(tr("AN"), this);
    segmentPatternCheckbox = new QCheckBox("%04d", this);
    segmentPatternCheckbox->setToolTip(tr("Add segment pattern for split/segment recording"));
    segmentPatternCheckbox->setEnabled(false); // Initially disabled

    // geometryComboBox und infoTextComboBox entfernt – waren nicht im Layout
    // und verursachten schwebendes "Select option" Widget

    initializeSelectionOverlay();
    initializeROIOverlay();
    initializeBoxInput();

    // Context menu for global framerate selector
    framerateSelector->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(framerateSelector, &QComboBox::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Framerate Options"), this);

        QAction *addAction = contextMenu.addAction(tr("Add new framerate..."));
        connect(addAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newFps = QInputDialog::getText(this, tr("Add Framerate"),
                                                   tr("Enter framerate (fps):"),
                                                   QLineEdit::Normal, "", &ok);
            if (ok && !newFps.isEmpty()) {
                bool isNumber;
                newFps.toDouble(&isNumber);
                if (isNumber) {
                    // Get current custom framerates
                    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
                    settings.beginGroup(m_tabGroup);
                    QStringList customFramerates;
                    for (int i = 0; i < 10; ++i) {
                        QString key = QString("Global/CustomFramerate%1").arg(i + 1);
                        QString value = settings.value(key).toString();
                        if (!value.isEmpty()) customFramerates.append(value);
                    }

                    // Add new framerate if not exists
                    if (!customFramerates.contains(newFps)) {
                        customFramerates.append(newFps);

                        // Save to settings
                        for (int i = 0; i < 10; ++i) {
                            settings.remove(QString("Global/CustomFramerate%1").arg(i + 1));
                        }
                        for (int i = 0; i < customFramerates.size() && i < 10; ++i) {
                            settings.setValue(QString("Global/CustomFramerate%1").arg(i + 1), customFramerates[i]);
                        }

                        // Slider-Maximum erweitern und Wert übernehmen
                        settings.endGroup();
                        updateFramerateOptions(resolutionSelector->currentText());
                        framerateSelector->setCurrentText(newFps);
                    } else {
                        settings.endGroup();
                    }
                }
            }
        });

        contextMenu.addSeparator();

        QString currentValue = framerateSelector->currentText();
        QAction *deleteAction = contextMenu.addAction(tr("Delete '%1'").arg(currentValue));
        deleteAction->setEnabled(!currentValue.isEmpty());
        connect(deleteAction, &QAction::triggered, this, [this, currentValue]() {
            // Get current custom framerates
            QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
            settings.beginGroup(m_tabGroup);
            QStringList customFramerates;
            for (int i = 0; i < 10; ++i) {
                QString key = QString("Global/CustomFramerate%1").arg(i + 1);
                QString value = settings.value(key).toString();
                if (!value.isEmpty()) customFramerates.append(value);
            }

            // Remove framerate
            customFramerates.removeAll(currentValue);

            // Save to settings
            for (int i = 0; i < 10; ++i) {
                settings.remove(QString("Global/CustomFramerate%1").arg(i + 1));
            }
            for (int i = 0; i < customFramerates.size() && i < 10; ++i) {
                settings.setValue(QString("Global/CustomFramerate%1").arg(i + 1), customFramerates[i]);
            }

            // Update dropdown - trigger resolution change to reload values
            QString currentResolution = resolutionSelector->currentText();
            settings.endGroup();
            updateFramerateOptions(currentResolution);
        });

        contextMenu.exec(framerateSelector->mapToGlobal(pos));
    });

    appSelector->setToolTip(tr("Select the application to run (e.g., rpicam-still or rpicam-vid)."));
    cameraSelector->setToolTip(tr("Select the camera to use."));
    // Camera selector as narrow as possible: only the index ("1") needs space
    cameraSelector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    cameraSelector->setMinimumContentsLength(1);
    cameraSelector->setMaximumWidth(70);
    formatSelector->setToolTip(tr("Pixel format filter: 0 = Auto, 1..n = available sensor modes.\nThe mapping appears here after camera detection."));
    formatSelector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    formatSelector->setMinimumContentsLength(1);
    formatSelector->setFixedWidth(40);
    resolutionSelector->setToolTip(tr("Choose the resolution for the camera."));
    framerateSelector->setEditable(true);
    framerateSelector->setFixedWidth(60);
    framerateSelector->setToolTip(tr("Framerate (fps): vertical slider with integers from 0 to the maximum sensor framerate.\n0 = Auto (no --framerate argument).\nRight-click to add or delete custom values."));
    previewSelector->setToolTip(tr("Choose a preview mode for the camera."));
    outputFileName->setToolTip(tr("Specify the output file name."));
    browseButton->setToolTip(tr("Browse for a location to save the output file."));
    timestampCheckbox->setToolTip(tr("Enable this option to add a timestamp to the output file."));
    autoNamingCheckbox->setToolTip(tr("enable auto naming"));
    timelapseInput->setToolTip(tr("Set the interval for timelapse photography."));
    // setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), QGuiApplication::primaryScreen()->availableGeometry()));
    // This line is commented out so positioning is handled exclusively by positionAppAndPreview.

    // Cap the maximum height to avoid an overly tall window
    setMaximumHeight(950);

    // Load custom timeout values from the configuration
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    customTimeoutEntries.clear();
    for (int i = 0; i < 10; ++i) {
        QString key = QString("CustomTimeout%1").arg(i + 1);
        QString value = settings.value(key, "").toString();
        if (!value.isEmpty()) {
            customTimeoutEntries.append(value);
        }
    }
    // Wenn keine Custom-Werte vorhanden, nutze Defaults
    if (customTimeoutEntries.isEmpty()) {
        customTimeoutEntries << "0" << "5000" << "10000" << "15000" << "20000";
    }
    settings.endGroup();
    updateTimeoutSelector();
    timeoutSelector->setCurrentText("0");
    timeoutSelector->setToolTip(tr("Capture timeout in milliseconds (0 = no timeout)\nRight-click to add or delete values"));

    // Kontextmenü für Timeout-Selector
    timeoutSelector->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(timeoutSelector, &QComboBox::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Timeout Options"), this);

        QAction *addAction = contextMenu.addAction(tr("Add new value..."));
        connect(addAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newValue = QInputDialog::getText(this, tr("Add Timeout Value"),
                                                     tr("Enter timeout value (in ms):"),
                                                     QLineEdit::Normal, "", &ok);
            if (ok && !newValue.isEmpty()) {
                // Validiere dass es eine Zahl ist
                bool isNumber;
                newValue.toInt(&isNumber);
                if (isNumber && !customTimeoutEntries.contains(newValue)) {
                    customTimeoutEntries.append(newValue);
                    saveTimeoutEntries();
                    updateTimeoutSelector();
                    timeoutSelector->setCurrentText(newValue);
                }
            }
        });

        contextMenu.addSeparator();

        QString currentValue = timeoutSelector->currentText();
        QAction *deleteAction = contextMenu.addAction(tr("Delete '%1'").arg(currentValue));
        deleteAction->setEnabled(customTimeoutEntries.size() > 1); // Mindestens ein Wert muss bleiben
        connect(deleteAction, &QAction::triggered, this, [this, currentValue]() {
            if (customTimeoutEntries.size() > 1) {
                customTimeoutEntries.removeAll(currentValue);
                saveTimeoutEntries();
                updateTimeoutSelector();
                if (timeoutSelector->count() > 0) {
                    timeoutSelector->setCurrentIndex(0);
                }
            }
        });

        contextMenu.exec(timeoutSelector->mapToGlobal(pos));
    });
    appSelector->addItems({"rpicam-vid", "rpicam-jpeg", "rpicam-still", "rpicam-raw", "rpicam-hello"});

    customAppEntries.clear(); // Load custom apps from the configuration
    settings.beginGroup(m_tabGroup);
    for (int i = 0; i < 5; ++i) {
        QString key = QString("CustomApp%1").arg(i + 1);
        QString value = settings.value(key, "").toString();
        if (!value.isEmpty()) {
            customAppEntries.append(value); // Füge den Namen zur Liste hinzu
        }
    }
    settings.endGroup();
    updateAppSelector();

    // No hardcoded resolutions: the dropdown is filled exclusively from the
    // detected sensor modes (parseListCamerasOutput, runs below via
    // "rpicam-hello --list-cameras").

    previewSelector->clear();

    // Detect version early so dropdown shows the right set of items
    checkRpicamRtCapability();

    if (m_hasPreviewBackend) {
        // rpicam-apps >= 1.13: use --preview-backend
        previewSelector->addItem("", "");
        previewSelector->addItem(tr("Fullscreen"), "--fullscreen");
        previewSelector->addItem(tr("Wayland-EGL"), "wayland-egl");
        previewSelector->addItem(tr("EGL"), "egl");
        previewSelector->addItem(tr("DRM"), "drm");
        previewSelector->addItem(tr("Qt"), "qt");
        previewSelector->addItem(tr("No Preview"), "--nopreview");
        // Default: On remote sessions (XRDP/VNC/SSH) we MUST use Qt backend;
        //          on local sessions leave empty (auto-detect from compositor/GPU).
        if (isRemoteSession()) {
            previewSelector->setCurrentIndex(previewSelector->findData("qt"));
        } else {
            previewSelector->setCurrentIndex(0); // empty = auto-detect
        }
    } else {
        // rpicam-apps < 1.13: legacy --qt-preview
        previewSelector->addItem("", "");
        previewSelector->addItem(tr("Fullscreen"), "--fullscreen");
        previewSelector->addItem(tr("Qt-Preview"), "--qt-preview");
        previewSelector->addItem(tr("No Preview"), "--nopreview");
        // Default: On remote sessions (XRDP/VNC/SSH) we MUST use --qt-preview;
        //          on local sessions leave empty (auto-detect).
        if (isRemoteSession()) {
            previewSelector->setCurrentIndex(previewSelector->findData("--qt-preview"));
        } else {
            previewSelector->setCurrentIndex(0); // empty = auto-detect
        }
    }
    postProcessFileSelector->setEditable(true); // Allow custom user entries
    postProcessFileSelector->setCurrentText(""); // Default: empty

    tuningFileSelector->setEditable(true); // Allow custom user entries
    tuningFileSelector->setCurrentText(""); // Default: empty

    mainLayout = new QVBoxLayout;

    // Create tuningFileLayout and postProcessLayout first (needed for codecFilesGroup)
    auto *tuningFileLayout = new QHBoxLayout;
    tuningFileLayout->addWidget(new QLabel(tr("Tuning File:")));
    tuningFileSelector->setFixedWidth(250);
    tuningFileLayout->addWidget(tuningFileSelector);
    tuningFileLayout->addWidget(tuningFileBrowseButton);
    tuningFileLayout->addWidget(resetTuningFileButton);

    auto *postProcessLayout = new QHBoxLayout;
    postProcessLayout->addWidget(new QLabel(tr("Post-Process File:")));
    postProcessFileSelector->setFixedWidth(250);
    postProcessLayout->addWidget(postProcessFileSelector);
    postProcessLayout->addWidget(postProcessFileBrowseButton);
    postProcessLayout->addWidget(resetPostProcessFileButton);

    // =============================================================
    // INFO TEXT GROUP
    // =============================================================
    auto *infoTextGroup = new QGroupBox(tr("Info Text Overlay"), this);
    infoTextGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *infoTextMainLayout = new QVBoxLayout(infoTextGroup);
    // Create checkboxes (defaults marked with *)
    infoTextFrameCheckbox = new QCheckBox(tr("Frame No *"), this);
    infoTextFpsCheckbox = new QCheckBox(tr("FPS *"), this);
    infoTextExpCheckbox = new QCheckBox(tr("Exposure *"), this);
    infoTextAgCheckbox = new QCheckBox(tr("Analog Gain *"), this);
    infoTextDgCheckbox = new QCheckBox(tr("Digital Gain *"), this);
    infoTextRgCheckbox = new QCheckBox(tr("Red Gain"), this);
    infoTextBgCheckbox = new QCheckBox(tr("Blue Gain"), this);
    infoTextFocusCheckbox = new QCheckBox(tr("Focus"), this);
    infoTextAelockCheckbox = new QCheckBox(tr("AE Lock"), this);
    infoTextLpCheckbox = new QCheckBox(tr("Lens Pos"), this);
    infoTextAfstateCheckbox = new QCheckBox(tr("AF State"), this);

    // Add tooltips
    infoTextFrameCheckbox->setToolTip(tr("Frame number (%frame)"));
    infoTextFpsCheckbox->setToolTip(tr("Framerate (%fps)"));
    infoTextExpCheckbox->setToolTip(tr("Shutter speed (%exp)"));
    infoTextAgCheckbox->setToolTip(tr("Analogue gain (%ag)"));
    infoTextDgCheckbox->setToolTip(tr("Digital gain (%dg)"));
    infoTextRgCheckbox->setToolTip(tr("Red colour gain (%rg)"));
    infoTextBgCheckbox->setToolTip(tr("Blue colour gain (%bg)"));
    infoTextFocusCheckbox->setToolTip(tr("Focus FoM value (%focus)"));
    infoTextAelockCheckbox->setToolTip(tr("AE locked status (%aelock)"));
    infoTextLpCheckbox->setToolTip(tr("Lens position (%lp)"));
    infoTextAfstateCheckbox->setToolTip(tr("AF state (%afstate)"));

    // Grid Layout für Checkboxen (3 Spalten) + Reset button in letzter Zeile
    auto *infoTextGridLayout = new QGridLayout;
    infoTextGridLayout->addWidget(infoTextFrameCheckbox, 0, 0);
    infoTextGridLayout->addWidget(infoTextFpsCheckbox, 0, 1);
    infoTextGridLayout->addWidget(infoTextExpCheckbox, 0, 2);
    infoTextGridLayout->addWidget(infoTextAgCheckbox, 1, 0);
    infoTextGridLayout->addWidget(infoTextDgCheckbox, 1, 1);
    infoTextGridLayout->addWidget(infoTextRgCheckbox, 1, 2);
    infoTextGridLayout->addWidget(infoTextBgCheckbox, 2, 0);
    infoTextGridLayout->addWidget(infoTextFocusCheckbox, 2, 1);
    infoTextGridLayout->addWidget(infoTextAelockCheckbox, 2, 2);
    infoTextGridLayout->addWidget(infoTextLpCheckbox, 3, 0);
    infoTextGridLayout->addWidget(infoTextAfstateCheckbox, 3, 1);

    // Reset button in der letzten Zeile, rechts
    auto *infoTextResetButton = new QPushButton("✕", this);
    infoTextResetButton->setFixedWidth(20);
    infoTextResetButton->setToolTip(tr("Reset all info text options"));
    infoTextGridLayout->addWidget(infoTextResetButton, 3, 2, Qt::AlignRight);

    infoTextMainLayout->addLayout(infoTextGridLayout);

    // Connect reset button
    connect(infoTextResetButton, &QPushButton::clicked, this, [this, infoTextResetButton]() {
        if (infoTextFrameCheckbox) infoTextFrameCheckbox->setChecked(false);
        if (infoTextFpsCheckbox) infoTextFpsCheckbox->setChecked(false);
        if (infoTextExpCheckbox) infoTextExpCheckbox->setChecked(false);
        if (infoTextAgCheckbox) infoTextAgCheckbox->setChecked(false);
        if (infoTextDgCheckbox) infoTextDgCheckbox->setChecked(false);
        if (infoTextRgCheckbox) infoTextRgCheckbox->setChecked(false);
        if (infoTextBgCheckbox) infoTextBgCheckbox->setChecked(false);
        if (infoTextFocusCheckbox) infoTextFocusCheckbox->setChecked(false);
        if (infoTextAelockCheckbox) infoTextAelockCheckbox->setChecked(false);
        if (infoTextLpCheckbox) infoTextLpCheckbox->setChecked(false);
        if (infoTextAfstateCheckbox) infoTextAfstateCheckbox->setChecked(false);
        infoTextResetButton->setStyleSheet("");
    });

    // Connect all checkboxes to update reset button color
    auto updateInfoTextResetColor = [infoTextResetButton, this]() {
        bool anyChecked = (infoTextFrameCheckbox && infoTextFrameCheckbox->isChecked()) ||
                          (infoTextFpsCheckbox && infoTextFpsCheckbox->isChecked()) ||
                          (infoTextExpCheckbox && infoTextExpCheckbox->isChecked()) ||
                          (infoTextAgCheckbox && infoTextAgCheckbox->isChecked()) ||
                          (infoTextDgCheckbox && infoTextDgCheckbox->isChecked()) ||
                          (infoTextRgCheckbox && infoTextRgCheckbox->isChecked()) ||
                          (infoTextBgCheckbox && infoTextBgCheckbox->isChecked()) ||
                          (infoTextFocusCheckbox && infoTextFocusCheckbox->isChecked()) ||
                          (infoTextAelockCheckbox && infoTextAelockCheckbox->isChecked()) ||
                          (infoTextLpCheckbox && infoTextLpCheckbox->isChecked()) ||
                          (infoTextAfstateCheckbox && infoTextAfstateCheckbox->isChecked());
        infoTextResetButton->setStyleSheet(anyChecked ? "color: red;" : "");
    };

    // Connect checkboxes to update reset button color
    connect(infoTextFrameCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextFpsCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextExpCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextAgCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextDgCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextRgCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextBgCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextFocusCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextAelockCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextLpCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });
    connect(infoTextAfstateCheckbox, &QCheckBox::toggled, this, [updateInfoTextResetColor](bool) { updateInfoTextResetColor(); });

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(infoTextGroup, "UI/General/InfoTextGroup", [this]() { adjustWindowToOptimalSize(); }));

    // Note: infoTextGroup will be added to mainLayout later in correct order

    // =============================================================
    // GEOMETRY GROUP
    // =============================================================
    auto *geometryGroup = new QGroupBox(tr("Image Geometry & ROI"), this);
    geometryGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *geometryLayout = new QVBoxLayout(geometryGroup);

    // Transform checkboxes + ROI in one row
    auto *transformLayout = new QHBoxLayout;
    hflipCheckbox = new QCheckBox(tr("H-Flip"), this);
    vflipCheckbox = new QCheckBox(tr("V-Flip"), this);
    rotationCheckbox = new QCheckBox(tr("Rotation 180°"), this);

    hflipCheckbox->setToolTip(tr("Mirror image horizontally (--hflip)"));
    vflipCheckbox->setToolTip(tr("Mirror image vertically (--vflip)"));
    rotationCheckbox->setToolTip(tr("Rotate image 180 degrees (--rotation 180)"));

    roiInput = new CustomLineEdit(this);
    roiInput->setPlaceholderText("0.0,0.0,1.0,1.0");
    roiInput->setToolTip(tr("Region of Interest (x,y,width,height) as decimal values 0.0-1.0. Double-click to start interactive selection in preview window (aspect ratio locked to video resolution)."));
    roiInput->setFixedWidth(200);

    roiResetButton = new QPushButton("✕", this);
    roiResetButton->setFixedWidth(20);
    roiResetButton->setToolTip(tr("Reset geometry transforms and ROI"));

    QLabel *roiLabel = new QLabel(tr("ROI:"), this);

    transformLayout->addWidget(hflipCheckbox);
    transformLayout->addWidget(vflipCheckbox);
    transformLayout->addWidget(rotationCheckbox);
    transformLayout->addStretch();
    transformLayout->addWidget(roiLabel);
    transformLayout->addWidget(roiInput);
    transformLayout->addWidget(roiResetButton);

    geometryLayout->addLayout(transformLayout);

    // Combined reset: geometry transforms + ROI
    auto *geometryTransformResetButton = roiResetButton;
    connect(geometryTransformResetButton, &QPushButton::clicked, this, [this, geometryTransformResetButton]() {
        if (hflipCheckbox) hflipCheckbox->setChecked(false);
        if (vflipCheckbox) vflipCheckbox->setChecked(false);
        if (rotationCheckbox) rotationCheckbox->setChecked(false);
        if (roiInput) roiInput->clear();
        geometryTransformResetButton->setStyleSheet("");
    });

    // Connect checkboxes to update reset button color
    auto updateGeometryTransformResetColor = [geometryTransformResetButton, this]() {
        bool anyChecked = (hflipCheckbox && hflipCheckbox->isChecked()) ||
                          (vflipCheckbox && vflipCheckbox->isChecked()) ||
                          (rotationCheckbox && rotationCheckbox->isChecked());
        geometryTransformResetButton->setStyleSheet(anyChecked ? "color: red;" : "");
        if (!isInitializing) updateGlobalResetButtonColor();
    };

    connect(hflipCheckbox, &QCheckBox::toggled, this, updateGeometryTransformResetColor);
    connect(vflipCheckbox, &QCheckBox::toggled, this, updateGeometryTransformResetColor);
    connect(rotationCheckbox, &QCheckBox::toggled, this, updateGeometryTransformResetColor);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(geometryGroup, "UI/General/GeometryGroup", [this]() { adjustWindowToOptimalSize(); }));

    // Note: geometryGroup will be added to mainLayout later in correct order

    // =============================================================
    // PROCESSING FILES GROUP
    // =============================================================
    auto *processingFilesGroup = new QGroupBox(tr("Processing Files"), this);
    processingFilesGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *processingFilesLayout = new QVBoxLayout(processingFilesGroup);

    // Tuning File Layout
    processingFilesLayout->addLayout(tuningFileLayout);

    // Post Process Layout
    processingFilesLayout->addLayout(postProcessLayout);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(processingFilesGroup, "UI/General/ProcessingFilesGroup", [this]() { adjustWindowToOptimalSize(); }));

    // Note: processingFilesGroup will be added to mainLayout later in correct order

    // Input-Layout wird jetzt im globalen Bereich angezeigt
    BoxInput->setPlaceholderText(tr("Double-click to toggle overlay"));
    BoxInput->setToolTip(tr("Double-click to toggle the overlay visibility."));
    BoxInput->setFixedWidth(150); // set overlay input field width to 150px

    // Overlay wird jetzt im globalen Bereich angezeigt
    doubleSizeCheckbox = new QCheckBox(tr("x2"), this);
    doubleSizeCheckbox->setToolTip(tr("Double the overlay size (preview area x2)"));
    auto *resetButton = new QPushButton("✕", this);
    resetButton->setToolTip(tr("Reset overlay to default position and size"));
    overlayResetButton = resetButton;
    resetButton->setFixedWidth(20);

    // Output-Layout Optionen
    auto *resetOutputFileButton = new QPushButton("✕", this);
    this->resetOutputFileButton = resetOutputFileButton; // Referenz speichern
    resetOutputFileButton->setFixedWidth(20);
    resetOutputFileButton->setToolTip(tr("Reset Output File"));

    // Output Mode Selection (File vs. GStreamer vs. TCP vs. UDP)
    outputModeFile = new QRadioButton(tr("File"), this);
    outputModeGStreamer = new QRadioButton("GStreamer", this);
    outputModeTCP = new QRadioButton("TCP", this);
    outputModeUDP = new QRadioButton("UDP", this);

    // Create button group for exclusive selection
    outputModeGroup = new QButtonGroup(this);
    outputModeGroup->addButton(outputModeFile);
    outputModeGroup->addButton(outputModeGStreamer);
    outputModeGroup->addButton(outputModeTCP);
    outputModeGroup->addButton(outputModeUDP);

    outputModeFile->setChecked(true); // Default: File output
    outputModeFile->setToolTip(tr("Output to file (standard file recording)"));
    outputModeGStreamer->setToolTip(tr("Output to GStreamer pipeline (streaming)"));
    outputModeTCP->setToolTip(tr("Output to TCP stream: tcp://0.0.0.0:8888?listen=1"));
    outputModeUDP->setToolTip(tr("Output to UDP stream: udp://127.0.0.1:8888"));

    auto *outputModeLayout = new QHBoxLayout;
    auto *outputModeLabel = new QLabel(tr("Output Mode:"));
    outputModeLabel->setFixedWidth(100);
    outputModeLayout->addWidget(outputModeLabel);
    outputModeLayout->addWidget(outputModeFile);

    // Container for streaming modes (GStreamer, TCP, UDP) - will be hidden for still apps
    streamingModesWidget = new QWidget(this);
    auto *streamingLayout = new QHBoxLayout(streamingModesWidget);
    streamingLayout->setContentsMargins(0, 0, 0, 0);
    streamingLayout->addWidget(outputModeGStreamer);
    streamingLayout->addWidget(outputModeTCP);
    streamingLayout->addWidget(outputModeUDP);
    outputModeLayout->addWidget(streamingModesWidget);

    // Spacer zwischen File/Streaming und Encoding (wird bei Still-Apps sichtbar)
    outputModeLayout->addStretch(0);

    // Encoding Selector (nur für rpicam-still/jpeg)
    auto *encodingLabel = new QLabel(tr("Encoding:"));
    encodingLabel->setFixedWidth(100); // Same width as "Output Mode:" label

    encodingSelector = new QComboBox(this);
    encodingSelector->addItem("JPEG", "jpg");
    encodingSelector->addItem("PNG", "png");
    encodingSelector->addItem("BMP", "bmp");
    encodingSelector->addItem(tr("RGB (24-bit)"), "rgb");
    encodingSelector->addItem("RGB24", "rgb24");
    encodingSelector->addItem(tr("RGB48 (48-bit)"), "rgb48");
    encodingSelector->addItem("YUV420", "yuv420");
    encodingSelector->setCurrentIndex(0); // Default: JPEG
    encodingSelector->setFixedWidth(150);
    encodingSelector->setToolTip(tr("Image encoding format. File extension will update automatically."));

    encodingResetButton = new QPushButton("✕", this);
    encodingResetButton->setFixedWidth(20);
    encodingResetButton->setToolTip(tr("Reset Encoding to JPEG"));

    // Container Widget für Encoding (right-aligned: stretch, then label, dropdown and reset)
    encodingWidget = new QWidget(this);
    auto *encodingLayout = new QHBoxLayout(encodingWidget);
    encodingLayout->setContentsMargins(0, 0, 0, 0);
    encodingLayout->addStretch(); // Push everything to the right
    encodingLayout->addWidget(encodingLabel);
    encodingLayout->addSpacing(78); // Fixed spacing between label and combobox
    encodingLayout->addWidget(encodingSelector);
    encodingLayout->addWidget(encodingResetButton);
    encodingWidget->setVisible(false); // Default hidden

    outputModeLayout->addWidget(encodingWidget); // Add to same row as Output Mode

    // No stretch after encoding - it should stay right-aligned

    // Connect TCP/UDP mode changes to set output automatically
    connect(outputModeTCP, &QRadioButton::toggled, this, [this, resetOutputFileButton](bool checked) {
        if (checked) {
            outputFileName->setText("tcp://0.0.0.0:8888?listen=1");
            // Enable output controls for TCP (like File mode)
            outputFileName->setEnabled(true);
            browseButton->setEnabled(false); // Browse macht bei TCP keinen Sinn
            autoNamingCheckbox->setEnabled(false);
            timestampCheckbox->setEnabled(false);
            segmentPatternCheckbox->setEnabled(false);
            resetOutputFileButton->setEnabled(true);
            if (gstreamerTab) gstreamerTab->setEnabled(false);
        }
    });
    connect(outputModeUDP, &QRadioButton::toggled, this, [this, resetOutputFileButton](bool checked) {
        if (checked) {
            outputFileName->setText("udp://127.0.0.1:8888");
            // Enable output controls for UDP (like File mode)
            outputFileName->setEnabled(true);
            browseButton->setEnabled(false); // Browse macht bei UDP keinen Sinn
            autoNamingCheckbox->setEnabled(false);
            timestampCheckbox->setEnabled(false);
            segmentPatternCheckbox->setEnabled(false);
            resetOutputFileButton->setEnabled(true);
            if (gstreamerTab) gstreamerTab->setEnabled(false);
        }
    });

    // Connect Output Mode changes to enable/disable controls and tab
    connect(outputModeFile, &QRadioButton::toggled, this, [this, resetOutputFileButton](bool checked) {
        if (checked) {
            // Enable/disable Output File controls
            outputFileName->setEnabled(true);
            browseButton->setEnabled(true);
            autoNamingCheckbox->setEnabled(true);
            timestampCheckbox->setEnabled(true);
            segmentPatternCheckbox->setEnabled(true);
            resetOutputFileButton->setEnabled(true);

            // Disable GStreamer Tab
            if (gstreamerTab) {
                gstreamerTab->setEnabled(false);
            }
        }
    });

    // Connect GStreamer mode to disable output controls and enable GStreamer tab
    connect(outputModeGStreamer, &QRadioButton::toggled, this, [this, resetOutputFileButton](bool checked) {
        if (checked) {
            outputFileName->setEnabled(false);
            browseButton->setEnabled(false);
            autoNamingCheckbox->setEnabled(false);
            timestampCheckbox->setEnabled(false);
            segmentPatternCheckbox->setEnabled(false);
            resetOutputFileButton->setEnabled(false);

            // Enable GStreamer Tab and make it visible
            if (gstreamerTab) {
                gstreamerTab->setEnabled(true);
                // Force tab to be visible by enabling it in settings
                QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
                settings.beginGroup(m_tabGroup);
                settings.setValue("gstreamerTabEnabled", true);
                settings.endGroup();
                // Reorder tabs to show GStreamer tab
                tabVisibilityService->updateGstreamerTabVisibility();
                // Switch to GStreamer tab
                if (tabWidget) {
                    int gstIndex = tabWidget->indexOf(gstreamerTab);
                    if (gstIndex >= 0) {
                        tabWidget->setCurrentIndex(gstIndex);
                    }
                }
            }

            // Auto-select libav codec for GStreamer output
            if (codecSelector) {
                codecSelector->setCurrentText("libav");
            }
        }
    });

    auto *outputLayout = new QHBoxLayout;
    auto *outputFileLabel = new QLabel(tr("Output File:"));
    outputFileLabel->setFixedWidth(100);
    outputLayout->addWidget(outputFileLabel);
    outputLayout->addWidget(autoNamingCheckbox);
    //outputLayout->addWidget(segmentationCheckbox);
    outputLayout->addWidget(timestampCheckbox);
    outputLayout->addWidget(segmentPatternCheckbox);
    outputLayout->addWidget(outputFileName);
    outputLayout->addWidget(browseButton);
    outputLayout->addWidget(resetOutputFileButton);

    updateResetButtonColor(resetOutputFileButton, !outputFileName->text().isEmpty(), 0);

    // Timeout-Reset-Button korrekt initialisieren und im Layout platzieren
    timeoutResetButton = new QPushButton("✕", this);
    timeoutResetButton->setFixedWidth(20);
    timeoutResetButton->setToolTip(tr("Reset Timeout"));

    // ====== JETZT KÖNNEN WIR DIE WIDGETS KONFIGURIEREN (NACH DER ERSTELLUNG!) ======

    // Shutter Slider (logarithmic scale, created with other Adjust sliders)

    // Lade benutzerdefinierte Timelapse-Werte aus der Konfiguration (KEINE Defaults!)
    customTimelapseEntries.clear();
    settings.beginGroup(m_tabGroup);
    for (int i = 0; i < 10; ++i) {
        QString key = QString("CustomTimelapse%1").arg(i + 1);
        QString value = settings.value(key, "").toString();
        if (!value.isEmpty()) {
            customTimelapseEntries.append(value);
        }
    }
    settings.endGroup();
    updateTimelapseSelector();
    timelapseInput->setToolTip(tr("Timelapse interval in milliseconds\nRight-click to add or delete values"));

    // Kontextmenü für Timelapse-Selector
    timelapseInput->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(timelapseInput, &QComboBox::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Timelapse Options"), this);

        QAction *addAction = contextMenu.addAction(tr("Add new value..."));
        connect(addAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newValue = QInputDialog::getText(this, tr("Add Timelapse Value"),
                                                     tr("Enter timelapse interval (in ms):"),
                                                     QLineEdit::Normal, "", &ok);
            if (ok && !newValue.isEmpty()) {
                bool isNumber;
                newValue.toInt(&isNumber);
                if (isNumber && !customTimelapseEntries.contains(newValue)) {
                    customTimelapseEntries.append(newValue);
                    saveTimelapseEntries();
                    updateTimelapseSelector();
                    timelapseInput->setCurrentText(newValue);
                }
            }
        });

        contextMenu.addSeparator();

        QString currentValue = timelapseInput->currentText();
        QAction *deleteAction = contextMenu.addAction(tr("Delete '%1'").arg(currentValue));
        deleteAction->setEnabled(!currentValue.isEmpty() && customTimelapseEntries.size() > 0);
        connect(deleteAction, &QAction::triggered, this, [this, currentValue]() {
            customTimelapseEntries.removeAll(currentValue);
            saveTimelapseEntries();
            updateTimelapseSelector();
            if (timelapseInput->count() > 0) {
                timelapseInput->setCurrentIndex(0);
            } else {
                timelapseInput->setCurrentText("");
            }
        });

        contextMenu.exec(timelapseInput->mapToGlobal(pos));
    });

    // HDR Configuration (UI in Image Tab)
    hdrSelector->addItems({"off", "auto", "sensor", "single-exp"});
    hdrSelector->setCurrentText("off");
    hdrSelector->setToolTip(tr("Enable High Dynamic Range: off, auto, sensor (for Camera Module 3), single-exp (PiSP multiframe)"));
    hdrSelector->setFixedWidth(170);

    // Denoise Configuration (UI in Image Tab)
    denoiseSelector->addItems({"auto", "off", "cdn_off", "cdn_fast", "cdn_hq"});
    denoiseSelector->setCurrentText("auto");
    denoiseSelector->setToolTip(tr("Sets the Denoise operating mode: auto, off, cdn_off, cdn_fast, cdn_hq"));
    denoiseSelector->setFixedWidth(170);

    // Flicker Period Configuration (UI in Image Tab)
    flickerPeriodSelector->addItem(tr("Off"), "off");
    flickerPeriodSelector->addItem(tr("50Hz (10000us)"), "10000us");
    flickerPeriodSelector->addItem(tr("60Hz (8333us)"), "8333us");
    flickerPeriodSelector->setCurrentIndex(0);
    flickerPeriodSelector->setEditable(true); // Allow direct editing
    flickerPeriodSelector->setToolTip(tr("Manual flicker correction period:\n"
                                       "Off: No flicker correction\n"
                                       "50Hz: Cancel 50Hz flicker (10000us)\n"
                                       "60Hz: Cancel 60Hz flicker (8333us)\n"
                                       "Custom: Type custom value (e.g. 10000us, 8500us)"));
    flickerPeriodSelector->setFixedWidth(170);

    // Autofocus Mode Konfiguration
    autofocusModeSelector->addItems({"auto", "manual", "continuous"});
    autofocusModeSelector->setCurrentText("auto"); // Auto ist der libcamera-Standard
    autofocusModeSelector->setToolTip(tr("Control to set the mode of the AF (autofocus) algorithm:\n"
                                       "manual: No AF, lens position set manually\n"
                                       "auto: Single AF scan when triggered\n"
                                       "continuous: Continuous AF (CAF)"));
    autofocusModeSelector->setFixedWidth(170);
    resetAutofocusModeButton->setFixedWidth(20);
    resetAutofocusModeButton->setToolTip(tr("Reset Autofocus Mode selection"));

    // Autofocus Range Konfiguration
    autofocusRangeSelector->addItems({"normal", "macro", "full"});
    autofocusRangeSelector->setCurrentText("normal"); // Standardwert setzen
    autofocusRangeSelector->setToolTip(tr("Set the range of focus distances that is scanned"));
    autofocusRangeSelector->setFixedWidth(170);
    resetAutofocusRangeButton->setFixedWidth(20);
    resetAutofocusRangeButton->setToolTip(tr("Reset Autofocus Range selection"));

    // Autofocus Speed Konfiguration
    autofocusSpeedSelector->addItems({"normal", "fast"});
    autofocusSpeedSelector->setCurrentText("normal"); // Standardwert setzen
    autofocusSpeedSelector->setToolTip(tr("Control that determines whether the AF algorithm is to move the lens as quickly as possible or more steadily"));
    autofocusSpeedSelector->setFixedWidth(170);
    resetAutofocusSpeedButton->setFixedWidth(20);
    resetAutofocusSpeedButton->setToolTip(tr("Reset Autofocus Speed selection"));

    // Autofocus Window Konfiguration
    autofocusWindowInput->setPlaceholderText("0.333,0.333,0.333,0.333");
    autofocusWindowInput->setToolTip(tr("Sets AfMetering to AfMeteringWindows and set region used (x,y,width,height). Uses normalized coordinates 0.0-1.0. Double-click to start interactive selection (aspect ratio locked to video resolution)."));
    autofocusWindowInput->setFixedWidth(150);
    resetAutofocusWindowButton->setFixedWidth(20);
    resetAutofocusWindowButton->setToolTip(tr("Reset Autofocus Window to middle third default"));

    // Lens Position Konfiguration
    lensPositionSlider->setRange(0, 5000); // Wertebereich von 0 bis 50.0 (multipliziert mit 100)
    lensPositionSlider->setSingleStep(1); // Schritte von 0.01 (multipliziert mit 100)
    lensPositionSlider->setValue(0); // Standardwert 0 (disabled)
    lensPositionInput->setValidator(new QDoubleValidator(0.0, 50.0, 1, this)); // Wertebereich 0.0 bis 50.0, 1 Dezimalstelle
    lensPositionInput->setText("0.0"); // Standardwert (disabled)
    lensPositionInput->setFixedWidth(40);
    // lensPositionSlider->setFixedWidth(270);  // Entfernt, damit Slider sich dynamisch ausdehnen kann
    resetLensPositionButton->setFixedWidth(20);
    resetLensPositionButton->setToolTip(tr("Reset Lens Position"));

    // Timelapse
    timelapseResetButton = new QPushButton("✕", this);
    timelapseResetButton->setFixedWidth(20);
    timelapseResetButton->setToolTip(tr("Reset Timelapse"));

    // NOTE: tuningFileLayout and postProcessLayout already created earlier (line ~263)
    // NOTE: codecLayout already created and added to codecFilesGroup (line ~392)

    // =============================================================
    // TIMINGS GROUP - 2 rows with two parameters each
    // =============================================================
    auto *jumbleGroup = new QGroupBox(tr("Timings"), this);
    jumbleGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *jumbleLayout = new QVBoxLayout(jumbleGroup);

    const int timingsLabelWidth = 150;

    // Row 0: Timeout (left) | stretch | Sync (right)
    auto *timingsRow0Layout = new QHBoxLayout;
    QLabel *timeoutLabel = new QLabel(tr("Timeout (ms):"), this);
    timeoutLabel->setMinimumWidth(timingsLabelWidth);
    timingsRow0Layout->addWidget(timeoutLabel);
    timeoutSelector->setFixedWidth(140);
    timingsRow0Layout->addWidget(timeoutSelector);
    timingsRow0Layout->addWidget(timeoutResetButton);
    timingsRow0Layout->addStretch();
    syncLabel = new QLabel(tr("Sync:"), this);
    syncLabel->setMinimumWidth(timingsLabelWidth);
    timingsRow0Layout->addWidget(syncLabel);
    syncSelector = new QComboBox(this);
    syncSelector->addItems({"off", "server", "client"});
    syncSelector->setCurrentText("off");
    syncSelector->setFixedWidth(140);
    syncSelector->setToolTip(tr("Multi-camera synchronization (--sync)\noff = no sync\nserver = this camera is sync master\nclient = sync to server camera"));
    timingsRow0Layout->addWidget(syncSelector);
    syncResetButton = new QPushButton("✕", this);
    syncResetButton->setFixedWidth(20);
    syncResetButton->setToolTip(tr("Reset Sync to off"));
    timingsRow0Layout->addWidget(syncResetButton);
    jumbleLayout->addLayout(timingsRow0Layout);

    // Row 1: Timelapse (only visible in rpicam-still / rpicam-jpeg)
    timelapseRowWidget = new QWidget(this);
    auto *timelapseRowLayout = new QHBoxLayout(timelapseRowWidget);
    timelapseRowLayout->setContentsMargins(0, 0, 0, 0);
    timelapseLabel = new QLabel(tr("Timelapse (ms):"), this);
    timelapseLabel->setMinimumWidth(timingsLabelWidth);
    timelapseRowLayout->addWidget(timelapseLabel);
    timelapseInput->setFixedWidth(140);
    timelapseRowLayout->addWidget(timelapseInput);
    timelapseRowLayout->addWidget(timelapseResetButton);
    timelapseRowLayout->addStretch();
    jumbleLayout->addWidget(timelapseRowWidget);

    // Connect sync reset button
    connect(syncResetButton, &QPushButton::clicked, this, [this]() {
        if (syncSelector) syncSelector->setCurrentText("off");
        if (syncResetButton) syncResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });

    // Connect sync selector to update reset button color
    connect(syncSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        bool isDefault = (syncSelector->currentText() == "off");
        if (syncResetButton) syncResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        if (!isInitializing) {
            updateGlobalResetButtonColor();
        }
    });

    updateTimelapseVisibility();
    connect(appSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateTimelapseVisibility);

    // Low Resolution selector - moved to Video Tab
    loresResetButton = new QPushButton("✕", this);
    loresResetButton->setFixedWidth(20);
    loresResetButton->setToolTip(tr("Reset Low Resolution selection"));
    // Widget wird jetzt im Video Tab erstellt

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(jumbleGroup, "UI/General/TimingsGroup", [this]() { adjustWindowToOptimalSize(); }));

    // Note: jumbleGroup will be added to mainLayout later in correct order

    // =============================================================
    // Output Settings Group (ganz oben im General Tab)
    // =============================================================
    auto *outputFileGroup = new QGroupBox(tr("Output Settings"), this);
    outputFileGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *outputFileGroupLayout = new QVBoxLayout;
    outputFileGroupLayout->setSpacing(5); // Smaller spacing between rows
    outputFileGroup->setLayout(outputFileGroupLayout);
    outputFileGroupLayout->addLayout(outputModeLayout);  // Output Mode Radio Buttons (includes encoding)
    outputFileGroupLayout->addLayout(outputLayout);       // Output File Settings

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(outputFileGroup, "UI/General/OutputFileGroup", [this]() {
        updateUIForApp(appSelector->currentText());
        adjustWindowToOptimalSize();
    }));

    // Metadata File Output - Custom order: Label, Format, Input, Browse, Reset
    // Only visible for rpicam-still and rpicam-jpeg
    metadataWidget = new QWidget(this);
    auto *metadataLayout = new QHBoxLayout(metadataWidget);
    metadataLayout->setContentsMargins(0, 0, 0, 0);
    auto *metadataFileLabel = new QLabel(tr("Metadata File:"), this);
    metadataFileLabel->setFixedWidth(100);
    metadataLayout->addWidget(metadataFileLabel);

    // Auto-Naming Checkbox
    metadataAutoNamingCheckbox->setToolTip(tr("Automatically use output filename with metadata extension"));
    metadataLayout->addWidget(metadataAutoNamingCheckbox);

    // Metadata Format Selector (comes after auto checkbox)
    metadataFormatSelector->addItems({"json", "txt"});
    metadataFormatSelector->setCurrentText("json");
    metadataFormatSelector->setFixedWidth(80);
    metadataFormatSelector->setToolTip(tr("Metadata format"));
    metadataLayout->addWidget(metadataFormatSelector);

    // Input Field (same width as outputFileName)
    metadataFileEdit->setPlaceholderText(tr("Save metadata to file (or '-' for stdout)"));
    metadataFileEdit->setFixedWidth(250);
    metadataLayout->addWidget(metadataFileEdit);

    // Browse Button
    metadataFileButton->setText(tr("Browse..."));
    metadataFileButton->setFixedWidth(80);
    metadataFileButton->setToolTip(tr("Select metadata output file"));
    metadataLayout->addWidget(metadataFileButton);

    // Reset Button
    metadataResetButton->setFixedWidth(20);
    metadataResetButton->setToolTip(tr("Reset Metadata settings"));
    metadataLayout->addWidget(metadataResetButton);

    // Initially hidden - will be shown when still/jpeg is selected
    metadataWidget->setVisible(false);

    outputFileGroupLayout->addWidget(metadataWidget);

    // =============================================================
    // ADD ALL GROUPS TO MAIN LAYOUT IN CORRECT ORDER
    // =============================================================
    // 1. Output File Settings (ganz oben)
    mainLayout->addWidget(outputFileGroup);
    // 2. Processing Files (Tuning & Post-Process)
    mainLayout->addWidget(processingFilesGroup);
    // 3. Jumble
    mainLayout->addWidget(jumbleGroup);
    // 4. Info Text Overlay
    mainLayout->addWidget(infoTextGroup);
    // 5. Image Geometry & ROI
    mainLayout->addWidget(geometryGroup);

    // Kamera-Erkennungsinfos erscheinen NICHT hier im General-Tab,
    // sondern unter Help -> System Information sowie im Log.
    cameraInfo->hide();
    mainLayout->addWidget(parameterWidget);
    mainLayout->addStretch();  // Prevent groups from expanding when others collapse

    // Container layout for the entire GUI
    auto *outerLayout = new QVBoxLayout;

    // Tab-Widget erstellen
    tabWidget = new QTabWidget(this);
    tabRegistryService = new TabRegistryService(tabWidget, m_tabGroup, this);
    tabVisibilityService = new TabVisibilityService(tabRegistryService, this);

    // General Tab (bisheriges Layout)
    generalTab = new QWidget();
    generalTab->setLayout(mainLayout);
    tabRegistryService->registerTab(generalTab, "General", 0, "", true);

    // ===== Video Tab (formerly Output Tab) =====
    outputTab = new QWidget;
    outputTabLayout = new QVBoxLayout(outputTab);
    tabRegistryService->registerTab(outputTab, "Video", 1, "", true);

    // Codec Group - moved from General tab
    codecGroup = new QGroupBox(tr("Codec Settings"), this);
    codecGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    auto *codecGroupLayout = new QGridLayout;
    codecGroup->setLayout(codecGroupLayout);

    // Feste Spaltenbreiten für konsistente Abstände
    codecGroupLayout->setColumnMinimumWidth(0, 150);  // Label-Spalte (Codec:, Profile:, etc.)
    codecGroupLayout->setColumnStretch(4, 1);  // Stretch-Spalte für Platz vor Reset-Button

    // Row 0: Codec + Level + Inline
    codecLabel = new QLabel(tr("Codec:"), this);
    codecGroupLayout->addWidget(codecLabel, 0, 0);

    codecSelector = new QComboBox(this);
    codecSelector->addItem(""); // Leeres Feld für "null"
    codecSelector->addItems({"h264", "mjpeg", "yuv420", "libav"});
    codecSelector->setCurrentText("h264");
    codecSelector->setToolTip(tr("Select the codec to use. Leave empty for default (null)."));
    codecGroupLayout->addWidget(codecSelector, 0, 1);

    levelLabel = new QLabel(tr("Level:"), this);
    codecGroupLayout->addWidget(levelLabel, 0, 2);
    levelSelector = new QComboBox(this);
    levelSelector->addItems({"4.0", "4.1", "4.2"});
    levelSelector->setCurrentIndex(-1);
    levelSelector->setPlaceholderText(tr("None"));
    levelSelector->setToolTip(tr("H.264 level specification (4.0, 4.1, 4.2)"));
    codecGroupLayout->addWidget(levelSelector, 0, 3);

    inlineHeadersCheckbox = new QCheckBox(tr("Inline"), this);
    inlineHeadersCheckbox->setToolTip(tr("Force PPS/SPS headers in every I-frame (--inline)"));
    codecGroupLayout->addWidget(inlineHeadersCheckbox, 0, 4);

    // Low Latency (nur für libav Codec, in Row 0 nach Inline)
    lowLatencyCheckbox = new QCheckBox(tr("Low Latency"), this);
    lowLatencyCheckbox->setToolTip(tr("Enable low latency presets for streaming (--low-latency)\nOnly works with libav codec"));
    codecGroupLayout->addWidget(lowLatencyCheckbox, 0, 5);

    // Row 1: Profile
    profileLabel = new QLabel(tr("Profile:"), this);
    codecGroupLayout->addWidget(profileLabel, 1, 0);
    profileSelector = new QComboBox(this);
    profileSelector->addItems({"baseline", "main", "high"});
    profileSelector->setCurrentIndex(-1);
    profileSelector->setPlaceholderText(tr("None"));
    profileSelector->setToolTip(tr("H.264 profile level (baseline, main, high)\nUsed for both H264 and LibAV codecs"));
    codecGroupLayout->addWidget(profileSelector, 1, 1);

    // Row 2: LibAV Format (nur für libav Codec)
    libavFormatLabel = new QLabel(tr("LibAV Format:"), this);
    codecGroupLayout->addWidget(libavFormatLabel, 2, 0);
    libavFormatSelector = new QComboBox(this);
    libavFormatSelector->addItems({"mpegts", "mp4", "avi", "flv", "mkv", "mov"});
    libavFormatSelector->setCurrentText("mpegts");
    libavFormatSelector->setToolTip(tr("LibAV output format (--libav-format)"));
    codecGroupLayout->addWidget(libavFormatSelector, 2, 1);

    // Row 3: LibAV Video Codec (nur für libav Codec)
    libavVideoCodecLabel = new QLabel(tr("LibAV Video Codec:"), this);
    codecGroupLayout->addWidget(libavVideoCodecLabel, 3, 0);
    libavVideoCodecSelector = new QComboBox(this);
    libavVideoCodecSelector->addItems({"h264_v4l2m2m", "libx264", "libx265", "vp8", "vp9"});
    libavVideoCodecSelector->setCurrentText("h264_v4l2m2m");
    libavVideoCodecSelector->setToolTip(tr("LibAV video codec (--libav-video-codec)"));
    codecGroupLayout->addWidget(libavVideoCodecSelector, 3, 1);

    // Row 4: LibAV Codec Options (nur für libav Codec)
    libavCodecOptsLabel = new QLabel(tr("LibAV Codec Options:"), this);
    codecGroupLayout->addWidget(libavCodecOptsLabel, 4, 0);
    libavCodecOptsSelector = new QComboBox(this);
    libavCodecOptsSelector->setEditable(true);
    libavCodecOptsSelector->setMaximumWidth(300);
    libavCodecOptsSelector->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    libavCodecOptsSelector->addItems({
        "",  // Keine Optionen (default)
        "preset=ultrafast",
        "preset=superfast",
        "preset=veryfast",
        "preset=faster",
        "preset=fast",
        "preset=medium",
        "preset=slow",
        "preset=slower",
        "preset=veryslow",
        "tune=zerolatency",
        "tune=film",
        "tune=animation",
        "crf=23",
        "crf=20",
        "crf=18",
        "preset=ultrafast;tune=zerolatency",
        "preset=fast;tune=zerolatency",
        "preset=medium;crf=23"
    });
    libavCodecOptsSelector->setCurrentText("");
    libavCodecOptsSelector->setToolTip(tr("LibAV codec options (--libav-video-codec-opts)\nProfile is set via Profile dropdown above.\nAdditional options: preset, tune, crf, etc.\nFormat: key=value;key2=value2\nExamples:\n- preset=ultrafast (fastest encoding)\n- tune=zerolatency (minimal delay)\n- crf=23 (quality, lower=better)"));
    codecGroupLayout->addWidget(libavCodecOptsSelector, 4, 1); // Nur Spalte 1, nicht spannen

    qDebug() << "[LIBAV] Elements created successfully";

    // Initial visibility: H264 ist default, also H264-Optionen anzeigen
    // H264: profile, level, inline visible
    profileLabel->setVisible(true);
    profileSelector->setVisible(true);
    levelLabel->setVisible(true);
    levelSelector->setVisible(true);
    inlineHeadersCheckbox->setVisible(true);
    // LibAV: format, video-codec, codec-opts, low-latency hidden
    libavFormatLabel->setVisible(false);
    libavFormatSelector->setVisible(false);
    libavVideoCodecLabel->setVisible(false);
    libavVideoCodecSelector->setVisible(false);
    libavCodecOptsLabel->setVisible(false);
    libavCodecOptsSelector->setVisible(false);
    lowLatencyCheckbox->setVisible(false);

    // Codec Reset Button (Row 4 für LibAV, Row 1 für H264)
    codecResetButton = new QPushButton("✕", this);
    codecResetButton->setFixedWidth(20);
    codecResetButton->setToolTip(tr("Reset Codec Settings to defaults"));
    codecGroupLayout->addWidget(codecResetButton, 1, 6);
    connect(codecResetButton, &QPushButton::clicked, this, [this]() {
        if (codecSelector) codecSelector->setCurrentText("h264");
        if (profileSelector) profileSelector->setCurrentIndex(-1);
        if (levelSelector) levelSelector->setCurrentIndex(-1);
        if (inlineHeadersCheckbox) inlineHeadersCheckbox->setChecked(false);
        if (libavFormatSelector) libavFormatSelector->setCurrentText("mpegts");
        if (libavVideoCodecSelector) libavVideoCodecSelector->setCurrentText("h264_v4l2m2m");
        if (libavCodecOptsSelector) libavCodecOptsSelector->setCurrentText("");
        if (lowLatencyCheckbox) lowLatencyCheckbox->setChecked(false);
        codecResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });

    // Connect signals for codec settings to update reset button color
    connect(codecSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (isInitializing) return;
        bool isDefault = (codecSelector->currentText() == "h264" || codecSelector->currentText().isEmpty()) &&
                        (!profileSelector || profileSelector->currentIndex() == -1) &&
                        (!levelSelector || levelSelector->currentIndex() == -1) &&
                        (!inlineHeadersCheckbox || !inlineHeadersCheckbox->isChecked()) &&
                        (!libavFormatSelector || libavFormatSelector->currentText() == "mpegts") &&
                        (!libavVideoCodecSelector || libavVideoCodecSelector->currentText() == "h264_v4l2m2m");
        codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
    if (profileSelector) {
        connect(profileSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            if (isInitializing) return;
            bool isDefault = (codecSelector->currentText() == "h264" || codecSelector->currentText().isEmpty()) &&
                            profileSelector->currentIndex() == -1 &&
                            (!levelSelector || levelSelector->currentIndex() == -1) &&
                            (!inlineHeadersCheckbox || !inlineHeadersCheckbox->isChecked());
            codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (levelSelector) {
        connect(levelSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            if (isInitializing) return;
            bool isDefault = (codecSelector->currentText() == "h264" || codecSelector->currentText().isEmpty()) &&
                            (!profileSelector || profileSelector->currentIndex() == -1) &&
                            levelSelector->currentIndex() == -1 &&
                            (!inlineHeadersCheckbox || !inlineHeadersCheckbox->isChecked());
            codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (inlineHeadersCheckbox) {
        connect(inlineHeadersCheckbox, &QCheckBox::toggled, this, [this]() {
            if (isInitializing) return;
            bool isDefault = (codecSelector->currentText() == "h264" || codecSelector->currentText().isEmpty()) &&
                            (!profileSelector || profileSelector->currentIndex() == -1) &&
                            (!levelSelector || levelSelector->currentIndex() == -1) &&
                            !inlineHeadersCheckbox->isChecked();
            codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (libavFormatSelector) {
        connect(libavFormatSelector, &QComboBox::currentTextChanged, this, [this]() {
            if (isInitializing) return;
            bool isDefault = libavFormatSelector->currentText() == "mpegts";
            codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (libavVideoCodecSelector) {
        connect(libavVideoCodecSelector, &QComboBox::currentTextChanged, this, [this]() {
            if (isInitializing) return;
            bool isDefault = libavVideoCodecSelector->currentText() == "h264_v4l2m2m";
            codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (libavCodecOptsSelector) {
        connect(libavCodecOptsSelector, &QComboBox::currentTextChanged, this, [this]() {
            if (isInitializing) return;
            bool isDefault = libavCodecOptsSelector->currentText().trimmed().isEmpty();
            codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (lowLatencyCheckbox) {
        connect(lowLatencyCheckbox, &QCheckBox::toggled, this, [this]() {
            if (isInitializing) return;
            bool isDefault = !lowLatencyCheckbox->isChecked();
            codecResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }

    // Make group collapsible
    auto *codecHelper = CollapsibleHelper::makeCollapsible(codecGroup, "UI/Video/CodecGroup", [this]() { adjustWindowToOptimalSize(); });
    allCollapsibleHelpers.append(codecHelper);
    // When expanded, trigger codec visibility update
    connect(codecHelper, &CollapsibleHelper::expanded, this, [this]() {
        if (codecSelector) {
            // Trigger codec visibility update by emitting currentTextChanged
            emit codecSelector->currentTextChanged(codecSelector->currentText());
        }
    });

    outputTabLayout->addWidget(codecGroup);

    // Low Resolution Stream Group (wie Codec Settings formatiert)
    auto *loresGroup = new QGroupBox(tr("Low Resolution Stream"), this);
    loresGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    auto *loresGroupLayout = new QGridLayout(loresGroup);
    loresGroupLayout->setContentsMargins(10, 10, 10, 10);

    // Gleiche Spaltenbreiten wie bei Codec Settings für konsistente Abstände
    loresGroupLayout->setColumnMinimumWidth(0, 150);  // Label-Spalte
    loresGroupLayout->setColumnMinimumWidth(1, 250);  // Dropdown-Spalte (feste Breite)
    loresGroupLayout->setColumnStretch(4, 1);  // Stretch-Spalte für Platz vor Reset-Button

    // Row 0: Low Resolution Selector (wie Codec Selector mit gleicher Spaltenstruktur)
    auto *loresLabel = new QLabel(tr("Low Resolution:"), this);
    loresGroupLayout->addWidget(loresLabel, 0, 0);
    loresGroupLayout->addWidget(loresComboBox, 0, 1);

    // Leere Spalte 2 (wie bei Codec, wo Profile/Level erscheinen können)
    // Dies sorgt für gleiche Spaltenbreiten wie codecGroupLayout

    // Reset-Button rechts in Spalte 5 (wie bei Codec Settings)
    loresResetButton->setToolTip(tr("Reset Low Resolution settings"));
    loresGroupLayout->addWidget(loresResetButton, 0, 5, Qt::AlignRight);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(loresGroup, "UI/Video/LoresGroup", [this]() { adjustWindowToOptimalSize(); }));

    outputTabLayout->addWidget(loresGroup);

    // Quality & Bitrate Group
    auto *qualityGroup = new QGroupBox(tr("Quality & Bitrate"), this);
    qualityGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "}"
    );
    auto *qualityLayout = new QGridLayout;
    qualityGroup->setLayout(qualityLayout);
    qualityLayout->setColumnMinimumWidth(0, 150);  // Label column
    qualityLayout->setColumnStretch(4, 1);  // Stretch before reset button

    // Bitrate
    qualityLayout->addWidget(new QLabel(tr("Bitrate:"), this), 0, 0);
    bitrateSpinBox = new QSpinBox(this);
    bitrateSpinBox->setRange(0, 100000);
    bitrateSpinBox->setValue(0);
    bitrateSpinBox->setSpecialValueText("Default");
    bitrateSpinBox->setSuffix(" kbps");
    bitrateSpinBox->setToolTip(tr("Video bitrate in kilobits per second (--bitrate)\n0 = use default"));
    bitrateSpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    qualityLayout->addWidget(bitrateSpinBox, 0, 1);

    // Quality
    qualityLayout->addWidget(new QLabel(tr("Quality:"), this), 0, 2);
    qualitySpinBox = new QSpinBox(this);
    qualitySpinBox->setRange(0, 100);
    qualitySpinBox->setValue(0);
    qualitySpinBox->setSpecialValueText("Default");
    qualitySpinBox->setToolTip(tr("MJPEG quality level (--quality, 1-100)\n0 = use default"));
    qualitySpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    qualityLayout->addWidget(qualitySpinBox, 0, 3);

    // Intra Period (Keyframe Interval)
    qualityLayout->addWidget(new QLabel(tr("Intra Period:"), this), 1, 0);
    intraSpinBox = new QSpinBox(this);
    intraSpinBox->setRange(0, 300);
    intraSpinBox->setValue(0);
    intraSpinBox->setSpecialValueText("Default");
    intraSpinBox->setToolTip(tr("Intra frame period / keyframe interval (--intra)\n0 = use default"));
    intraSpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    qualityLayout->addWidget(intraSpinBox, 1, 1);

    // Frames Limit
    qualityLayout->addWidget(new QLabel(tr("Max Frames:"), this), 1, 2);
    framesSpinBox = new QSpinBox(this);
    framesSpinBox->setRange(0, 999999);
    framesSpinBox->setValue(0);
    framesSpinBox->setSpecialValueText("Unlimited");
    framesSpinBox->setToolTip(tr("Maximum number of frames to record (--frames)\n0 = unlimited"));
    framesSpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    qualityLayout->addWidget(framesSpinBox, 1, 3);

    // Quality & Bitrate Reset Button
    auto *qualityResetButton = new QPushButton("✕", this);
    qualityResetButton->setFixedWidth(20);
    qualityResetButton->setToolTip(tr("Reset Quality & Bitrate to defaults"));
    qualityLayout->addWidget(qualityResetButton, 1, 4, Qt::AlignRight);
    connect(qualityResetButton, &QPushButton::clicked, this, [this, qualityResetButton]() {
        if (bitrateSpinBox) bitrateSpinBox->setValue(0);
        if (qualitySpinBox) qualitySpinBox->setValue(0);
        if (intraSpinBox) intraSpinBox->setValue(0);
        if (framesSpinBox) framesSpinBox->setValue(0);
        qualityResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });

    // Connect signals for quality settings to update reset button color
    if (bitrateSpinBox) {
        connect(bitrateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, qualityResetButton]() {
            if (isInitializing) return;
            bool isDefault = bitrateSpinBox->value() == 0 &&
                            (!qualitySpinBox || qualitySpinBox->value() == 0) &&
                            (!intraSpinBox || intraSpinBox->value() == 0) &&
                            (!framesSpinBox || framesSpinBox->value() == 0);
            qualityResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (qualitySpinBox) {
        connect(qualitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, qualityResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!bitrateSpinBox || bitrateSpinBox->value() == 0) &&
                            qualitySpinBox->value() == 0 &&
                            (!intraSpinBox || intraSpinBox->value() == 0) &&
                            (!framesSpinBox || framesSpinBox->value() == 0);
            qualityResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (intraSpinBox) {
        connect(intraSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, qualityResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!bitrateSpinBox || bitrateSpinBox->value() == 0) &&
                            (!qualitySpinBox || qualitySpinBox->value() == 0) &&
                            intraSpinBox->value() == 0 &&
                            (!framesSpinBox || framesSpinBox->value() == 0);
            qualityResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (framesSpinBox) {
        connect(framesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, qualityResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!bitrateSpinBox || bitrateSpinBox->value() == 0) &&
                            (!qualitySpinBox || qualitySpinBox->value() == 0) &&
                            (!intraSpinBox || intraSpinBox->value() == 0) &&
                            framesSpinBox->value() == 0;
            qualityResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(qualityGroup, "UI/Video/QualityGroup", [this]() { adjustWindowToOptimalSize(); }));

    outputTabLayout->addWidget(qualityGroup);

    // NOTE: Recording Options Group will be added here (from line ~1553/1737)
    // This will be added AFTER it's created later in the code

    outputTabLayout->addStretch();

    // Video Tab wird später über reorderAllTabs() eingefügt

    // ===== Widget Initialization for Image Tab =====
    // These need to be initialized BEFORE the Image Tab is created

    // AWB Reset Button
    resetAwbButton = new QPushButton("✕", this);
    resetAwbButton->setFixedWidth(20);
    resetAwbButton->setToolTip(tr("Reset AWB selection"));

    // Metering Reset Button
    meteringResetButton = new QPushButton("✕", this);
    meteringResetButton->setFixedWidth(20);
    meteringResetButton->setToolTip(tr("Reset Metering selection"));

    // HDR Reset Button
    hdrResetButton = new QPushButton("✕", this);
    hdrResetButton->setFixedWidth(20);
    hdrResetButton->setToolTip(tr("Reset HDR selection"));

    // Denoise Reset Button
    denoiseResetButton = new QPushButton("✕", this);
    denoiseResetButton->setFixedWidth(20);
    denoiseResetButton->setToolTip(tr("Reset Denoise selection"));

    // Flicker Period Reset Button (already initialized above, just set properties)
    flickerPeriodResetButton->setFixedWidth(20);
    flickerPeriodResetButton->setToolTip(tr("Reset Flicker Period selection"));

    // ===== Image Tab =====
    imageTab = new QWidget;
    auto *imageLayout = new QVBoxLayout(imageTab);
    tabRegistryService->registerTab(imageTab, "Adjust", 2, "", true);

    // Image Quality Group
    auto *imageQualityGroup = new QGroupBox(tr("Image Adjustments"), this);
    imageQualityGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *imageQualityLayout = new QGridLayout(imageQualityGroup);
    imageQualityLayout->setColumnMinimumWidth(0, 150);  // Label column
    imageQualityLayout->setColumnStretch(1, 1);  // Slider column stretches

    int iqRow = 0;

    // Sharpness slider
    sharpnessSlider->setRange(0, 50);
    sharpnessSlider->setSingleStep(1);
    sharpnessSlider->setValue(10);
    sharpnessInput->setValidator(new QDoubleValidator(0.0, 5.0, 1, this));
    sharpnessInput->setText("1.0");
    sharpnessInput->setFixedWidth(40);
    auto *sharpnessResetButton = new QPushButton("✕", this);
    sharpnessResetButton->setFixedWidth(20);
    QLabel *sharpnessLabel = new QLabel(tr("Sharpness:"), this);
    sharpnessLabel->setMinimumWidth(150);
    imageQualityLayout->addWidget(sharpnessLabel, iqRow, 0);
    imageQualityLayout->addWidget(sharpnessSlider, iqRow, 1);
    imageQualityLayout->addWidget(sharpnessInput, iqRow, 2);
    imageQualityLayout->addWidget(sharpnessResetButton, iqRow, 3);
    iqRow++;

    // Brightness slider
    brightnessSlider->setRange(-10, 10);
    brightnessSlider->setSingleStep(1);
    brightnessSlider->setValue(0);
    brightnessInput->setValidator(new QDoubleValidator(-1.0, 1.0, 1, this));
    brightnessInput->setText("0.0");
    brightnessInput->setFixedWidth(40);
    auto *brightnessResetButton = new QPushButton("✕", this);
    brightnessResetButton->setFixedWidth(20);
    QLabel *brightnessLabel = new QLabel(tr("Brightness:"), this);
    brightnessLabel->setMinimumWidth(150);
    imageQualityLayout->addWidget(brightnessLabel, iqRow, 0);
    imageQualityLayout->addWidget(brightnessSlider, iqRow, 1);
    imageQualityLayout->addWidget(brightnessInput, iqRow, 2);
    imageQualityLayout->addWidget(brightnessResetButton, iqRow, 3);
    iqRow++;

    // Contrast slider
    contrastSlider->setSingleStep(1);
    contrastSlider->setValue(10);
    contrastInput->setValidator(new QDoubleValidator(0.0, 5.0, 1, this));
    contrastInput->setText("1.0");
    contrastInput->setFixedWidth(40);
    auto *contrastResetButton = new QPushButton("✕", this);
    contrastResetButton->setFixedWidth(20);
    QLabel *contrastLabel = new QLabel(tr("Contrast:"), this);
    contrastLabel->setMinimumWidth(150);
    imageQualityLayout->addWidget(contrastLabel, iqRow, 0);
    imageQualityLayout->addWidget(contrastSlider, iqRow, 1);
    imageQualityLayout->addWidget(contrastInput, iqRow, 2);
    imageQualityLayout->addWidget(contrastResetButton, iqRow, 3);
    iqRow++;

    // Saturation slider
    saturationSlider->setRange(0, 10);
    saturationSlider->setSingleStep(1);
    saturationSlider->setValue(10);
    saturationInput->setValidator(new QDoubleValidator(0.0, 1.0, 1, this));
    saturationInput->setText("1.0");
    saturationInput->setFixedWidth(40);
    auto *saturationResetButton = new QPushButton("✕", this);
    saturationResetButton->setFixedWidth(20);
    QLabel *saturationLabel = new QLabel(tr("Saturation:"), this);
    saturationLabel->setMinimumWidth(150);
    imageQualityLayout->addWidget(saturationLabel, iqRow, 0);
    imageQualityLayout->addWidget(saturationSlider, iqRow, 1);
    imageQualityLayout->addWidget(saturationInput, iqRow, 2);
    imageQualityLayout->addWidget(saturationResetButton, iqRow, 3);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(imageQualityGroup, "UI/Still/ImageQualityGroup", [this]() { adjustWindowToOptimalSize(); }));

    imageLayout->addWidget(imageQualityGroup);

    // =============================================================
    // EXPOSURE & GAIN GROUP
    // =============================================================
    auto *exposureGainGroup = new QGroupBox(tr("Exposure"), this);
    exposureGainGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *exposureGainLayout = new QGridLayout(exposureGainGroup);
    exposureGainLayout->setColumnMinimumWidth(0, 150);  // Label column
    exposureGainLayout->setColumnStretch(1, 1);  // Slider column stretches

    int egRow = 0;

    // EV (Exposure Compensation)
    evSlider->setRange(-99, 99);
    evSlider->setSingleStep(1);
    evSlider->setValue(0);
    evInput->setValidator(new QDoubleValidator(-9.9, 9.9, 1, this));
    evInput->setText("0.0");
    evInput->setFixedWidth(40);
    auto *evResetButton = new QPushButton("✕", this);
    evResetButton->setFixedWidth(20);
    QLabel *evLabel = new QLabel(tr("EV:"), this);
    evLabel->setMinimumWidth(150);
    exposureGainLayout->addWidget(evLabel, egRow, 0);
    exposureGainLayout->addWidget(evSlider, egRow, 1);
    exposureGainLayout->addWidget(evInput, egRow, 2);
    exposureGainLayout->addWidget(evResetButton, egRow, 3);
    egRow++;

    // Analogue Gain
    gainSlider->setRange(0, 200);
    gainSlider->setSingleStep(1);
    gainSlider->setValue(0);
    gainInput->setValidator(new QDoubleValidator(0.0, 20.0, 1, this));
    gainInput->setText("0.0");
    gainInput->setFixedWidth(40);
    auto *gainResetButton = new QPushButton("✕", this);
    gainResetButton->setFixedWidth(20);
    QLabel *gainLabel = new QLabel(tr("Analogue Gain:"), this);
    gainLabel->setMinimumWidth(150);
    exposureGainLayout->addWidget(gainLabel, egRow, 0);
    exposureGainLayout->addWidget(gainSlider, egRow, 1);
    exposureGainLayout->addWidget(gainInput, egRow, 2);
    exposureGainLayout->addWidget(gainResetButton, egRow, 3);
    egRow++;

    // Shutter (logarithmic scale 100 µs to 200000 µs)
    shutterSlider = new QSlider(Qt::Horizontal, this);
    shutterSlider->setRange(0, 1000);
    shutterSlider->setSingleStep(1);
    shutterSlider->setValue(0);
    shutterValueInput = new QLineEdit(this);
    shutterValueInput->setValidator(new QIntValidator(0, 200000, this));
    shutterValueInput->setText("0");
    shutterValueInput->setFixedWidth(40);
    shutterSliderResetButton = new QPushButton("✕", this);
    shutterSliderResetButton->setFixedWidth(20);
    shutterSliderResetButton->setToolTip(tr("Reset Shutter"));
    shutterLabel = new QLabel(tr("Shutter:"), this);
    shutterLabel->setMinimumWidth(150);
    exposureGainLayout->addWidget(shutterLabel, egRow, 0);
    exposureGainLayout->addWidget(shutterSlider, egRow, 1);
    exposureGainLayout->addWidget(shutterValueInput, egRow, 2);
    exposureGainLayout->addWidget(shutterSliderResetButton, egRow, 3);
    egRow++;

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(exposureGainGroup, "UI/Image/ExposureGainGroup", [this]() { adjustWindowToOptimalSize(); }));

    imageLayout->addWidget(exposureGainGroup);

    // =============================================================
    // WHITE BALANCE (AWB) GROUP
    // =============================================================
    auto *awbGroup = new QGroupBox(tr("White Balance (AWB)"), this);
    awbGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *awbGroupLayout = new QGridLayout(awbGroup);
    awbGroupLayout->setColumnMinimumWidth(0, 150);  // Label column
    awbGroupLayout->setColumnStretch(2, 1);  // Stretch before reset button

    int awbRow = 0;

    // AWB Mode + CCM combined row: AWB Mode: [dropdown] [x]   CCM: [matrix input] [x]
    awbSelector->setFixedWidth(170);
    QLabel *awbModeLabel = new QLabel(tr("AWB Mode:"), this);
    awbModeLabel->setMinimumWidth(150);
    awbGroupLayout->addWidget(awbModeLabel, awbRow, 0);

    auto *awbCcmRow = new QHBoxLayout;
    awbCcmRow->addWidget(awbSelector);
    awbCcmRow->addWidget(resetAwbButton);
    awbCcmRow->addSpacing(20);
    QLabel *ccmLabel = new QLabel(tr("CCM:"), this);
    ccmLabel->setToolTip(tr("Colour correction matrix (9 comma-separated values). Requires explicit AWB gains."));
    ccmInput = new QLineEdit(this);
    ccmInput->setFixedWidth(220);
    ccmInput->setPlaceholderText("1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0");
    ccmInput->setToolTip(tr("3x3 colour correction matrix. Example: 1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0"));
    ccmResetButton = new QPushButton("✕", this);
    ccmResetButton->setFixedWidth(20);
    ccmResetButton->setToolTip(tr("Reset CCM"));
    connect(ccmResetButton, &QPushButton::clicked, this, [this]() {
        ccmInput->clear();
        updateResetButtonColor(ccmResetButton, 0, 0);
    });
    connect(ccmInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        updateResetButtonColor(ccmResetButton, text.isEmpty() ? 0 : 1, 0);
    });
    awbCcmRow->addWidget(ccmLabel);
    awbCcmRow->addWidget(ccmInput);
    awbCcmRow->addWidget(ccmResetButton);
    awbCcmRow->addStretch();  // Fill empty space when CCM is hidden (rpicam-apps < 1.13)
    awbGroupLayout->addLayout(awbCcmRow, awbRow, 1, 1, 3);

    // CCM requires rpicam-apps >= 1.13; hide on older versions
    if (!m_hasPreviewBackend) {
        ccmLabel->setVisible(false);
        ccmInput->setVisible(false);
        ccmResetButton->setVisible(false);
    }

    awbRow++;

    // AWB Gain Red
    awbGainRedSlider->setRange(0, 80);
    awbGainRedSlider->setSingleStep(1);
    awbGainRedSlider->setValue(15);
    awbGainRedInput->setValidator(new QDoubleValidator(0.0, 8.0, 1, this));
    awbGainRedInput->setText("1.5");
    awbGainRedInput->setFixedWidth(40);
    awbGainRedResetButton = new QPushButton("✕", this);
    awbGainRedResetButton->setFixedWidth(20);
    awbGainRedResetButton->setToolTip(tr("Reset AWB Gain Red"));
    QLabel *awbGainRedLabel = new QLabel(tr("AWB Gain R:"), this);
    awbGainRedLabel->setMinimumWidth(150);
    awbGroupLayout->addWidget(awbGainRedLabel, awbRow, 0);
    awbGroupLayout->addWidget(awbGainRedSlider, awbRow, 1);
    awbGroupLayout->addWidget(awbGainRedInput, awbRow, 2);
    awbGroupLayout->addWidget(awbGainRedResetButton, awbRow, 3);
    awbRow++;

    // AWB Gain Blue
    awbGainBlueSlider->setRange(0, 80);
    awbGainBlueSlider->setSingleStep(1);
    awbGainBlueSlider->setValue(12);
    awbGainBlueInput->setValidator(new QDoubleValidator(0.0, 8.0, 1, this));
    awbGainBlueInput->setText("1.2");
    awbGainBlueInput->setFixedWidth(40);
    awbGainBlueResetButton = new QPushButton("✕", this);
    awbGainBlueResetButton->setFixedWidth(20);
    awbGainBlueResetButton->setToolTip(tr("Reset AWB Gain Blue"));
    QLabel *awbGainBlueLabel = new QLabel(tr("AWB Gain B:"), this);
    awbGainBlueLabel->setMinimumWidth(150);
    awbGroupLayout->addWidget(awbGainBlueLabel, awbRow, 0);
    awbGroupLayout->addWidget(awbGainBlueSlider, awbRow, 1);
    awbGroupLayout->addWidget(awbGainBlueInput, awbRow, 2);
    awbGroupLayout->addWidget(awbGainBlueResetButton, awbRow, 3);

    // Connect AWB Gain reset buttons
    connect(awbGainRedResetButton, &QPushButton::clicked, this, [this]() {
        // Block slider signal to prevent double-send via valueChanged
        awbGainRedSlider->blockSignals(true);
        awbGainRedSlider->setValue(15);
        awbGainRedSlider->blockSignals(false);
        awbGainRedInput->setText("1.5");
        updateResetButtonColor(awbGainRedResetButton, 1.5, DEFAULT_AWB_GAIN_RED);
        if (isControlSocketActive()) {
            double blue = awbGainBlueSlider->value() / 10.0;
            // Both gains at default: just send awb:auto (matching piStudio's onReset).
            // Sending awbgains before awb:auto would briefly enter manual mode and
            // prevent the camera from re-evaluating AWB properly.
            if (blue == 1.2) {
                sendSliderToSocket("awb", "auto");
            } else {
                sendSliderToSocket("awbgains", QString("1.5,%1").arg(blue, 0, 'f', 1));
            }
        }
        // Always reset AWB combo to auto when both gains are back at default
        double blue = awbGainBlueInput->text().toDouble();
        if (blue == 1.2) {
            awbSelector->blockSignals(true);
            awbSelector->setCurrentText("auto");
            awbSelector->blockSignals(false);
            updateResetButtonColor(resetAwbButton, 0, 0);
        }
    });
    connect(awbGainBlueResetButton, &QPushButton::clicked, this, [this]() {
        // Block slider signal to prevent double-send via valueChanged
        awbGainBlueSlider->blockSignals(true);
        awbGainBlueSlider->setValue(12);
        awbGainBlueSlider->blockSignals(false);
        awbGainBlueInput->setText("1.2");
        updateResetButtonColor(awbGainBlueResetButton, 1.2, DEFAULT_AWB_GAIN_BLUE);
        if (isControlSocketActive()) {
            double red = awbGainRedSlider->value() / 10.0;
            // Both gains at default: just send awb:auto (matching piStudio's onReset).
            // Sending awbgains before awb:auto would briefly enter manual mode and
            // prevent the camera from re-evaluating AWB properly.
            if (red == 1.5) {
                sendSliderToSocket("awb", "auto");
            } else {
                sendSliderToSocket("awbgains", QString("%1,1.2").arg(red, 0, 'f', 1));
            }
        }
        // Always reset AWB combo to auto when both gains are back at default
        double red = awbGainRedInput->text().toDouble();
        if (red == 1.5) {
            awbSelector->blockSignals(true);
            awbSelector->setCurrentText("auto");
            awbSelector->blockSignals(false);
            updateResetButtonColor(resetAwbButton, 0, 0);
        }
    });

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(awbGroup, "UI/Image/AWBGroup", [this]() { adjustWindowToOptimalSize(); }));

    imageLayout->addWidget(awbGroup);

    // =============================================================
    // PROCESSING GROUP (HDR & Denoise)
    // =============================================================
    auto *processingGroup = new QGroupBox(tr("Image Processing"), this);
    processingGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *processingLayout = new QVBoxLayout(processingGroup);

    const int procLabelWidth = 150;

    // Row 0: HDR (left) | stretch | Denoise (right)
    auto *procRow0Layout = new QHBoxLayout;
    hdrSelector->setFixedWidth(140);
    QLabel *hdrLabel = new QLabel(tr("HDR:"), this);
    hdrLabel->setMinimumWidth(procLabelWidth);
    procRow0Layout->addWidget(hdrLabel);
    procRow0Layout->addWidget(hdrSelector);
    procRow0Layout->addWidget(hdrResetButton);
    procRow0Layout->addStretch();
    denoiseSelector->setFixedWidth(140);
    QLabel *denoiseLabel = new QLabel(tr("Denoise:"), this);
    denoiseLabel->setMinimumWidth(procLabelWidth);
    procRow0Layout->addWidget(denoiseLabel);
    procRow0Layout->addWidget(denoiseSelector);
    procRow0Layout->addWidget(denoiseResetButton);
    processingLayout->addLayout(procRow0Layout);

    // Row 1: Metering (left) | stretch | Flicker Period (right)
    auto *procRow1Layout = new QHBoxLayout;
    meteringSelector->setFixedWidth(140);
    QLabel *meteringLabel = new QLabel(tr("Metering:"), this);
    meteringLabel->setMinimumWidth(procLabelWidth);
    procRow1Layout->addWidget(meteringLabel);
    procRow1Layout->addWidget(meteringSelector);
    procRow1Layout->addWidget(meteringCustomInput);
    procRow1Layout->addWidget(meteringResetButton);
    procRow1Layout->addStretch();
    flickerPeriodSelector->setFixedWidth(140);
    QLabel *flickerLabel = new QLabel(tr("Flicker Period:"), this);
    flickerLabel->setMinimumWidth(procLabelWidth);
    procRow1Layout->addWidget(flickerLabel);
    procRow1Layout->addWidget(flickerPeriodSelector);
    procRow1Layout->addWidget(flickerPeriodResetButton);
    processingLayout->addLayout(procRow1Layout);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(processingGroup, "UI/Image/ProcessingGroup", [this]() { adjustWindowToOptimalSize(); }));

    imageLayout->addWidget(processingGroup);

    imageLayout->addStretch();

    // Image Tab wird später über reorderAllTabs() eingefügt

    setupFocusTab();

    setupZoomTab();

    // Still Tab erstellen (rpicam-still / rpicam-jpeg Parameter)
    setupStillTab();

    // Audio Tab erstellen
    setupAudioTab();
    tabVisibilityService->updateAudioTabVisibility();

    // GStreamer Tab erstellen
    setupGstreamerTab();
    tabVisibilityService->updateGstreamerTabVisibility();

    // GST-Launch Tab erstellen (Stream Viewer)
    setupGstLaunchTab();
    tabVisibilityService->updateGstTabVisibility();

    // Inference Tab erstellen
    setupInferenceTab();
    tabVisibilityService->updateInferenceTabVisibility();

    // Actions Tab erstellen
    setupActionsTab();
    tabVisibilityService->updateActionsTabVisibility();

    // Tools Tab erstellen
    setupToolsTab();
    tabVisibilityService->updateToolsTabVisibility();

    // Auto-refresh beim Focus/Zoom Tab wechseln
    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        QString tabName = tabWidget->tabText(index);
        if (tabName == "Focus") {
            // Kleiner Delay damit Tab vollständig geladen ist
            QTimer::singleShot(100, refreshFocusPositionButton, &QPushButton::click);
        } else if (tabName == "Zoom") {
            QTimer::singleShot(100, refreshZoomPositionButton, &QPushButton::click);
        }
    });

    // Start V4L2 polling only if focus or zoom hardware is explicitly enabled
    {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        bool focusEnabled = settings.value("V4L2/FocusEnabled", false).toBool();
        bool zoomEnabled  = settings.value("V4L2/ZoomEnabled", false).toBool();
        QString focusDevice = settings.value("V4L2/FocusDevice", "").toString();
        QString zoomDevice  = settings.value("V4L2/ZoomDevice", "").toString();
        settings.endGroup();

        if (focusEnabled || zoomEnabled) {
            // Prefer focus device, fall back to zoom device
            QString v4l2Device = !focusDevice.isEmpty() ? focusDevice : zoomDevice;
            if (!v4l2Device.isEmpty())
                v4l2DeviceInput->setText(v4l2Device);
            QString initDevice = v4l2DeviceInput->text().isEmpty()
                ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
                : v4l2DeviceInput->text();
            m_v4l2Controller->openDevice(initDevice);
        }
    }

    // ========== EXPERT TAB ==========
    // Tab wird dynamisch erstellt/entfernt basierend auf expertTabEnabled
    expertTab = new QWidget;
    auto *expertLayout = new QVBoxLayout(expertTab);
    tabRegistryService->registerTab(expertTab, "Expert", 4, "expertTabEnabled", true);
    expertLayout->setSpacing(10);
    expertLayout->setContentsMargins(10, 10, 10, 10);

    // Viewfinder Mode Group (piStudio Style)
    auto *viewfinderGroup = new QGroupBox(tr("Viewfinder Mode"), expertTab);
    viewfinderGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    auto *viewfinderGroupLayout = new QGridLayout(viewfinderGroup);
    viewfinderGroupLayout->setContentsMargins(10, 10, 10, 10);
    viewfinderGroupLayout->setColumnMinimumWidth(0, 150);  // Label column
    viewfinderGroupLayout->setColumnMinimumWidth(1, 250);  // Widget column
    viewfinderGroupLayout->setColumnStretch(4, 1);  // Stretch before xReset

    // Row 0: Viewfinder Mode Dropdown
    auto *viewfinderModeLabel = new QLabel(tr("Sensor Mode:"), expertTab);
    viewfinderModeSelector = new QComboBox(expertTab);
    viewfinderModeSelector->addItem(tr("Auto (use capture resolution)"), "");
    viewfinderModeSelector->setToolTip(tr("Select camera sensor mode for preview stream.\nLower resolution = better performance."));
    viewfinderModeResetButton = new QPushButton("✕", expertTab);
    viewfinderModeResetButton->setFixedWidth(20);
    connect(viewfinderModeResetButton, &QPushButton::clicked, [this]() {
        viewfinderModeSelector->setCurrentIndex(0);
        viewfinderModeResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(viewfinderModeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (isInitializing) return;
        bool isDefault = viewfinderModeSelector->currentIndex() == 0;
        viewfinderModeResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
    viewfinderGroupLayout->addWidget(viewfinderModeLabel, 0, 0);
    viewfinderGroupLayout->addWidget(viewfinderModeSelector, 0, 1);
    viewfinderGroupLayout->addWidget(viewfinderModeResetButton, 0, 5, Qt::AlignRight);

    // Row 1: Viewfinder Width
    auto *viewfinderWidthLabel = new QLabel(tr("Preview Width:"), expertTab);
    viewfinderWidthSpinBox = new QSpinBox(expertTab);
    viewfinderWidthSpinBox->setRange(0, 4096);
    viewfinderWidthSpinBox->setValue(0);
    viewfinderWidthSpinBox->setSpecialValueText(tr("Auto"));
    viewfinderWidthSpinBox->setToolTip(tr("Width of viewfinder frames from camera (0 = auto)."));
    viewfinderWidthResetButton = new QPushButton("✕", expertTab);
    viewfinderWidthResetButton->setFixedWidth(20);
    connect(viewfinderWidthResetButton, &QPushButton::clicked, [this]() {
        viewfinderWidthSpinBox->setValue(0);
        viewfinderWidthResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(viewfinderWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        if (isInitializing) return;
        bool isDefault = viewfinderWidthSpinBox->value() == 0;
        viewfinderWidthResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
    viewfinderGroupLayout->addWidget(viewfinderWidthLabel, 1, 0);
    viewfinderGroupLayout->addWidget(viewfinderWidthSpinBox, 1, 1);
    viewfinderGroupLayout->addWidget(viewfinderWidthResetButton, 1, 5, Qt::AlignRight);

    // Row 2: Viewfinder Height
    auto *viewfinderHeightLabel = new QLabel(tr("Preview Height:"), expertTab);
    viewfinderHeightSpinBox = new QSpinBox(expertTab);
    viewfinderHeightSpinBox->setRange(0, 4096);
    viewfinderHeightSpinBox->setValue(0);
    viewfinderHeightSpinBox->setSpecialValueText(tr("Auto"));
    viewfinderHeightSpinBox->setToolTip(tr("Height of viewfinder frames from camera (0 = auto)."));
    viewfinderHeightResetButton = new QPushButton("✕", expertTab);
    viewfinderHeightResetButton->setFixedWidth(20);
    connect(viewfinderHeightResetButton, &QPushButton::clicked, [this]() {
        viewfinderHeightSpinBox->setValue(0);
        viewfinderHeightResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(viewfinderHeightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        if (isInitializing) return;
        bool isDefault = viewfinderHeightSpinBox->value() == 0;
        viewfinderHeightResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
    viewfinderGroupLayout->addWidget(viewfinderHeightLabel, 2, 0);
    viewfinderGroupLayout->addWidget(viewfinderHeightSpinBox, 2, 1);
    viewfinderGroupLayout->addWidget(viewfinderHeightResetButton, 2, 5, Qt::AlignRight);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(viewfinderGroup, "UI/Expert/ViewfinderGroup", [this]() { adjustWindowToOptimalSize(); }));

    expertLayout->addWidget(viewfinderGroup);

    // Buffer Management Group (piStudio Style)
    auto *bufferGroup = new QGroupBox(tr("Buffer Management"), expertTab);
    bufferGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    auto *bufferLayout = new QGridLayout(bufferGroup);
    bufferLayout->setContentsMargins(10, 10, 10, 10);
    bufferLayout->setColumnMinimumWidth(0, 150);  // Label column
    bufferLayout->setColumnMinimumWidth(1, 250);  // Widget column
    bufferLayout->setColumnStretch(4, 1);  // Stretch before xReset

    // Row 0: Buffer Count
    auto *bufferCountLabel = new QLabel(tr("Buffer Count:"), expertTab);
    bufferCountSpinBox = new QSpinBox(expertTab);
    bufferCountSpinBox->setRange(0, 12);
    bufferCountSpinBox->setValue(0);
    bufferCountSpinBox->setSpecialValueText(tr("Default"));
    bufferCountSpinBox->setToolTip(tr("Number of in-flight buffers for video/raw/still (0 = default)."));
    bufferCountResetButton = new QPushButton("✕", expertTab);
    bufferCountResetButton->setFixedWidth(20);
    connect(bufferCountResetButton, &QPushButton::clicked, [this]() {
        bufferCountSpinBox->setValue(0);
        bufferCountResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(bufferCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        if (isInitializing) return;
        bool isDefault = bufferCountSpinBox->value() == 0;
        bufferCountResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
    bufferLayout->addWidget(bufferCountLabel, 0, 0);
    bufferLayout->addWidget(bufferCountSpinBox, 0, 1);
    bufferLayout->addWidget(bufferCountResetButton, 0, 5, Qt::AlignRight);

    // Row 1: Viewfinder Buffer Count
    auto *vfBufferCountLabel = new QLabel(tr("Preview Buffers:"), expertTab);
    viewfinderBufferCountSpinBox = new QSpinBox(expertTab);
    viewfinderBufferCountSpinBox->setRange(0, 12);
    viewfinderBufferCountSpinBox->setValue(0);
    viewfinderBufferCountSpinBox->setSpecialValueText(tr("Default"));
    viewfinderBufferCountSpinBox->setToolTip(tr("Number of in-flight buffers for preview window (0 = default)."));
    viewfinderBufferCountResetButton = new QPushButton("✕", expertTab);
    viewfinderBufferCountResetButton->setFixedWidth(20);
    connect(viewfinderBufferCountResetButton, &QPushButton::clicked, [this]() {
        viewfinderBufferCountSpinBox->setValue(0);
        viewfinderBufferCountResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(viewfinderBufferCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        if (isInitializing) return;
        bool isDefault = viewfinderBufferCountSpinBox->value() == 0;
        viewfinderBufferCountResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
    bufferLayout->addWidget(vfBufferCountLabel, 1, 0);
    bufferLayout->addWidget(viewfinderBufferCountSpinBox, 1, 1);
    bufferLayout->addWidget(viewfinderBufferCountResetButton, 1, 5, Qt::AlignRight);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(bufferGroup, "UI/Expert/BufferGroup", [this]() { adjustWindowToOptimalSize(); }));

    expertLayout->addWidget(bufferGroup);

    expertLayout->addStretch();

    // Alle Tabs in korrekter Reihenfolge anzeigen (inkl. General, Video, Image)
    tabRegistryService->reorderAllTabs();

    // App switcher und Overlay im globalen Bereich (über Start-Button)
    // Kompakte Einzel-Zeile: alle globalen Controls + Geometry in einer Zeile
    auto *globalInputLayout = new QHBoxLayout;
    appSelector->setFixedWidth(130);
    globalInputLayout->addWidget(appSelector);
    globalInputLayout->addWidget(cameraSelector);
    globalInputLayout->addWidget(formatSelector);
    globalInputLayout->addWidget(resolutionSelector);
    globalInputLayout->addWidget(framerateSelector);
    globalInputLayout->addStretch();
    globalInputLayout->addWidget(previewSelector);
    globalInputLayout->addWidget(BoxInput);
    globalInputLayout->addWidget(overlayResetButton);
    globalInputLayout->addWidget(doubleSizeCheckbox);
    // Global X reset for ALL settings, right-most in the row.
    // Moved here from the former bottom button row (which was removed
    // when Save/Load Config moved into the File menu).
    globalResetButton = new QPushButton("✕", this);
    globalResetButton->setFixedWidth(20);
    globalResetButton->setToolTip(tr("Reset all settings to default values"));
    globalInputLayout->addWidget(globalResetButton);
    connect(globalResetButton, &QPushButton::clicked, this, &MainWindow::resetAllToDefaults);
    updateGlobalResetButtonColor();
    outerLayout->addLayout(globalInputLayout);

    // Create Send Signal Button (will be configured later in recording options section)
    sendSignalButton = new QPushButton("Sigusr1", this);
    sendSignalButton->setToolTip(tr("Manually send SIGUSR1 to pause/resume recording\n(Only works when Signal Recording is enabled and rpicam-vid is running)"));
    sendSignalButton->setEnabled(false); // Initially disabled
    sendSignalButton->setVisible(false); // Initially hidden
    sendSignalButton->setMaximumWidth(100); // Half width of Start/Stop button

    // Start/Stop Button with Signal Button (left of it, only visible when active)
    auto *startStopLayout = new QHBoxLayout;
    startStopLayout->addWidget(sendSignalButton);  // Left side: Signal button
    startStopLayout->addWidget(startStopButton);
    outerLayout->addLayout(startStopLayout);
    outerLayout->addWidget(tabWidget); // Add the tab widget
    auto *outerWidget = new QWidget(this);
    outerWidget->setLayout(outerLayout);
    setCentralWidget(outerWidget);
    connect(appSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateTimelapseVisibility);

    connect(appSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateParameterFields);
    connect(cameraSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateCameraInfo);
    // User resolution change: snap the framerate to the highest detected
    // integer value for the newly selected resolution.
    connect(resolutionSelector, &QComboBox::currentTextChanged, this, [this](const QString &res) {
        updateFramerateOptions(res, true);
    });
    connect(formatSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { applyFormatFilter(); });
    connect(resolutionSelector, &QComboBox::currentTextChanged, this, [this]() {
        // GStreamerModule handles aspect-ratio recalculation internally (P19)
        // when resolutionSelector changes via setResolutionSelector()
    });
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::openSaveFileDialog);
    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::startRpiCamApp);
    connect(postProcessFileBrowseButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = postProcessFileSelector->currentText();
        if (initialPath.isEmpty() || !QFileInfo(initialPath).isAbsolute()) {
            initialPath = QDir(guiPostProcessFilePath).filePath(initialPath);
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Post-Process File"), initialPath, "JSON Files (*.json);;All Files (*.*)");
        if (!fileName.isEmpty()) {
            postProcessFileSelector->setCurrentText(fileName); // Setze den ausgewählten Dateinamen
        }
    });

    connect(tuningFileBrowseButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = tuningFileSelector->currentText();
        if (initialPath.isEmpty() || !QFileInfo(initialPath).isAbsolute()) {
            initialPath = QDir(guiTuningFilePath).filePath(initialPath);
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Tuning File"), initialPath, "JSON Files (*.json);;All Files (*.*)");
        if (!fileName.isEmpty()) {
            tuningFileSelector->setCurrentText(fileName); // Set the selected file name
        }
    });
    connect(&process, &QProcess::stateChanged, this, &MainWindow::updateButtonVisibility);

    // Parse ROI selection output from the rpicam-apps fork
    // (feature/rt-roi): "ROI selected: --roi 0.25,0.30,0.50,0.40".
    // The values are fed into the ROI input field so the user can persist
    // them via Set Defaults / profiles. Format is identical (relative
    // x,y,w,h), so config-file compatibility is unaffected.
    // Tolerant to channel (stdout/stderr) and to line splits across read
    // chunks (small rolling buffer).
    // NOTE: the fork's right-click reset also emits this line with the
    // full-frame values 0,0,1,1. piStudio's "no ROI" state is an EMPTY
    // field (placeholder 0.0,0.0,1.0,1.0), so a full-frame reset clears the
    // field instead of writing 0,0,1,1 (which would otherwise be persisted
    // into Defaults/Profiles as a bogus explicit ROI).
    auto handleProcessChunk = [this](const QByteArray &chunk) {
        m_procScanBuffer.append(QString::fromLocal8Bit(chunk));
        if (m_procScanBuffer.size() > 512) {
            m_procScanBuffer = m_procScanBuffer.right(256);
        }
        static const QRegularExpression reRoi(
            R"(ROI selected:\s+--roi\s*([\d.]+),([\d.]+),([\d.]+),([\d.]+))");
        QRegularExpressionMatch m = reRoi.match(m_procScanBuffer);
        if (m.hasMatch()) {
            const QString roiText = QString("%1,%2,%3,%4")
                .arg(m.captured(1), m.captured(2), m.captured(3), m.captured(4));

            bool ok = true;
            const double x = m.captured(1).toDouble(&ok);
            const double y = m.captured(2).toDouble(&ok);
            const double w = m.captured(3).toDouble(&ok);
            const double h = m.captured(4).toDouble(&ok);
            const bool fullFrame = ok &&
                qAbs(x) < 1e-6 && qAbs(y) < 1e-6 &&
                qAbs(w - 1.0) < 1e-6 && qAbs(h - 1.0) < 1e-6;

            if (roiInput) {
                if (fullFrame) {
                    roiInput->clear(); // reset to piStudio's "no ROI" state
                } else {
                    roiInput->setText(roiText);
                }
            }
            updateROIResetButtonColor();
            appendLog(fullFrame
                ? tr("ROI reset to full frame (field cleared)")
                : tr("ROI received from rpicam-apps preview: %1").arg(roiText));
            m_procScanBuffer.clear();
        }
    };

    // Connect stdout/stderr to capture metadata and other process output.
    // The ROI parser runs regardless of the ShowProcessOutput setting.
    connect(&process, &QProcess::readyReadStandardOutput, this, [this, handleProcessChunk]() {
        QByteArray data = process.readAllStandardOutput();
        handleProcessChunk(data);
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        if (settings.value("Debug/ShowProcessOutput", false).toBool()) {
            QString output = QString::fromLocal8Bit(data);
            if (!output.trimmed().isEmpty()) {
                appendLog("<span style='color: #4a9eff;'>[Process Output] " + output.trimmed() + "</span>");
            }
        }
    });

    connect(&process, &QProcess::readyReadStandardError, this, [this, handleProcessChunk]() {
        QByteArray data = process.readAllStandardError();
        handleProcessChunk(data);
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        if (settings.value("Debug/ShowProcessOutput", false).toBool()) {
            QString error = QString::fromLocal8Bit(data);
            if (!error.trimmed().isEmpty()) {
                appendLog("<span style='color: #ff6b6b;'>[Process Error] " + error.trimmed() + "</span>");
            }
        }
    });

    updateParameterFields();
    QProcess process;
    process.start("rpicam-hello", QStringList() << "--list-cameras");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError().trimmed();
    if (!error.isEmpty()) {
        // Hinweise auf stderr (z. B. dmaHeap-Warnungen) sind unkritisch –
        // die Kamera-Erkennung läuft trotzdem über stdout weiter.
        appendLog("rpicam-hello --list-cameras (stderr):\n" + error);
    }
    parseListCamerasOutput(output);

    // MenuBar wird NICHT mehr lokal erzeugt – die shared MenuBar
    // in main.cpp (Corner-Widget) übernimmt Setup/View/Help.
    // createMenus() und menuBar()-Styling deaktiviert, damit kein
    // schwebendes Widget zurückbleibt.

    // Codec selector (already created and added to codecFilesGroup at line ~392)
    updateCodecVisibility(appSelector->currentText());

    // AWB selector (Configuration only - UI in Image Tab)
    awbSelector->addItems({"auto", "incandescent", "tungsten", "fluorescent", "indoor", "daylight", "cloudy", "custom"});
    awbSelector->setCurrentText("auto");
    awbSelector->setToolTip(tr("Select the AWB (Auto White Balance) mode."));

    // Metering selector (Configuration only - UI in Image Tab)
    meteringSelector->addItems({"Select option:", "centre", "spot", "average", "custom"});
    meteringSelector->setCurrentText("Select option:");
    meteringSelector->setToolTip(tr("Set the metering mode (centre, spot, average, custom)"));

    meteringCustomInput->setPlaceholderText(tr("select option:"));
    meteringCustomInput->setVisible(false);
    meteringCustomInput->setToolTip(tr("Enter custom metering coordinates (e.g., 0.3,0.3,0.3,0.3)"));

    // NOTE: Low Resolution selector moved earlier (after shutter, line ~638)

    // ========== Recording Options Group ==========
    auto *recordingOptionsGroup = new QGroupBox(tr("Recording Options"), this);
    recordingOptionsGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"  // Blauer Rahmen
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );

    auto *recordingOptionsLayout = new QVBoxLayout;
    recordingOptionsGroup->setLayout(recordingOptionsLayout);

    // Signal Recording checkbox with warning label on the right
    auto *signalRecordingHLayout = new QHBoxLayout;
    signalRecordingCheckbox = new QCheckBox(tr("Enable Signal Recording (--signal)"), this);
    signalRecordingCheckbox->setToolTip(tr("Enable pause/resume recording via SIGUSR1 signal"));
    signalRecordingCheckbox->setChecked(false);
    signalRecordingHLayout->addWidget(signalRecordingCheckbox);

    // Warning label for codec incompatibility (inline, right side)
    auto *codecWarningLabel = new QLabel(this);
    codecWarningLabel->setStyleSheet("QLabel { color: #e74c3c; font-style: italic; margin-left: 10px; }");
    codecWarningLabel->setVisible(false);
    codecWarningLabel->setObjectName("codecWarningLabel");
    signalRecordingHLayout->addWidget(codecWarningLabel);
    signalRecordingHLayout->addStretch();

    recordingOptionsLayout->addLayout(signalRecordingHLayout);

    // Keypress Recording
    auto *keypressRecordingHLayout = new QHBoxLayout;
    keypressRecordingCheckbox = new QCheckBox(tr("Keypress Recording (ENTER)"), this);
    keypressRecordingCheckbox->setToolTip(tr("Enable ENTER key to pause/resume recording (--keypress)\nOnly works with MJPEG codec"));
    keypressRecordingCheckbox->setEnabled(false); // Disabled by default, enabled only with MJPEG codec
    keypressRecordingHLayout->addWidget(keypressRecordingCheckbox);
    keypressRecordingHLayout->addStretch();
    recordingOptionsLayout->addLayout(keypressRecordingHLayout);

    // Note: Send Signal Button was already created and added to layout (see line ~1376)
    // No need to create it here again

    // Initial State
    auto *initialStateLayout = new QHBoxLayout;
    QLabel *initialStateLabel = new QLabel(tr("Initial State:"), this);
    initialStateLabel->setFixedWidth(120);
    initialStateLayout->addWidget(initialStateLabel);
    initialStateComboBox = new QComboBox(this);
    initialStateComboBox->addItems({"record", "pause"});
    initialStateComboBox->setCurrentText("pause");
    initialStateComboBox->setToolTip(tr("Start recording or paused (--initial)"));
    initialStateComboBox->setEnabled(false);
    initialStateLayout->addWidget(initialStateComboBox);
    initialStateLayout->addStretch();
    recordingOptionsLayout->addLayout(initialStateLayout);

    // Split Files checkbox
    splitFilesCheckbox = new QCheckBox(tr("Split Files (--split)"), this);
    splitFilesCheckbox->setToolTip(tr("Create separate file for each recording segment"));
    splitFilesCheckbox->setChecked(false);
    splitFilesCheckbox->setEnabled(false);
    recordingOptionsLayout->addWidget(splitFilesCheckbox);

    // Connect splitFilesCheckbox to segmentPatternCheckbox
    connect(splitFilesCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        bool splitEnabled = (state == Qt::Checked);
        qDebug() << "[Split Files] State changed to:" << splitEnabled;

        // Update %04d checkbox (only enable if codec is MJPEG)
        if (segmentPatternCheckbox && codecSelector) {
            bool isMjpeg = (codecSelector->currentText() == "mjpeg");
            if (splitEnabled && isMjpeg) {
                // Enable and auto-check when split is activated AND codec is MJPEG
                segmentPatternCheckbox->setEnabled(true);
                segmentPatternCheckbox->setChecked(true);
                qDebug() << "[%04d] Enabled and checked (split + MJPEG)";
            } else {
                // Disable and uncheck when split is deactivated OR codec is not MJPEG
                segmentPatternCheckbox->setEnabled(false);
                segmentPatternCheckbox->setChecked(false);
                qDebug() << "[%04d] Disabled and unchecked";
            }
        }

        // Disable circular buffer when split is enabled (incompatible modes)
        if (circularBufferInput) {
            if (splitEnabled) {
                circularBufferInput->clear();
                circularBufferInput->setEnabled(false);
                circularBufferInput->setPlaceholderText(tr("N/A (incompatible with split)"));
                qDebug() << "[Circular Buffer] Disabled (incompatible with split files)";
            } else {
                // Re-enable based on codec compatibility
                if (codecSelector && codecSelector->currentText() == "mjpeg") {
                    circularBufferInput->setEnabled(true);
                    circularBufferInput->setPlaceholderText(tr("0"));
                    qDebug() << "[Circular Buffer] Re-enabled";
                }
            }
        }

        // Disable segment duration when split is disabled (no segments without split)
        if (segmentDurationInput) {
            if (splitEnabled) {
                // Re-enable based on codec compatibility
                if (codecSelector && codecSelector->currentText() == "mjpeg") {
                    segmentDurationInput->setEnabled(true);
                    segmentDurationInput->setPlaceholderText(tr("0"));
                    qDebug() << "[Segment Duration] Enabled (split files active)";
                }
            } else {
                segmentDurationInput->clear();
                segmentDurationInput->setEnabled(false);
                segmentDurationInput->setPlaceholderText(tr("N/A (requires split files)"));
                qDebug() << "[Segment Duration] Disabled (split files inactive)";
            }
        }
    });

    // Initialize %04d checkbox and circular buffer state based on current splitFilesCheckbox state
    // Only if splitFilesCheckbox is both checked AND enabled (i.e., signal recording is active)
    if (segmentPatternCheckbox && splitFilesCheckbox && circularBufferInput && codecSelector) {
        bool isMjpeg = (codecSelector->currentText() == "mjpeg");
        if (splitFilesCheckbox->isChecked() && splitFilesCheckbox->isEnabled() && isMjpeg) {
            segmentPatternCheckbox->setEnabled(true);
            segmentPatternCheckbox->setChecked(true);
            qDebug() << "[%04d] Initial state: Enabled and checked (split + MJPEG active)";

            // Disable circular buffer (incompatible with split)
            circularBufferInput->clear();
            circularBufferInput->setEnabled(false);
            circularBufferInput->setPlaceholderText(tr("N/A (incompatible with split)"));
            qDebug() << "[Circular Buffer] Initial state: Disabled (incompatible with split)";
        } else {
            // Split files is not active -> disable segment duration
            if (segmentDurationInput) {
                segmentDurationInput->clear();
                segmentDurationInput->setEnabled(false);
                segmentDurationInput->setPlaceholderText(tr("N/A (requires split files)"));
                qDebug() << "[Segment Duration] Initial state: Disabled (split files inactive)";
            }
        }
    }

    // Separator
    auto *separatorLine = new QFrame(this);
    separatorLine->setFrameShape(QFrame::HLine);
    separatorLine->setFrameShadow(QFrame::Sunken);
    recordingOptionsLayout->addWidget(separatorLine);

    // Segment Duration
    auto *segmentLayout = new QHBoxLayout;
    QLabel *segmentLabel = new QLabel(tr("Segment Duration:"), this);
    segmentLabel->setFixedWidth(120);
    segmentLayout->addWidget(segmentLabel);
    segmentDurationInput = new QLineEdit(this);
    segmentDurationInput->setFixedWidth(80);
    segmentDurationInput->setPlaceholderText("0");
    segmentDurationInput->setValidator(new QIntValidator(0, 999999, this));
    segmentDurationInput->setToolTip(tr("Break recording into segments (milliseconds, 0=disabled). Only works with MJPEG codec!"));
    segmentDurationInput->setEnabled(false); // Disabled by default, enabled with compatible codec
    segmentLayout->addWidget(segmentDurationInput);
    segmentLayout->addWidget(new QLabel(tr("ms"), this));
    segmentLayout->addStretch();
    recordingOptionsLayout->addLayout(segmentLayout);

    // Circular Buffer
    auto *circularLayout = new QHBoxLayout;
    QLabel *circularLabel = new QLabel(tr("Circular Buffer:"), this);
    circularLabel->setFixedWidth(120);
    circularLayout->addWidget(circularLabel);
    circularBufferInput = new QLineEdit(this);
    circularBufferInput->setFixedWidth(80);
    circularBufferInput->setPlaceholderText("0");
    circularBufferInput->setValidator(new QIntValidator(0, 999999, this));
    circularBufferInput->setToolTip(tr("Use circular buffer (MB, 0=disabled). Only works with MJPEG codec!"));
    circularBufferInput->setEnabled(false); // Disabled by default, enabled with compatible codec
    circularLayout->addWidget(circularBufferInput);
    circularLayout->addWidget(new QLabel(tr("MB"), this));
    circularLayout->addStretch();
    recordingOptionsLayout->addLayout(circularLayout);

    // Flush Checkbox
    flushCheckbox = new QCheckBox(tr("Flush output immediately (--flush)"), this);
    flushCheckbox->setToolTip(tr("Flush output data as soon as possible"));
    recordingOptionsLayout->addWidget(flushCheckbox);

    // Save PTS File
    auto *ptsLayout = new QHBoxLayout;
    QLabel *ptsLabel = new QLabel(tr("Save PTS to:"), this);
    ptsLabel->setFixedWidth(120);
    ptsLayout->addWidget(ptsLabel);
    savePtsInput = new QLineEdit(this);
    savePtsInput->setPlaceholderText(tr("timestamps.txt"));
    savePtsInput->setToolTip(tr("Save frame timestamps to file (--save-pts)"));
    ptsLayout->addWidget(savePtsInput);
    auto *ptsBrowseButton = new QPushButton(tr("Browse..."), this);
    connect(ptsBrowseButton, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save PTS File"), "", "Text Files (*.txt);;All Files (*)");
        if (!fileName.isEmpty()) {
            savePtsInput->setText(fileName);
        }
    });
    ptsLayout->addWidget(ptsBrowseButton);
    auto *recordingResetButton = new QPushButton("✕", this);
    recordingResetButton->setFixedWidth(20);
    recordingResetButton->setToolTip(tr("Reset Recording Options to defaults"));
    ptsLayout->addStretch();
    ptsLayout->addWidget(recordingResetButton);
    recordingOptionsLayout->addLayout(ptsLayout);

    connect(recordingResetButton, &QPushButton::clicked, this, [this, recordingResetButton]() {
        if (signalRecordingCheckbox) signalRecordingCheckbox->setChecked(false);
        if (keypressRecordingCheckbox) keypressRecordingCheckbox->setChecked(false);
        if (initialStateComboBox) initialStateComboBox->setCurrentText("pause");
        if (splitFilesCheckbox) splitFilesCheckbox->setChecked(false);
        if (segmentDurationInput) segmentDurationInput->clear();
        if (circularBufferInput) circularBufferInput->clear();
        if (segmentPatternCheckbox) segmentPatternCheckbox->setChecked(false);
        if (flushCheckbox) flushCheckbox->setChecked(false);
        if (savePtsInput) savePtsInput->clear();
        recordingResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });

    // Connect signals for recording options to update reset button color
    if (signalRecordingCheckbox) {
        connect(signalRecordingCheckbox, &QCheckBox::toggled, this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = !signalRecordingCheckbox->isChecked() &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (keypressRecordingCheckbox) {
        connect(keypressRecordingCheckbox, &QCheckBox::toggled, this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            !keypressRecordingCheckbox->isChecked() &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (initialStateComboBox) {
        connect(initialStateComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            initialStateComboBox->currentText() == "pause" &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (splitFilesCheckbox) {
        connect(splitFilesCheckbox, &QCheckBox::toggled, this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            !splitFilesCheckbox->isChecked() &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (segmentDurationInput) {
        connect(segmentDurationInput, &QLineEdit::textChanged, this, [this, recordingResetButton]() {
            if (isInitializing) return;

            // Disable circular buffer if segment duration is set (incompatible modes)
            if (circularBufferInput) {
                QString segmentValue = segmentDurationInput->text().trimmed();
                if (!segmentValue.isEmpty() && segmentValue.toInt() > 0) {
                    circularBufferInput->clear();
                    circularBufferInput->setEnabled(false);
                    circularBufferInput->setPlaceholderText(tr("N/A (incompatible with segment)"));
                    qDebug() << "[Circular Buffer] Disabled (incompatible with segment duration)";
                } else {
                    // Re-enable based on codec and split compatibility
                    bool splitActive = (splitFilesCheckbox && splitFilesCheckbox->isChecked());
                    if (!splitActive && codecSelector && codecSelector->currentText() == "mjpeg") {
                        circularBufferInput->setEnabled(true);
                        circularBufferInput->setPlaceholderText(tr("0"));
                        qDebug() << "[Circular Buffer] Re-enabled (segment cleared)";
                    }
                }
            }

            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            segmentDurationInput->text().isEmpty() &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (circularBufferInput) {
        connect(circularBufferInput, &QLineEdit::textChanged, this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            circularBufferInput->text().isEmpty() &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (flushCheckbox) {
        connect(flushCheckbox, &QCheckBox::toggled, this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            !flushCheckbox->isChecked() &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }
    if (savePtsInput) {
        connect(savePtsInput, &QLineEdit::textChanged, this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            (!segmentPatternCheckbox || !segmentPatternCheckbox->isChecked()) &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            savePtsInput->text().isEmpty();
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }

    // Connect segmentPatternCheckbox to update recording reset button
    if (segmentPatternCheckbox) {
        connect(segmentPatternCheckbox, &QCheckBox::toggled, this, [this, recordingResetButton]() {
            if (isInitializing) return;
            bool isDefault = (!signalRecordingCheckbox || !signalRecordingCheckbox->isChecked()) &&
                            (!keypressRecordingCheckbox || !keypressRecordingCheckbox->isChecked()) &&
                            (!initialStateComboBox || initialStateComboBox->currentText() == "pause") &&
                            (!splitFilesCheckbox || !splitFilesCheckbox->isChecked()) &&
                            (!segmentDurationInput || segmentDurationInput->text().isEmpty()) &&
                            (!circularBufferInput || circularBufferInput->text().isEmpty()) &&
                            !segmentPatternCheckbox->isChecked() &&
                            (!flushCheckbox || !flushCheckbox->isChecked()) &&
                            (!savePtsInput || savePtsInput->text().isEmpty());
            recordingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
            updateGlobalResetButtonColor();
        });
    }

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(recordingOptionsGroup, "UI/Video/RecordingOptionsGroup", [this]() { adjustWindowToOptimalSize(); }));

    // Add Recording Options Group to Video Tab (formerly Output Tab)
    if (outputTab && outputTabLayout) {
        outputTabLayout->insertWidget(outputTabLayout->count() - 1, recordingOptionsGroup);  // Insert before stretch
    }

    // Connect signal recording checkbox to enable/disable related controls
    connect(signalRecordingCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        bool signalEnabled = (state == Qt::Checked);
        bool keypressEnabled = keypressRecordingCheckbox ? keypressRecordingCheckbox->isChecked() : false;
        bool anyRecordingEnabled = signalEnabled || keypressEnabled;

        // Initial state and split files should be enabled if EITHER signal or keypress is active
        initialStateComboBox->setEnabled(anyRecordingEnabled);
        splitFilesCheckbox->setEnabled(anyRecordingEnabled);

        // Disable circular buffer when signal recording is enabled with split files
        // (these modes are incompatible according to rpicam-vid_help.txt)
        if (signalEnabled && splitFilesCheckbox && splitFilesCheckbox->isChecked() && circularBufferInput) {
            circularBufferInput->clear();
            circularBufferInput->setEnabled(false);
            circularBufferInput->setPlaceholderText(tr("N/A (incompatible with split)"));
            qDebug() << "[Circular Buffer] Disabled due to Signal Recording + Split Files";
        }

        // Update %04d checkbox based on signal recording, split files, and codec
        if (segmentPatternCheckbox && codecSelector) {
            bool isMjpeg = (codecSelector->currentText() == "mjpeg");
            if (signalEnabled && splitFilesCheckbox && splitFilesCheckbox->isChecked() && isMjpeg) {
                // Signal recording enabled, split files checked, AND codec is MJPEG -> enable %04d
                segmentPatternCheckbox->setEnabled(true);
                segmentPatternCheckbox->setChecked(true);
                qDebug() << "[%04d] Signal Recording + Split Files + MJPEG -> %04d enabled";
            } else {
                // Conditions not met -> disable %04d
                segmentPatternCheckbox->setEnabled(false);
                segmentPatternCheckbox->setChecked(false);
                qDebug() << "[%04d] Conditions not met -> %04d disabled";
            }
        }

        // Update sendSignalButton visibility when Signal Recording checkbox changes
        updateButtonVisibility();

        // Also enable/disable segment and circular based on codec compatibility
        if (signalEnabled && codecSelector) {
            QString codec = codecSelector->currentText();
            bool isMjpeg = (codec == "mjpeg");
            if (segmentDurationInput) segmentDurationInput->setEnabled(isMjpeg);
            if (circularBufferInput) circularBufferInput->setEnabled(isMjpeg);
        } else {
            if (segmentDurationInput) segmentDurationInput->setEnabled(false);
            if (circularBufferInput) circularBufferInput->setEnabled(false);
        }
    });

    // Connect keypress recording checkbox to enable/disable related controls
    connect(keypressRecordingCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        bool keypressEnabled = (state == Qt::Checked);
        bool signalEnabled = signalRecordingCheckbox ? signalRecordingCheckbox->isChecked() : false;
        bool anyRecordingEnabled = signalEnabled || keypressEnabled;

        // Initial state and split files should be enabled if EITHER signal or keypress is active
        initialStateComboBox->setEnabled(anyRecordingEnabled);
        splitFilesCheckbox->setEnabled(anyRecordingEnabled);

        qDebug() << "[Keypress Recording] State changed. Keypress:" << keypressEnabled
                 << "Signal:" << signalEnabled << "=> Controls enabled:" << anyRecordingEnabled;
    });

    // Send Signal Button - manually trigger SIGUSR1
    connect(sendSignalButton, &QPushButton::clicked, this, [this]() {
        if (this->process.state() == QProcess::Running && appSelector->currentText() == "rpicam-vid") {
            qint64 pid = this->process.processId();
            if (pid > 0 && signalRecordingCheckbox && signalRecordingCheckbox->isChecked()) {
                qDebug() << "[MANUAL SIGNAL] Sending SIGUSR1 to PID:" << pid;
                if (kill(static_cast<pid_t>(pid), SIGUSR1) == 0) {
                    // Toggle the state
                    isRecordingPaused = !isRecordingPaused;

                    // Update button text and style based on current state
                    if (isRecordingPaused) {
                        sendSignalButton->setText(tr("Resume"));
                        sendSignalButton->setStyleSheet(
                            "QPushButton {"
                            "background-color: #007acc;"  // Same blue as Start button
                            "color: white;"
                            "font-weight: bold;"
                            "padding: 10px 20px;"  // Exactly same as Start/Stop
                            "border: none;"
                            "border-radius: 5px;"
                            "outline: none;"
                            "}"
                            "QPushButton:pressed {"
                            "background-color: #005f99;"
                            "}"
                        );
                        qDebug() << "[MANUAL SIGNAL] Recording PAUSED - button shows RESUME";
                    } else {
                        sendSignalButton->setText(tr("Pause"));
                        sendSignalButton->setStyleSheet(
                            "QPushButton {"
                            "background-color: #ff8800;"  // Same orange as Stop button
                            "color: white;"
                            "font-weight: bold;"
                            "padding: 10px 20px;"  // Exactly same as Start/Stop
                            "border: none;"
                            "border-radius: 5px;"
                            "outline: none;"
                            "}"
                            "QPushButton:pressed {"
                            "background-color: #e67700;"
                            "}"
                        );
                        qDebug() << "[MANUAL SIGNAL] Recording ACTIVE - button shows PAUSE";
                    }
                } else {
                    qDebug() << "[MANUAL SIGNAL] Failed to send signal to PID:" << pid;
                }
            }
        }
    });

    // Connect circular buffer input to disable split files when a value is entered
    // (circular buffer and split files are incompatible modes)
    connect(circularBufferInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (splitFilesCheckbox && !text.trimmed().isEmpty() && text.trimmed() != "0") {
            // A non-zero value was entered in circular buffer -> disable split files
            if (splitFilesCheckbox->isChecked()) {
                splitFilesCheckbox->setChecked(false);
                qDebug() << "[Split Files] Disabled due to Circular Buffer value:" << text;
            }
        }
    });

    connect(resetButton, &QPushButton::clicked, this, [this]() {
        QString defaultBoxValue = calculateBoxInput(+30);
        BoxInput->setText(defaultBoxValue);
    });
    connect(doubleSizeCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        Q_UNUSED(state); // Der Zustand wird hier nicht direkt verwendet
        QString updatedBoxValue = calculateBoxInput(+30); // Neuberechnung durchführen
        BoxInput->setText(updatedBoxValue); // Aktualisiere das BoxInput-Feld
        qDebug() << "BoxInput updated after x2 checkbox state change:" << updatedBoxValue;
    });

    // Connectors
    // SelectionOverlay nur für BoxInput
    connect(selectionOverlay, &SelectionOverlay::selectionChanged, this, [this](const QRect &selection) {
        // Nur für BoxInput verwenden
        BoxInput->setText(QString("%1,%2,%3,%4")
            .arg(selection.x())
            .arg(selection.y())
            .arg(selection.width())
            .arg(selection.height()));
        updateOverlayResetButtonColor(overlayResetButton);
        updateBoxInputFromSelection(selection);
    });
    connect(appSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateCodecVisibility);

    connect(outputFileName, &QLineEdit::textChanged, this, [this, resetOutputFileButton](const QString &text) {
        updateResetButtonColor(resetOutputFileButton, !text.isEmpty(), 0);
        updateOutputFileResetButtonColor();
    });

    // Verbindung für den Reset-Button
    connect(resetOutputFileButton, &QPushButton::clicked, this, [this, resetOutputFileButton]() {
        outputFileName->clear();
        autoNamingCheckbox->setChecked(false);
        timestampCheckbox->setChecked(false);

        // Zurück zu File-Mode
        if (outputModeFile) {
            outputModeFile->setChecked(true);
        }

        updateResetButtonColor(resetOutputFileButton, 0, 0);

        // Auch den globalen Reset-Button aktualisieren
        if (!isInitializing) {
            updateGlobalResetButtonColor();
        }
    });

    // Codec selector connection is now in Video Tab (codecResetButton)
    connect(codecSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        qDebug() << "[CODEC] Changed to:" << text;

        // Codec-spezifische Elemente ein-/ausblenden
        bool isH264 = (text == "h264");
        bool isLibav = (text == "libav");
        bool isMjpeg = (text == "mjpeg");
        bool isYuv420 = (text == "yuv420");

        // H264 & LibAV: profile (beide nutzen H264-Profile)
        if (profileLabel) profileLabel->setVisible(isH264 || isLibav);
        if (profileSelector) profileSelector->setVisible(isH264 || isLibav);

        // H264 Only: level
        if (levelLabel) levelLabel->setVisible(isH264);
        if (levelSelector) levelSelector->setVisible(isH264);

        // H264 & LibAV: Inline Headers (für beide codecs)
        if (inlineHeadersCheckbox) inlineHeadersCheckbox->setVisible(isH264 || isLibav);

        // LibAV Only: format, video-codec, codec-opts, low-latency
        if (libavFormatLabel) libavFormatLabel->setVisible(isLibav);
        if (libavFormatSelector) libavFormatSelector->setVisible(isLibav);
        if (libavVideoCodecLabel) libavVideoCodecLabel->setVisible(isLibav);
        if (libavVideoCodecSelector) libavVideoCodecSelector->setVisible(isLibav);
        if (libavCodecOptsLabel) libavCodecOptsLabel->setVisible(isLibav);
        if (libavCodecOptsSelector) libavCodecOptsSelector->setVisible(isLibav);
        if (lowLatencyCheckbox) lowLatencyCheckbox->setVisible(isLibav);

        // Reset-Button repositionieren: Row 0 für MJPEG/YUV420, Row 1 für H264, Row 4 für LibAV
        auto *codecGroupLayout = qobject_cast<QGridLayout*>(codecGroup->layout());
        if (codecGroupLayout && codecResetButton) {
            codecGroupLayout->removeWidget(codecResetButton);
            int resetRow = 0;  // Default für MJPEG/YUV420
            if (isH264) resetRow = 1;
            if (isLibav) resetRow = 4;
            codecGroupLayout->addWidget(codecResetButton, resetRow, 6);
        }

        // MJPEG hat quality (wird separat behandelt)
        // YUV420 hat keine speziellen Optionen

        // Audio ist nur für libav verfügbar - deaktiviere bei anderem Codec
        if (!isLibav && enableAudioCheckBox && enableAudioCheckBox->isChecked()) {
            enableAudioCheckBox->setChecked(false);
            appendLog(tr("Audio recording disabled: codec changed to non-libav"));
        }

        qDebug() << "[CODEC] Visibility: H264=" << isH264 << "LibAV=" << isLibav << "MJPEG=" << isMjpeg;

        // Update auto-naming extension if auto-naming is enabled
        if (autoNamingCheckbox && autoNamingCheckbox->isChecked()) {
            QString app = appSelector->currentText();
            if (app == "rpicam-vid" || app == "rpicam-focus" || app == "rpicam-focus008") {
                QString baseName = "video";

                // Füge "_audio" hinzu wenn Audio aktiviert ist
                if (enableAudioCheckBox && enableAudioCheckBox->isChecked() && isLibav) {
                    baseName += "_audio";
                }

                QString extension;
                if (text == "mjpeg") {
                    extension = ".mjpeg";
                } else if (text == "yuv420") {
                    extension = ".avi";  // YUV in AVI container
                } else {
                    extension = ".mp4";  // H264, libav and default: MP4 container
                }

                QString fileName = QDir(guiOutputFilePath).filePath(baseName + extension);
                outputFileName->setText(fileName);
            }
        }

        // Check codec compatibility with pause/resume recording features
        // Pi 0-4: H.264 hardware encoder supports --segment/--split/--signal (with --inline)
        // Pi 5: Only MJPEG supported (libav encoder limitation)
        // libav encoder does NOT support: --circular, --segment, --split, --pause, --signal, --keypress
        // Current: MJPEG-only for Pi 5 compatibility
        // Note: isMjpeg already declared above (line ~3724)
        QLabel *warningLabel = findChild<QLabel*>("codecWarningLabel");

        // Handle Signal Recording checkbox (MJPEG only)
        if (signalRecordingCheckbox) {
            if (!isMjpeg && signalRecordingCheckbox->isChecked()) {
                signalRecordingCheckbox->setChecked(false);
                if (warningLabel) {
                    warningLabel->setText(tr("⚠ Signal/Keypress Recording requires MJPEG codec"));
                    warningLabel->setVisible(true);
                }
            }
            signalRecordingCheckbox->setEnabled(isMjpeg);

            if (!isMjpeg) {
                signalRecordingCheckbox->setToolTip(tr("Signal recording requires MJPEG codec (--signal only works with MJPEG)"));
            } else {
                signalRecordingCheckbox->setToolTip(tr("Enable pause/resume recording via SIGUSR1 signal (MJPEG only)"));
            }
        }

        // Handle Keypress Recording checkbox (MJPEG only)
        if (keypressRecordingCheckbox) {
            if (!isMjpeg && keypressRecordingCheckbox->isChecked()) {
                keypressRecordingCheckbox->setChecked(false);
                if (warningLabel) {
                    warningLabel->setText(tr("⚠ Signal/Keypress Recording requires MJPEG codec"));
                    warningLabel->setVisible(true);
                }
            }
            keypressRecordingCheckbox->setEnabled(isMjpeg);

            if (!isMjpeg) {
                keypressRecordingCheckbox->setToolTip(tr("Keypress recording requires MJPEG codec (--keypress only works with MJPEG)"));
            } else {
                keypressRecordingCheckbox->setToolTip(tr("Enable ENTER key to pause/resume recording (--keypress)\nCan be used independently or together with Signal Recording"));
            }
        }

        // Handle Split Files, Segment Duration, and Circular Buffer (MJPEG only on Pi 5)
        if (splitFilesCheckbox) {
            if (!isMjpeg && splitFilesCheckbox->isChecked()) {
                splitFilesCheckbox->setChecked(false);
            }
            splitFilesCheckbox->setEnabled(isMjpeg);
            if (!isMjpeg) {
                splitFilesCheckbox->setToolTip(tr("Split files requires MJPEG codec (Pi 5 limitation)"));
            } else {
                splitFilesCheckbox->setToolTip(tr("Create separate files for each pause/resume cycle (--split)"));
            }
        }

        if (segmentDurationInput) {
            segmentDurationInput->setEnabled(isMjpeg);
            if (!isMjpeg) {
                segmentDurationInput->setToolTip(tr("Segment duration requires MJPEG codec (Pi 5 limitation)"));
            } else {
                segmentDurationInput->setToolTip(tr("Break recording into segments (milliseconds, 0=disabled)"));
            }
        }

        if (circularBufferInput) {
            circularBufferInput->setEnabled(isMjpeg);
            if (!isMjpeg) {
                circularBufferInput->setToolTip(tr("Circular buffer requires MJPEG codec"));
            } else {
                circularBufferInput->setToolTip(tr("Use circular buffer (MB, 0=disabled)"));
            }
        }

        // Handle Segment Pattern (%04d) checkbox
        if (segmentPatternCheckbox) {
            if (!isMjpeg && segmentPatternCheckbox->isChecked()) {
                segmentPatternCheckbox->setChecked(false);
            }
            segmentPatternCheckbox->setEnabled(isMjpeg);
            if (!isMjpeg) {
                segmentPatternCheckbox->setToolTip(tr("Segment pattern requires MJPEG codec"));
            } else {
                segmentPatternCheckbox->setToolTip(tr("Add segment pattern for split/segment recording"));
            }
        }

        // Hide warning if codec is compatible or if both checkboxes are unchecked
        if (warningLabel && (isMjpeg ||
            (!signalRecordingCheckbox->isChecked() && !keypressRecordingCheckbox->isChecked()))) {
            warningLabel->setVisible(false);
        }
    });

    // AWB connect
    connect(awbSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        bool hasSelection = (text != "auto");
        updateResetButtonColor(resetAwbButton, hasSelection ? 1 : 0, 0);
        if (isControlSocketActive() && !text.isEmpty()) sendSliderToSocket("awb", text.toLower());
    });
    connect(resetAwbButton, &QPushButton::clicked, this, [this]() {
        awbSelector->setCurrentIndex(0);
        awbSelector->setCurrentText("auto");
        updateResetButtonColor(resetAwbButton, 0, 0);
    });

    // Metering connect
    connect(meteringSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        bool hasSelection = (text != "Select option:");
        meteringCustomInput->setVisible(text == "custom");
        updateResetButtonColor(meteringResetButton, hasSelection ? 1 : 0, 0);
        if (isControlSocketActive() && !text.isEmpty() && text != "Select option:") sendSliderToSocket("metering", text.toLower());
    });
    connect(meteringResetButton, &QPushButton::clicked, this, [this]() {
        meteringSelector->setCurrentIndex(0);
        meteringSelector->setCurrentText("Select option:");
        meteringCustomInput->clear();
        meteringCustomInput->setVisible(false);
        updateResetButtonColor(meteringResetButton, 0, 0);
        if (isControlSocketActive()) sendSliderToSocket("metering", "centre");
    });
    connect(meteringCustomInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        // Custom input validation could be added here if needed
        Q_UNUSED(text);
    });

    // Low Resolution connect (new custom widget)
    connect(loresComboBox, &LoresComboBox::loresConfigChanged, this, [this]() {
        bool hasConfig = loresComboBox->isLoresEnabled();
        updateResetButtonColor(loresResetButton, hasConfig ? 1 : 0, 0);
    });

    connect(loresResetButton, &QPushButton::clicked, this, [this]() {
        loresComboBox->reset();
        updateResetButtonColor(loresResetButton, 0, 0);
    });

    // Sharpness connect -warum hier 3 absätze(reset)?
    connect(sharpnessSlider, &QSlider::valueChanged, this, [this, sharpnessResetButton](int value) {
        double sharpness = value / 10.0;
        sharpnessInput->setText(QString::number(sharpness, 'f', 1));
        updateResetButtonColor(sharpnessResetButton, sharpness, DEFAULT_SHARPNESS);
        if (isControlSocketActive()) sendSliderToSocket("sharpness", QString::number(sharpness, 'f', 1));
    });
    connect(sharpnessInput, &QLineEdit::textChanged, this, [this, sharpnessResetButton](const QString &text) {
        double sharpness = text.toDouble();
        sharpnessSlider->setValue(static_cast<int>(sharpness * 10));
        updateResetButtonColor(sharpnessResetButton, sharpness, DEFAULT_SHARPNESS);
    });
    connect(sharpnessResetButton, &QPushButton::clicked, this, [this]() {
        sharpnessSlider->setValue(10); // Standardwert (1.0 * 10)
        sharpnessInput->setText("1.0");
    });
    connect(evSlider, &QSlider::valueChanged, this, [this, evResetButton](int value) {
        double ev = value / 10.0;
        evInput->setText(QString::number(ev, 'f', 1));
        updateResetButtonColor(evResetButton, ev, DEFAULT_EV);
        if (isControlSocketActive()) sendSliderToSocket("ev", QString::number(ev, 'f', 1));
    });
    connect(evInput, &QLineEdit::textChanged, this, [this, evResetButton](const QString &text) {
        double ev = text.toDouble();
        evSlider->setValue(static_cast<int>(ev * 10));
        updateResetButtonColor(evResetButton, ev, DEFAULT_EV);
    });
    connect(evResetButton, &QPushButton::clicked, this, [this]() {
        evSlider->setValue(0); // Standardwert (0.0 * 10)
        evInput->setText("0.0");
        });
    connect(gainSlider, &QSlider::valueChanged, this, [this, gainResetButton](int value) {
        double gain = value / 10.0;
        gainInput->setText(QString::number(gain, 'f', 1));
        updateResetButtonColor(gainResetButton, gain, DEFAULT_GAIN);
        if (isControlSocketActive()) sendSliderToSocket("gain", QString::number(gain, 'f', 1));
    });
    connect(gainInput, &QLineEdit::textChanged, this, [this, gainResetButton](const QString &text) {
        double gain = text.toDouble();
        gainSlider->setValue(static_cast<int>(gain * 10));
        updateResetButtonColor(gainResetButton, gain, DEFAULT_GAIN);
    });
    connect(gainResetButton, &QPushButton::clicked, this, [this]() {
        gainSlider->setValue(0); // Standardwert (0.0 * 10)
        gainInput->setText("0.0");
    });

    // AWB Gain Red Slider/Input Synchronisierung
    connect(awbGainRedSlider, &QSlider::valueChanged, this, [this](int value) {
        double awbGainRed = value / 10.0;
        awbGainRedInput->setText(QString::number(awbGainRed, 'f', 1));
        updateResetButtonColor(awbGainRedResetButton, awbGainRed, DEFAULT_AWB_GAIN_RED);
        if (!isInitializing) updateGlobalResetButtonColor();
        // Switch AWB combo to "custom" so the camera stops auto-adjusting
        if (!isInitializing && value != 15 && awbSelector->currentText() != "custom") {
            awbSelector->blockSignals(true);
            awbSelector->setCurrentText("custom");
            awbSelector->blockSignals(false);
            updateResetButtonColor(resetAwbButton, 1, 0);
            if (isControlSocketActive()) sendSliderToSocket("awb", "custom");
        }
        if (isControlSocketActive()) {
            double blue = awbGainBlueSlider->value() / 10.0;
            sendSliderToSocket("awbgains", QString("%1,%2").arg(awbGainRed, 0, 'f', 1).arg(blue, 0, 'f', 1));
        }
    });
    connect(awbGainRedInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        double awbGainRed = text.toDouble();
        awbGainRedSlider->setValue(static_cast<int>(awbGainRed * 10));
        updateResetButtonColor(awbGainRedResetButton, awbGainRed, DEFAULT_AWB_GAIN_RED);
        if (!isInitializing) updateGlobalResetButtonColor();
    });

    // AWB Gain Blue Slider/Input Synchronisierung
    connect(awbGainBlueSlider, &QSlider::valueChanged, this, [this](int value) {
        double awbGainBlue = value / 10.0;
        awbGainBlueInput->setText(QString::number(awbGainBlue, 'f', 1));
        updateResetButtonColor(awbGainBlueResetButton, awbGainBlue, DEFAULT_AWB_GAIN_BLUE);
        if (!isInitializing) updateGlobalResetButtonColor();
        // Switch AWB combo to "custom" so the camera stops auto-adjusting
        if (!isInitializing && value != 12 && awbSelector->currentText() != "custom") {
            awbSelector->blockSignals(true);
            awbSelector->setCurrentText("custom");
            awbSelector->blockSignals(false);
            updateResetButtonColor(resetAwbButton, 1, 0);
            if (isControlSocketActive()) sendSliderToSocket("awb", "custom");
        }
        if (isControlSocketActive()) {
            double red = awbGainRedSlider->value() / 10.0;
            sendSliderToSocket("awbgains", QString("%1,%2").arg(red, 0, 'f', 1).arg(awbGainBlue, 0, 'f', 1));
        }
    });
    connect(awbGainBlueInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        double awbGainBlue = text.toDouble();
        awbGainBlueSlider->setValue(static_cast<int>(awbGainBlue * 10));
        updateResetButtonColor(awbGainBlueResetButton, awbGainBlue, DEFAULT_AWB_GAIN_BLUE);
        if (!isInitializing) updateGlobalResetButtonColor();
    });

    connect(brightnessSlider, &QSlider::valueChanged, this, [this, brightnessResetButton](int value) {
        double brightness = value / 10.0;
        brightnessInput->setText(QString::number(brightness, 'f', 1));
        updateResetButtonColor(brightnessResetButton, brightness, DEFAULT_BRIGHTNESS);
        if (isControlSocketActive()) sendSliderToSocket("brightness", QString::number(brightness, 'f', 1));
    });
    connect(brightnessInput, &QLineEdit::textChanged, this, [this, brightnessResetButton](const QString &text) {
        double brightness = text.toDouble();
        brightnessSlider->setValue(static_cast<int>(brightness * 10));
        updateResetButtonColor(brightnessResetButton, brightness, DEFAULT_BRIGHTNESS);
    });
    connect(brightnessResetButton, &QPushButton::clicked, this, [this]() {
        brightnessSlider->setValue(0); // Standardwert (0.0 * 10)
        brightnessInput->setText("0.0");
    });
    connect(contrastSlider, &QSlider::valueChanged, this, [this, contrastResetButton](int value) {
        double contrast = value / 10.0;
        contrastInput->setText(QString::number(contrast, 'f', 1));
        updateResetButtonColor(contrastResetButton, contrast, DEFAULT_CONTRAST);
        if (isControlSocketActive()) sendSliderToSocket("contrast", QString::number(contrast, 'f', 1));
    });
    connect(contrastInput, &QLineEdit::textChanged, this, [this, contrastResetButton](const QString &text) {
        double contrast = text.toDouble();
        contrastSlider->setValue(static_cast<int>(contrast * 10));
        updateResetButtonColor(contrastResetButton, contrast, DEFAULT_CONTRAST);
    });
    connect(contrastResetButton, &QPushButton::clicked, this, [this]() {
        contrastSlider->setValue(10); // Standardwert (1.0 * 10)
        contrastInput->setText("1.0");
    });
    connect(saturationSlider, &QSlider::valueChanged, this, [this, saturationResetButton](int value) {
        double saturation = value / 10.0;
        saturationInput->setText(QString::number(saturation, 'f', 1));
        updateResetButtonColor(saturationResetButton, saturation, DEFAULT_SATURATION);
        if (isControlSocketActive()) sendSliderToSocket("saturation", QString::number(saturation, 'f', 1));
    });
    connect(saturationInput, &QLineEdit::textChanged, this, [this, saturationResetButton](const QString &text) {
        double saturation = text.toDouble();
        saturationSlider->setValue(static_cast<int>(saturation * 10));
        updateResetButtonColor(saturationResetButton, saturation, DEFAULT_SATURATION);
    });
    saturationResetButton->setFixedWidth(20);
    connect(saturationResetButton, &QPushButton::clicked, this, [this]() {
        saturationSlider->setValue(10); // Standardwert (1.0 * 10)
        saturationInput->setText("1.0");
    });

    // Framerate socket live send
    connect(framerateSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        // Shutter max depends on framerate — update range before sending
        updateShutterMaxRange();

        if (isControlSocketActive() && !text.isEmpty()) {
            bool ok;
            int fps = text.toInt(&ok);
            if (ok && fps > 0) {
                // Clamp to caps:maxfps if known
                int maxFps = m_controlSocket ? m_controlSocket->maxFps() : 120;
                if (fps > maxFps) fps = maxFps;
                sendSliderToSocket("framerate", QString::number(fps));
            }
        }
    });

    connect(selectionOverlay, &SelectionOverlay::overlayClosed, this, [this]() {
    QRect selectedArea = selectionOverlay->getSelectedArea();
    if (selectedArea.width() <= 0 || selectedArea.height() <= 0) {
        QMessageBox::warning(this, tr("Ungültige Auswahl"), tr("Bitte eine gültige Auswahl treffen."));
        return;
    }
});
    connect(autoNamingCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
    if (checked) {
        timestampCheckbox->setChecked(true);
        QString app = appSelector->currentText();
        QString baseName = "video";
        QString extension = ".mp4"; // Default

        if (app == "rpicam-vid" || app == "rpicam-focus" || app == "rpicam-focus008") {
            baseName = "video";
            // Determine extension based on codec
            QString codec = codecSelector->currentText();
            qDebug() << "[AutoNaming] App:" << app << "Codec:" << codec;

            // Füge "_audio" hinzu wenn Audio aktiviert ist und Codec libav ist
            if (enableAudioCheckBox && enableAudioCheckBox->isChecked() && codec == "libav") {
                baseName += "_audio";
                qDebug() << "[AutoNaming] Audio enabled, baseName:" << baseName;
            }

            if (codec == "mjpeg") {
                extension = ".mjpeg";
                qDebug() << "[AutoNaming] Setting extension to .mjpeg";
            } else if (codec == "h264") {
                extension = ".mp4";  // H264 in MP4 container for playback
                qDebug() << "[AutoNaming] Setting extension to .mp4 (h264)";
            } else if (codec == "yuv420") {
                extension = ".avi";  // YUV in AVI container for playback
                qDebug() << "[AutoNaming] Setting extension to .avi (yuv420)";
            } else {
                extension = ".mp4"; // Default fallback
                qDebug() << "[AutoNaming] Setting extension to .mp4 (fallback)";
            }
        } else if (app == "rpicam-jpeg" || app == "rpicam-still") {
            baseName = "image";
            // Hole Extension aus Encoding-Selector
            if (encodingSelector) {
                QString encoding = encodingSelector->currentData().toString();
                extension = getExtensionForEncoding(encoding);
                qDebug() << "[AutoNaming] Still-App Encoding:" << encoding << "→" << extension;
            } else {
                extension = ".jpg"; // Fallback
                qDebug() << "[AutoNaming] encodingSelector NULL, using fallback .jpg";
            }
        }

        QString fileName = QDir(guiOutputFilePath).filePath(baseName + extension);
        qDebug() << "[AutoNaming] Final filename:" << fileName;
        outputFileName->setText(fileName);
    } else {
        timestampCheckbox->setChecked(false); // Deaktiviere die Timestamp-Checkbox
        outputFileName->clear(); // Leere das Output-Feld
    }

    // Update Output File Reset Button Color
    updateOutputFileResetButtonColor();
});

// Separate connect for timestampCheckbox to update Output File Reset Button
connect(timestampCheckbox, &QCheckBox::toggled, this, [this]() {
    updateOutputFileResetButtonColor();
});

connect(resetPostProcessFileButton, &QPushButton::clicked, this, [this]() {
    postProcessFileSelector->setCurrentIndex(-1);
    postProcessFileSelector->setCurrentText("");
    updateResetButtonColor(resetPostProcessFileButton, 0, 0); // Farbe zurücksetzen
});
connect(postProcessFileSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    bool hasSelection = !text.isEmpty();
    updateResetButtonColor(resetPostProcessFileButton, hasSelection ? 1 : 0, 0);
});

connect(resetTuningFileButton, &QPushButton::clicked, this, [this]() {
    tuningFileSelector->setCurrentIndex(-1);
    tuningFileSelector->setCurrentText("");
    updateResetButtonColor(resetTuningFileButton, 0, 0); // Farbe zurücksetzen
});
connect(tuningFileSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    bool hasSelection = !text.isEmpty();
    updateResetButtonColor(resetTuningFileButton, hasSelection ? 1 : 0, 0);
});

// OLD: Geometry/InfoText/ROI Layouts mit CheckableComboBox wurden durch neue Gruppen ersetzt (line ~280-420)
// OLD: Signal-Slots für geometryResetButton, infoTextResetButton, geometryComboBox, infoTextComboBox entfernt
// NEW: Checkboxes verwenden jetzt direkte Checkbox-Verbindungen

// ROI Signal-Slots
connect(roiResetButton, &QPushButton::clicked, this, [this]() {
    roiInput->setText("0.0,0.0,1.0,1.0");
    updateROIResetButtonColor();
});

connect(roiInput, &QLineEdit::textChanged, this, [this]() {
    updateROIResetButtonColor();
    if (isControlSocketActive() && !roiInput->text().isEmpty()) sendSliderToSocket("roi", roiInput->text());
});

// ROI Preview-Integration
connect(roiInput, &CustomLineEdit::doubleClicked, this, [this]() {
    // Setze Target auf ROI_INPUT
    currentROITarget = ROISelectionTarget::ROI_INPUT;

    // Setze Seitenverhältnis basierend auf aktueller Video-Auflösung
    double aspectRatio = getCurrentVideoAspectRatio();
    roiOverlay->setAspectRatio(aspectRatio);

    // Prüfe ob Vorschau bereits läuft
    if (this->process.state() == QProcess::Running) {
        // Preview is already running - use the current BoxInput coordinates for the ROI overlay
        appendLog(tr("Starting ROI selection overlay..."));

        // Use the current BoxInput coordinates
        QString boxCoords = BoxInput->text();
        appendLog("Using current BoxInput coordinates: " + boxCoords);

        // Feed the BoxInput coordinates into the preview window
        roiOverlay->setPreviewFromBoxInput(boxCoords);
        roiOverlay->startROISelection();
    } else {
        // Start the preview for ROI selection
        appendLog(tr("Starting preview for ROI selection..."));

        // Force the Qt preview mode for the ROI selection preview
        int savedIdx = previewSelector->currentIndex();
        int qtIdx = previewSelector->findData("--qt-preview");
        if (qtIdx < 0) qtIdx = previewSelector->findData("qt");
        if (qtIdx >= 0) previewSelector->setCurrentIndex(qtIdx);

        // Start the rpicam app
        startRpiCamApp();

        // Wait briefly, then show the ROI overlay
        QTimer::singleShot(2000, this, [this, savedIdx]() {
            if (this->process.state() == QProcess::Running) {
                // Use the current BoxInput coordinates for the preview window
                QString boxCoords = BoxInput->text();
                appendLog("Using current BoxInput coordinates: " + boxCoords);

                roiOverlay->setPreviewFromBoxInput(boxCoords);
                roiOverlay->startROISelection();
            } else {
                appendLog(tr("Failed to start preview. Check camera connection."));
                // Restore the preview mode
                previewSelector->setCurrentIndex(savedIdx);
            }
        });
    }
});

// Nach dem Setzen von BoxInput:
connect(BoxInput, &QLineEdit::textChanged, this, [this]() {
    updateOverlayResetButtonColor(overlayResetButton);
});

// After overlay reset: clear manual flag so default is used going forward
connect(overlayResetButton, &QPushButton::clicked, this, [this]() {
    QString defaultBoxValue = getDefaultBoxInput();
    BoxInput->setText(defaultBoxValue);
    updateOverlayResetButtonColor(overlayResetButton);
});

// Im Konstruktor nach dem Aufbau der GUI:
connect(timelapseInput, &QComboBox::currentTextChanged, this, &MainWindow::updateOutputFileNameForTimelapse);
connect(appSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateOutputFileNameForTimelapse);
connect(outputFileName, &QLineEdit::editingFinished, this, &MainWindow::updateOutputFileNameForTimelapse);

// Update metadata widget visibility when app changes
connect(appSelector, &QComboBox::currentTextChanged, this, [this](const QString &app) {
    bool showMetadata = (app == "rpicam-still" || app == "rpicam-jpeg");
    if (metadataWidget) {
        metadataWidget->setVisible(showMetadata);
    }
});

// Update UI for still/video apps (encoding vs streaming modes)
connect(appSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateUIForApp);

// Update output file extension when encoding changes
connect(encodingSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::updateOutputFileExtension);

// Reset Encoding to JPEG
if (encodingResetButton) {
    connect(encodingResetButton, &QPushButton::clicked, this, [this]() {
        if (encodingSelector) {
            encodingSelector->setCurrentIndex(0); // JPEG is at index 0
            encodingResetButton->setStyleSheet("color: black;");
            updateGlobalResetButtonColor();
        }
    });

    // Update reset button color when encoding changes
    connect(encodingSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (isInitializing) return;
        bool isDefault = (encodingSelector->currentIndex() == 0); // JPEG is default
        encodingResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
}

// Still Tab Reset Buttons
// Capture Control Group Reset
if (captureControlResetButton) {
    connect(captureControlResetButton, &QPushButton::clicked, this, [this]() {
        if (autofocusOnCaptureCheckbox) autofocusOnCaptureCheckbox->setChecked(false);
        if (zslCheckbox) zslCheckbox->setChecked(false);
        if (immediateCheckbox) immediateCheckbox->setChecked(false);
        if (framestartSpinBox) framestartSpinBox->setValue(0);
        captureControlResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });

    // Update color when any value changes
    auto updateCaptureControlResetColor = [this]() {
        if (isInitializing) return;
        bool hasChanges = (autofocusOnCaptureCheckbox && autofocusOnCaptureCheckbox->isChecked()) ||
                         (zslCheckbox && zslCheckbox->isChecked()) ||
                         (immediateCheckbox && immediateCheckbox->isChecked()) ||
                         (framestartSpinBox && framestartSpinBox->value() > 0);
        captureControlResetButton->setStyleSheet(hasChanges ? "color: red;" : "color: black;");
        updateGlobalResetButtonColor();
    };

    if (autofocusOnCaptureCheckbox) connect(autofocusOnCaptureCheckbox, &QCheckBox::toggled, this, updateCaptureControlResetColor);
    if (zslCheckbox) connect(zslCheckbox, &QCheckBox::toggled, this, updateCaptureControlResetColor);
    if (immediateCheckbox) connect(immediateCheckbox, &QCheckBox::toggled, this, updateCaptureControlResetColor);
    if (framestartSpinBox) connect(framestartSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, updateCaptureControlResetColor);
}

if (thumbResetButton && thumbLineEdit) {
    connect(thumbResetButton, &QPushButton::clicked, this, [this]() {
        thumbLineEdit->clear();
        thumbResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(thumbLineEdit, &QLineEdit::textChanged, this, [this]() {
        if (isInitializing) return;
        thumbResetButton->setStyleSheet(thumbLineEdit->text().isEmpty() ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
}

if (restartResetButton && restartSpinBox) {
    connect(restartResetButton, &QPushButton::clicked, this, [this]() {
        restartSpinBox->setValue(0);
        restartResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(restartSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        if (isInitializing) return;
        restartResetButton->setStyleSheet(restartSpinBox->value() == 0 ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
}

if (exifResetButton && exifLineEdit) {
    connect(exifResetButton, &QPushButton::clicked, this, [this]() {
        exifLineEdit->clear();
        exifResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });
    connect(exifLineEdit, &QLineEdit::textChanged, this, [this]() {
        if (isInitializing) return;
        exifResetButton->setStyleSheet(exifLineEdit->text().isEmpty() ? "color: black;" : "color: red;");
        updateGlobalResetButtonColor();
    });
}

// File Management Group Reset
if (fileManagementResetButton) {
    connect(fileManagementResetButton, &QPushButton::clicked, this, [this]() {
        if (latestLineEdit) latestLineEdit->clear();
        if (rawCheckbox) rawCheckbox->setChecked(false);
        fileManagementResetButton->setStyleSheet("color: black;");
        updateGlobalResetButtonColor();
    });

    // Update color when any value changes
    auto updateFileManagementResetColor = [this]() {
        if (isInitializing) return;
        bool hasChanges = (latestLineEdit && !latestLineEdit->text().isEmpty()) ||
                         (rawCheckbox && rawCheckbox->isChecked());
        fileManagementResetButton->setStyleSheet(hasChanges ? "color: red;" : "color: black;");
        updateGlobalResetButtonColor();
    };

    if (latestLineEdit) connect(latestLineEdit, &QLineEdit::textChanged, this, updateFileManagementResetColor);
    if (rawCheckbox) connect(rawCheckbox, &QCheckBox::toggled, this, updateFileManagementResetColor);
}

// Initial UI update based on current app selection
updateUIForApp(appSelector->currentText());

// Im Konstruktor, nach den jeweiligen Eingabefeldern:
// Entferne diese Zeilen:
// auto *timeoutResetButton = new QPushButton("✕", this);
// timeoutResetButton->setFixedWidth(20);
// timeoutResetButton->setToolTip("Reset Timeout");
//
// auto *shutterResetButton = new QPushButton("✕", this);
// shutterResetButton->setFixedWidth(20);
// shutterResetButton->setToolTip("Reset Shutter");

connect(timelapseResetButton, &QPushButton::clicked, this, [this]() {
    timelapseInput->setCurrentIndex(0); // Setze auf leeren Eintrag
    updateResetButtonColor(timelapseResetButton, timelapseInput->currentText().toInt(), 0);
});
connect(timelapseInput, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    updateResetButtonColor(timelapseResetButton, text.toInt(), 0);
});

// Shutter slider -> input sync (logarithmic scale, max depends on framerate)
connect(shutterSlider, &QSlider::valueChanged, this, [this](int pos) {
    int us = 0;
    if (pos > 0) {
        int maxUs = shutterMaxUs();
        double logRange = std::log10(static_cast<double>(maxUs)) - 2.0; // 2.0 = log10(100)
        if (logRange <= 0.0) logRange = 1.0;
        double logVal = 2.0 + (pos / 1000.0) * logRange;
        us = static_cast<int>(std::round(std::pow(10.0, logVal)));
        if (us < 100) us = 100;
        if (us > maxUs) us = maxUs;
    }
    updateShutterDisplay(us);
    updateResetButtonColor(shutterSliderResetButton, us, 0);
    if (isControlSocketActive() && us > 0) sendSliderToSocket("shutter", QString::number(us));
});
connect(shutterValueInput, &QLineEdit::editingFinished, this, [this]() {
    int maxUs = shutterMaxUs();
    int us = parseShutterInput();
    if (us >= 0 && us <= maxUs) {
        int pos = 0;
        if (us >= 100) {
            double logRange = std::log10(static_cast<double>(maxUs)) - 2.0;
            if (logRange <= 0.0) logRange = 1.0;
            double logPos = (std::log10(static_cast<double>(us)) - 2.0) * 1000.0 / logRange;
            pos = static_cast<int>(std::round(logPos));
            if (pos < 1) pos = 1;
            if (pos > 1000) pos = 1000;
        }
        // Block signals to avoid feedback loop
        shutterSlider->blockSignals(true);
        shutterSlider->setValue(pos);
        shutterSlider->blockSignals(false);
    }
});
connect(shutterSliderResetButton, &QPushButton::clicked, this, [this]() {
    shutterSlider->setValue(0);
    updateShutterDisplay(0);
    updateResetButtonColor(shutterSliderResetButton, 0, 0);
    if (isControlSocketActive()) sendSliderToSocket("shutter", "0");
});

// Denoise connect
connect(hdrSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    bool hasSelection = (text != "off");
    updateResetButtonColor(hdrResetButton, hasSelection ? 1 : 0, 0);
    if (isControlSocketActive() && !text.isEmpty()) sendSliderToSocket("hdr", text.toLower());
});
connect(hdrResetButton, &QPushButton::clicked, this, [this]() {
    hdrSelector->setCurrentText("off");
    updateResetButtonColor(hdrResetButton, 0, 0);
});

connect(denoiseSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    bool hasSelection = (text != "auto");
    updateResetButtonColor(denoiseResetButton, hasSelection ? 1 : 0, 0);
    if (isControlSocketActive() && !text.isEmpty()) sendSliderToSocket("denoise", text.toLower());
});
connect(denoiseResetButton, &QPushButton::clicked, this, [this]() {
    denoiseSelector->setCurrentText("auto");
    updateResetButtonColor(denoiseResetButton, 0, 0);
});

// Flicker Period connect - update reset button on change
connect(flickerPeriodSelector, &QComboBox::currentTextChanged, this, [this](const QString &) {
    QString data = flickerPeriodSelector->currentData().toString();
    bool hasSelection = !data.isEmpty() && data != "off";
    // If user typed custom value (no data role), treat as non-default
    if (data.isEmpty() && !flickerPeriodSelector->currentText().trimmed().isEmpty()) hasSelection = true;
    updateResetButtonColor(flickerPeriodResetButton, hasSelection ? 1 : 0, 0);
    updateGlobalResetButtonColor();
});
connect(flickerPeriodResetButton, &QPushButton::clicked, this, [this]() {
    flickerPeriodSelector->setCurrentIndex(0); // "Off"
    updateResetButtonColor(flickerPeriodResetButton, 0, 0);
    updateGlobalResetButtonColor();
});

// Metadata Auto-Naming Checkbox
connect(metadataAutoNamingCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
    metadataFileEdit->setEnabled(!checked);
    metadataFileButton->setEnabled(!checked);
    // Update reset button color when auto-naming is toggled
    bool hasMetadata = checked || !metadataFileEdit->text().trimmed().isEmpty();
    updateResetButtonColor(metadataResetButton, hasMetadata ? 1 : 0, 0);
    updateGlobalResetButtonColor();
});

// Metadata File connect - enable format selector and update colors
connect(metadataFileEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
    bool hasMetadata = !text.trimmed().isEmpty() || metadataAutoNamingCheckbox->isChecked();
    updateResetButtonColor(metadataResetButton, hasMetadata ? 1 : 0, 0);
    updateGlobalResetButtonColor();
});

// Metadata File Picker Button
connect(metadataFileButton, &QPushButton::clicked, this, [this]() {
    // Use guiMetadataPath as default directory
    QString defaultPath = guiMetadataPath + "/metadata.json";
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Metadata File",
        defaultPath,
        "JSON Files (*.json);;Text Files (*.txt);;All Files (*)"
    );
    if (!filePath.isEmpty()) {
        metadataFileEdit->setText(filePath);
    }
});

// Metadata Format Selector - update global reset
connect(metadataFormatSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
    if (!isInitializing && metadataFormatSelector->isEnabled()) {
        updateGlobalResetButtonColor();
    }
});

// Metadata Reset Button
connect(metadataResetButton, &QPushButton::clicked, this, [this]() {
    metadataFileEdit->clear();
    metadataFormatSelector->setCurrentText("json");
    metadataAutoNamingCheckbox->setChecked(false);
    metadataFileEdit->setEnabled(true);
    metadataFileButton->setEnabled(true);
    updateResetButtonColor(metadataResetButton, 0, 0);
    updateGlobalResetButtonColor();
});

// Autofocus Mode connect
connect(autofocusModeSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    bool hasSelection = (text != "default");
    updateResetButtonColor(resetAutofocusModeButton, hasSelection ? 1 : 0, 0);
});
connect(resetAutofocusModeButton, &QPushButton::clicked, this, [this]() {
    autofocusModeSelector->setCurrentText("auto");
    updateResetButtonColor(resetAutofocusModeButton, 0, 0);
    updateGlobalResetButtonColor();
});

// Autofocus Range connect
connect(autofocusRangeSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    bool hasSelection = (text != "normal");
    updateResetButtonColor(resetAutofocusRangeButton, hasSelection ? 1 : 0, 0);
});
connect(resetAutofocusRangeButton, &QPushButton::clicked, this, [this]() {
    autofocusRangeSelector->setCurrentText("normal");
    updateResetButtonColor(resetAutofocusRangeButton, 0, 0);
});

// Autofocus Speed connect
connect(autofocusSpeedSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    bool hasSelection = (text != "normal");
    updateResetButtonColor(resetAutofocusSpeedButton, hasSelection ? 1 : 0, 0);
});
connect(resetAutofocusSpeedButton, &QPushButton::clicked, this, [this]() {
    autofocusSpeedSelector->setCurrentText("normal");
    updateResetButtonColor(resetAutofocusSpeedButton, 0, 0);
});

// Autofocus Window connect
connect(resetAutofocusWindowButton, &QPushButton::clicked, this, [this]() {
    autofocusWindowInput->setText("0.333,0.333,0.333,0.333");
    updateResetButtonColor(resetAutofocusWindowButton, 0, 0);
});
connect(autofocusWindowInput, &QLineEdit::textChanged, this, [this]() {
    QString currentWindow = autofocusWindowInput->text();
    QString defaultWindow = "0.333,0.333,0.333,0.333";
    bool hasSelection = (currentWindow != defaultWindow && !currentWindow.isEmpty());
    updateResetButtonColor(resetAutofocusWindowButton, hasSelection ? 1 : 0, 0);
});

// Autofocus Window double-click for ROI-like selection
connect(autofocusWindowInput, &CustomLineEdit::doubleClicked, this, [this]() {
    // Set target to AUTOFOCUS_WINDOW
    currentROITarget = ROISelectionTarget::AUTOFOCUS_WINDOW;

    // Set aspect ratio based on the current video resolution
    double aspectRatio = getCurrentVideoAspectRatio();
    roiOverlay->setAspectRatio(aspectRatio);

    // Check whether the preview is already running
    if (this->process.state() == QProcess::Running) {
        // Preview is already running - use the current BoxInput coordinates for the ROI overlay
        appendLog(tr("Starting Autofocus Window selection overlay..."));

        // Use the current BoxInput coordinates
        QString boxCoords = BoxInput->text();
        appendLog("Using current BoxInput coordinates: " + boxCoords);

        // Feed the BoxInput coordinates into the preview window
        roiOverlay->setPreviewFromBoxInput(boxCoords);
        roiOverlay->startROISelection();
    } else {
        // Start the preview for Autofocus Window selection
        appendLog(tr("Starting preview for Autofocus Window selection..."));

        // Force the Qt preview mode for the selection preview
        int savedIdx = previewSelector->currentIndex();
        int qtIdx = previewSelector->findData("--qt-preview");
        if (qtIdx < 0) qtIdx = previewSelector->findData("qt");
        if (qtIdx >= 0) previewSelector->setCurrentIndex(qtIdx);

        // Start the rpicam app
        startRpiCamApp();

        // Wait briefly, then show the ROI overlay
        QTimer::singleShot(2000, this, [this, savedIdx]() {
            if (this->process.state() == QProcess::Running) {
                // Use the current BoxInput coordinates for the preview window
                QString boxCoords = BoxInput->text();
                appendLog("Using current BoxInput coordinates: " + boxCoords);

                roiOverlay->setPreviewFromBoxInput(boxCoords);
                roiOverlay->startROISelection();
            } else {
                appendLog(tr("Failed to start preview. Check camera connection."));
                // Restore the preview mode
                previewSelector->setCurrentIndex(savedIdx);
            }
        });
    }
});

// Lens Position connect
connect(lensPositionSlider, &QSlider::valueChanged, this, [this](int value) {
    double lensPos = value / 100.0;  // Scale from 0-5000 to 0.0-50.0
    lensPositionInput->setText(QString::number(lensPos, 'f', 1));
    updateResetButtonColor(resetLensPositionButton, lensPos, DEFAULT_LENS_POSITION);
});
connect(lensPositionInput, &QLineEdit::textChanged, this, [this](const QString &text) {
    double lensPos = text.toDouble();
    lensPositionSlider->setValue(static_cast<int>(lensPos * 100));  // Scale from 0.0-50.0 to 0-5000
    updateResetButtonColor(resetLensPositionButton, lensPos, DEFAULT_LENS_POSITION);
});
connect(resetLensPositionButton, &QPushButton::clicked, this, [this]() {
    lensPositionSlider->setValue(0);  // Default 0.0
    lensPositionInput->setText("0.0");
});

// Manual Focus Controls - Helper function for v4l2 device
auto getV4L2Device = [this]() -> QString {
    return v4l2DeviceInput->text().isEmpty()
        ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
        : v4l2DeviceInput->text();
};

// Helper function for Zoom v4l2 device
auto getZoomV4L2Device = [this]() -> QString {
    return v4l2DeviceInput->text().isEmpty()
        ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
        : v4l2DeviceInput->text();
};

// Refresh Focus Position
connect(refreshFocusPositionButton, &QPushButton::clicked, this, [this, getV4L2Device]() {
    QString device = getV4L2Device();
    if (currentFocusPositionLabel) currentFocusPositionLabel->setText(tr("reading..."));
    int position = 0;
    qDebug() << "RefreshFocus: reading device:" << device;
    QString err;
    if (V4L2::getControlValue(device, V4L2_CID_FOCUS_ABSOLUTE, position, &err)) {
        currentFocusPositionLabel->setText(QString("%1").arg(position));
        qDebug() << "SetFocusPositionLabel to:" << currentFocusPositionLabel->text();
        int sliderValue = static_cast<int>((static_cast<double>(position) / 32767.0) * 100);
        sliderValue = qBound(0, sliderValue, 100);
        if (focusAbsoluteSlider) {
            focusAbsoluteSlider->blockSignals(true);
            focusAbsoluteSlider->setValue(sliderValue);
            focusAbsoluteSlider->blockSignals(false);
        }
        if (focusAbsoluteInput) {
            focusAbsoluteInput->blockSignals(true);
            focusAbsoluteInput->setText(QString::number(position));
            focusAbsoluteInput->blockSignals(false);
        }
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog(QString("DEBUG: Parsed position: %1").arg(position));
        }
    } else {
        currentFocusPositionLabel->setText(tr("Error reading"));
        qDebug() << "SetFocusPositionLabel to: Error reading";
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog(QString("DEBUG: GetControl failed: %1").arg(err));
        }
        qDebug() << "RefreshFocus: read failed:" << err;
    }
});


// Focus Far (previously Up - negative relative = further away)
connect(focusFarButton, &QPushButton::clicked, this, [this, getV4L2Device]() {
    int value = currentFocusStepSize;
    int signedValue = -value;
    QString device = getV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_FOCUS_RELATIVE, signedValue, &err);
    appendLog(QString("Focus Far: setControlValue focus_relative=%1 - Result: %2").arg(signedValue).arg(ok ? "OK" : "ERROR"));
    // Auto-refresh position after movement
    if (!ok && DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }
    QTimer::singleShot(100, refreshFocusPositionButton, &QPushButton::click);
});

// Focus Near (previously Down - positive relative = closer)
connect(focusNearButton, &QPushButton::clicked, this, [this, getV4L2Device]() {
    int value = currentFocusStepSize;
    QString device = getV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_FOCUS_RELATIVE, value, &err);
    appendLog(QString("Focus Near: setControlValue focus_relative=%1 - Result: %2").arg(value).arg(ok ? "OK" : "ERROR"));
    // Auto-refresh position after movement
    if (!ok && DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }
    QTimer::singleShot(100, refreshFocusPositionButton, &QPushButton::click);
});

connect(focusAbsoluteSlider, &QSlider::valueChanged, this, [this, getV4L2Device](int sliderPercent) {
    // Slider: 0-100 (percentage)
    // Convert to absolute value: 0-32767, rounded to nearest 100
    int absoluteValue = static_cast<int>((sliderPercent / 100.0) * 32767);
    int rounded = qRound(absoluteValue / 100.0) * 100;

    focusAbsoluteInput->setText(QString::number(rounded));

    QString device = getV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_FOCUS_ABSOLUTE, rounded, &err);
    appendLog(QString("Focus Absolute: %1 (%2%) - Result: %3").arg(rounded).arg(sliderPercent).arg(ok ? "OK" : "ERROR"));
    if (ok) {
        if (currentFocusPositionLabel) currentFocusPositionLabel->setText(QString("%1").arg(rounded));
    }
    if (!ok && DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }

    // Auto-refresh position after movement
    QTimer::singleShot(4000, refreshFocusPositionButton, &QPushButton::click);
});

// Lambda für Focus Absolute Input - wird von editingFinished und OK-Button verwendet
auto applyFocusAbsoluteInput = [this, getV4L2Device]() {
    QString text = focusAbsoluteInput->text();
    int absoluteValue = text.toInt();
    if (absoluteValue < 0 || absoluteValue > 32767) return;

    // Round to nearest 100
    int rounded = qRound(absoluteValue / 100.0) * 100;

    // Convert to slider percentage (0-100)
    int sliderPercent = static_cast<int>((rounded / 32767.0) * 100);

    focusAbsoluteSlider->blockSignals(true);
    focusAbsoluteSlider->setValue(sliderPercent);
    focusAbsoluteSlider->blockSignals(false);

    if (rounded != absoluteValue) {
        focusAbsoluteInput->blockSignals(true);
        focusAbsoluteInput->setText(QString::number(rounded));
        focusAbsoluteInput->blockSignals(false);
    }

    // Execute via ioctl to avoid spawning v4l2-ctl process
    QString device = getV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_FOCUS_ABSOLUTE, rounded, &err);
    int percentage = static_cast<int>((rounded / 32767.0) * 100);
    appendLog(QString("Focus Absolute Input: %1 (%2%) - Result: %3").arg(rounded).arg(percentage).arg(ok ? "OK" : "ERROR"));
    if (ok) {
        if (currentFocusPositionLabel) currentFocusPositionLabel->setText(QString("%1").arg(rounded));
    }
    if (!ok && DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }

    // Auto-refresh position after movement
    QTimer::singleShot(4000, refreshFocusPositionButton, &QPushButton::click);
};

connect(focusAbsoluteInput, &QLineEdit::editingFinished, this, applyFocusAbsoluteInput);
connect(focusAbsoluteOkButton, &QPushButton::clicked, this, applyFocusAbsoluteInput);

// Focus Favorites
connect(saveFocusFavoriteButton, &QPushButton::clicked, this, [this, getV4L2Device, getZoomV4L2Device]() {
    QString device = getV4L2Device();
    QString name = focusFavoriteNameInput->text().trimmed();

    // Read focus position
    int focusPosition = 0;
    QString err;
    if (!V4L2::getControlValue(device, V4L2_CID_FOCUS_ABSOLUTE, focusPosition, &err)) {
        currentFocusPositionLabel->setText(tr("Error reading"));
        return;
    }

    // Read zoom position
    int zoomPosition = 0;
    QString zoomDevice = getZoomV4L2Device();
    QString zoomErr;
    bool hasZoom = V4L2::getControlValue(zoomDevice, V4L2_CID_ZOOM_ABSOLUTE, zoomPosition, &zoomErr);

    // auto-generate unique name if empty
    if (name.isEmpty()) {
        int nextNum = 1;
        while (coupledFavorites.contains(QString("Pos%1").arg(nextNum))) {
            nextNum++;
        }
        name = QString("Pos%1").arg(nextNum);
        appendLog(tr("Auto-generated name: %1").arg(name));
    }

    QVariantMap data;
    data["focus"] = focusPosition;
    data["hasFocus"] = true;
    data["zoom"] = hasZoom ? zoomPosition : 0;
    data["hasZoom"] = hasZoom;

    coupledFavorites[name] = data;
    rebuildFavoritesLists();

    appendLog(tr("Saved '%1': Focus=%2, Zoom=%3").arg(name).arg(focusPosition).arg(hasZoom ? QString::number(zoomPosition) : tr("n/a")));
    focusFavoriteNameInput->clear();

    currentFocusPositionLabel->setText(QString::number(focusPosition));
    if (hasZoom) currentZoomPositionLabel->setText(QString::number(zoomPosition));
});

// Single-click: Load name into input field (for editing)
connect(focusFavoritesList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
    QString name = item->data(Qt::UserRole).toString();
    focusFavoriteNameInput->setText(name);
});

// Double-click: Restore BOTH Focus AND Zoom positions
connect(focusFavoritesList, &QListWidget::itemDoubleClicked, this, [this, getV4L2Device, getZoomV4L2Device](QListWidgetItem *item) {
    QString name = item->data(Qt::UserRole).toString();

    if (!coupledFavorites.contains(name)) {
        appendLog(tr("Error: Favorite '%1' not found").arg(name));
        return;
    }

    QVariantMap data = coupledFavorites[name];

    // Restore Focus position
    if (data["hasFocus"].toBool()) {
        int focusPosition = data["focus"].toInt();
    QString device = getV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_FOCUS_ABSOLUTE, focusPosition, &err);

        // Update GUI without triggering signals
        focusAbsoluteSlider->blockSignals(true);
        focusAbsoluteInput->blockSignals(true);
        focusAbsoluteSlider->setValue(static_cast<int>((focusPosition / 32767.0) * 100));
        focusAbsoluteInput->setText(QString::number(focusPosition));
        focusAbsoluteSlider->blockSignals(false);
        focusAbsoluteInput->blockSignals(false);

        appendLog(tr("Restored Focus: %1 - Result: %2").arg(focusPosition).arg(ok ? "OK" : "ERROR"));
        if (!ok && DebugLogger::isLogToWidgetEnabled()) {
            appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
        }

        // Refresh actual position after delay (motor needs time to move)
        QTimer::singleShot(4000, refreshFocusPositionButton, &QPushButton::click);
    }

    // Restore Zoom position (if coupled)
    if (data["hasZoom"].toBool()) {
        int zoomPosition = data["zoom"].toInt();
    QString device = getZoomV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_ZOOM_ABSOLUTE, zoomPosition, &err);

        // Update GUI without triggering signals
        zoomAbsoluteSlider->blockSignals(true);
        zoomAbsoluteInput->blockSignals(true);
        zoomAbsoluteSlider->setValue(zoomPosition); // direct 0-32767
        zoomAbsoluteInput->setText(QString::number(zoomPosition));
        zoomAbsoluteSlider->blockSignals(false);
        zoomAbsoluteInput->blockSignals(false);

        appendLog(tr("Restored Zoom: %1 - Result: %2").arg(zoomPosition).arg(ok ? "OK" : "ERROR"));
        if (!ok && DebugLogger::isLogToWidgetEnabled()) {
            appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
        }

        // Refresh actual position after delay (motor needs time to move)
        QTimer::singleShot(4000, refreshZoomPositionButton, &QPushButton::click);
    }
});

connect(deleteFocusFavoriteButton, &QPushButton::clicked, this, [this]() {
    QListWidgetItem *currentItem = focusFavoritesList->currentItem();
    if (currentItem) {
        QString name = currentItem->data(Qt::UserRole).toString();
        coupledFavorites.remove(name);
        rebuildFavoritesLists();
        appendLog(tr("Deleted favorite: '%1'").arg(name));
    } else {
        appendLog(tr("No focus favorite selected for deletion"));
    }
});

// ===== Zoom Controls Connections =====

// Refresh Zoom Position Button
// NOTE: zoom_absolute is write-only in HocusFocus, cannot be read from hardware
// We track the position in software instead
connect(refreshZoomPositionButton, &QPushButton::clicked, this, [this, getZoomV4L2Device]() {
    QString device = getZoomV4L2Device();
    // Check if the device actually supports zoom control
    if (!V4L2::hasControl(device, V4L2_CID_ZOOM_ABSOLUTE)) return;
    if (currentZoomPositionLabel) currentZoomPositionLabel->setText(tr("reading..."));

    int position = 0;
    qDebug() << "RefreshZoom: reading device:" << device;
    QString err;
    if (V4L2::getControlValue(device, V4L2_CID_ZOOM_ABSOLUTE, position, &err)) {
        qDebug() << "RefreshZoom: value" << position;
        currentZoomPositionLabel->setText(QString("%1").arg(position));
        // Update slider and input to reflect current position
        // Slider has range 0-32767, so use position directly
        zoomAbsoluteSlider->blockSignals(true);
        zoomAbsoluteInput->blockSignals(true);
        zoomAbsoluteSlider->setValue(position);
        zoomAbsoluteInput->setText(QString::number(position));
        zoomAbsoluteSlider->blockSignals(false);
        zoomAbsoluteInput->blockSignals(false);
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog(QString("DEBUG: Slider set to position: %1").arg(position));
        }
    } else {
        currentZoomPositionLabel->setText(tr("Error reading"));
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog(QString("DEBUG: GetControl failed: %1").arg(err));
        }
        qDebug() << "RefreshZoom: read failed:" << err;
    }
});

// Initial refresh of Focus and Zoom positions after connect statements are set up
// Initial refresh of Focus and Zoom positions — only if device supports the controls
if (refreshFocusPositionButton) {
    QString dev = v4l2DeviceInput->text().isEmpty()
        ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
        : v4l2DeviceInput->text();
    if (V4L2::hasControl(dev, V4L2_CID_FOCUS_ABSOLUTE)) {
        QTimer::singleShot(100, refreshFocusPositionButton, &QPushButton::click);
        qDebug() << "Initial Focus position refresh scheduled (via button click)";
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog("DEBUG: Initial Focus position refresh scheduled (via button click)");
        }
    }
}
if (refreshZoomPositionButton) {
    QString dev = v4l2DeviceInput->text().isEmpty()
        ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
        : v4l2DeviceInput->text();
    if (V4L2::hasControl(dev, V4L2_CID_ZOOM_ABSOLUTE)) {
        QTimer::singleShot(100, refreshZoomPositionButton, &QPushButton::click);
        qDebug() << "Initial Zoom position refresh scheduled (via button click)";
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog("DEBUG: Initial Zoom position refresh scheduled (via button click)");
        }
    }
}

// Zoom Far (negative relative = zoom out)
connect(zoomFarButton, &QPushButton::clicked, this, [this, getZoomV4L2Device]() {
    int value = -currentZoomStepSize;
    QString device = getZoomV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_ZOOM_RELATIVE, value, &err);
    appendLog(QString("Zoom Far: setControlValue zoom_relative=%1 - Result: %2").arg(value).arg(ok ? "OK" : "ERROR"));
    if (!ok && DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }
    QTimer::singleShot(100, refreshZoomPositionButton, &QPushButton::click);
});

// Zoom Near (positive relative = zoom in)
connect(zoomNearButton, &QPushButton::clicked, this, [this, getZoomV4L2Device]() {
    int value = currentZoomStepSize;
    QString device = getZoomV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_ZOOM_RELATIVE, value, &err);
    appendLog(QString("Zoom Near: setControlValue zoom_relative=%1 - Result: %2").arg(value).arg(ok ? "OK" : "ERROR"));
    if (!ok && DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }
    QTimer::singleShot(100, refreshZoomPositionButton, &QPushButton::click);
});

connect(zoomAbsoluteSlider, &QSlider::valueChanged, this, [this, getZoomV4L2Device](int value) {
    // Slider range is 0-32767 (direct hardware value)
    // Round to nearest 100
    int rounded = qRound(value / 100.0) * 100;

    zoomAbsoluteInput->setText(QString::number(rounded));

    QString device = getZoomV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_ZOOM_ABSOLUTE, rounded, &err);
    appendLog(QString("Zoom Absolute: %1 - Result: %2").arg(rounded).arg(ok ? "OK" : "ERROR"));
    if (ok) {
        if (currentZoomPositionLabel) currentZoomPositionLabel->setText(QString("%1").arg(rounded));
    }
    if (!ok && DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }

    // Auto-refresh position after movement
    QTimer::singleShot(100, refreshZoomPositionButton, &QPushButton::click);
});

// Lambda für Zoom Absolute Input - wird von editingFinished und OK-Button verwendet
auto applyZoomAbsoluteInput = [this, getZoomV4L2Device]() {
    QString text = zoomAbsoluteInput->text();
    int absoluteValue = text.toInt();
    if (absoluteValue < 0 || absoluteValue > 32767) return;

    // Round to nearest 100
    int rounded = qRound(absoluteValue / 100.0) * 100;

    zoomAbsoluteSlider->blockSignals(true);
    zoomAbsoluteSlider->setValue(rounded);
    zoomAbsoluteSlider->blockSignals(false);

    if (rounded != absoluteValue) {
        zoomAbsoluteInput->blockSignals(true);
        zoomAbsoluteInput->setText(QString::number(rounded));
        zoomAbsoluteInput->blockSignals(false);
    }

    QString device = getZoomV4L2Device();
    QString err;
    bool ok = V4L2::setControlValue(device, V4L2_CID_ZOOM_ABSOLUTE, rounded, &err);
    if (ok) {
        if (currentZoomPositionLabel) currentZoomPositionLabel->setText(QString("%1").arg(rounded));
    } else if (DebugLogger::isLogToWidgetEnabled()) {
        appendLog(QString("DEBUG: VIDIOC_S_CTRL failed: %1").arg(err));
    }

    // Auto-refresh position after movement
    QTimer::singleShot(100, refreshZoomPositionButton, &QPushButton::click);
};

connect(zoomAbsoluteInput, &QLineEdit::editingFinished, this, applyZoomAbsoluteInput);
connect(zoomAbsoluteOkButton, &QPushButton::clicked, this, applyZoomAbsoluteInput);

// Timeout-Reset-Button: Setzt Timeout auf Default ("0") und aktualisiert Farbe
connect(timeoutResetButton, &QPushButton::clicked, this, [this]() {
    timeoutSelector->setCurrentText("0");
    updateResetButtonColor(timeoutResetButton, timeoutSelector->currentText().toInt(), 0);
});
connect(timeoutSelector, &QComboBox::currentTextChanged, this, [this](const QString &text) {
    updateResetButtonColor(timeoutResetButton, text.toInt(), 0);
});

// Initialisiere ROI Reset-Button Farbe
updateROIResetButtonColor();

// Globalen Reset-Button mit allen Änderungen verbinden
connect(BoxInput, &QLineEdit::textChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(outputFileName, &QLineEdit::textChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(autoNamingCheckbox, &QCheckBox::toggled, this, &MainWindow::updateGlobalResetButtonColor);
connect(timestampCheckbox, &QCheckBox::toggled, this, &MainWindow::updateGlobalResetButtonColor);
connect(timeoutSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(timelapseInput, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(shutterValueInput, &QLineEdit::textChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(hdrSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(denoiseSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(postProcessFileSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(codecSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(awbSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(meteringSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(meteringCustomInput, &QLineEdit::textChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(loresComboBox, &LoresComboBox::loresConfigChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(sharpnessSlider, &QSlider::valueChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(evSlider, &QSlider::valueChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(gainSlider, &QSlider::valueChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(brightnessSlider, &QSlider::valueChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(contrastSlider, &QSlider::valueChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(saturationSlider, &QSlider::valueChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(autofocusModeSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(autofocusRangeSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(autofocusSpeedSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(autofocusWindowInput, &QLineEdit::textChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(lensPositionSlider, &QSlider::valueChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(tuningFileSelector, &QComboBox::currentTextChanged, this, &MainWindow::updateGlobalResetButtonColor);
// OLD: geometryComboBox, infoTextComboBox removed - now using checkboxes
connect(roiInput, &QLineEdit::textChanged, this, &MainWindow::updateGlobalResetButtonColor);
connect(doubleSizeCheckbox, &QCheckBox::toggled, this, &MainWindow::updateGlobalResetButtonColor);
connect(ccmInput, &QLineEdit::textChanged, this, &MainWindow::updateGlobalResetButtonColor);

// Initialisiere globalen Reset-Button Farbe
updateGlobalResetButtonColor();

// Auto-refresh Timer für Focus Position (alle 0.5 Sekunden, besser responsiv)
focusPositionRefreshTimer = new QTimer(this);
connect(focusPositionRefreshTimer, &QTimer::timeout, this, [this]() {
    // Nur refreshen wenn Focus Tab aktiv ist (Index 1 = Focus Tab)
    if (tabWidget && tabWidget->currentIndex() == 1) {
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog("DEBUG: Auto-refresh executing (Focus)...");
        }
        refreshFocusPositionButton->click();
    }
});
// Manual refresh timer no longer needed - polling handles updates
// focusPositionRefreshTimer->start(500);

// Auto-refresh Timer für Zoom Position (alle 0.5 Sekunden, besser responsiv)
zoomPositionRefreshTimer = new QTimer(this);
connect(zoomPositionRefreshTimer, &QTimer::timeout, this, [this]() {
    // Nur refreshen wenn Zoom Tab aktiv ist (Index 2 = Zoom Tab)
    if (tabWidget && tabWidget->currentIndex() == 2) {
        if (DebugLogger::isLogToWidgetEnabled()) {
            appendLog("DEBUG: Auto-refresh executing (Zoom)...");
        }
        refreshZoomPositionButton->click();
    }
});
// Manual refresh timer no longer needed - polling handles updates
// zoomPositionRefreshTimer->start(500);

    // Control Socket for live parameter updates (PR #917)
    initControlSocket();

    // Initialize button visibility after all widgets are created
    updateButtonVisibility();
}


// Shutter display: set label unit and input field value from µs
void MainWindow::updateShutterDisplay(int us)
{
    if (!shutterValueInput || !shutterLabel) return;
    if (us <= 0) {
        shutterLabel->setText(tr("Shutter (µs):"));
        shutterValueInput->setText("0");
        return;
    }
    if (us >= 1000000) {
        shutterLabel->setText(tr("Shutter (s):"));
        shutterValueInput->setText(QString::number(us / 1000000.0, 'f', 2));
    } else if (us >= 1000) {
        shutterLabel->setText(tr("Shutter (ms):"));
        double ms = us / 1000.0;
        // < 100 ms: show one decimal (e.g. "99.9"); >= 100 ms: integer only (e.g. "102")
        if (ms >= 100.0)
            shutterValueInput->setText(QString::number(static_cast<int>(std::round(ms))));
        else
            shutterValueInput->setText(QString::number(ms, 'f', 1));
    } else {
        shutterLabel->setText(tr("Shutter (µs):"));
        shutterValueInput->setText(QString::number(us));
    }
}

// Parse input field value back to µs (interprets number in current label unit)
int MainWindow::parseShutterInput() const
{
    if (!shutterValueInput || !shutterLabel) return 0;
    QString t = shutterValueInput->text().trimmed();
    if (t.isEmpty()) return 0;
    bool ok;
    double val = t.toDouble(&ok);
    if (!ok) return 0;
    QString lbl = shutterLabel->text();
    if (lbl.contains("(ms)"))
        return static_cast<int>(std::round(val * 1000.0));
    if (lbl.contains("(s)"))
        return static_cast<int>(std::round(val * 1000000.0));
    return static_cast<int>(std::round(val)); // µs
}

// Shutter maximum depends on framerate: cannot exceed 1e6 / fps µs
int MainWindow::shutterMaxUs() const
{
    if (!framerateSelector) return 200000;

    QString frText = framerateSelector->currentText().trimmed();
    if (frText.isEmpty()) return 200000;

    bool ok;
    double fps = frText.toDouble(&ok);
    if (!ok || fps <= 0.0) return 200000;

    // Shutter cannot exceed the interval between frames (in µs)
    int maxFromFps = static_cast<int>(1000000.0 / fps);
    // Never let the slider go above 1 second
    if (maxFromFps > 1000000) maxFromFps = 1000000;
    // Minimum sensible range floor
    if (maxFromFps < 100) maxFromFps = 100;
    return maxFromFps;
}

void MainWindow::updateShutterMaxRange()
{
    if (!shutterValueInput || !shutterSlider) return;

    // Remember previous µs value to detect if the scale change actually
    // alters the effective shutter exposure.
    int prevUs = parseShutterInput();

    int maxUs = shutterMaxUs();

    // Update input validator
    auto *v = const_cast<QIntValidator *>(
        qobject_cast<const QIntValidator *>(shutterValueInput->validator()));
    if (v) v->setRange(0, maxUs);

    // Recalculate what the current slider position means in the new log scale.
    // Since the mapping changed (e.g. 40fps→20fps doubles maxUs), position 1000
    // now maps to a different µs value. Block signals to avoid unnecessary
    // socket re-sends during this scale-only update.
    int pos = shutterSlider->value();
    int us = 0;
    if (pos > 0) {
        double logRange = std::log10(static_cast<double>(maxUs)) - 2.0;
        if (logRange <= 0.0) logRange = 1.0;
        double logVal = 2.0 + (pos / 1000.0) * logRange;
        us = static_cast<int>(std::round(std::pow(10.0, logVal)));
        if (us < 100) us = 100;
        if (us > maxUs) us = maxUs;
    }

    // If the current value exceeds the new maximum (higher fps → smaller max),
    // clamp the slider to max position so the display and subsequent reads match.
    if (pos > 0 && us == maxUs) {
        shutterSlider->blockSignals(true);
        shutterSlider->setValue(1000);
        shutterSlider->blockSignals(false);
    }

    updateShutterDisplay(us);
    updateResetButtonColor(shutterSliderResetButton, us, 0);

    // If the effective µs value changed (scale shrunk or grew), push the new
    // shutter value to the running rpicam process via control socket.
    if (us != prevUs && isControlSocketActive() && us > 0) {
        sendSliderToSocket("shutter", QString::number(us));
    }
}

void MainWindow::setupFocusTab()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    // Focus tab
    autofocusTab = new QWidget;
    auto *autofocusLayout = new QVBoxLayout(autofocusTab);
    tabRegistryService->registerTab(autofocusTab, "Focus", 6, "focusTabEnabled", false);

    // ===== Autofocus Parameters Section =====
    auto *autofocusParamsGroup = new QGroupBox(tr("Autofocus Parameters"), this);
    autofocusParamsGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *autofocusParamsLayout = new QVBoxLayout(autofocusParamsGroup);

    const int labelWidth = 110;  // Feste Breite für alle Labels

    // Zeile 1: Mode und Range
    auto *row1Layout = new QHBoxLayout;
    QLabel *modeLabel = new QLabel(tr("Mode:"), this);
    modeLabel->setFixedWidth(labelWidth);
    row1Layout->addWidget(modeLabel);
    row1Layout->addWidget(autofocusModeSelector);
    row1Layout->addStretch();  // Spacer vor Reset-Button
    row1Layout->addWidget(resetAutofocusModeButton);
    row1Layout->addSpacing(20);  // Abstand zwischen Mode und Range
    QLabel *rangeLabel = new QLabel(tr("Range:"), this);
    rangeLabel->setFixedWidth(labelWidth);
    row1Layout->addWidget(rangeLabel);
    row1Layout->addWidget(autofocusRangeSelector);
    row1Layout->addStretch();  // Spacer vor Reset-Button
    row1Layout->addWidget(resetAutofocusRangeButton);
    autofocusParamsLayout->addLayout(row1Layout);

    // Zeile 2: Speed und Window
    autofocusWindowInput->setFixedWidth(170); // Gleiche Breite wie Dropdowns
    auto *row2Layout = new QHBoxLayout;
    QLabel *speedLabel = new QLabel(tr("Speed:"), this);
    speedLabel->setFixedWidth(labelWidth);
    row2Layout->addWidget(speedLabel);
    row2Layout->addWidget(autofocusSpeedSelector);
    row2Layout->addStretch();  // Spacer vor Reset-Button
    row2Layout->addWidget(resetAutofocusSpeedButton);
    row2Layout->addSpacing(20);  // Abstand zwischen Speed und Window
    QLabel *windowLabel = new QLabel(tr("Window:"), this);
    windowLabel->setFixedWidth(labelWidth);
    row2Layout->addWidget(windowLabel);
    row2Layout->addWidget(autofocusWindowInput);
    row2Layout->addStretch();  // Spacer vor Reset-Button
    row2Layout->addWidget(resetAutofocusWindowButton);
    autofocusParamsLayout->addLayout(row2Layout);

    // Zeile 3: Lens Position (Slider über volle Breite)
    auto *lensPositionLayout = new QHBoxLayout;
    QLabel *lensPositionLabel = new QLabel(tr("Lens Position:"), this);
    lensPositionLabel->setFixedWidth(labelWidth);
    lensPositionLayout->addWidget(lensPositionLabel);
    lensPositionLayout->addWidget(lensPositionInput);
    lensPositionLayout->addWidget(lensPositionSlider, 1);  // Slider mit Stretch-Factor 1 (nutzt verfügbaren Platz)
    lensPositionLayout->addStretch();  // Erster Spacer (entspricht dem Stretch nach Range/Window)
    lensPositionLayout->addSpacing(10);  // Festes Spacing vor Reset-Button
    lensPositionLayout->addWidget(resetLensPositionButton);
    autofocusParamsLayout->addLayout(lensPositionLayout);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(autofocusParamsGroup, "UI/Focus/AutofocusParamsGroup", [this]() { adjustWindowToOptimalSize(); }));

    autofocusLayout->addWidget(autofocusParamsGroup);

    // ===== Manual Focus Controls Section =====
    auto *manualFocusGroup = new QGroupBox(tr("Manual Focus Controls"), this);
    manualFocusGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *manualFocusGroupLayout = new QVBoxLayout(manualFocusGroup);

    // Mehr Abstand zwischen Manual Focus Controls und Device Configuration
    manualFocusGroupLayout->addSpacing(10);

    // V4L2 Device Configuration
    auto *deviceGroup = new QGroupBox(tr("Device Configuration"), this);
    deviceGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *deviceGroupLayout = new QHBoxLayout(deviceGroup);
    v4l2DeviceInput = new QLineEdit(this);
    v4l2DeviceInput->setText(QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0));
    v4l2DeviceInput->setToolTip(tr("V4L2 subdevice for autofocus control (e.g. /dev/v4l-subdev0)"));
    v4l2DeviceInput->setFixedWidth(150);
    deviceGroupLayout->addWidget(new QLabel(tr("V4L2 Device:"), this));
    deviceGroupLayout->addWidget(v4l2DeviceInput);

    // V4L2Controller – Hardware-Adapter für Focus/Zoom Polling
    m_v4l2Controller = new V4L2Controller(this);
    connect(m_v4l2Controller, &V4L2Controller::deviceOpened, this, [this](const QString &dev) {
        appendLog(QString("DEBUG: V4L2 device opened: %1").arg(dev));
    });
    connect(m_v4l2Controller, &V4L2Controller::deviceOpenFailed, this, [this](const QString &dev) {
        appendLog(QString("DEBUG: Failed to open V4L2 device: %1").arg(dev));
    });
    connect(m_v4l2Controller, &V4L2Controller::focusPositionChanged, this, [this](int raw) {
        if (currentFocusPositionLabel) currentFocusPositionLabel->setText(QString::number(raw));
        int sliderValue = static_cast<int>((static_cast<double>(raw) / 32767.0) * 100);
        sliderValue = qBound(0, sliderValue, 100);
        if (focusAbsoluteSlider && focusAbsoluteSlider->value() != sliderValue) {
            focusAbsoluteSlider->blockSignals(true);
            focusAbsoluteSlider->setValue(sliderValue);
            focusAbsoluteSlider->blockSignals(false);
        }
        if (focusAbsoluteInput && !focusAbsoluteInput->hasFocus()
                && focusAbsoluteInput->text() != QString::number(raw)) {
            focusAbsoluteInput->blockSignals(true);
            focusAbsoluteInput->setText(QString::number(raw));
            focusAbsoluteInput->blockSignals(false);
        }
    });
    connect(m_v4l2Controller, &V4L2Controller::zoomPositionChanged, this, [this](int raw) {
        if (currentZoomPositionLabel) currentZoomPositionLabel->setText(QString::number(raw));
        if (zoomAbsoluteSlider && zoomAbsoluteSlider->value() != raw) {
            zoomAbsoluteSlider->blockSignals(true);
            zoomAbsoluteSlider->setValue(raw);
            zoomAbsoluteSlider->blockSignals(false);
        }
        if (zoomAbsoluteInput && !zoomAbsoluteInput->hasFocus()
                && zoomAbsoluteInput->text() != QString::number(raw)) {
            zoomAbsoluteInput->blockSignals(true);
            zoomAbsoluteInput->setText(QString::number(raw));
            zoomAbsoluteInput->blockSignals(false);
        }
    });

    connect(v4l2DeviceInput, &QLineEdit::editingFinished, this, [this]() {
        QString newDev = v4l2DeviceInput->text().isEmpty()
            ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
            : v4l2DeviceInput->text();
        m_v4l2Controller->openDevice(newDev);
    });
    deviceGroupLayout->addStretch();

    // Current Focus Position Display
    QLabel *currentFocusPosLabel = new QLabel(tr("Current Position:"), this);
    currentFocusPosLabel->setFixedWidth(130);  // Breiter für längeren Text
    currentFocusPositionLabel = new QLabel(tr("Unknown"), this);
    currentFocusPositionLabel->setMinimumWidth(60);  // Minimale Breite für Zahlen
    refreshFocusPositionButton = new QPushButton(tr("Refresh"), this);
    refreshFocusPositionButton->setFixedWidth(80);
    deviceGroupLayout->addWidget(currentFocusPosLabel);
    deviceGroupLayout->addWidget(currentFocusPositionLabel);
    deviceGroupLayout->addStretch();  // Stretch füllt verfügbaren Platz
    deviceGroupLayout->addWidget(refreshFocusPositionButton);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(deviceGroup, "UI/Focus/DeviceGroup", [this]() { adjustWindowToOptimalSize(); }));

    manualFocusGroupLayout->addWidget(deviceGroup);

    // Abstand zwischen Device Configuration und Relative Controls
    manualFocusGroupLayout->addSpacing(10);

    // Focus Movements (formerly Relative Focus Movement)
    auto *relativeGroup = new QGroupBox(tr("Focus Movements"), this);
    relativeGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *relativeGroupLayout = new QVBoxLayout(relativeGroup);

    // Step Size Radio Buttons (6 options without labels, tooltips instead)
    auto *focusStepSizeLayout = new QHBoxLayout();

    int focusStep1 = settings.value("Focus/StepSize1", 100).toInt();
    int focusStep2 = settings.value("Focus/StepSize2", 300).toInt();
    int focusStep3 = settings.value("Focus/StepSize3", 1000).toInt();
    int focusStep4 = settings.value("Focus/StepSize4", 3000).toInt();
    int focusStep5 = settings.value("Focus/StepSize5", 10000).toInt();
    int focusStep6 = settings.value("Focus/StepSize6", 32767).toInt();

    focusStepButtonGroup = new QButtonGroup(this);

    auto *focusStepRadio1 = new QRadioButton(this);
    auto *focusStepRadio2 = new QRadioButton(this);
    auto *focusStepRadio3 = new QRadioButton(this);
    auto *focusStepRadio4 = new QRadioButton(this);
    auto *focusStepRadio5 = new QRadioButton(this);
    auto *focusStepRadio6 = new QRadioButton(this);

    focusStepRadio1->setToolTip(tr("Step Size: %1").arg(focusStep1));
    focusStepRadio2->setToolTip(tr("Step Size: %1").arg(focusStep2));
    focusStepRadio3->setToolTip(tr("Step Size: %1").arg(focusStep3));
    focusStepRadio4->setToolTip(tr("Step Size: %1").arg(focusStep4));
    focusStepRadio5->setToolTip(tr("Step Size: %1").arg(focusStep5));
    focusStepRadio6->setToolTip(tr("Step Size: %1").arg(focusStep6));

    focusStepButtonGroup->addButton(focusStepRadio1, 0);
    focusStepButtonGroup->addButton(focusStepRadio2, 1);
    focusStepButtonGroup->addButton(focusStepRadio3, 2);
    focusStepButtonGroup->addButton(focusStepRadio4, 3);
    focusStepButtonGroup->addButton(focusStepRadio5, 4);
    focusStepButtonGroup->addButton(focusStepRadio6, 5);

    focusStepRadio2->setChecked(true);  // Default: zweiter Wert (300)
    currentFocusStepSize = focusStep2;  // Set default

    focusStepSizeLayout->addWidget(focusStepRadio1);
    focusStepSizeLayout->addWidget(focusStepRadio2);
    focusStepSizeLayout->addWidget(focusStepRadio3);
    focusStepSizeLayout->addWidget(focusStepRadio4);
    focusStepSizeLayout->addWidget(focusStepRadio5);
    focusStepSizeLayout->addWidget(focusStepRadio6);
    focusStepSizeLayout->addStretch();

    // Update currentFocusStepSize when radio button changes (reads dynamically from settings)
    connect(focusStepButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        const int values[] = {
            settings.value("Focus/StepSize1", 100).toInt(),
            settings.value("Focus/StepSize2", 300).toInt(),
            settings.value("Focus/StepSize3", 1000).toInt(),
            settings.value("Focus/StepSize4", 3000).toInt(),
            settings.value("Focus/StepSize5", 10000).toInt(),
            settings.value("Focus/StepSize6", 32767).toInt()
        };
        if (id >= 0 && id <= 5) {
            currentFocusStepSize = values[id];
            QString tip = tr("Move focus far (step: %1)").arg(currentFocusStepSize);
            if (focusFarButton) focusFarButton->setToolTip(tip);
            tip = tr("Move focus near (step: %1)").arg(currentFocusStepSize);
            if (focusNearButton) focusNearButton->setToolTip(tip);
        }
        settings.endGroup();
    });

    // Far / Near buttons + Calibrate button in one row
    auto *focusButtonLayout = new QHBoxLayout();

    focusFarButton = new QPushButton(tr("Far"), this);
    focusFarButton->setFixedWidth(80);
    focusFarButton->setToolTip(tr("Move focus far (step: %1)").arg(currentFocusStepSize));

    focusNearButton = new QPushButton(tr("Near"), this);
    focusNearButton->setFixedWidth(80);
    focusNearButton->setToolTip(tr("Move focus near (step: %1)").arg(currentFocusStepSize));

    focusCalibrationButton = new QPushButton(tr("Calibrate Focus Range"), this);
    focusCalibrationButton->setToolTip(tr("Calibrates the focus range using v4l2-ctl --set-ctrl calibrate=0"));

    parfocalButton = new ToggleSwitch(this);
    parfocalButton->setToolTip(tr("Toggle parfocal mode (v4l2-ctl --set-ctrl=parfocal=1/0)"));

    focusButtonLayout->addWidget(focusFarButton);
    focusButtonLayout->addWidget(focusNearButton);
    focusButtonLayout->addStretch();
    focusButtonLayout->addWidget(new QLabel(tr("Parfocal"), this));
    focusButtonLayout->addWidget(parfocalButton);
    focusButtonLayout->addSpacing(50);
    focusButtonLayout->addWidget(focusCalibrationButton);

    // Absolute focus row (Position label + input + OK + slider)
    auto *absoluteLayout = new QHBoxLayout();

    focusAbsoluteInput = new QLineEdit(this);
    focusAbsoluteInput->setText("0");
    focusAbsoluteInput->setValidator(new QIntValidator(0, 32767, this));
    focusAbsoluteInput->setFixedWidth(60);

    focusAbsoluteSlider = new QSlider(Qt::Horizontal, this);
    focusAbsoluteSlider->setRange(0, 100);
    focusAbsoluteSlider->setValue(0);
    focusAbsoluteSlider->setSingleStep(1);
    focusAbsoluteSlider->setPageStep(10);
    focusAbsoluteSlider->setTickPosition(QSlider::TicksBelow);
    focusAbsoluteSlider->setTickInterval(3276); // ~10% intervals

    focusAbsoluteOkButton = new QPushButton(tr("OK"), this);
    focusAbsoluteOkButton->setFixedWidth(40);
    focusAbsoluteOkButton->setToolTip(tr("Apply focus position"));

    QLabel *absoluteFocusLabel = new QLabel(tr("Absl. Pos:"), this);
    absoluteFocusLabel->setFixedWidth(80);  // Gleiche Breite wie in Autofocus Parameters
    absoluteLayout->addWidget(absoluteFocusLabel);
    absoluteLayout->addWidget(focusAbsoluteInput);
    absoluteLayout->addWidget(focusAbsoluteOkButton);
    absoluteLayout->addWidget(focusAbsoluteSlider, 1);  // Stretch-Factor 1

    relativeGroupLayout->addLayout(focusStepSizeLayout);
    relativeGroupLayout->addLayout(focusButtonLayout);
    relativeGroupLayout->addSpacing(20);
    relativeGroupLayout->addLayout(absoluteLayout);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(relativeGroup, "UI/Focus/RelativeGroup", [this]() { adjustWindowToOptimalSize(); }));

    manualFocusGroupLayout->addWidget(relativeGroup);

    // Abstand zwischen Focus Movements und Favorites
    manualFocusGroupLayout->addSpacing(10);

    // Focus Favorites
    auto *favoritesGroup = new QGroupBox(tr("Focus Favorites"), this);
    favoritesGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *favoritesGroupLayout = new QVBoxLayout(favoritesGroup);
    auto *favoritesHeaderLayout = new QHBoxLayout;

    // Name input für Favoriten
    focusFavoriteNameInput = new QLineEdit(this);
    focusFavoriteNameInput->setPlaceholderText(tr("Name (e.g. vorne 1)"));
    focusFavoriteNameInput->setFixedWidth(120);

    saveFocusFavoriteButton = new QPushButton(tr("Save"), this);
    saveFocusFavoriteButton->setFixedWidth(60);

    deleteFocusFavoriteButton = new QPushButton(tr("Delete"), this);
    deleteFocusFavoriteButton->setFixedWidth(60);

    favoritesHeaderLayout->addWidget(focusFavoriteNameInput);
    favoritesHeaderLayout->addWidget(saveFocusFavoriteButton);
    favoritesHeaderLayout->addWidget(deleteFocusFavoriteButton);
    favoritesHeaderLayout->addStretch();

    focusFavoritesList = new QListWidget(this);
    focusFavoritesList->setMaximumHeight(80);
    focusFavoritesList->setVisible(false); // Zunächst versteckt

    favoritesGroupLayout->addLayout(favoritesHeaderLayout);
    favoritesGroupLayout->addWidget(focusFavoritesList);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(favoritesGroup, "UI/Focus/FavoritesGroup", [this]() { adjustWindowToOptimalSize(); }));

    manualFocusGroupLayout->addWidget(favoritesGroup);

    // Focus Calibration Button verbinden
    connect(focusCalibrationButton, &QPushButton::clicked, this, [this]() {
        QString device = v4l2DeviceInput->text().isEmpty()
            ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
            : v4l2DeviceInput->text();

        // v4l2-ctl Kommando ausführen
        QProcess calibrationProcess;
        QStringList arguments;
        arguments << "-d" << device << "--set-ctrl" << "calibrate=0";

        appendLog(tr("Starting focus range calibration..."));
        appendLog(QString("Command: v4l2-ctl -d %1 --set-ctrl calibrate=0").arg(device));
        appendLog(tr("Please wait... hardware calibration takes up to 13 seconds"));

        // Button während Kalibrierung deaktivieren
        focusCalibrationButton->setEnabled(false);
        focusCalibrationButton->setText(tr("Calibrating..."));

    // Execute calibration via ioctlto avoid spawning a v4l2-ctl process
    QString err;
    bool ok = V4L2::setControlValue(device, 0x009809ab, 0, &err);

    if (ok) {
            appendLog(tr("Calibration command sent. Hardware is now calibrating..."));

            // Countdown für Hardware-Kalibrierung
            int *remainingTime = new int(13); // 13 Sekunden

            QTimer *countdownTimer = new QTimer(this);
            connect(countdownTimer, &QTimer::timeout, this, [this, remainingTime, countdownTimer]() {
                (*remainingTime)--;

                if (*remainingTime > 0) {
                    focusCalibrationButton->setText(tr("Please wait... %1s").arg(*remainingTime));
                } else {
                    // Countdown beendet - JETZT erst refreshen
                    countdownTimer->stop();
                    appendLog(tr("Hardware calibration completed. Updating position data..."));

                    // Position direkt nach Ablauf der 13 Sekunden aktualisieren
                    appendLog("DEBUG: 13-second timer finished. Executing refresh now...");
                    refreshFocusPositionButton->click();
                    appendLog("DEBUG: Refresh button clicked after calibration.");

                    // Button wieder aktivieren
                    focusCalibrationButton->setEnabled(true);
                    focusCalibrationButton->setText(tr("Calibrate Focus Range"));

                    // Cleanup
                    delete remainingTime;
                    countdownTimer->deleteLater();
                }
            });
            countdownTimer->start(1000); // Jede Sekunde update

        } else {
            appendLog(tr("Focus calibration command failed!"));
            if (!err.isEmpty()) {
                appendLog(tr("Error: ") + err);
            }

            // Button sofort wieder aktivieren bei Fehler
            focusCalibrationButton->setEnabled(true);
            focusCalibrationButton->setText(tr("Calibrate Focus Range"));
        }
    });

    // Parfocal Button: read initial state
    {
        QString device = v4l2DeviceInput->text().isEmpty()
            ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
            : v4l2DeviceInput->text();
        QProcess getProc;
        getProc.start("v4l2-ctl", {"-d", device, "--get-ctrl=parfocal"});
        if (getProc.waitForFinished(2000)) {
            QString out = getProc.readAllStandardOutput().trimmed();
            // output format: "parfocal: 1"
            QStringList parts = out.split(":");
            if (parts.size() == 2 && parts[1].trimmed() == "1") {
                parfocalButton->setChecked(true);
            } else {
                parfocalButton->setChecked(false);
            }
        }
    }

    // Parfocal Button: Toggle-Verbindung
    connect(parfocalButton, &QAbstractButton::toggled, this, [this](bool checked) {
        QString device = v4l2DeviceInput->text().isEmpty()
            ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
            : v4l2DeviceInput->text();
        QString value = checked ? "1" : "0";
        QProcess proc;
        proc.start("v4l2-ctl", {"-d", device, "--set-ctrl=parfocal=" + value});
        proc.waitForFinished(2000);
        appendLog(QString("v4l2-ctl -d %1 --set-ctrl=parfocal=%2").arg(device, value));
    });

    // Parfocal Polling-Timer: Status alle 5s prüfen wenn Focus-Tab aktiv
    parfocalPollTimer = new QTimer(this);
    parfocalPollTimer->setInterval(2000);
    connect(parfocalPollTimer, &QTimer::timeout, this, [this]() {
        // Nur pollen wenn Focus-Tab gerade sichtbar ist
        if (!autofocusTab || tabWidget->currentWidget() != autofocusTab)
            return;
        QString device = v4l2DeviceInput->text().isEmpty()
            ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
            : v4l2DeviceInput->text();
        int val = 0;
        if (V4L2::getControlValue(device, 0x009809ac, val)) {
            bool newState = (val == 1);
            if (parfocalButton->isChecked() != newState) {
                parfocalButton->blockSignals(true);
                parfocalButton->setChecked(newState);
                parfocalButton->blockSignals(false);
            }
        }
    });
    parfocalPollTimer->start();

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(manualFocusGroup, "UI/Focus/ManualFocusGroup", [this]() { adjustWindowToOptimalSize(); }));

    autofocusLayout->addWidget(manualFocusGroup);

    // Stretch am Ende für besseres Layout
    autofocusLayout->addStretch();

    // Focus Tab wird dynamisch eingefügt basierend auf focusTabEnabled
    tabVisibilityService->updateFocusTabVisibility();
    settings.endGroup();
}

void MainWindow::setupZoomTab()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    // ===== Zoom Tab =====
    zoomTab = new QWidget;
    auto *zoomLayout = new QVBoxLayout(zoomTab);
    tabRegistryService->registerTab(zoomTab, "Zoom", 7, "zoomTabEnabled", false);

    // ===== Manual Zoom Controls Section =====
    auto *manualZoomGroup = new QGroupBox(tr("Manual Zoom Controls"), this);
    manualZoomGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    auto *manualZoomGroupLayout = new QVBoxLayout(manualZoomGroup);

    // Spacing
    manualZoomGroupLayout->addSpacing(10);

    // Current Zoom Position Display
    auto *zoomDeviceGroup = new QGroupBox(tr("Zoom Configuration"), this);
    zoomDeviceGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *zoomDeviceGroupLayout = new QHBoxLayout(zoomDeviceGroup);

    // Fixer Spacer anstatt V4L2 Device Label+Input (wie in Focus Device Configuration)
    // Focus hat: Label "V4L2 Device:" (~90px) + Input (150px) = ~240px
    zoomDeviceGroupLayout->addSpacing(240);
    zoomDeviceGroupLayout->addStretch();

    // Current Zoom Position Display (rechtsbündig wie in Focus)
    QLabel *currentPosLabel = new QLabel(tr("Current Position:"), this);
    currentPosLabel->setFixedWidth(130);  // Breiter für längeren Text
    currentZoomPositionLabel = new QLabel("0", this);
    currentZoomPositionLabel->setMinimumWidth(60);  // Minimale Breite für Zahlen
    currentZoomPositionLabel->setToolTip(tr("Note: Zoom position is write-only in hardware and tracked in software"));
    refreshZoomPositionButton = new QPushButton(tr("Refresh"), this);
    refreshZoomPositionButton->setFixedWidth(80);
    refreshZoomPositionButton->setToolTip(tr("Updates the display from current slider/input value"));

    zoomDeviceGroupLayout->addWidget(currentPosLabel);
    zoomDeviceGroupLayout->addWidget(currentZoomPositionLabel);
    zoomDeviceGroupLayout->addStretch();  // Stretch füllt verfügbaren Platz
    zoomDeviceGroupLayout->addWidget(refreshZoomPositionButton);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(zoomDeviceGroup, "UI/Zoom/DeviceGroup", [this]() { adjustWindowToOptimalSize(); }));

    manualZoomGroupLayout->addWidget(zoomDeviceGroup);

    // Spacing
    manualZoomGroupLayout->addSpacing(10);

    // Zoom Relative Controls
    auto *zoomRelativeGroup = new QGroupBox(tr("Relative Zoom Movement"), this);
    zoomRelativeGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *zoomRelativeGroupLayout = new QVBoxLayout(zoomRelativeGroup);

    // Step Size Radio Buttons (6 options without labels, tooltips instead)
    auto *stepSizeLayout = new QHBoxLayout();

    zoomStepButtonGroup = new QButtonGroup(this);

    // Read step size values from settings (defaults: 100, 300, 1000, 3000, 10000, 32767)
    int stepSize1 = settings.value("Zoom/StepSize1", 100).toInt();
    int stepSize2 = settings.value("Zoom/StepSize2", 300).toInt();
    int stepSize3 = settings.value("Zoom/StepSize3", 1000).toInt();
    int stepSize4 = settings.value("Zoom/StepSize4", 3000).toInt();
    int stepSize5 = settings.value("Zoom/StepSize5", 10000).toInt();
    int stepSize6 = settings.value("Zoom/StepSize6", 32767).toInt();

    auto *stepRadio1 = new QRadioButton(this);
    auto *stepRadio2 = new QRadioButton(this);
    auto *stepRadio3 = new QRadioButton(this);
    auto *stepRadio4 = new QRadioButton(this);
    auto *stepRadio5 = new QRadioButton(this);
    auto *stepRadio6 = new QRadioButton(this);

    stepRadio1->setToolTip(tr("Step Size: %1").arg(stepSize1));
    stepRadio2->setToolTip(tr("Step Size: %1").arg(stepSize2));
    stepRadio3->setToolTip(tr("Step Size: %1").arg(stepSize3));
    stepRadio4->setToolTip(tr("Step Size: %1").arg(stepSize4));
    stepRadio5->setToolTip(tr("Step Size: %1").arg(stepSize5));
    stepRadio6->setToolTip(tr("Step Size: %1").arg(stepSize6));

    stepRadio2->setChecked(true);  // Default to 300 (second option)
    currentZoomStepSize = stepSize2;

    zoomStepButtonGroup->addButton(stepRadio1, 0);
    zoomStepButtonGroup->addButton(stepRadio2, 1);
    zoomStepButtonGroup->addButton(stepRadio3, 2);
    zoomStepButtonGroup->addButton(stepRadio4, 3);
    zoomStepButtonGroup->addButton(stepRadio5, 4);
    zoomStepButtonGroup->addButton(stepRadio6, 5);

    stepSizeLayout->addWidget(stepRadio1);
    stepSizeLayout->addWidget(stepRadio2);
    stepSizeLayout->addWidget(stepRadio3);
    stepSizeLayout->addWidget(stepRadio4);
    stepSizeLayout->addWidget(stepRadio5);
    stepSizeLayout->addWidget(stepRadio6);
    stepSizeLayout->addStretch();

    // Update currentZoomStepSize when radio button is clicked (reads dynamically from settings)
    connect(zoomStepButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        const int values[] = {
            settings.value("Zoom/StepSize1", 100).toInt(),
            settings.value("Zoom/StepSize2", 300).toInt(),
            settings.value("Zoom/StepSize3", 1000).toInt(),
            settings.value("Zoom/StepSize4", 3000).toInt(),
            settings.value("Zoom/StepSize5", 10000).toInt(),
            settings.value("Zoom/StepSize6", 32767).toInt()
        };
        if (id >= 0 && id <= 5) {
            currentZoomStepSize = values[id];
            QString tip = tr("Move zoom far (step: %1)").arg(currentZoomStepSize);
            if (zoomFarButton) zoomFarButton->setToolTip(tip);
            tip = tr("Move zoom near (step: %1)").arg(currentZoomStepSize);
            if (zoomNearButton) zoomNearButton->setToolTip(tip);
        }
        settings.endGroup();
    });

    // Buttons
    auto *buttonLayout = new QHBoxLayout();

    zoomFarButton = new QPushButton(tr("Far"), this);
    zoomFarButton->setFixedWidth(80);
    zoomFarButton->setToolTip(tr("Move zoom far (step: %1)").arg(currentZoomStepSize));

    zoomNearButton = new QPushButton(tr("Near"), this);
    zoomNearButton->setFixedWidth(80);
    zoomNearButton->setToolTip(tr("Move zoom near (step: %1)").arg(currentZoomStepSize));

    buttonLayout->addWidget(zoomFarButton);
    buttonLayout->addWidget(zoomNearButton);
    buttonLayout->addStretch();

    zoomRelativeGroupLayout->addLayout(stepSizeLayout);
    zoomRelativeGroupLayout->addLayout(buttonLayout);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(zoomRelativeGroup, "UI/Zoom/RelativeGroup", [this]() { adjustWindowToOptimalSize(); }));

    manualZoomGroupLayout->addWidget(zoomRelativeGroup);

    // Spacing
    manualZoomGroupLayout->addSpacing(10);

    // Zoom Absolute Controls
    auto *zoomAbsoluteGroup = new QGroupBox(tr("Absolute Zoom Position"), this);
    zoomAbsoluteGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *zoomAbsoluteGroupLayout = new QHBoxLayout(zoomAbsoluteGroup);

    zoomAbsoluteInput = new QLineEdit(this);
    zoomAbsoluteInput->setText("0");
    zoomAbsoluteInput->setValidator(new QIntValidator(0, 32767, this));
    zoomAbsoluteInput->setFixedWidth(60);

    zoomAbsoluteSlider = new QSlider(Qt::Horizontal, this);
    zoomAbsoluteSlider->setRange(0, 32767); // Direct hardware range
    zoomAbsoluteSlider->setValue(0);
    zoomAbsoluteSlider->setSingleStep(100);
    zoomAbsoluteSlider->setPageStep(1000);
    zoomAbsoluteSlider->setTickPosition(QSlider::TicksBelow);
    zoomAbsoluteSlider->setTickInterval(3276); // ~10 ticks for 0-32767 range
    // zoomAbsoluteSlider->setFixedWidth(270);  // Entfernt: Slider soll dynamisch expandieren

    zoomAbsoluteOkButton = new QPushButton(tr("OK"), this);
    zoomAbsoluteOkButton->setFixedWidth(40);
    zoomAbsoluteOkButton->setToolTip(tr("Apply zoom position"));

    QLabel *absoluteZoomLabel = new QLabel(tr("Position:"), this);
    absoluteZoomLabel->setFixedWidth(110);  // Gleiche Breite wie in Autofocus Parameters
    zoomAbsoluteGroupLayout->addWidget(absoluteZoomLabel);
    zoomAbsoluteGroupLayout->addWidget(zoomAbsoluteInput);
    zoomAbsoluteGroupLayout->addWidget(zoomAbsoluteOkButton);
    zoomAbsoluteGroupLayout->addWidget(zoomAbsoluteSlider, 1);  // Stretch-Factor 1

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(zoomAbsoluteGroup, "UI/Zoom/AbsoluteGroup", [this]() { adjustWindowToOptimalSize(); }));

    manualZoomGroupLayout->addWidget(zoomAbsoluteGroup);

    // Spacing
    manualZoomGroupLayout->addSpacing(10);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(manualZoomGroup, "UI/Zoom/ManualZoomGroup", [this]() { adjustWindowToOptimalSize(); }));

    zoomLayout->addWidget(manualZoomGroup);

    // Mehr Abstand unter der Gruppe
    zoomLayout->addSpacing(15);

    // Stretch am Ende für besseres Layout
    zoomLayout->addStretch();

    // Zoom Tab wird dynamisch eingefügt basierend auf zoomTabEnabled
    tabVisibilityService->updateZoomTabVisibility();
    settings.endGroup();
}

// X-Reset wird im showEvent() durchgeführt, wenn das Fenster vollständig initialisiert ist

MainWindow::~MainWindow() {
    // Fensterposition und -größe speichern (speichere die gesamte Geometrie inkl. Frame)
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    settings.setValue("Window/Geometry", saveGeometry());
    // Save size explicitly for Wayland where window position cannot be restored
    settings.setValue("Window/Size", size());
    settings.setValue("Window/Pos", pos());

    // P13b: GStreamer-Einstellungen persistieren
    if (m_gstreamerTab) m_gstreamerTab->saveSettings(settings);
    settings.endGroup();

    // P14: GstLaunch-Prozesse werden vom GstLaunchModule-Destruktor gestoppt.
    // Die alten streamViewerTabs/testSourceTabs Maps sind leer wenn Plugin aktiv.

    DebugLogger::shutdown();
    // V4L2Controller ist QObject-Kind und wird automatisch zerstört;
    // explizites Schließen stellt sicher, dass der fd vor dem Destruktor freigegeben wird.
    if (m_v4l2Controller) {
        m_v4l2Controller->closeDevice();
    }
}
void MainWindow::updateParameterFields() {
    parameterWidget->update();
    parameterWidget->repaint();
}

void MainWindow::updateCameraInfo(int index) {
    // Kamera-Details werden nicht mehr als Feld im General-Tab angezeigt,
    // sondern stehen im Log und unter Help -> System Information.
    Q_UNUSED(index);
    cameraInfo->hide();
}
void MainWindow::updateFramerateOptions(const QString &resolution, bool snapToMax) {
    if (!m_fpsSliderPopup) {
        return;
    }

    // 1) Erkannte Sensor-Framerates aus der Kamera-Erkennung verwenden
    //    (keine hardkodierten Werte – kommt alles aus --list-cameras).
    QStringList detectedFps;
    QString activeFormat;
    if (formatSelector && formatSelector->currentIndex() > 0) {
        activeFormat = formatSelector->currentData().toString();
    }

    if (!resolution.isEmpty()) {
        if (activeFormat.isEmpty()) {
            // Alle Formate: fps-Union aus m_resolutionFps
            if (m_resolutionFps.contains(resolution)) {
                detectedFps = m_resolutionFps.value(resolution);
            }
        } else {
            // Gefiltert: nur fps dieses Formats für diese Auflösung
            auto fmtIt = m_formatData.constFind(activeFormat);
            if (fmtIt != m_formatData.constEnd() && fmtIt->contains(resolution)) {
                detectedFps = fmtIt->value(resolution);
            }
        }
    }

    double detectedMax = 0.0;
    for (const QString &fps : detectedFps) {
        detectedMax = qMax(detectedMax, fps.toDouble());
    }

    // 2) Custom-Framerates aus den Settings erweitern das Slider-Maximum
    double customMax = 0.0;
    {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        for (int i = 0; i < 10; ++i) {
            QString value = settings.value(QString("Global/CustomFramerate%1").arg(i + 1)).toString();
            customMax = qMax(customMax, value.toDouble());
        }
        settings.endGroup();
    }

    // 3) Slider-Range: 0 (= Auto), dann 0.1..0.9 (9 Zehntel-Schritte),
    //    dann ganze Zahlen 1..Max. Ohne Sensordaten: generischer Fallback 30.
    int maxInt = static_cast<int>(qFloor(qMax(detectedMax, customMax)));
    if (maxInt < 1) {
        maxInt = 30;
    }

    QString current = framerateSelector->currentText().trimmed();
    // Leeres Feld = kein gespeicherter Wert → Default (Maximum), NICHT 0.
    // Nur eine explizite "0" bedeutet Auto.
    int currentSlider = current.isEmpty() ? -1 : fpsTextToSlider(current);

    m_fpsSliderPopup->blockSignals(true);
    m_fpsSliderPopup->setRange(0, maxInt + 9);
    // snapToMax: the user changed the resolution — jump to the highest
    // integer framerate of the new resolution instead of keeping the old
    // value. All other callers (loading, format filter, camera switch)
    // keep their previous behaviour.
    if (currentSlider >= 0 && currentSlider <= maxInt + 9 && !snapToMax) {
        m_fpsSliderPopup->setValue(currentSlider);
    } else {
        // Default: maximale Sensor-Framerate
        m_fpsSliderPopup->setValue(maxInt + 9);
        framerateSelector->setCurrentText(QString::number(maxInt));
    }
    m_fpsSliderPopup->blockSignals(false);
}

void MainWindow::applyFormatFilter() {
    if (!formatSelector) {
        return;
    }

    QString activeFormat;
    if (formatSelector->currentIndex() > 0) {
        activeFormat = formatSelector->currentData().toString();
    }

    QString currentResolution = resolutionSelector->currentText();
    QString currentFramerate = framerateSelector->currentText();

    // Benutzerdefinierte Auflösungen sammeln (nicht aus der Erkennung)
    QStringList customResolutions;
    for (int i = 0; i < resolutionSelector->count(); ++i) {
        QString res = resolutionSelector->itemText(i);
        if (!cameraResolutions.contains(res)) {
            customResolutions.append(res);
        }
    }

    // Auflösungs-Dropdown neu aufbauen:
    // - "Auto" (leeres Format): alle erkannten Auflösungen
    // - Format gewählt: nur Auflösungen, die dieses Format anbietet
    resolutionSelector->blockSignals(true);
    resolutionSelector->clear();

    if (activeFormat.isEmpty()) {
        for (const QString &res : cameraResolutions) {
            resolutionSelector->addItem(res);
        }
    } else {
        const auto fmtIt = m_formatData.constFind(activeFormat);
        for (const QString &res : cameraResolutions) {
            if (fmtIt != m_formatData.constEnd() && fmtIt->contains(res)) {
                resolutionSelector->addItem(res);
            }
        }
    }

    // Benutzerdefinierte Auflösungen immer am Ende behalten
    for (const QString &res : customResolutions) {
        if (resolutionSelector->findText(res) == -1) {
            resolutionSelector->addItem(res);
        }
    }
    resolutionSelector->blockSignals(false);

    // Vorherige Auswahl wiederherstellen, falls noch vorhanden
    int idx = resolutionSelector->findText(currentResolution);
    resolutionSelector->setCurrentIndex(idx >= 0 ? idx : 0);

    // Framerate-Dropdown für die aktuelle Auflösung neu aufbauen
    updateFramerateOptions(resolutionSelector->currentText());

    // Vorherige Framerate wiederherstellen, falls noch vorhanden
    if (!currentFramerate.isEmpty() && framerateSelector->findText(currentFramerate) != -1) {
        framerateSelector->setCurrentText(currentFramerate);
    }

    // Sensor-Mode-Liste im Expert-Tab ebenfalls filtern
    updateViewfinderModes();
}

void MainWindow::openSaveFileDialog() {
    QString initialPath = outputFileName->text();
    if (initialPath.isEmpty() || !QFileInfo(initialPath).isAbsolute()) {
        initialPath = QDir(guiOutputFilePath).filePath(initialPath);
    }
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), initialPath, tr("All Files (*.*);;JPEG Files (*.jpg);;PNG Files (*.png)"));
    if (!fileName.isEmpty()) {
        outputFileName->setText(fileName); // Setze den ausgewählten Dateinamen
    } else {
    }
}
// Start/Stop button click handler. In sync mode both camera tabs are
// controlled together; otherwise only this instance reacts. Only instances
// that are in the matching state are touched, so a half-running pair
// (e.g. after an auto-stop) is handled safely.
void MainWindow::onStartStopClicked() {
    const bool running = (process.state() == QProcess::Running);
    if (m_syncStartStop && m_sibling) {
        if (running) {
            stopRpiCamApp();
            if (m_sibling->process.state() == QProcess::Running)
                m_sibling->stopRpiCamApp();
        } else {
            // Start this instance FIRST so its solo preview position is
            // computed and fixed; the sibling then sees it via
            // isPreviewActive() and picks the opposite position.
            if (process.state() == QProcess::NotRunning)
                startRpiCamApp();
            if (m_sibling->process.state() == QProcess::NotRunning)
                m_sibling->startRpiCamApp();
        }
    } else if (running) {
        stopRpiCamApp();
    } else {
        startRpiCamApp();
    }
}

// Enable/disable the sync mode. Mirrored to the sibling instance so both
// badges always show the same state. Requires a second camera.
void MainWindow::setSyncStartStop(bool enabled) {
    if (enabled && !m_sibling) return; // no second camera: sync not available
    m_syncStartStop = enabled;
    updateSyncBadgeStyle();
    if (m_sibling) {
        m_sibling->m_syncStartStop = enabled;
        m_sibling->updateSyncBadgeStyle();
    }
}

// Refresh the SYNC badge: green when active, gray when inactive.
void MainWindow::updateSyncBadgeStyle() {
    if (!m_syncIndicator) return;
    m_syncIndicator->setText(QStringLiteral(
        "<a style='color:%1; text-decoration:none;' href='#sync'>SYNC</a>")
        .arg(m_syncStartStop ? QStringLiteral("#00e676") : QStringLiteral("#9e9e9e")));
    m_syncIndicator->setToolTip(m_syncStartStop
        ? tr("Sync active: Start/Stop controls both camera tabs. Click to disable.")
        : tr("Sync: Start/Stop this camera together with the other camera tab. Click to enable."));
}

// Position the RT and SYNC badges on the Start/Stop button. SYNC sits left
// of the RT badge (or of the right edge when RT is hidden).
void MainWindow::repositionStartButtonBadges() {
    if (!startStopButton) return;
    int y = startStopButton->height() - 2;
    if (m_controlSocketIndicator) {
        m_controlSocketIndicator->adjustSize();
        y = startStopButton->height() - m_controlSocketIndicator->height() - 2;
        int x = startStopButton->width() - m_controlSocketIndicator->width() - 4;
        m_controlSocketIndicator->move(x, y);
    }
    if (m_syncIndicator && m_syncIndicator->isVisible()) {
        m_syncIndicator->adjustSize();
        int rtW = (m_controlSocketIndicator && m_controlSocketIndicator->isVisible())
                      ? m_controlSocketIndicator->width() + 4 : 0;
        m_syncIndicator->move(startStopButton->width() - rtW - m_syncIndicator->width() - 4, y);
    }
}

// V2-Style Start/Stop Button mit Runtime im Button-Text
void MainWindow::updateButtonVisibility() {
    disconnect(startStopButton, &QPushButton::clicked, nullptr, nullptr);
    if (process.state() == QProcess::NotRunning) {
        startStopButton->setText(tr("Start"));
        startStopButton->setStyleSheet(
            "QPushButton {"
            "background-color: #007acc;"
            "color: white;"
            "font-weight: bold;"
            "padding: 10px 20px;"
            "border: none;"
            "border-radius: 5px;"
            "outline: none;"
            "}"
            "QPushButton:pressed {"
            "background-color: #005f99;"
            "}"
        );
        connect(startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);

        // Runtime-Timer stoppen
        runtimeTimer->stop();

    } else if (process.state() == QProcess::Running) {
        startStopButton->setStyleSheet(
            "QPushButton {"
            "background-color: #e74c3c;"
            "color: white;"
            "font-weight: bold;"
            "padding: 10px 20px;"
            "border: none;"
            "border-radius: 5px;"
            "outline: none;"
            "}"
            "QPushButton:pressed {"
            "background-color: #c0392b;"
            "}"
        );
        connect(startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);

        // Runtime-Timer starten und Button-Text initialisieren
        startTime = QTime::currentTime();
        startStopButton->setText(tr("Stop") + " 00:00:00");
        runtimeTimer->start(1000); // Update jede Sekunde
    }

    // Update sendSignalButton state
    if (sendSignalButton && appSelector && keypressRecordingCheckbox) {
        bool isRunning = (this->process.state() == QProcess::Running);
        bool isRpicamVid = (appSelector->currentText() == "rpicam-vid");
        bool keypressEnabled = (keypressRecordingCheckbox ? keypressRecordingCheckbox->isChecked() : false);

        // Show and enable button ONLY if KEYPRESS recording is enabled AND running AND rpicam-vid
        // Signal recording uses SIGUSR1 which is sent automatically, not via GUI button
        bool shouldShow = (keypressEnabled && isRpicamVid && isRunning);

        // Debug: Uncomment to debug button visibility
        // qDebug() << "[BUTTON] updateButtonVisibility. Running:" << isRunning
        //          << "IsVid:" << isRpicamVid << "KeypressRec:" << keypressEnabled
        //          << "=> Button visible:" << shouldShow;

        sendSignalButton->setEnabled(shouldShow);
        sendSignalButton->setVisible(shouldShow);

        // When starting, set initial state based on initialStateComboBox
        if (isRunning && keypressEnabled && isRpicamVid) {
            // Determine initial state from combobox
            QString initialState = initialStateComboBox ? initialStateComboBox->currentText() : "pause";
            isRecordingPaused = (initialState == "pause");

            // Update button text and style based on initial state
            if (isRecordingPaused) {
                sendSignalButton->setText(tr("Resume"));
                sendSignalButton->setStyleSheet(
                    "QPushButton {"
                    "background-color: #007acc;"  // Same blue as Start button
                    "color: white;"
                    "font-weight: bold;"
                    "padding: 10px 20px;"  // Exactly same as Start/Stop
                    "border: none;"
                    "border-radius: 5px;"
                    "outline: none;"
                    "}"
                    "QPushButton:pressed {"
                    "background-color: #005f99;"
                    "}"
                );
                qDebug() << "[BUTTON] Initial state: PAUSED - button shows RESUME";
            } else {
                sendSignalButton->setText(tr("Pause"));
                sendSignalButton->setStyleSheet(
                    "QPushButton {"
                    "background-color: #ff8800;"  // Same orange as Stop button
                    "color: white;"
                    "font-weight: bold;"
                    "padding: 10px 20px;"  // Exactly same as Start/Stop
                    "border: none;"
                    "border-radius: 5px;"
                    "outline: none;"
                    "}"
                    "QPushButton:pressed {"
                    "background-color: #e67700;"
                    "}"
                );
                qDebug() << "[BUTTON] Initial state: RECORDING - button shows PAUSE";
            }
        } else if (!shouldShow) {
            // Reset button when disabled/hidden
            sendSignalButton->setText(tr("Sigusr1"));
            sendSignalButton->setStyleSheet(
                "QPushButton {"
                "background-color: #bdc3c7;"  // Light gray for disabled
                "color: white;"
                "font-weight: bold;"
                "padding: 10px 20px;"  // Exactly same as Start/Stop
                "border: none;"
                "border-radius: 5px;"
                "outline: none;"
                "}"
            );
        }
    }

    updateTabIndicator();
}

void MainWindow::updateTabIndicator() {
    // Always set a fixed-width right-side button so tab labels stay the same width.
    // Orange dot when active, transparent when inactive.
    QMainWindow *outerWin = qobject_cast<QMainWindow *>(window());
    if (!outerWin) return;
    QTabWidget *outerTabs = qobject_cast<QTabWidget *>(outerWin->centralWidget());
    if (!outerTabs) return;
    int idx = outerTabs->indexOf(this);
    if (idx < 0) return;

    // Remove any previous indicator
    outerTabs->tabBar()->setTabButton(idx, QTabBar::RightSide, nullptr);

    static QPixmap activeDot = []() {
        QPixmap dot(12, 10);
        dot.fill(Qt::transparent);
        QPainter painter(&dot);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#ff8800"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(2, 1, 8, 8);
        painter.end();
        return dot;
    }();

    static QPixmap inactiveDot = []() {
        QPixmap dot(12, 10);
        dot.fill(Qt::transparent);
        return dot;
    }();

    auto *indicator = new QLabel();
    indicator->setFixedSize(12, 10);
    indicator->setPixmap(process.state() == QProcess::Running ? activeDot : inactiveDot);

    outerTabs->tabBar()->setTabButton(idx, QTabBar::RightSide, indicator);
}

void MainWindow::showHelp() {
    HelpDialog *helpDialog = new HelpDialog(this);
    helpDialog->show();
    helpDialog->raise();
    helpDialog->activateWindow();
}
void MainWindow::showSupportDialog() {
    HelpDialog *d = new HelpDialog(this, 2);
    d->show();
    d->raise();
    d->activateWindow();
}
// Guaranteed tooltip: shows the text on mouse enter regardless of label
// quirks (QLabel tooltips can be suppressed by text interaction flags such
// as TextSelectableByMouse). Used for the System Info feature rows.
// The text is wrapped in a fixed-width HTML table: QToolTip does not wrap
// plain text automatically in Qt5, so long texts would become one single
// line without it.
class TooltipEventFilter : public QObject {
public:
    explicit TooltipEventFilter(const QString &text, QWidget *parent)
        : QObject(parent), m_text(text) { parent->installEventFilter(this); }
protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Enter) {
            const QString html =
                QString("<table width='320'><tr><td>%1</td></tr></table>")
                    .arg(m_text.toHtmlEscaped());
            QToolTip::showText(QCursor::pos(), html, qobject_cast<QWidget*>(watched));
            return true;
        }
        if (event->type() == QEvent::Leave) {
            QToolTip::hideText();
            return true;
        }
        return QObject::eventFilter(watched, event);
    }
private:
    QString m_text;
};

void MainWindow::showSystemInfo() {
    // ── Shell-Befehl ausführen ─────────────────────────────────────────
    auto runCmd = [](const QString &prog, const QStringList &args,
                     int timeoutMs = 3000) -> QString {
        QProcess p;
        p.start(prog, args);
        p.waitForFinished(timeoutMs);
        QString out = p.readAllStandardOutput().trimmed();
        if (out.isEmpty()) out = p.readAllStandardError().trimmed();
        return out.isEmpty() ? QString() : out;
    };

    // ── Label+Wert-Zeile im FormLayout ─────────────────────────────────
    auto infoRow = [](QFormLayout *form, const QString &label, const QString &value) {
        auto *val = new QLabel(value.isEmpty() ? tr("—") : value);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        val->setWordWrap(true);
        form->addRow(label, val);
    };

    // Feature-Zeile mit farbigem Hintergrund + Rahmen (Support-Status)
    auto stateRow = [](QFormLayout *form, const QString &label, bool supported,
                        const QString &tooltip = QString()) {
        auto *val = new QLabel(supported ? tr("Supported") : tr("Not supported"));
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        val->setFixedWidth(250);
        val->setStyleSheet(supported
            ? "QLabel { background-color: #d4edda;"
              "  border: 1px solid #28a745; border-radius: 4px;"
              "  padding: 3px 8px; font-weight: bold; }"
            : "QLabel { background-color: #f8d7da;"
              "  border: 1px solid #dc3545; border-radius: 4px;"
              "  padding: 3px 8px; font-weight: bold; }");
        auto *lbl = new QLabel(label);
        if (!tooltip.isEmpty()) {
            // Event-filter based: works even where QLabel::setToolTip fails
            // (e.g. labels with selectable text).
            new TooltipEventFilter(tooltip, lbl);
            new TooltipEventFilter(tooltip, val);
        }
        form->addRow(lbl, val);
    };

    // ── Daten sammeln ──────────────────────────────────────────────────
    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    QString sessionId   = qgetenv("XDG_SESSION_ID");
    QString xrdpSession = qgetenv("XRDP_SESSION");
    QString display     = qgetenv("DISPLAY");
    QString waylandDisp = qgetenv("WAYLAND_DISPLAY");
    bool    isRemote    = !xrdpSession.isEmpty();

    QString loginctlRemote, loginctlService, loginctlDesktop, loginctlSeat;
    if (!sessionId.isEmpty()) {
        auto out = runCmd("loginctl", {"show-session", sessionId,
            "-p", "Remote", "-p", "Service", "-p", "Desktop", "-p", "Seat"});
        for (const QString &line : out.split('\n')) {
            if (line.startsWith("Remote="))  loginctlRemote  = line.mid(7);
            if (line.startsWith("Service=")) loginctlService = line.mid(8);
            if (line.startsWith("Desktop=")) loginctlDesktop = line.mid(8);
            if (line.startsWith("Seat="))    loginctlSeat    = line.mid(5);
        }
    }
    if (!isRemote && loginctlRemote == "yes") isRemote = true;

    QString xorgProc     = runCmd("pgrep", {"-a", "Xorg"});
    QString xwaylandProc = runCmd("pgrep", {"-a", "Xwayland"});
    QString labwcProc    = runCmd("pgrep", {"-a", "labwc"});

    QString driDevices = runCmd("ls", {"-1", "/dev/dri"});
    bool hasGpu = !driDevices.isEmpty();

    QString camTxt = runCmd("v4l2-ctl", {"--list-devices"});
    // Vollständige Modusliste – entscheidendste Kamera-Info, daher ganz oben.
    // (--list konfiguriert pro Kamera den Sensor → großzügiger Timeout.)
    QString camModesTxt = runCmd("rpicam-hello", {"--list"}, 15000);

    // ── Standard group-box style (uniform) ─────────────────────────
    static const char *GROUP_STYLE =
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}";

    // ── Dialog ─────────────────────────────────────────────────────────
    QDialog dlg(this);
    dlg.setWindowTitle(tr("System Information"));
    dlg.resize(620, 500);

    auto *outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(8, 8, 8, 8);

    // Header
    auto *hdr = new QHBoxLayout;
    auto *iconLbl = new QLabel;
    iconLbl->setPixmap(QIcon(QLatin1String(AppMeta::ICON_RESOURCE)).pixmap(44, 44));
    hdr->addWidget(iconLbl);
    auto *hdrTxt = new QVBoxLayout;
    auto *titleLbl = new QLabel(
        QString("<b style='font-size:15px;'>%1</b>"
                " &nbsp;<span style='color:#0093DD;'>v%2</span>")
            .arg(QLatin1String(AppMeta::NAME), VERSION_STRING));
    hdrTxt->addWidget(titleLbl);
    auto *dateLbl = new QLabel(
        QDateTime::currentDateTime().toString("yyyy-MM-dd  HH:mm:ss"));
    dateLbl->setStyleSheet("color: #888; font-size: 11px;");
    hdrTxt->addWidget(dateLbl);
    hdr->addLayout(hdrTxt);
    hdr->addStretch();
    outer->addLayout(hdr);

    auto *sep = new QFrame; sep->setFrameShape(QFrame::HLine);
    outer->addWidget(sep);

    // ScrollArea
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content_w = new QWidget;
    auto *mainLay = new QVBoxLayout(content_w);
    mainLay->setContentsMargins(0, 4, 0, 4);
    mainLay->setSpacing(8);

    auto addGroup = [&](const QString &title) -> QFormLayout* {
        auto *grp = new QGroupBox(title);
        grp->setStyleSheet(GROUP_STYLE);
        auto *frm = new QFormLayout(grp);
        frm->setLabelAlignment(Qt::AlignRight);
        mainLay->addWidget(grp);
        return frm;
    };

    // Gruppe mit mehrzeiligem Monospace-Text (für Kommandoausgaben)
    auto addMonoGroup = [&](const QString &title, const QString &text) {
        auto *grp = new QGroupBox(title);
        grp->setStyleSheet(GROUP_STYLE);
        auto *vl = new QVBoxLayout(grp);
        auto *lbl = new QLabel(text.isEmpty() ? tr("(nicht verfügbar)") : text);
        lbl->setFont(QFont("monospace", 9));
        lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        vl->addWidget(lbl);
        mainLay->addWidget(grp);
    };

    // Kameras mit vollständiger Modusliste – ganz oben (wichtigste Info).
    // piStudio design: a separate group box per camera/sensor.
    {
        struct CamBlock { QString title; QString body; };
        QList<CamBlock> blocks;
        static const QRegularExpression reCam(R"(^(\d+)\s*:\s*(.+)$)");
        const QStringList camLines = camModesTxt.split('\n');
        int curIdx = -1;
        for (const QString &line : camLines) {
            QRegularExpressionMatch m = reCam.match(line);
            if (m.hasMatch()) {
                CamBlock b;
                // Titel bewusst minimal: nur die Kamera-Nummer.
                // Alle Details stehen im Textfeld der GroupBox.
                b.title = tr("Camera %1").arg(m.captured(1));
                b.body = line + "\n";
                blocks.append(b);
                curIdx = blocks.size() - 1;
            } else if (curIdx >= 0) {
                blocks[curIdx].body += line + "\n";
            }
        }
        if (blocks.isEmpty()) {
            addMonoGroup(tr("Cameras (rpicam-hello --list)"), camModesTxt);
        } else {
            for (const CamBlock &b : blocks) {
                addMonoGroup(b.title, b.body.trimmed());
            }
        }
    }

    // rpicam-apps: Version + Feature-Capabilities (gecachte Erkennung)
    checkRpicamRtCapability();
    {
        QString version = m_rpicamAppsVersion;
        if (version.isEmpty()) {
            // Fallback: --version direkt auswerten
            QString vout = runCmd("rpicam-vid", {"--version"});
            static const QRegularExpression reVer(R"(rpicam-apps build:\s*v?(\S+))");
            QRegularExpressionMatch m = reVer.match(vout);
            if (m.hasMatch()) version = m.captured(1);
        }
        auto *f = addGroup(tr("rpicam-apps"));
        // Version: blau hervorgehoben (geprüftes Element)
        {
            auto *verLbl = new QLabel(version.isEmpty() ? tr("(nicht verfügbar)") : version);
            verLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
            verLbl->setFixedWidth(250);
            verLbl->setStyleSheet(
                "QLabel { background-color: #e7f3fe;"
                " border: 1px solid #86b7fe; border-radius: 4px;"
                " padding: 3px 8px; font-weight: bold; }");
            f->addRow(tr("Version:"), verLbl);
        }
        // Feature checks: green = supported, red = not supported
        stateRow(f, tr("rpicam-rt:"), m_hasRpicamRt,
            tr("Runtime control (rpicam-rt): live parameter updates while streaming, "
               "provided by the rpicam-apps fork feature/rt-roi. "
               "Without it the green RT badge stays hidden."));
        stateRow(f, tr("ROI selection:"), m_hasRoiSelection,
            tr("Interactive Region-of-Interest selection in the rpicam-apps Qt preview "
               "(feature/rt-roi fork). The selected coordinates are fed into "
               "%1's ROI field for persistence via Set Defaults / profiles.")
                .arg(QLatin1String(AppMeta::NAME)));
        stateRow(f, tr("Preview backend:"), m_hasPreviewBackend,
            tr("rpicam-apps 1.13+ supports --preview-backend (egl, qt, drm, "
               "wayland-egl) to choose the preview renderer. The available backends "
               "appear in the preview dropdown."));
    }

    // Session
    {
        auto *f = addGroup(tr("Session"));
        infoRow(f, tr("Art:"),     isRemote ? tr("Remote (XRDP)") : tr("Lokal"));

        // Display-Server farbig: Wayland = rot (eingeschränkt), X11 = grün
        bool isWaylandServer = (sessionType.compare("wayland", Qt::CaseInsensitive) == 0);
        QString serverText = sessionType.isEmpty() ? QString() : sessionType.toUpper();
        auto *serverLbl = new QLabel(serverText.isEmpty() ? tr("—") : serverText);
        serverLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        serverLbl->setFixedWidth(250);
        if (isWaylandServer) {
            serverLbl->setStyleSheet(
                "QLabel { background-color: #f8d7da;"
                " border: 1px solid #dc3545; border-radius: 4px;"
                " padding: 3px 8px; font-weight: bold; }");
            serverLbl->setToolTip(tr("Wayland: limited functionality "
                "(no global mouse capture for ROI selection, limited window placement). "
                "Use an X11 session for full functionality."));
        } else if (!serverText.isEmpty()) {
            serverLbl->setStyleSheet(
                "QLabel { background-color: #d4edda;"
                " border: 1px solid #28a745; border-radius: 4px;"
                " padding: 3px 8px; font-weight: bold; }");
            serverLbl->setToolTip(tr("X11: full functionality."));
        }
        f->addRow(tr("Display-Server:"), serverLbl);

        infoRow(f, tr("Display:"), display);
        if (!waylandDisp.isEmpty())
            infoRow(f, tr("Wayland:"), waylandDisp);
        infoRow(f, tr("Session-ID:"), sessionId);
        if (!loginctlService.isEmpty())
            infoRow(f, tr("Dienst:"), loginctlService);
        if (!loginctlDesktop.isEmpty())
            infoRow(f, tr("Desktop:"), loginctlDesktop);
        if (!loginctlSeat.isEmpty())
            infoRow(f, tr("Seat:"), loginctlSeat);
    }

    // System
    {
        auto *f = addGroup(tr("System"));
        infoRow(f, tr("Betriebssystem:"), QSysInfo::prettyProductName());
        infoRow(f, tr("Kernel:"),         QSysInfo::kernelVersion());
        infoRow(f, tr("Architektur:"),    QSysInfo::currentCpuArchitecture());
        infoRow(f, tr("Qt-Version:"),     qVersion());
    }

    // GPU / DRM
    {
        auto *f = addGroup(tr("GPU / DRM"));
        infoRow(f, tr("Status:"), hasGpu ? tr("Verfügbar") : tr("Nicht verfügbar"));
        QStringList driList = driDevices.split('\n', Qt::SkipEmptyParts);
        infoRow(f, tr("Geräte:"), driList.isEmpty() ? tr("— (kein /dev/dri)") : driList.join(", "));
    }

    // Display-Server
    {
        auto *f = addGroup(tr("Display-Server"));
        auto cmdPart = [](const QString &proc) -> QString {
            if (proc.isEmpty()) return QString();
            int sp = proc.indexOf(' ');
            return sp > 0 ? proc.mid(sp + 1) : proc;
        };
        if (!xorgProc.isEmpty())     infoRow(f, "Xorg:",     cmdPart(xorgProc));
        if (!xwaylandProc.isEmpty()) infoRow(f, "Xwayland:", cmdPart(xwaylandProc));
        if (!labwcProc.isEmpty())    infoRow(f, "labwc:",    cmdPart(labwcProc));
        if (xorgProc.isEmpty() && xwaylandProc.isEmpty() && labwcProc.isEmpty())
            infoRow(f, tr("Hinweis:"), tr("Kein Display-Server-Prozess gefunden"));
    }

    // Kameras (v4l2-Geräteknoten – ergänzend zur Modusliste oben)
    addMonoGroup(tr("Cameras (v4l2-ctl)"), camTxt);

    mainLay->addStretch();
    scroll->setWidget(content_w);
    outer->addWidget(scroll, 1);

    // ── Buttons: Kopieren | Schließen ──────────────────────────────────
    auto *btnBox = new QHBoxLayout;
    auto *copyBtn = new QPushButton(tr("In Zwischenablage kopieren"));
    auto *closeBtn = new QPushButton(tr("Schließen"));
    btnBox->addWidget(copyBtn);
    btnBox->addStretch();
    btnBox->addWidget(closeBtn);
    outer->addLayout(btnBox);

    QObject::connect(copyBtn, &QPushButton::clicked, [&]() {
        QStringList lines;
        lines << QString("=== %1 System Information ===")
                     .arg(QLatin1String(AppMeta::NAME));
        lines << "Datum: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        lines << "Version: " VERSION_STRING;
        lines << "";
        lines << "--- Kameras (rpicam-hello --list) ---";
        lines << (camModesTxt.isEmpty() ? "(nicht verfügbar)" : camModesTxt);
        lines << "";
        lines << "--- rpicam-apps ---";
        lines << "Version:         " + (m_rpicamAppsVersion.isEmpty() ? "(nicht verfügbar)" : m_rpicamAppsVersion);
        lines << "rpicam-rt:       " + QString(m_hasRpicamRt ? "Yes" : "No");
        lines << "ROI selection:   " + QString(m_hasRoiSelection ? "Yes" : "No");
        lines << "Preview backend: " + QString(m_hasPreviewBackend ? "Yes" : "No");
        lines << "";
        lines << "--- Session ---";
        lines << "Art:             " + QString(isRemote ? "Remote (XRDP)" : "Lokal");
        lines << "Display-Server:  " + sessionType.toUpper();
        lines << "Display:         " + display;
        if (!waylandDisp.isEmpty()) lines << "Wayland:         " + waylandDisp;
        lines << "Session-ID:      " + sessionId;
        if (!loginctlService.isEmpty()) lines << "Dienst:          " + loginctlService;
        if (!loginctlDesktop.isEmpty()) lines << "Desktop:         " + loginctlDesktop;
        if (!loginctlSeat.isEmpty())    lines << "Seat:            " + loginctlSeat;
        lines << "";
        lines << "--- System ---";
        lines << "OS:              " + QSysInfo::prettyProductName();
        lines << "Kernel:          " + QSysInfo::kernelVersion();
        lines << "Architektur:     " + QSysInfo::currentCpuArchitecture();
        lines << "Qt:              " + QString(qVersion());
        lines << "";
        lines << "--- GPU / DRM ---";
        lines << "GPU:             " + QString(hasGpu ? "Verfügbar" : "Nicht verfügbar");
        lines << "Geräte:          " + driDevices;
        lines << "";
        lines << "--- Display-Server ---";
        if (!xorgProc.isEmpty()) lines << "Xorg:            " + xorgProc;
        if (!xwaylandProc.isEmpty()) lines << "Xwayland:        " + xwaylandProc;
        if (!labwcProc.isEmpty()) lines << "labwc:           " + labwcProc;
        lines << "";
        lines << "--- Kameras ---";
        lines << (camTxt.isEmpty() ? "(nicht verfügbar)" : camTxt);

        QApplication::clipboard()->setText(lines.join('\n'));
        copyBtn->setText(tr("Kopiert!"));
        QTimer::singleShot(2000, copyBtn, [copyBtn]() {
            copyBtn->setText(QObject::tr("In Zwischenablage kopieren"));
        });
    });

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    dlg.exec();
}
void MainWindow::updateCodecVisibility(const QString &selectedApp) {
    // Nur noch Timelapse-Sichtbarkeit aktualisieren
    updateTimelapseVisibility();
}

void MainWindow::showAboutDialog(int tabIndex) {
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle(tr("About %1").arg(QLatin1String(AppMeta::NAME)));
    aboutDialog.setFixedSize(700, 750);

    QVBoxLayout *mainLayout = new QVBoxLayout(&aboutDialog);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Header with logo and app info
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(15);

    // Logo on the left
    QLabel *logoLabel = new QLabel();
    QPixmap logoPixmap(QLatin1String(AppMeta::LOGO_RESOURCE));
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    headerLayout->addWidget(logoLabel, 0, Qt::AlignTop);

    // Text on the right with background logo
    QWidget *headerTextWidget = new QWidget();
    headerTextWidget->setMinimumSize(400, 110); // 20px lower header (tabs closer)

    QVBoxLayout *headerTextLayout = new QVBoxLayout(headerTextWidget);
    headerTextLayout->setSpacing(2);
    headerTextLayout->setContentsMargins(0, 0, 0, 0);

    // NOTE: The h2 title is commented out because the new logo (piStudio_text.svg)
    // already contains the app name as text - showing it twice would be redundant.
    /*
    QLabel *titleLabel = new QLabel(QString("<h2 style='margin: 0;'>%1</h2>")
                                        .arg(QLatin1String(AppMeta::NAME)));
    titleLabel->setAlignment(Qt::AlignLeft);
    headerTextLayout->addWidget(titleLabel);
    */

    QLabel *versionLabel = new QLabel(
        QStringLiteral("<p style='margin:2px 0;'>%1 <b>%2</b></p>")
            .arg(tr("Version"), QLatin1String(VERSION_STRING)));
    versionLabel->setAlignment(Qt::AlignLeft);
    headerTextLayout->addWidget(versionLabel);

    QLabel *descLabel = new QLabel(
        tr("<p style='margin:2px 0;'><i>Camera App for Raspberry Pi OS</i></p>"));
    descLabel->setAlignment(Qt::AlignLeft);
    descLabel->setWordWrap(true);
    headerTextLayout->addWidget(descLabel);

    QLabel *copyrightLabel = new QLabel(tr(
        "<p style='margin: 2px 0;'>Copyright © 2025-%1 <b>Kletternaut</b></p>"
        "<p style='margin: 2px 0;'><a href='%2'>%3</a></p>")
        .arg(QDate::currentDate().year())
        .arg(AppMeta::repoUrl(), AppMeta::repoUrl()));
    copyrightLabel->setAlignment(Qt::AlignLeft);
    copyrightLabel->setOpenExternalLinks(true);
    headerTextLayout->addWidget(copyrightLabel);

    headerTextLayout->addStretch();

    // Background logo - positioned after text layout is complete
    QLabel *bgLogoLabel = new QLabel(headerTextWidget);
    QPixmap bgPixmap(":/Kletternaut_logo874x530_alpha.png");
    if (!bgPixmap.isNull()) {
        bgPixmap = bgPixmap.scaledToHeight(80, Qt::SmoothTransformation);
        bgLogoLabel->setPixmap(bgPixmap);
        // Position at bottom right corner with 130px right and 5px down offset
        int xPos = headerTextWidget->minimumWidth() - bgPixmap.width() + 130;
        // Top-based offset: 5px from the top, shifted up by 50px per user
        // request. Clamp to 0 so the logo stays inside the (now shorter)
        // header widget.
        int yPos = qMax(0, headerTextWidget->minimumHeight() - bgPixmap.height() + 5 - 50);
        bgLogoLabel->setGeometry(xPos, yPos, bgPixmap.width(), bgPixmap.height());
        bgLogoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        bgLogoLabel->lower();
    }

    headerLayout->addWidget(headerTextWidget, 1);

    mainLayout->addLayout(headerLayout);
    mainLayout->addSpacing(10);

    // Tab widget like in the main window
    QTabWidget *tabWidget = new QTabWidget(&aboutDialog);
    tabWidget->setFocusPolicy(Qt::NoFocus);

    // === Tab 1: Third-Party Libraries ===
    QWidget *librariesTab = new QWidget();
    QVBoxLayout *libLayout = new QVBoxLayout(librariesTab);
    libLayout->setSpacing(10);

    QScrollArea *libScrollArea = new QScrollArea();
    libScrollArea->setWidgetResizable(true);
    libScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *libScrollWidget = new QWidget();
    libScrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *libScrollLayout = new QVBoxLayout(libScrollWidget);

    // === Direct Dependencies Group ===
    QGroupBox *directDepGroup = new QGroupBox(tr("Direct Dependencies"));
    directDepGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    QVBoxLayout *directDepLayout = new QVBoxLayout(directDepGroup);

    QLabel *directDepLabel = new QLabel(tr(
        "<p>%1 is dynamically linked against the following libraries:</p>"
        "<p><b>Qt5 (Widgets, Network, X11Extras, Gui, Core)</b><br>"
        "License: LGPL-3.0<br>"
        "Copyright: The Qt Company Ltd.<br>"
        "Website: <a href='https://www.qt.io/'>https://www.qt.io/</a></p>"

        "<p><b>X11 Libraries (libX11, libXext, libXfixes)</b><br>"
        "License: MIT License<br>"
        "Copyright: X.Org Foundation<br>"
        "Website: <a href='https://www.x.org/'>https://www.x.org/</a><br>"
        "Used for: Screen selection overlay with XQueryPointer</p>"

        "<p><b>Video4Linux2 (V4L2)</b><br>"
        "License: GPL-2.0 (Kernel API - header-only, no linking)<br>"
        "Part of the Linux Kernel<br>"
        "Website: <a href='https://www.kernel.org/'>https://www.kernel.org/</a></p>"
    ).arg(QLatin1String(AppMeta::NAME)));
    directDepLabel->setWordWrap(true);
    directDepLabel->setOpenExternalLinks(true);
    directDepLayout->addWidget(directDepLabel);

    libScrollLayout->addWidget(directDepGroup);

    // === System Libraries Group ===
    QGroupBox *systemLibGroup = new QGroupBox(tr("System Libraries"));
    systemLibGroup->setStyleSheet(directDepGroup->styleSheet());
    QVBoxLayout *systemLibLayout = new QVBoxLayout(systemLibGroup);

    QLabel *systemLibLabel = new QLabel(tr(
        "<p>Standard C/C++ runtime libraries provided by your system:</p>"
        "<p>• GNU C Library (glibc)<br>"
        "• GNU C++ Standard Library (libstdc++)<br>"
        "• GCC runtime library (libgcc)</p>"
    ));
    systemLibLabel->setWordWrap(true);
    systemLibLayout->addWidget(systemLibLabel);

    libScrollLayout->addWidget(systemLibGroup);

    // === External Tools Group ===
    QGroupBox *extToolsGroup = new QGroupBox(tr("External Tools"));
    extToolsGroup->setStyleSheet(directDepGroup->styleSheet());
    QVBoxLayout *extToolsLayout = new QVBoxLayout(extToolsGroup);

    QLabel *extToolsLabel = new QLabel(tr(
        "<p><b>rpicam-apps</b> (rpicam-vid, rpicam-still, etc.)<br>"
        "%1 is a graphical frontend for rpicam-apps. The rpicam-apps tools "
        "must be installed separately and are responsible for their own dependencies.<br>"
        "License: BSD-2-Clause<br>"
        "Website: <a href='https://github.com/raspberrypi/rpicam-apps'>https://github.com/raspberrypi/rpicam-apps</a></p>"

        "<p><b>rpicam-apps fork feature/rt-roi</b> (optional, by Kletternaut)<br>"
        "Adds runtime control (rpicam-rt) and interactive ROI selection to rpicam-apps: "
        "camera parameters can be changed live while streaming, and a region of interest "
        "can be selected visually in the Qt preview. %2 detects both features "
        "via the rpicam-rt:1 / roi_selection:1 capabilities and integrates them "
        "(RT badge, live sliders, ROI takeover into the ROI field).<br>"
        "License: BSD-2-Clause (same as rpicam-apps)<br>"
        "Website: <a href='https://github.com/Kletternaut/rpicam-apps/tree/feature/rt-roi'>https://github.com/Kletternaut/rpicam-apps/tree/feature/rt-roi</a></p>"

        "<p><b>ffmpeg</b> (optional)<br>"
        "Used for video conversion (MJPEG to H.264/MP4) when sending videos to Telegram.<br>"
        "License: LGPL-2.1+ or GPL-2.0+ (depending on build configuration)<br>"
        "Website: <a href='https://ffmpeg.org/'>https://ffmpeg.org/</a></p>"
    ).arg(QLatin1String(AppMeta::NAME), QLatin1String(AppMeta::NAME)));
    extToolsLabel->setWordWrap(true);
    extToolsLabel->setOpenExternalLinks(true);
    extToolsLayout->addWidget(extToolsLabel);

    libScrollLayout->addWidget(extToolsGroup);
    libScrollLayout->addStretch();

    libScrollArea->setWidget(libScrollWidget);
    libLayout->addWidget(libScrollArea);

    tabWidget->addTab(librariesTab, tr("Third-Party Libraries"));

    // === Tab 2: License Information ===
    QWidget *licenseTab = new QWidget();
    QVBoxLayout *licLayout = new QVBoxLayout(licenseTab);
    licLayout->setSpacing(10);

    QScrollArea *licScrollArea = new QScrollArea();
    licScrollArea->setWidgetResizable(true);
    licScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *licScrollWidget = new QWidget();
    licScrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *licScrollLayout = new QVBoxLayout(licScrollWidget);

    // === License Group ===
    QGroupBox *licenseGroup = new QGroupBox(tr("%1 License").arg(QLatin1String(AppMeta::NAME)));
    licenseGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    QVBoxLayout *licenseLayout = new QVBoxLayout(licenseGroup);

    QLabel *licenseLabel = new QLabel(tr(
        "<p><b>PolyForm Noncommercial License 1.0.0</b></p>"
        "<p>%1 is licensed under the PolyForm Noncommercial License 1.0.0. "
        "Noncommercial use is permitted free of charge. "
        "Commercial use requires a separate license from the copyright holder.</p>"
        "<p>This software is provided \"as is\", without warranty of any kind, "
        "to the extent permitted by law.</p>"
        "<p>Full license text: "
        "<a href='%2/blob/main/LICENSE.md'>LICENSE.md</a></p>"
    ).arg(QLatin1String(AppMeta::NAME), AppMeta::repoUrl()));
    licenseLabel->setWordWrap(true);
    licenseLabel->setOpenExternalLinks(true);
    licenseLayout->addWidget(licenseLabel);

    licScrollLayout->addWidget(licenseGroup);

    // === Dynamic Linking Group ===
    QGroupBox *linkingGroup = new QGroupBox(tr("Dynamic Linking Notice"));
    linkingGroup->setStyleSheet(licenseGroup->styleSheet());
    QVBoxLayout *linkingLayout = new QVBoxLayout(linkingGroup);

    QLabel *linkingLabel = new QLabel(tr(
        "<p>%1 dynamically links against Qt5 and X11 libraries. "
        "No library code is included in the %2 binary distribution. "
        "All required libraries must be installed separately on your system.</p>"
        "<p>Dynamically linking the LGPL-licensed Qt5 libraries "
        "is permitted by their licenses.</p>"
    ).arg(QLatin1String(AppMeta::NAME), QLatin1String(AppMeta::NAME)));
    linkingLabel->setWordWrap(true);
    linkingLayout->addWidget(linkingLabel);

    licScrollLayout->addWidget(linkingGroup);

    // === Source Code Group ===
    QGroupBox *sourceGroup = new QGroupBox(tr("Source Code"));
    sourceGroup->setStyleSheet(licenseGroup->styleSheet());
    QVBoxLayout *sourceLayout = new QVBoxLayout(sourceGroup);

    QLabel *sourceLabel = new QLabel(tr(
        "<p>The complete source code of %1 is available at:<br>"
        "<a href='%2'>%3</a></p>"
    ).arg(QLatin1String(AppMeta::NAME), AppMeta::repoUrl(), AppMeta::repoUrl()));
    sourceLabel->setWordWrap(true);
    sourceLabel->setOpenExternalLinks(true);
    sourceLayout->addWidget(sourceLabel);

    licScrollLayout->addWidget(sourceGroup);

    // === Third-Party Licenses Group ===
    QGroupBox *thirdPartyLicGroup = new QGroupBox(tr("Third-Party Licenses"));
    thirdPartyLicGroup->setStyleSheet(licenseGroup->styleSheet());
    QVBoxLayout *thirdPartyLicLayout = new QVBoxLayout(thirdPartyLicGroup);

    QLabel *thirdPartyLicLabel = new QLabel(tr(
        "<p>All third-party libraries retain their original licenses. "
        "For detailed license information of installed libraries, please refer to "
        "your system's package manager (e.g., <tt>dpkg -L &lt;package&gt;</tt> or "
        "<tt>/usr/share/doc/&lt;package&gt;/copyright</tt>).</p>"
    ));
    thirdPartyLicLabel->setWordWrap(true);
    thirdPartyLicLayout->addWidget(thirdPartyLicLabel);

    licScrollLayout->addWidget(thirdPartyLicGroup);
    licScrollLayout->addStretch();

    licScrollArea->setWidget(licScrollWidget);
    licLayout->addWidget(licScrollArea);

    tabWidget->addTab(licenseTab, tr("License"));

    // === Tab 3: Disclaimer ===
    QWidget *disclaimerTab = new QWidget();
    QVBoxLayout *disclaimerTabLayout = new QVBoxLayout(disclaimerTab);
    disclaimerTabLayout->setSpacing(10);

    QScrollArea *disclaimerScrollArea = new QScrollArea();
    disclaimerScrollArea->setWidgetResizable(true);
    disclaimerScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *disclaimerScrollWidget = new QWidget();
    disclaimerScrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *disclaimerScrollLayout = new QVBoxLayout(disclaimerScrollWidget);

    const QString groupBoxStyle =
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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

    // === Warranty Disclaimer Group ===
    QGroupBox *warrantyGroup = new QGroupBox(tr("Warranty Disclaimer"));
    warrantyGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout *warrantyLayout = new QVBoxLayout(warrantyGroup);

    QLabel *warrantyLabel = new QLabel(tr(
        "<p>This software is provided <b>\"as is\"</b>, without warranty of any kind, "
        "express or implied. Use at your own risk.</p>"
        "<p>The author accepts no liability for any damage, data loss, hardware issues, "
        "or other consequences arising from the use or misuse of this software.</p>"
        "<p>As stated in the license: the Software is provided without any warranty. "
        "The author assumes no liability for damages arising from its use, "
        "unless caused by intentional misconduct or gross negligence.</p>"
    ));
    warrantyLabel->setWordWrap(true);
    warrantyLayout->addWidget(warrantyLabel);
    disclaimerScrollLayout->addWidget(warrantyGroup);

    // === Trademark Disclaimer Group ===
    QGroupBox *trademarkGroup = new QGroupBox(tr("Trademark Disclaimer"));
    trademarkGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout *trademarkLayout = new QVBoxLayout(trademarkGroup);

    QLabel *trademarkLabel = new QLabel(tr(
        "<p>%1 is an independent open-source project and is not affiliated with "
        "or endorsed by Raspberry Pi Ltd. \"Raspberry Pi\" is a trademark of Raspberry Pi Ltd.</p>"
    ).arg(QLatin1String(AppMeta::NAME)));
    trademarkLabel->setWordWrap(true);
    trademarkLayout->addWidget(trademarkLabel);
    disclaimerScrollLayout->addWidget(trademarkGroup);

    // === Development Group ===
    QGroupBox *devGroup = new QGroupBox(tr("Development"));
    devGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout *devLayout = new QVBoxLayout(devGroup);

    QLabel *devLabel = new QLabel(tr(
        "<p>This software was created with AI assistance.</p>"
    ));
    devLabel->setWordWrap(true);
    devLayout->addWidget(devLabel);

    disclaimerScrollLayout->addWidget(devGroup);

    disclaimerScrollLayout->addStretch();
    disclaimerScrollArea->setWidget(disclaimerScrollWidget);
    disclaimerTabLayout->addWidget(disclaimerScrollArea);

    tabWidget->addTab(disclaimerTab, tr("Disclaimer"));

    // === Tab 4: Credits ===
    QWidget *creditsTab = new QWidget();
    QVBoxLayout *creditsLayout = new QVBoxLayout(creditsTab);
    creditsLayout->setSpacing(10);

    QScrollArea *creditsScrollArea = new QScrollArea();
    creditsScrollArea->setWidgetResizable(true);
    creditsScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *creditsScrollWidget = new QWidget();
    creditsScrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *creditsScrollLayout = new QVBoxLayout(creditsScrollWidget);

    // === Based On Group ===
    QGroupBox *basedOnGroup = new QGroupBox(tr("Based on"));
    basedOnGroup->setStyleSheet(devGroup->styleSheet());
    QVBoxLayout *basedOnLayout = new QVBoxLayout(basedOnGroup);

    QLabel *basedOnLabel = new QLabel(tr(
        "<p><b>rpicam-apps</b> by Raspberry Pi Foundation<br>"
        "License: BSD-2-Clause<br>"
        "Website: <a href='https://github.com/raspberrypi/rpicam-apps'>https://github.com/raspberrypi/rpicam-apps</a></p>"
    ));
    basedOnLabel->setWordWrap(true);
    basedOnLabel->setOpenExternalLinks(true);
    basedOnLayout->addWidget(basedOnLabel);

    creditsScrollLayout->addWidget(basedOnGroup);

    // === Special Thanks Group ===
    QGroupBox *thanksGroup = new QGroupBox(tr("Special Thanks"));
    thanksGroup->setStyleSheet(devGroup->styleSheet());
    QVBoxLayout *thanksLayout = new QVBoxLayout(thanksGroup);

    QLabel *thanksLabel = new QLabel(tr(
        "<p>• GitHub for providing free platform and development tools<br>"
        "• The Raspberry Pi Foundation for rpicam-apps<br>"
        "• The Qt Project for the excellent GUI framework<br>"
        "• The Linux Kernel developers for V4L2<br>"
        "• The FFmpeg and GStreamer communities<br>"
        "• All open-source contributors"
    ));
    thanksLabel->setWordWrap(true);
    thanksLayout->addWidget(thanksLabel);

    creditsScrollLayout->addWidget(thanksGroup);
    creditsScrollLayout->addStretch();

    creditsScrollArea->setWidget(creditsScrollWidget);
    creditsLayout->addWidget(creditsScrollArea);

    tabWidget->addTab(creditsTab, tr("Credits"));

    // === Tab 5: Donate ===
    QWidget *donateTab = new QWidget();
    QVBoxLayout *donateLayout = new QVBoxLayout(donateTab);
    donateLayout->setSpacing(10);

    QScrollArea *donateScrollArea = new QScrollArea();
    donateScrollArea->setWidgetResizable(true);
    donateScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *donateScrollWidget = new QWidget();
    donateScrollWidget->setStyleSheet("background-color: #f8f9fa;");
    QVBoxLayout *donateScrollLayout = new QVBoxLayout(donateScrollWidget);

    // Support Message
    QGroupBox *supportGroup = new QGroupBox(tr("Support %1 Development")
                                                .arg(QLatin1String(AppMeta::NAME)));
    supportGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    QVBoxLayout *supportLayout = new QVBoxLayout(supportGroup);

    QLabel *supportMsg = new QLabel(tr(
        "<p>If you find <b>%1</b> useful and would like to support its development, "
        "consider making a donation. Your support helps maintain and improve this free, "
        "open-source software.</p>"
        "<p>Donations help cover the monthly costs for development tools, "
        "hardware, and testing equipment.</p>"
        "<p><b>Thank you for your support!</b></p>"
    ).arg(QLatin1String(AppMeta::NAME)));
    supportMsg->setWordWrap(true);
    supportLayout->addWidget(supportMsg);

    donateScrollLayout->addWidget(supportGroup);

    // Donation Methods
    QGroupBox *methodsGroup = new QGroupBox(tr("How to Donate"));
    methodsGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
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
        "}"
    );
    QVBoxLayout *methodsLayout = new QVBoxLayout(methodsGroup);

    // PayPal Button with QR Code in horizontal layout
    QHBoxLayout *paypalLayout = new QHBoxLayout();

    QPushButton *paypalButton = new QPushButton(tr("Donate via PayPal"));
    paypalButton->setFixedHeight(30);
    connect(paypalButton, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://paypal.me/Kletternaut"));
    });
    paypalLayout->addWidget(paypalButton);

    // Small QR Code on the right side
    QLabel *qrCodeLabel = new QLabel();
    QPixmap qrPixmap(":/paypal_donate_qr.png");
    if (!qrPixmap.isNull()) {
        qrCodeLabel->setPixmap(qrPixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        qrCodeLabel->setAlignment(Qt::AlignCenter);
        qrCodeLabel->setToolTip(tr("Scan with smartphone to donate via PayPal"));
        paypalLayout->addWidget(qrCodeLabel);
    }

    methodsLayout->addLayout(paypalLayout);

    QLabel *methodsInfo = new QLabel(tr(
        "<p>PayPal is a secure and easy way to make a one-time donation. "
        "Click the button or scan the QR code with your smartphone.</p>"
    ));
    methodsInfo->setWordWrap(true);
    methodsLayout->addWidget(methodsInfo);

    donateScrollLayout->addWidget(methodsGroup);
    donateScrollLayout->addStretch();

    donateScrollArea->setWidget(donateScrollWidget);
    donateLayout->addWidget(donateScrollArea);

    tabWidget->addTab(donateTab, tr("Donate"));

    mainLayout->addWidget(tabWidget);

    // Set requested tab
    if (tabIndex >= 0 && tabIndex < tabWidget->count()) {
        tabWidget->setCurrentIndex(tabIndex);
    }

    // Close Button
    QPushButton *closeButton = new QPushButton(tr("Close"));
    closeButton->setFixedHeight(30);
    connect(closeButton, &QPushButton::clicked, &aboutDialog, &QDialog::accept);
    mainLayout->addWidget(closeButton, 0, Qt::AlignCenter);

    aboutDialog.exec();
}
void MainWindow::updateBoxInputFromSelection(const QRect &selection) {
    QString boxText = QString("%1,%2,%3,%4")
                          .arg(selection.x())
                          .arg(selection.y())
                          .arg(selection.width())
                          .arg(selection.height());
    BoxInput->setText(boxText);
}

void MainWindow::calculateLoresWidth(int height) {
    // This function is now handled internally by LoresComboBox
    // Left for backward compatibility but no longer used
    Q_UNUSED(height);
}

void MainWindow::calculateLoresHeight(int width) {
    // This function is now handled internally by LoresComboBox
    // Left for backward compatibility but no longer used
    Q_UNUSED(width);
}
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (selectionOverlay && selectionOverlay->isVisible()) {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            selectionOverlay->setGeometry(screen->geometry()); // Vollbild setzen
        }
    }

    // Keep BoxInput in sync with current window geometry after resize (e.g., collapsible groups).
    // Mirrors moveEvent behavior: recalculate and reset button to black so camera start
    // does not skip the fresh calculation (which would happen if button is red = "manually set").
    if (!isInitializing) {
        QSettings resSettings(AppPaths::globalConf(), QSettings::IniFormat);
        resSettings.beginGroup(m_tabGroup);
        bool useCustomGeometry = resSettings.value("Preview/UseCustomGeometry", false).toBool();
        resSettings.endGroup();

        // Only update BoxInput if preview is NOT running.
        // When preview is running its window cannot be moved; BoxInput must stay
        // at the value used at launch so siblings read the correct actual position.
        if (!useCustomGeometry && process.state() != QProcess::Running) {
            BoxInput->setText(calculateBoxInput(+30));
            if (overlayResetButton) {
                overlayResetButton->setStyleSheet("color: black;");
            }
        }
    }

    // Overlay-Reset-Button-Farbe aktualisieren
    updateOverlayResetButtonColor(overlayResetButton);
}
void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);

    // Only update BoxInput when preview is not running.
    // A running preview window cannot be moved; keeping BoxInput at its launch
    // value ensures siblings always read the real window position.
    if (process.state() == QProcess::Running) {
        return;
    }

    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    bool useCustomGeometry = settings.value("Preview/UseCustomGeometry", false).toBool();
    if (!useCustomGeometry) {
        BoxInput->setText(calculateBoxInput(+30));
        if (overlayResetButton) {
            overlayResetButton->setStyleSheet("color: black;");
        }
        if (!isInitializing && globalResetButton) {
            globalResetButton->setStyleSheet("color: black;");
        }
    }
    settings.endGroup();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Reposition the RT + SYNC badges when the Start/Stop button resizes
    if (obj == startStopButton && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
        repositionStartButtonBadges();
    }

    // MCIM: intercept Move events on the outer window to keep BoxInput in sync.
    // Skip when preview is running — its window cannot be moved, so BoxInput
    // must retain the launch value so siblings read the correct position.
    if (obj == window() && event->type() == QEvent::Move) {
        if (process.state() != QProcess::Running) {
            QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
            settings.beginGroup(m_tabGroup);
            if (!settings.value("Preview/UseCustomGeometry", false).toBool()) {
                BoxInput->setText(calculateBoxInput(+30));
                if (overlayResetButton) {
                    overlayResetButton->setStyleSheet("color: black;");
                }
            }
            settings.endGroup();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setupStillTab() {
    stillTab = new QWidget;
    tabRegistryService->registerTab(stillTab, "Still", 3, "", true);
    auto *mainLayout = new QVBoxLayout(stillTab);

    // =============================================================
    // CAPTURE CONTROL GROUP
    // =============================================================
    auto *captureControlGroup = new QGroupBox(tr("Capture Control"), this);
    captureControlGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *captureControlLayout = new QGridLayout(captureControlGroup);

    // --autofocus-on-capture
    autofocusOnCaptureCheckbox = new QCheckBox(tr("Autofocus on Capture"), this);
    autofocusOnCaptureCheckbox->setToolTip(tr("Run autofocus cycle when capture is requested (--autofocus-on-capture)"));
    captureControlLayout->addWidget(autofocusOnCaptureCheckbox, 0, 0);

    // --zsl
    zslCheckbox = new QCheckBox(tr("Zero Shutter Lag"), this);
    zslCheckbox->setToolTip(tr("Enable zero shutter lag mode (--zsl)"));
    captureControlLayout->addWidget(zslCheckbox, 0, 1);

    // --immediate
    immediateCheckbox = new QCheckBox(tr("Immediate Capture"), this);
    immediateCheckbox->setToolTip(tr("Capture image immediately without preview (--immediate)"));
    captureControlLayout->addWidget(immediateCheckbox, 1, 0);

    // --framestart
    captureControlLayout->addWidget(new QLabel(tr("Frame Start:")), 1, 1);
    framestartSpinBox = new QSpinBox(this);
    framestartSpinBox->setRange(0, 1000000);
    framestartSpinBox->setValue(0);
    framestartSpinBox->setToolTip(tr("Start frame number for numbered output files (--framestart)"));
    captureControlLayout->addWidget(framestartSpinBox, 1, 2);

    // Group Reset Button
    captureControlResetButton = new QPushButton("✕", this);
    captureControlResetButton->setFixedWidth(20);
    captureControlResetButton->setToolTip(tr("Reset all Capture Control settings"));
    captureControlLayout->addWidget(captureControlResetButton, 1, 3);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(captureControlGroup, "UI/Still/CaptureControlGroup", [this]() { adjustWindowToOptimalSize(); }));

    mainLayout->addWidget(captureControlGroup);

    // =============================================================
    // OUTPUT FORMAT GROUP
    // =============================================================
    auto *outputFormatGroup = new QGroupBox(tr("Output Format"), this);
    outputFormatGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *outputFormatLayout = new QGridLayout(outputFormatGroup);

    // --thumb
    outputFormatLayout->addWidget(new QLabel(tr("Thumbnail:")), 0, 0);
    thumbLineEdit = new QLineEdit(this);
    thumbLineEdit->setPlaceholderText(tr("width:height:quality (e.g. 320:240:70)"));
    thumbLineEdit->setToolTip(tr("Add thumbnail to JPEG file in format width:height:quality (--thumb)"));
    outputFormatLayout->addWidget(thumbLineEdit, 0, 1, 1, 2);

    thumbResetButton = new QPushButton("✕", this);
    thumbResetButton->setFixedWidth(20);
    thumbResetButton->setToolTip(tr("Reset Thumbnail"));
    outputFormatLayout->addWidget(thumbResetButton, 0, 3);

    // --restart
    outputFormatLayout->addWidget(new QLabel(tr("Restart Interval:")), 1, 0);
    restartSpinBox = new QSpinBox(this);
    restartSpinBox->setRange(0, 1000);
    restartSpinBox->setValue(0);
    restartSpinBox->setToolTip(tr("Set JPEG restart interval (0=none) (--restart)"));
    outputFormatLayout->addWidget(restartSpinBox, 1, 1, 1, 2);

    restartResetButton = new QPushButton("✕", this);
    restartResetButton->setFixedWidth(20);
    restartResetButton->setToolTip(tr("Reset Restart Interval to 0"));
    outputFormatLayout->addWidget(restartResetButton, 1, 3);

    // --exif
    outputFormatLayout->addWidget(new QLabel(tr("EXIF Tags:")), 2, 0);
    exifLineEdit = new QLineEdit(this);
    exifLineEdit->setPlaceholderText(tr("tag1=value1,tag2=value2"));
    exifLineEdit->setToolTip(tr("Add custom EXIF tags (--exif \"tag1=value1,tag2=value2\")"));
    outputFormatLayout->addWidget(exifLineEdit, 2, 1, 1, 2);

    exifResetButton = new QPushButton("✕", this);
    exifResetButton->setFixedWidth(20);
    exifResetButton->setToolTip(tr("Reset EXIF Tags"));
    outputFormatLayout->addWidget(exifResetButton, 2, 3);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(outputFormatGroup, "UI/Still/OutputFormatGroup", [this]() { adjustWindowToOptimalSize(); }));

    mainLayout->addWidget(outputFormatGroup);

    // =============================================================
    // FILE MANAGEMENT GROUP
    // =============================================================
    auto *fileManagementGroup = new QGroupBox(tr("File Management"), this);
    fileManagementGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *fileManagementLayout = new QGridLayout(fileManagementGroup);

    // --latest
    fileManagementLayout->addWidget(new QLabel(tr("Latest Symlink:")), 0, 0);
    latestLineEdit = new QLineEdit(this);
    latestLineEdit->setPlaceholderText(tr("Path for symlink to latest image"));
    latestLineEdit->setToolTip(tr("Create/update symlink to latest captured image (--latest)"));
    fileManagementLayout->addWidget(latestLineEdit, 0, 1, 1, 2);

    // --raw
    rawCheckbox = new QCheckBox(tr("Save RAW (DNG)"), this);
    rawCheckbox->setToolTip(tr("Save raw Bayer data in DNG format (--raw)"));
    fileManagementLayout->addWidget(rawCheckbox, 1, 0);

    // Group Reset Button
    fileManagementResetButton = new QPushButton("✕", this);
    fileManagementResetButton->setFixedWidth(20);
    fileManagementResetButton->setToolTip(tr("Reset all File Management settings"));
    fileManagementLayout->addWidget(fileManagementResetButton, 1, 3);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(fileManagementGroup, "UI/Still/FileManagementGroup", [this]() { adjustWindowToOptimalSize(); }));

    mainLayout->addWidget(fileManagementGroup);

    // Stretch am Ende
    mainLayout->addStretch();
}

void MainWindow::setupAudioTab() {
    audioTab = new QWidget;
    tabRegistryService->registerTab(audioTab, "Audio", 5, "audioTabEnabled", true);
    auto *mainLayout = new QVBoxLayout(audioTab);

    // =============================================================
    // ACTIVATION CONTROL - AUDIO RECORDING
    // =============================================================
    enableAudioCheckBox = new QCheckBox(tr("Enable Audio Recording"), this);
    enableAudioCheckBox->setChecked(false);  // Default: disabled
    enableAudioCheckBox->setToolTip(tr("Activate audio recording. Must be enabled to use audio functionality."));
    enableAudioCheckBox->setStyleSheet("font-weight: bold; color: #2E7D32;");

    audioResetButton = new QPushButton("✕", this);
    audioResetButton->setFixedWidth(20);
    audioResetButton->setToolTip(tr("Reset all audio settings to default values"));

    auto *enableRow = new QHBoxLayout;
    enableRow->addWidget(enableAudioCheckBox);
    enableRow->addStretch();
    enableRow->addWidget(audioResetButton);
    mainLayout->addLayout(enableRow);

    // =============================================================
    // AUDIO CODEC SETTINGS GROUP
    // =============================================================
    auto *codecGroup = new QGroupBox(tr("Audio Codec"), this);
    codecGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *codecLayout = new QGridLayout(codecGroup);

    // Codec Selection
    codecLayout->addWidget(new QLabel(tr("Codec:")), 0, 0);
    audioCodecSelector = new QComboBox(this);
    audioCodecSelector->addItems({"aac", "mp3", "opus"});
    audioCodecSelector->setCurrentText("aac");
    audioCodecSelector->setToolTip(tr("Audio codec for encoding"));
    codecLayout->addWidget(audioCodecSelector, 0, 1);

    // Bitrate Setting
    codecLayout->addWidget(new QLabel(tr("Bitrate:")), 0, 2);
    audioBitrateSpinBox = new QSpinBox(this);
    audioBitrateSpinBox->setRange(32, 320);
    audioBitrateSpinBox->setValue(128);
    audioBitrateSpinBox->setSuffix(" kbps");
    audioBitrateSpinBox->setToolTip(tr("Audio bitrate in kilobits per second"));
    codecLayout->addWidget(audioBitrateSpinBox, 0, 3);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(codecGroup, "UI/Audio/CodecGroup", [this]() { adjustWindowToOptimalSize(); }));

    mainLayout->addWidget(codecGroup);

    // =============================================================
    // AUDIO SOURCE SETTINGS GROUP
    // =============================================================
    auto *sourceGroup = new QGroupBox(tr("Audio Source"), this);
    sourceGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *sourceLayout = new QGridLayout(sourceGroup);

    // Source Selection
    sourceLayout->addWidget(new QLabel(tr("Source:")), 0, 0);
    audioSourceSelector = new QComboBox(this);
    audioSourceSelector->addItems({"pulse", "alsa"});
    audioSourceSelector->setCurrentText("pulse");
    audioSourceSelector->setToolTip(tr("Audio input source system"));
    sourceLayout->addWidget(audioSourceSelector, 0, 1);

    // Device Selection
    sourceLayout->addWidget(new QLabel(tr("Device:")), 0, 2);
    audioDeviceEdit = new QLineEdit(this);
    audioDeviceEdit->setPlaceholderText(tr("hw:0,0 or default"));
    audioDeviceEdit->setToolTip(tr("Audio input device identifier"));
    sourceLayout->addWidget(audioDeviceEdit, 0, 3);

    // Channels Setting
    sourceLayout->addWidget(new QLabel(tr("Channels:")), 1, 0);
    audioChannelsSpinBox = new QSpinBox(this);
    audioChannelsSpinBox->setRange(1, 8);
    audioChannelsSpinBox->setValue(2);
    audioChannelsSpinBox->setToolTip(tr("Number of audio channels (1=mono, 2=stereo)"));
    sourceLayout->addWidget(audioChannelsSpinBox, 1, 1);

    // Sample Rate Setting
    sourceLayout->addWidget(new QLabel(tr("Sample Rate:")), 1, 2);
    audioSampleRateSelector = new QComboBox(this);
    audioSampleRateSelector->addItems({"8000", "16000", "22050", "44100", "48000", "96000"});
    audioSampleRateSelector->setCurrentText("44100");
    audioSampleRateSelector->setToolTip(tr("Audio sample rate in Hz"));
    sourceLayout->addWidget(audioSampleRateSelector, 1, 3);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(sourceGroup, "UI/Audio/SourceGroup", [this]() { adjustWindowToOptimalSize(); }));

    mainLayout->addWidget(sourceGroup);

    // =============================================================
    // SYNC SETTINGS GROUP
    // =============================================================
    auto *syncGroup = new QGroupBox(tr("Synchronization"), this);
    syncGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *syncLayout = new QGridLayout(syncGroup);

    // AV Sync Offset
    syncLayout->addWidget(new QLabel(tr("AV Sync:")), 0, 0);
    audioAvSyncSpinBox = new QSpinBox(this);
    audioAvSyncSpinBox->setRange(-1000000, 1000000);
    audioAvSyncSpinBox->setValue(0);
    audioAvSyncSpinBox->setSuffix(" μs");
    audioAvSyncSpinBox->setToolTip(tr("Audio/video synchronization offset in microseconds"));
    syncLayout->addWidget(audioAvSyncSpinBox, 0, 1);

    // Make group collapsible
    allCollapsibleHelpers.append(CollapsibleHelper::makeCollapsible(syncGroup, "UI/Audio/SyncGroup", [this]() { adjustWindowToOptimalSize(); }));

    mainLayout->addWidget(syncGroup);

    mainLayout->addStretch();

    // Load saved settings first (before connecting signals to avoid triggering saves during load)
    loadAudioSettings();

    // Initialize state
    updateAudioControlsState();
    updateAudioResetButtonColor();

    // Connect signals
    connect(enableAudioCheckBox, &QCheckBox::toggled, this, &MainWindow::onAudioToggled);
    connect(audioResetButton, &QPushButton::clicked, this, &MainWindow::resetAudioToDefaults);

    // Connect signals to save settings when changed (after loading to avoid triggering during init)
    connect(enableAudioCheckBox, &QCheckBox::toggled, this, &MainWindow::saveAudioSettings);
    connect(audioCodecSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::saveAudioSettings);
    connect(audioBitrateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::saveAudioSettings);
    connect(audioSourceSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::saveAudioSettings);
    connect(audioDeviceEdit, &QLineEdit::textChanged, this, &MainWindow::saveAudioSettings);
    connect(audioChannelsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::saveAudioSettings);
    connect(audioSampleRateSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::saveAudioSettings);
    connect(audioAvSyncSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::saveAudioSettings);

    // Connect signals to update reset button color
    connect(enableAudioCheckBox, &QCheckBox::toggled, this, &MainWindow::updateAudioResetButtonColor);
    connect(audioCodecSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateAudioResetButtonColor);
    connect(audioBitrateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateAudioResetButtonColor);
    connect(audioSourceSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateAudioResetButtonColor);
    connect(audioDeviceEdit, &QLineEdit::textChanged, this, &MainWindow::updateAudioResetButtonColor);
    connect(audioChannelsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateAudioResetButtonColor);
    connect(audioSampleRateSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateAudioResetButtonColor);
    connect(audioAvSyncSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateAudioResetButtonColor);
}

void MainWindow::setupGstreamerTab() {
    m_gstreamerTab = new GStreamerTab(this);
    m_gstreamerTab->initialize(m_tabGroup, tabRegistryService, allCollapsibleHelpers, this);
    m_gstreamerTab->setResolutionSelector(resolutionSelector);
    QSettings gstSettings(AppPaths::globalConf(), QSettings::IniFormat);
    gstSettings.beginGroup(m_tabGroup);
    m_gstreamerTab->loadSettings(gstSettings);
    gstSettings.endGroup();
    gstreamerTab = m_gstreamerTab->tab();
    // gstreamerTab->setEnabled(false) wird bereits in GStreamerModule::setup() gesetzt
}

void MainWindow::setupGstLaunchTab() {
    m_gstLaunchTab = new GstLaunchTab(this);
    m_gstLaunchTab->initialize(m_tabGroup, tabRegistryService, allCollapsibleHelpers, this);
    gstLaunchTab = m_gstLaunchTab->tab();

    // Forward GST debug output directly to the main log window
    if (m_gstLaunchTab->module()) {
        connect(m_gstLaunchTab->module(), &GstLaunchModule::debugLog,
                this, [this](const QString &msg) {
            appendLog(msg);
        });
    }
}


void MainWindow::setupInferenceTab() {
    m_inferenceTab = new InferenceTab(this);
    m_inferenceTab->initialize(m_tabGroup, tabRegistryService, allCollapsibleHelpers, this);
    connect(m_inferenceTab, &InferenceTab::detectionToExecute,
            this, [this](const QString &object, int confidence, const QString &fullDetection) {
        if (m_actionsTab && m_actionsTab->module()) {
            m_actionsTab->module()->executeDetection(object, confidence, fullDetection);
        }
    });
    inferenceTab = m_inferenceTab->tab();
}
void MainWindow::setupActionsTab() {
    m_actionsTab = new ActionsTab(this);

    // CameraInterface — callbacks for current camera process
    CameraInterface ci;
    ci.isVidRunning            = [this]() { return process.state() == QProcess::Running; };
    ci.getProcessId            = [this]() { return process.processId(); };
    ci.isSignalRecordingEnabled = [this]() {
        return signalRecordingCheckbox && signalRecordingCheckbox->isChecked();
    };
    ci.getCurrentApp           = [this]() { return appSelector->currentText(); };
    ci.getBoxCoords            = [this]() { return BoxInput->text(); };
    ci.getOutputFileName       = [this]() { return outputFileName ? outputFileName->text() : QString(); };

    m_actionsTab->initialize(m_tabGroup, tabRegistryService, allCollapsibleHelpers, this);
    m_actionsTab->setCameraInterface(ci);

    // Signale des Moduls → MainWindow-Slots
    auto *mod = m_actionsTab->module();
    connect(mod, &ActionsModule::startTimedRecordingRequested,
            this, &MainWindow::startTimedRecording);
    connect(mod, &ActionsModule::setCodecMjpegRequested, this, [this]() {
        if (codecSelector) codecSelector->setCurrentText("mjpeg");
    });
    connect(mod, &ActionsModule::enableSignalRecordingRequested, this, [this]() {
        if (signalRecordingCheckbox) signalRecordingCheckbox->setChecked(true);
    });
    connect(mod, &ActionsModule::enableSplitFilesRequested, this, [this]() {
        if (splitFilesCheckbox) splitFilesCheckbox->setChecked(true);
    });
    connect(mod, &ActionsModule::setOutputModeFileRequested, this, [this]() {
        if (outputModeFile) outputModeFile->setChecked(true);
    });
    connect(mod, &ActionsModule::enableAutoNamingRequested, this, [this]() {
        if (autoNamingCheckbox) autoNamingCheckbox->setChecked(true);
    });
    connect(mod, &ActionsModule::enableSegmentPatternRequested, this, [this]() {
        if (segmentPatternCheckbox) segmentPatternCheckbox->setChecked(true);
    });

    // Update global reset button color when filter settings (cooldown/confidence) change
    connect(mod, &ActionsModule::actionsStateChanged,
            this, [this]() { if (!isInitializing) updateGlobalResetButtonColor(); });

    actionsTab = m_actionsTab->tab();
}

void MainWindow::setupToolsTab() {
    m_toolsTab = new ToolsTab(this);
    m_toolsTab->initialize(m_tabGroup, tabRegistryService, allCollapsibleHelpers, this);
    toolsTab = m_toolsTab->tab();
}


void MainWindow::setupLayout() {
    QVBoxLayout *mainLayout = new QVBoxLayout;
    auto *appLayout = new QHBoxLayout;
    appLayout->addWidget(new QLabel(tr("App:"), this));
    appLayout->addWidget(appSelector);
    mainLayout->addLayout(appLayout);
    auto *cameraLayout = new QHBoxLayout;
    cameraLayout->addWidget(new QLabel(tr("Cam:"), this));
    cameraLayout->addWidget(cameraSelector);
    mainLayout->addLayout(cameraLayout);
    auto *resolutionLayout = new QHBoxLayout;
    resolutionLayout->addWidget(new QLabel(tr("Size:"), this));
    resolutionLayout->addWidget(resolutionSelector);
    mainLayout->addLayout(resolutionLayout);
    auto *framerateLayout = new QHBoxLayout;
    framerateLayout->addWidget(new QLabel(tr("Framerate:"), this));
    framerateLayout->addWidget(framerateSelector);
    mainLayout->addLayout(framerateLayout);
    auto *boxLayout = new QVBoxLayout; // Verwende ein separates vertikales Layout
    auto *boxLabel = new QLabel(tr("Box:"), this);
    boxLayout->addWidget(boxLabel);   // Label in einer eigenen Zeile
    boxLayout->addWidget(BoxInput);   // QLineEdit in einer eigenen Zeile
    mainLayout->addLayout(boxLayout); // Füge das vertikale Layout zum Hauptlayout hinzu
    parameterWidget->setLayout(mainLayout);
}
void MainWindow::updateResetButtonColor(QPushButton *button, double currentValue, double defaultValue) {
    if (currentValue != defaultValue) {
        button->setStyleSheet("color: red;"); // Rot, wenn der Wert nicht Standard ist
    } else {
        button->setStyleSheet("color: black;"); // Schwarz, wenn der Wert Standard ist
    }
}
void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    // Im MCIM-Modus: EventFilter auf äußeres Fenster installieren damit
    // dessen MoveEvent die Preview-Position in diesem Widget aktualisiert.
    if (m_fixedCameraIdx >= 0 && window() != this) {
        window()->installEventFilter(this);
    }

    // Fensterposition und Custom Resolutions: einmalig beim ersten Anzeigen (pro Instanz)
    if (!m_showEventDone) {
        // Im MCIM-Modus ist MainWindow ein eingebettetes Widget – Geometrie wird vom
        // äußeren Fenster (main.cpp) verwaltet, nicht hier wiederherstellen.
        if (m_fixedCameraIdx < 0) {
            QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
            settings.beginGroup(m_tabGroup);
            if (settings.contains("Window/Geometry")) {
                QByteArray geometryData = settings.value("Window/Geometry").toByteArray();
                restoreGeometry(geometryData);
                qDebug() << "Restored window geometry from settings";
            }
            // Fallback for Wayland: explicitly restore size if geometry didn't take effect
            if (isNativeWaylandSession() && settings.contains("Window/Size")) {
                QSize savedSize = settings.value("Window/Size").toSize();
                if (savedSize.isValid() && savedSize.width() > 100 && savedSize.height() > 100) {
                    resize(savedSize);
                    qDebug() << "Restored window size on Wayland:" << savedSize;
                }
            }
            settings.endGroup();
        }
        loadCustomResolutions();
        m_showEventDone = true;

        // Set initial tab indicator (inactive placeholder) so all tab labels have equal width from the start
        updateTabIndicator();

        // X-Reset as final one-time initialization step (deferred so geometry is final).
        // Must stay inside !m_showEventDone so it does NOT fire on every tab switch.
        QTimer::singleShot(100, this, [this]() {
            QString defaultBoxValue = getDefaultBoxInput();
            BoxInput->setText(defaultBoxValue);
            updateOverlayResetButtonColor(overlayResetButton);

            // Perform initial codec compatibility check (without triggering reset button updates)
            if (codecSelector && signalRecordingCheckbox) {
                QString codec = codecSelector->currentText();
                QLabel *warningLabel = findChild<QLabel*>("codecWarningLabel");
                bool isMjpeg = (codec == "mjpeg");

                signalRecordingCheckbox->setEnabled(isMjpeg);
                if (segmentDurationInput) segmentDurationInput->setEnabled(isMjpeg);
                if (circularBufferInput)  circularBufferInput->setEnabled(isMjpeg);

                if (!isMjpeg) {
                    signalRecordingCheckbox->setToolTip(tr("Signal recording requires MJPEG codec"));
                    if (segmentDurationInput) segmentDurationInput->setToolTip(tr("Segment duration requires MJPEG codec"));
                    if (circularBufferInput)  circularBufferInput->setToolTip(tr("Circular buffer requires MJPEG codec"));
                }
            }

            // Apply user-defined startup defaults (if any) before unlocking signal handlers
            loadStartupDefaults();

            // Update shutter max range after defaults (framerate may have changed)
            updateShutterMaxRange();

            // Initialization complete — unlock global reset button updates
            isInitializing = false;
            if (globalResetButton) {
                globalResetButton->setStyleSheet("color: black;");
            }

            // Recalculate window size after defaults have been applied,
            // as widget visibility may have changed (e.g. codec switch hides/shows profile, level, etc.)
            adjustWindowToOptimalSize();

            qDebug() << "Post-startup X-Reset performed, BoxInput:" << BoxInput->text();
            qDebug() << "Initialization complete - global reset button set to black";
        });
    }
}

// Route log output to shared widget if set, otherwise to local outputLog
void MainWindow::appendLog(const QString &html)
{
    // Skip append when the shared log window is not visible to avoid
    // wasting CPU on hidden-widget layout updates during capture
    if (m_sharedLogWidget && !m_sharedLogWidget->isVisible()) return;

    QTextEdit *target = m_sharedLogWidget ? m_sharedLogWidget : outputLog;
    if (target) target->append(html);
}

// ---------------------------------------------------------------------------
// MCIM: fixCameraIndex – festen Kamera-Index setzen, keine Kamera-Auswahl.
// Muss nach dem Konstruktor, VOR show() aufgerufen werden.
// ---------------------------------------------------------------------------
void MainWindow::fixCameraIndex(int idx)
{
    m_fixedCameraIdx = idx;

    // Separate Konfig-Datei damit beide Instanzen unabhängige Settings haben
    if (idx != 0) {
        m_tabGroup = AppPaths::tabGroup(idx);
    }

    // cameraSelector auf genau diesen Index festnageln und ausblenden
    if (cameraSelector) {
        cameraSelector->blockSignals(true);
        cameraSelector->clear();
        cameraSelector->addItem(QString::number(idx), idx);
        cameraSelector->setCurrentIndex(0);
        cameraSelector->setVisible(false);
        cameraSelector->blockSignals(false);
    }

    // Hide the "Cam:" label next to the selector as well
    // (the label has no pointer of its own - traverse via parentWidget)
    if (cameraSelector && cameraSelector->parent()) {
        const auto siblings = cameraSelector->parent()->children();
        for (QObject *obj : siblings) {
            auto *lbl = qobject_cast<QLabel *>(obj);
            if (lbl && (lbl->text().contains("Cam") || lbl->text().contains("cam"))) {
                lbl->setVisible(false);
            }
        }
    }

    setWindowTitle(QString("%1 – Camera %2")
                       .arg(QLatin1String(AppMeta::NAME)).arg(idx));
}

QString MainWindow::calculateBoxInput(int additionalOffsetY) {
    // Im MCIM-Modus ist MainWindow ein eingebettetes Widget – this->geometry()
    // liefert dann relative Koordinaten. window() gibt immer das Top-Level-Fenster.
    QWidget *outerWin = this->window();
    int mainWindowWidth  = outerWin->geometry().width();
    int mainWindowHeight = outerWin->geometry().height();
    int mainWindowX      = outerWin->frameGeometry().x();
    int mainWindowY      = outerWin->frameGeometry().y();
    int frameOffsetY     = outerWin->frameGeometry().y() - outerWin->geometry().y();

    // Bildschirmabmessungen ermitteln - WICHTIG: Explizit den Screen basierend auf Position ermitteln
    // this->screen() kann beim moveEvent noch den alten Screen zurückgeben!
    QScreen *screen = QGuiApplication::screenAt(outerWin->pos());
    if (!screen) {
        screen = outerWin->screen(); // Fallback
    }
    QRect screenRect = screen->geometry();
    int screenWidth = screenRect.width();
    int screenHeight = screenRect.height();
    int screenX = screenRect.x(); // Screen-Offset für Multi-Monitor

    // Preview-Fenster-Größe berechnen
    int boxWidth = mainWindowWidth / 2;

    // Verwende das Seitenverhältnis der aktuellen Video-Auflösung
    double aspectRatio = getCurrentVideoAspectRatio();
    int boxHeight = static_cast<int>(boxWidth / aspectRatio);

    // x2-Modus berücksichtigen
    if (doubleSizeCheckbox && doubleSizeCheckbox->isChecked()) {
        boxWidth *= 2;
        boxHeight *= 2;
    }

    // Gerade Zahlen sicherstellen
    if (boxWidth % 2 != 0) boxWidth -= 1;
    if (boxHeight % 2 != 0) boxHeight -= 1;

    // Modus bestimmen: Portrait wenn Bildschirm höher als breit
    bool isPortraitMode = screenHeight > screenWidth;

    qDebug() << "[PREVIEW-POS] calculateBoxInput called"
             << " | screen:" << screenWidth << "x" << screenHeight << "@(" << screenX << "," << screenRect.y() << ")"
             << " | mainWin:" << mainWindowWidth << "x" << mainWindowHeight << "@(" << mainWindowX << "," << mainWindowY << ")"
             << " | box:" << boxWidth << "x" << boxHeight
             << " | portraitMode:" << isPortraitMode
             << " | hasSibling:" << (m_sibling != nullptr)
             << " | siblingRunning:" << (m_sibling ? m_sibling->isPreviewRunning() : false);

    int boxX, boxY;
    const int gap = 10; // Abstand zwischen Fenstern

    if (isPortraitMode) {
        // Portrait-Modus: Preview oberhalb des Hauptfensters
        boxX = mainWindowX; // Gleiche X-Position wie Hauptfenster (inkl. Rahmen)
        boxY = mainWindowY - boxHeight - gap + frameOffsetY; // Oberhalb mit Abstand, aber eine Titelleistenhöhe tiefer

        qDebug() << "[PREVIEW-POS] Portrait solo: trying above mainWin, boxY=" << boxY;

        // Prüfen ob genug Platz oben vorhanden (Multi-Monitor: screenRect.y() statt 0)
        if (boxY < screenRect.y()) {
            // Falls nicht genug Platz oben, unter dem Hauptfenster platzieren
            boxY = mainWindowY + mainWindowHeight + gap - frameOffsetY;
            qDebug() << "[PREVIEW-POS] Portrait solo: not enough space above, moved below mainWin, boxY=" << boxY;
        }
    } else {
        // Landscape-Modus: Preview links oder rechts vom Hauptfenster
        // Entscheiden basierend auf verfügbarem Platz
        // WICHTIG: spaceLeft/Right relativ zum aktuellen Screen berechnen
        int spaceLeft = mainWindowX - screenX;
        int spaceRight = (screenX + screenWidth) - (mainWindowX + mainWindowWidth);

        if (spaceLeft >= boxWidth + gap) {
            // Enough space on left: position preview to the left
            boxX = mainWindowX - boxWidth - gap;
        } else if (spaceRight >= boxWidth + gap) {
            // Enough space on right: position preview to the right
            boxX = mainWindowX + mainWindowWidth + gap;
        } else {
            // No ideal space on either side: fall back to left (with overlap)
            boxX = mainWindowX - boxWidth - gap;
        }

        // Y-Position: Vertikal am Hauptfenster ausrichten (unterhalb der Menüleiste)
        boxY = mainWindowY + additionalOffsetY + frameOffsetY;

        qDebug() << "[PREVIEW-POS] Landscape solo: box@(" << boxX << "," << boxY << ") | spaceLeft=" << spaceLeft << " spaceRight=" << spaceRight;
    }

    // Sibling-aware: place this preview on the opposite side of where the sibling is.
    if (m_sibling && m_sibling->isPreviewActive()) {
        QString siblingCoords = m_sibling->getBoxInputText();
        QStringList parts = siblingCoords.split(',');
        if (parts.size() == 4) {
            int sibX = parts[0].toInt();
            int sibY = parts[1].toInt();
            int sibW = parts[2].toInt();
            int sibH = parts[3].toInt();

            qDebug() << "[PREVIEW-POS] Sibling running at" << sibW << "x" << sibH << "@(" << sibX << "," << sibY << ")";

            if (isPortraitMode) {
                // Portrait mode: try side-by-side first (original behavior).
                // Only if it doesn't fit, stack vertically: above mainWin, or below sibling.
                int screenBtm = screenRect.y() + screenHeight;
                int screenMidX = screenX + screenWidth / 2;
                int tryRightX = sibX + sibW;
                int tryLeftX  = sibX - boxWidth;
                bool fitsRight = (tryRightX + boxWidth <= screenX + screenWidth);
                bool fitsLeft  = (tryLeftX >= screenX);

                if (fitsRight && sibX < screenMidX) {
                    boxX = tryRightX;
                    boxY = sibY;
                } else if (fitsLeft && sibX >= screenMidX) {
                    boxX = tryLeftX;
                    boxY = sibY;
                } else if (fitsLeft) {
                    boxX = tryLeftX;
                    boxY = sibY;
                } else if (fitsRight) {
                    boxX = tryRightX;
                    boxY = sibY;
                } else {
                    // No horizontal space: stack vertically.
                    // Try above main window first, then below sibling.
                    int tryYAbove = mainWindowY - boxHeight - gap + frameOffsetY;
                    int tryYBelowSib = sibY + sibH + gap;
                    if (tryYAbove >= screenRect.y()) {
                        boxX = mainWindowX;
                        boxY = tryYAbove;
                        qDebug() << "[PREVIEW-POS] Portrait sibling: no horizontal space, placed above mainWin, boxY=" << boxY;
                    } else if (tryYBelowSib + boxHeight <= screenBtm) {
                        boxX = sibX;
                        boxY = tryYBelowSib;
                        qDebug() << "[PREVIEW-POS] Portrait sibling: no horizontal space, placed below sibling, boxY=" << boxY;
                    } else {
                        // Neither fits perfectly. Clamp below sibling to screen bottom.
                        boxX = sibX;
                        boxY = qMin(tryYBelowSib, screenBtm - boxHeight);
                        qDebug() << "[PREVIEW-POS] Portrait sibling: no horizontal space, clamped below sibling, boxY=" << boxY;
                    }
                }

                qDebug() << "[PREVIEW-POS] Portrait sibling override: box@(" << boxX << "," << boxY << ")"
                         << " | sib@(" << sibX << "," << sibY << ") sibW=" << sibW << " sibH=" << sibH
                         << " | fitsRight=" << fitsRight << " fitsLeft=" << fitsLeft
                         << " | screenWidth=" << screenWidth << " boxX+boxW=" << (boxX + boxWidth);

                // Warn if box would be clipped by screen bounds
                if (boxX < screenRect.x()) {
                    qDebug() << "[PREVIEW-POS] *** WARNING: box x=" << boxX << " is LEFT of screen x=" << screenRect.x()
                             << " (off by" << (screenRect.x() - boxX) << "px)";
                }
                if (boxX + boxWidth > screenRect.x() + screenWidth) {
                    qDebug() << "[PREVIEW-POS] *** WARNING: box right=" << (boxX + boxWidth) << " exceeds screen right=" << (screenRect.x() + screenWidth)
                             << " (off by" << ((boxX + boxWidth) - (screenRect.x() + screenWidth)) << "px)";
                }
                if (boxY + boxHeight > screenBtm) {
                    qDebug() << "[PREVIEW-POS] *** WARNING: box bottom=" << (boxY + boxHeight) << " exceeds screen bottom=" << screenBtm
                             << " (off by" << ((boxY + boxHeight) - screenBtm) << "px)";
                }
            } else {
                // Landscape: two slots — top (soloY) and bottom (soloY + boxHeight + 40).
                // soloY is identical for both cameras (same outerWin).
                // Decide based on where the sibling actually is, not on screenMidY.
                // If sibling is close to soloY → sibling has top slot → we take bottom.
                // Otherwise sibling has bottom slot → we take top.
                int soloY = mainWindowY + additionalOffsetY + frameOffsetY;
                int screenBottom = screenRect.y() + screenHeight;
                if (qAbs(sibY - soloY) < boxHeight) {
                    // Sibling is at top slot → go below
                    boxY = sibY + sibH + 40;
                } else {
                    // Sibling is at bottom slot → go above (top slot)
                    boxY = soloY;
                }
                int boxYBeforeClamp = boxY;
                // Clamp to screen
                boxY = qMax(boxY, screenRect.y());
                boxY = qMin(boxY, screenBottom - boxHeight);

                qDebug() << "[PREVIEW-POS] Landscape sibling override: boxY before clamp=" << boxYBeforeClamp
                         << " after clamp=" << boxY << " | soloY=" << soloY
                         << " | sibY=" << sibY << " | screenBottom=" << screenBottom;
            }
        }
    }

    QString result = QString("%1,%2,%3,%4")
               .arg(boxX)
               .arg(boxY)
               .arg(boxWidth)
               .arg(boxHeight);

    qDebug() << "[PREVIEW-POS] Final result:" << result;

    return result;
}

QString MainWindow::getDefaultBoxInput() {
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    bool useCustom = settings.value("Preview/UseCustomGeometry", false).toBool();

    if (useCustom) {
        QString custom = settings.value("Preview/CustomBoxInput", "").toString();
        if (!custom.isEmpty()) {
            qDebug() << "Using custom preview geometry:" << custom;
            settings.endGroup();
            return custom;
        }
    }

    // Fallback: Calculate as before
    qDebug() << "Using calculated preview geometry";
    settings.endGroup();
    return calculateBoxInput(+30);
}

void MainWindow::openGlobalSetupDialog() {
    GuiSetupDialog setupDialog(QString(), 0, this, true);
    setupDialog.loadGuiSettings();

    connect(&setupDialog, &GuiSetupDialog::settingsSaved, this, [this]() {
        updateControlSocketVisibility();
        if (m_sibling) m_sibling->updateControlSocketVisibility();
    });

    if (setupDialog.exec() == QDialog::Accepted) {
        updateControlSocketVisibility();
        if (m_sibling) m_sibling->updateControlSocketVisibility();
    }
}

void MainWindow::openGuiSetupDialog() {
    GuiSetupDialog setupDialog(m_tabGroup, m_fixedCameraIdx < 0 ? 0 : m_fixedCameraIdx, this, false);
    // Pass the currently active lib paths (may be temporary values from a
    // loaded config file or an active profile) so the dialog can display
    // the effective value with a perm/temp marker.
    setupDialog.setEffectiveLibPaths(m_previewLibsPath, m_postProcessLibsPath, m_encoderLibsPath);
    setupDialog.loadGuiSettings();

    // Connect settingsSaved signal to update methods
    connect(&setupDialog, &GuiSetupDialog::settingsSaved, this, &MainWindow::updateStepSizeRadioButtons);
    connect(&setupDialog, &GuiSetupDialog::settingsSaved, this, &MainWindow::updateControlSocketVisibility);
    connect(&setupDialog, &GuiSetupDialog::settingsSaved, this, [this]() {
        // Reload the preview libs path edited in the setup dialog
        // (it is no longer edited in the main window row).
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        m_previewLibsPath = settings.value("Defaults/PreviewLibs", "").toString();
        m_postProcessLibsPath = settings.value("Defaults/PostProcessLibs", "").toString();
        m_encoderLibsPath = settings.value("Defaults/EncoderLibs", "").toString();
        settings.endGroup();
    });
    connect(&setupDialog, &GuiSetupDialog::settingsSaved, this, [this]() {
        // Reload V4L2 settings after setup dialog closes
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        bool focusEnabled = settings.value("V4L2/FocusEnabled", false).toBool();
        bool zoomEnabled  = settings.value("V4L2/ZoomEnabled", false).toBool();
        QString focusDevice = settings.value("V4L2/FocusDevice", "").toString();
        QString zoomDevice  = settings.value("V4L2/ZoomDevice", "").toString();
        settings.endGroup();

        if (m_v4l2Controller) {
            m_v4l2Controller->closeDevice();
            if (focusEnabled || zoomEnabled) {
                QString v4l2Device = !focusDevice.isEmpty() ? focusDevice : zoomDevice;
                if (!v4l2Device.isEmpty())
                    v4l2DeviceInput->setText(v4l2Device);
                QString device = v4l2DeviceInput->text().isEmpty()
                    ? QString("/dev/v4l-subdev%1").arg(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0)
                    : v4l2DeviceInput->text();
                m_v4l2Controller->openDevice(device);
            }
        }
    });

    if (setupDialog.exec() == QDialog::Accepted) {
        customAppEntries = setupDialog.getCustomAppEntries(); // Benutzerdefinierte Apps abrufen
        updateAppSelector(); // Dropdown aktualisieren
        loadGuiConfiguration(); // GUI-Konfiguration laden
        tabVisibilityService->updateExpertTabVisibility(); // Expert Tab ein-/ausblenden
        tabVisibilityService->updateFocusTabVisibility(); // Focus Tab ein-/ausblenden
        tabVisibilityService->updateZoomTabVisibility(); // Zoom Tab ein-/ausblenden
        tabVisibilityService->updateAudioTabVisibility(); // Audio Tab ein-/ausblenden
        tabVisibilityService->updateGstreamerTabVisibility(); // Gstreamer Tab ein-/ausblenden
        tabVisibilityService->updateGstTabVisibility(); // GST Tab ein-/ausblenden
        tabVisibilityService->updateInferenceTabVisibility(); // Inference Tab ein-/ausblenden
        tabVisibilityService->updateActionsTabVisibility(); // Actions Tab ein-/ausblenden
        tabVisibilityService->updateDebugTabVisibility(); // Debug Tab ein-/ausblenden

        // Custom Resolutions neu laden
        QString currentResolution = resolutionSelector->currentText(); // Aktuelle Auswahl merken
        loadCustomResolutions();

        // Versuche, die vorherige Auswahl wiederherzustellen
        int index = resolutionSelector->findText(currentResolution);
        if (index >= 0) {
            resolutionSelector->setCurrentIndex(index);
        }
    }
}

void MainWindow::updateTimelapseVisibility() {
    QString app = appSelector->currentText();
    bool show = (app == "rpicam-still" || app == "rpicam-jpeg");
    if (timelapseRowWidget) {
        timelapseRowWidget->setVisible(show);
    }
}

void MainWindow::loadGuiConfiguration() {
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);

    guiOutputFilePath = QDir::cleanPath(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiOutputPath", AppPaths::output()).toString(), AppPaths::output()));
    guiPostProcessFilePath = QDir::cleanPath(settings.value("Paths/GuiPostProcessPath", "/home/admin/rpicam-apps/assets").toString());
    guiTuningFilePath = QDir::cleanPath(settings.value("Paths/GuiTuningFilePath", AppPaths::tuningFileBasePath()).toString());
    guiMetadataPath = QDir::cleanPath(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiMetadataPath", AppPaths::output()).toString(), AppPaths::output()));
    rpicamConfigPath = QDir::cleanPath(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiRpicamConfigPath", AppPaths::config()).toString(), AppPaths::config()));

    qDebug() << "Loaded rpicamConfigPath:" << rpicamConfigPath;

    // Debug-Einstellungen werden später geladen, wenn die Widgets existieren

    updatePostProcessFileDropdown(); // <-- hier Dropdown füllen
    updateTuningFileDropdown(); // <-- hier Dropdown füllen
    // loadCustomResolutions() wird später aufgerufen, nachdem resolutionSelector initialisiert wurde
    settings.endGroup();
}

void MainWindow::updateStepSizeRadioButtons() {
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);

    // Lade die neuen Werte aus den Settings (6 Werte)
    int focusStep1 = settings.value("Focus/StepSize1", 100).toInt();
    int focusStep2 = settings.value("Focus/StepSize2", 300).toInt();
    int focusStep3 = settings.value("Focus/StepSize3", 1000).toInt();
    int focusStep4 = settings.value("Focus/StepSize4", 3000).toInt();
    int focusStep5 = settings.value("Focus/StepSize5", 10000).toInt();
    int focusStep6 = settings.value("Focus/StepSize6", 32767).toInt();

    int zoomStep1 = settings.value("Zoom/StepSize1", 100).toInt();
    int zoomStep2 = settings.value("Zoom/StepSize2", 300).toInt();
    int zoomStep3 = settings.value("Zoom/StepSize3", 1000).toInt();
    int zoomStep4 = settings.value("Zoom/StepSize4", 3000).toInt();
    int zoomStep5 = settings.value("Zoom/StepSize5", 10000).toInt();
    int zoomStep6 = settings.value("Zoom/StepSize6", 32767).toInt();

    // Aktualisiere Focus Radio Button Tooltips
    if (focusStepButtonGroup) {
        QList<QAbstractButton*> focusButtons = focusStepButtonGroup->buttons();
        if (focusButtons.size() >= 6) {
            focusButtons[0]->setToolTip(tr("Step Size: %1").arg(focusStep1));
            focusButtons[1]->setToolTip(tr("Step Size: %1").arg(focusStep2));
            focusButtons[2]->setToolTip(tr("Step Size: %1").arg(focusStep3));
            focusButtons[3]->setToolTip(tr("Step Size: %1").arg(focusStep4));
            focusButtons[4]->setToolTip(tr("Step Size: %1").arg(focusStep5));
            focusButtons[5]->setToolTip(tr("Step Size: %1").arg(focusStep6));

            // Update currentFocusStepSize based on currently selected button
            int checkedId = focusStepButtonGroup->checkedId();
            if (checkedId == 0) { currentFocusStepSize = focusStep1; }
            else if (checkedId == 1) { currentFocusStepSize = focusStep2; }
            else if (checkedId == 2) { currentFocusStepSize = focusStep3; }
            else if (checkedId == 3) { currentFocusStepSize = focusStep4; }
            else if (checkedId == 4) { currentFocusStepSize = focusStep5; }
            else if (checkedId == 5) { currentFocusStepSize = focusStep6; }
        }
    }
    // Update Near/Far button tooltips with current step size
    if (focusFarButton) focusFarButton->setToolTip(tr("Move focus far (step: %1)").arg(currentFocusStepSize));
    if (focusNearButton) focusNearButton->setToolTip(tr("Move focus near (step: %1)").arg(currentFocusStepSize));

    // Aktualisiere Zoom Radio Button Tooltips
    if (zoomStepButtonGroup) {
        QList<QAbstractButton*> zoomButtons = zoomStepButtonGroup->buttons();
        if (zoomButtons.size() >= 6) {
            zoomButtons[0]->setToolTip(tr("Step Size: %1").arg(zoomStep1));
            zoomButtons[1]->setToolTip(tr("Step Size: %1").arg(zoomStep2));
            zoomButtons[2]->setToolTip(tr("Step Size: %1").arg(zoomStep3));
            zoomButtons[3]->setToolTip(tr("Step Size: %1").arg(zoomStep4));
            zoomButtons[4]->setToolTip(tr("Step Size: %1").arg(zoomStep5));
            zoomButtons[5]->setToolTip(tr("Step Size: %1").arg(zoomStep6));

            // Update currentZoomStepSize based on currently selected button
            int checkedId = zoomStepButtonGroup->checkedId();
            if (checkedId == 0) { currentZoomStepSize = zoomStep1; }
            else if (checkedId == 1) { currentZoomStepSize = zoomStep2; }
            else if (checkedId == 2) { currentZoomStepSize = zoomStep3; }
            else if (checkedId == 3) { currentZoomStepSize = zoomStep4; }
            else if (checkedId == 4) { currentZoomStepSize = zoomStep5; }
            else if (checkedId == 5) { currentZoomStepSize = zoomStep6; }
        }
    }
    // Update Near/Far button tooltips with current step size
    if (zoomFarButton) zoomFarButton->setToolTip(tr("Move zoom far (step: %1)").arg(currentZoomStepSize));
    if (zoomNearButton) zoomNearButton->setToolTip(tr("Move zoom near (step: %1)").arg(currentZoomStepSize));

    qDebug() << "Updated step size radio buttons - Focus:" << currentFocusStepSize << "Zoom:" << currentZoomStepSize;
    settings.endGroup();
}

void MainWindow::updateControlSocketVisibility()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    // Read from [General] first, fall back to [CameraN-Tab] for migration
    settings.beginGroup("General");
    bool configured = settings.value("Control/Enabled", QVariant()).toBool();
    settings.endGroup();
    if (configured == false && settings.value("General/Control/Enabled", QVariant()).isNull()) {
        settings.beginGroup(m_tabGroup);
        configured = settings.value("Control/Enabled", true).toBool();
        settings.endGroup();
    }

    // Only show the RT indicator if both: user enabled it AND rpicam-apps supports it
    bool visible = configured && m_hasRpicamRt;

    if (m_controlSocketIndicator) {
        m_controlSocketIndicator->setVisible(visible);
    }
    if (!visible && m_controlSocket && m_controlSocket->isConnected()) {
        m_controlSocket->disconnectFromServer();
    }
}

void MainWindow::loadCustomResolutions() {
    qDebug() << "[CustomRes] Loading custom resolutions...";
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    QRegularExpression resFormat("^\\d+x\\d+$");

    // Verwende die von der Kamera erkannten Resolutions statt hardcodiert
    qDebug() << "[CustomRes] Camera resolutions:" << cameraResolutions;

    // Sammle alle Custom Resolutions aus der Config (1-3 sind die aktiven)
    QStringList customResolutions;
    for (int i = 1; i <= 3; ++i) {
        QString key = QString("CustomResolution%1").arg(i);
        QString resolution = settings.value(key, "").toString().trimmed();

        if (!resolution.isEmpty() && resFormat.match(resolution).hasMatch()) {
            customResolutions.append(resolution);
            qDebug() << "[CustomRes] Found in config:" << resolution;
        }
    }

    // Entferne veraltete Config-Einträge (CustomResolution4, CustomResolution5, etc.)
    QStringList allKeys = settings.allKeys();
    for (const QString &key : allKeys) {
        if (key.startsWith("CustomResolution")) {
            bool ok;
            int num = key.mid(16).toInt(&ok); // "CustomResolution" hat 16 Zeichen
            if (ok && num > 3) {
                qDebug() << "[CustomRes] Removing obsolete config key:" << key;
                settings.remove(key);
            }
        }
    }

    qDebug() << "[CustomRes] Custom resolutions from config:" << customResolutions;
    qDebug() << "[CustomRes] Current dropdown count:" << resolutionSelector->count();

    // Schritt 1: Entferne ALLE Einträge, die NICHT von der Kamera kommen
    for (int i = resolutionSelector->count() - 1; i >= 0; --i) {
        QString item = resolutionSelector->itemText(i);
        if (!cameraResolutions.contains(item)) {
            qDebug() << "[CustomRes] Removing non-camera resolution at index" << i << ":" << item;
            resolutionSelector->removeItem(i);
        }
    }

    qDebug() << "[CustomRes] After cleanup, dropdown count:" << resolutionSelector->count();

    // Schritt 2: Füge alle Custom Resolutions aus der Config hinzu (ohne Duplikate)
    for (const QString &resolution : customResolutions) {
        if (resolutionSelector->findText(resolution) == -1) {
            resolutionSelector->addItem(resolution);
            qDebug() << "[CustomRes] Added custom resolution:" << resolution;
        } else {
            qDebug() << "[CustomRes] Skipped duplicate resolution:" << resolution;
        }
    }

    qDebug() << "[CustomRes] Final dropdown count:" << resolutionSelector->count();
    settings.endGroup();
}

void MainWindow::parseConfigurationFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.exists()) {
        appendLog(tr("Configuration file does not exist: ") + filePath);
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendLog(tr("Failed to open configuration file: ") + filePath);
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("#") || line.isEmpty()) {
            continue;
        }
        QStringList parts = line.split("=", Qt::KeepEmptyParts);
        if (parts.size() != 2) {
            continue;
        }
        QString key = parts[0].trimmed();
        QString value = parts[1].trimmed();
        if (key == "ConfigFilePath") {
            configFilePath = value;
        } else if (key == "GuiOutputPath") {
            guiOutputFilePath = QDir::cleanPath(value + "/");
        } else if (key == "GuiPostProcessPath") {
            guiPostProcessFilePath = QDir::cleanPath(value + "/");
        } else if (key == "GuiTuningFilePath") {
            guiTuningFilePath = QDir::cleanPath(value + "/");
        } else if (key == "GuiRpicamConfigPath") {
            rpicamConfigPath = QDir::cleanPath(value + "/");
        } else {
        }
    }
    file.close();
    appendLog(tr("Configuration successfully loaded from ") + filePath);
}
void MainWindow::updateSelectionOverlayGeometry() {
    // SOP-style: SelectionOverlay is not fullscreen anymore!
    // It's dynamically positioned by SetRectangle() during mouse events
    // This method is kept for compatibility but does nothing now
}

// SOP-style: Get mouse coordinates directly from X11 (not Qt events!)
// Falls back to QCursor::pos() on Wayland where X11 is not available.
QPoint GetMousePhysicalCoordinates() {
    // Check whether we have a valid X11 connection
    static bool s_x11Available = []() {
        // XDG_SESSION_TYPE is the most reliable indicator
        QString sessionType = qgetenv("XDG_SESSION_TYPE");
        if (sessionType == "wayland") return false;
        // Also check if DISPLAY is set (X11 or XWayland)
        if (qgetenv("DISPLAY").isEmpty()) return false;
        return true;
    }();

    if (s_x11Available) {
        Display *display = QX11Info::display();
        Window app_root = QX11Info::appRootWindow();
        if (display && app_root) {
            Window root, child;
            int root_x, root_y;
            int win_x, win_y;
            unsigned int mask_return;
            if (XQueryPointer(display, app_root, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask_return)) {
                return QPoint(root_x, root_y);
            }
        }
    }

    // Fallback: use Qt's cursor position (works on all platforms)
    return QCursor::pos();
}

// Helper: detect if we are running under a native Wayland compositor (not XWayland)
bool isNativeWaylandSession() {
    if (qgetenv("XDG_SESSION_TYPE") == "wayland") return true;
    if (!qgetenv("WAYLAND_DISPLAY").isEmpty()) return true;
    return false;
}

// Helper: detect remote sessions (XRDP) where --qt-preview / --preview-backend qt
// is REQUIRED because no direct GPU access exists.
// Uses loginctl (D-Bus) to determine if session is remote – same method as
// the well-tested rpi_xrdp_check.sh diagnostics script.
bool isRemoteSession() {
    // Fast path: XRDP_SESSION env var
    if (!qgetenv("XRDP_SESSION").isEmpty()) return true;

    // Reliable path: ask loginctl whether this session is remote
    QString sessionId = qgetenv("XDG_SESSION_ID");
    if (!sessionId.isEmpty()) {
        QProcess proc;
        proc.start("loginctl", {"show-session", sessionId, "-p", "Remote"});
        if (proc.waitForFinished(2000) && proc.exitCode() == 0) {
            QString output = proc.readAllStandardOutput();
            if (output.contains("Remote=yes")) return true;
        }
    }

    return false;
}

void MainWindow::resetAllToDefaults() {
    // Preview/Overlay Reset
    QString defaultBoxValue = getDefaultBoxInput();
    BoxInput->setText(defaultBoxValue);
    updateOverlayResetButtonColor(overlayResetButton);

    // Output File Reset
    outputFileName->clear();
    autoNamingCheckbox->setChecked(false);
    timestampCheckbox->setChecked(false);
    // Zurück zu File-Mode
    if (outputModeFile) {
        outputModeFile->setChecked(true);
    }
    if (resetOutputFileButton) updateResetButtonColor(resetOutputFileButton, 0, 0);

    // Timeout Reset
    timeoutSelector->setCurrentText("0");
    if (timeoutResetButton) updateResetButtonColor(timeoutResetButton, 0, 0);

    // Timelapse Reset
    timelapseInput->setCurrentIndex(0); // Leerer Eintrag
    if (timelapseResetButton) updateResetButtonColor(timelapseResetButton, 0, 0);

    // Shutter Reset
    shutterSlider->setValue(0);
    updateShutterDisplay(0);

    // Sync Reset
    if (syncSelector) syncSelector->setCurrentText("off");
    if (syncResetButton) syncResetButton->setStyleSheet("color: black;");

    // HDR Reset
    hdrSelector->setCurrentText("off");
    if (hdrResetButton) updateResetButtonColor(hdrResetButton, 0, 0);

    // Denoise Reset
    denoiseSelector->setCurrentText("auto");
    if (denoiseResetButton) updateResetButtonColor(denoiseResetButton, 0, 0);

    // Flicker Period Reset
    if (flickerPeriodSelector) flickerPeriodSelector->setCurrentIndex(0); // "Off"
    if (flickerPeriodResetButton) updateResetButtonColor(flickerPeriodResetButton, 0, 0);

    // Metadata Reset
    if (metadataFileEdit) metadataFileEdit->clear();
    if (metadataFormatSelector) metadataFormatSelector->setCurrentText("json");
    if (metadataAutoNamingCheckbox) metadataAutoNamingCheckbox->setChecked(false);
    if (metadataFileEdit) metadataFileEdit->setEnabled(true);
    if (metadataFileButton) metadataFileButton->setEnabled(true);
    if (metadataResetButton) updateResetButtonColor(metadataResetButton, 0, 0);

    // Post-Process File Reset
    postProcessFileSelector->setCurrentIndex(-1);
    postProcessFileSelector->setCurrentText("");
    if (resetPostProcessFileButton) updateResetButtonColor(resetPostProcessFileButton, 0, 0);

    // Tuning File Reset
    tuningFileSelector->setCurrentIndex(-1);
    tuningFileSelector->setCurrentText("");
    if (resetTuningFileButton) updateResetButtonColor(resetTuningFileButton, 0, 0);

    // Codec Reset
    codecSelector->setCurrentText("h264");
    // codecResetButton color is updated in Video Tab lambda

    // Encoding Reset (für Still-Apps)
    if (encodingSelector) encodingSelector->setCurrentIndex(0); // JPEG
    if (encodingResetButton) encodingResetButton->setStyleSheet("color: black;");

    // Still Tab Parameters Reset
    if (autofocusOnCaptureCheckbox) autofocusOnCaptureCheckbox->setChecked(false);
    if (zslCheckbox) zslCheckbox->setChecked(false);
    if (immediateCheckbox) immediateCheckbox->setChecked(false);
    if (framestartSpinBox) framestartSpinBox->setValue(0);
    if (captureControlResetButton) captureControlResetButton->setStyleSheet("color: black;");
    if (thumbLineEdit) thumbLineEdit->clear();
    if (thumbResetButton) thumbResetButton->setStyleSheet("color: black;");
    if (restartSpinBox) restartSpinBox->setValue(0);
    if (restartResetButton) restartResetButton->setStyleSheet("color: black;");
    if (exifLineEdit) exifLineEdit->clear();
    if (exifResetButton) exifResetButton->setStyleSheet("color: black;");
    if (latestLineEdit) latestLineEdit->clear();
    if (rawCheckbox) rawCheckbox->setChecked(false);
    if (fileManagementResetButton) fileManagementResetButton->setStyleSheet("color: black;");

    // Video Parameters Reset
    if (profileSelector) profileSelector->setCurrentIndex(-1);
    if (levelSelector) levelSelector->setCurrentIndex(-1);
    if (inlineHeadersCheckbox) inlineHeadersCheckbox->setChecked(false);
    if (bitrateSpinBox) bitrateSpinBox->setValue(0);
    if (qualitySpinBox) qualitySpinBox->setValue(0);
    if (intraSpinBox) intraSpinBox->setValue(0);
    if (framesSpinBox) framesSpinBox->setValue(0);
    if (flushCheckbox) flushCheckbox->setChecked(false);
    if (savePtsInput) savePtsInput->clear();

    // Recording Options Reset
    if (signalRecordingCheckbox) signalRecordingCheckbox->setChecked(false);
    if (keypressRecordingCheckbox) keypressRecordingCheckbox->setChecked(false);
    if (initialStateComboBox) initialStateComboBox->setCurrentText("pause");
    if (splitFilesCheckbox) splitFilesCheckbox->setChecked(false);
    if (segmentDurationInput) segmentDurationInput->clear();
    if (circularBufferInput) circularBufferInput->clear();
    if (segmentPatternCheckbox) segmentPatternCheckbox->setChecked(false);

    // AWB Reset
    awbSelector->setCurrentText("auto");
    if (resetAwbButton) updateResetButtonColor(resetAwbButton, 0, 0);

    // CCM Reset
    if (ccmInput) ccmInput->clear();
    if (ccmResetButton) ccmResetButton->setStyleSheet("color: black;");

    // Preview Libs Reset
    m_previewLibsPath.clear();

    // Post-Process Libs Reset (path configured in the camera setup dialog)
    m_postProcessLibsPath.clear();

    // Encoder Libs Reset (path configured in the camera setup dialog)
    m_encoderLibsPath.clear();

    // Sync start/stop toggle
    setSyncStartStop(false);

    // Also remove the persisted values so they stay cleared after a restart.
    // For these parameters the settings key IS the current value (there
    // is no main-window widget for them), unlike the other Defaults/* keys
    // which are separate startup defaults.
    {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        settings.remove("Defaults/PreviewLibs");
        settings.remove("Defaults/PostProcessLibs");
        settings.remove("Defaults/EncoderLibs");
        settings.endGroup();
        settings.sync();
    }

    // Metering Reset
    meteringSelector->setCurrentText("Select option:");
    meteringCustomInput->clear();
    meteringCustomInput->setVisible(false);
    if (meteringResetButton) updateResetButtonColor(meteringResetButton, 0, 0);

    // Low Resolution Reset
    loresComboBox->reset();
    if (loresResetButton) updateResetButtonColor(loresResetButton, 0, 0);

    // Slider Resets
    sharpnessSlider->setValue(10); // 1.0 * 10
    sharpnessInput->setText("1.0");

    evSlider->setValue(0); // 0.0 * 10
    evInput->setText("0.0");

    gainSlider->setValue(0); // 0.0 * 10
    gainInput->setText("0.0");

    // AWB gains: block the slider signals so no awbgains is sent to a
    // running camera. The AWB selector reset above already sent awb:auto;
    // sending awbgains afterwards would put the camera back into manual
    // AWB mode and prevent it from re-evaluating AWB (same special case
    // as the individual AWB gain reset buttons).
    awbGainRedSlider->blockSignals(true);
    awbGainRedSlider->setValue(15); // 1.5 * 10
    awbGainRedSlider->blockSignals(false);
    awbGainRedInput->setText("1.5");

    awbGainBlueSlider->blockSignals(true);
    awbGainBlueSlider->setValue(12); // 1.2 * 10
    awbGainBlueSlider->blockSignals(false);
    awbGainBlueInput->setText("1.2");

    // Final AWB state for a running camera: explicitly send awb:auto so
    // the camera re-evaluates AWB even if the selector was already "auto"
    // (then no currentTextChanged fired). Idempotent double-send is harmless.
    if (isControlSocketActive()) {
        sendSliderToSocket("awb", "auto");
    }

    brightnessSlider->setValue(0); // 0.0 * 10
    brightnessInput->setText("0.0");

    contrastSlider->setValue(10); // 1.0 * 10
    contrastInput->setText("1.0");

    saturationSlider->setValue(10); // 1.0 * 10
    saturationInput->setText("1.0");

    // Autofocus Parameter Reset
    if (autofocusModeSelector) autofocusModeSelector->setCurrentText("auto");
    if (resetAutofocusModeButton) updateResetButtonColor(resetAutofocusModeButton, 0, 0);
    if (autofocusRangeSelector) autofocusRangeSelector->setCurrentText("normal");
    if (autofocusSpeedSelector) autofocusSpeedSelector->setCurrentText("normal");
    if (autofocusWindowInput) autofocusWindowInput->setText("0.333,0.333,0.333,0.333");

    // Geometry Reset
    if (hflipCheckbox) hflipCheckbox->setChecked(false);
    if (vflipCheckbox) vflipCheckbox->setChecked(false);
    if (rotationCheckbox) rotationCheckbox->setChecked(false);

    // Info-Text Reset
    if (infoTextFrameCheckbox) infoTextFrameCheckbox->setChecked(false);
    if (infoTextFpsCheckbox) infoTextFpsCheckbox->setChecked(false);
    if (infoTextExpCheckbox) infoTextExpCheckbox->setChecked(false);
    if (infoTextAgCheckbox) infoTextAgCheckbox->setChecked(false);
    if (infoTextDgCheckbox) infoTextDgCheckbox->setChecked(false);
    if (infoTextRgCheckbox) infoTextRgCheckbox->setChecked(false);
    if (infoTextBgCheckbox) infoTextBgCheckbox->setChecked(false);
    if (infoTextFocusCheckbox) infoTextFocusCheckbox->setChecked(false);
    if (infoTextAelockCheckbox) infoTextAelockCheckbox->setChecked(false);
    if (infoTextLpCheckbox) infoTextLpCheckbox->setChecked(false);
    if (infoTextAfstateCheckbox) infoTextAfstateCheckbox->setChecked(false);

    // ROI Reset
    roiInput->setText("0.0,0.0,1.0,1.0");
    updateROIResetButtonColor();

    // x2 Checkbox Reset
    doubleSizeCheckbox->setChecked(false);

    // ========== Actions Tab Reset ==========
    if (m_actionsTab && m_actionsTab->module()) {
        m_actionsTab->module()->resetActionCheckboxes();
        m_actionsTab->module()->resetFilterSettings(30, 70);
    }
    if (filterResetButton)         filterResetButton->setStyleSheet("color: black;");
    if (actionsResetButton)        actionsResetButton->setStyleSheet("color: black;");
    if (filterSettingsResetButton) filterSettingsResetButton->setStyleSheet("color: black;");

    // ========== Expert Tab Reset ==========
    if (viewfinderModeSelector) viewfinderModeSelector->setCurrentIndex(0);
    if (viewfinderModeResetButton) viewfinderModeResetButton->setStyleSheet("color: black;");

    // Pixelformat-Filter zurücksetzen (globale Zeile)
    if (formatSelector && formatSelector->count() > 0) {
        formatSelector->setCurrentIndex(0); // löst applyFormatFilter() aus
    }

    if (viewfinderWidthSpinBox) viewfinderWidthSpinBox->setValue(0);
    if (viewfinderWidthResetButton) viewfinderWidthResetButton->setStyleSheet("color: black;");

    if (viewfinderHeightSpinBox) viewfinderHeightSpinBox->setValue(0);
    if (viewfinderHeightResetButton) viewfinderHeightResetButton->setStyleSheet("color: black;");

    if (bufferCountSpinBox) bufferCountSpinBox->setValue(0);
    if (bufferCountResetButton) bufferCountResetButton->setStyleSheet("color: black;");

    if (viewfinderBufferCountSpinBox) viewfinderBufferCountSpinBox->setValue(0);
    if (viewfinderBufferCountResetButton) viewfinderBufferCountResetButton->setStyleSheet("color: black;");

    // Globalen Reset-Button zurücksetzen
    updateGlobalResetButtonColor();
}

// =============================================================
// STILL-IMAGE ENCODING SUPPORT
// =============================================================

void MainWindow::updateUIForApp(const QString &app) {
    bool isStillApp = (app == "rpicam-still" || app == "rpicam-jpeg");

    // Show/Hide: Streaming modes vs. Encoding
    if (streamingModesWidget) {
        streamingModesWidget->setVisible(!isStillApp);
    }
    if (encodingWidget) {
        encodingWidget->setVisible(isStillApp);
    }

    // Metadata Widget nur für Still-Apps sichtbar
    if (metadataWidget) {
        metadataWidget->setVisible(isStillApp);
    }

    // Segment Pattern (%04d) nur für Video-Apps sichtbar
    if (segmentPatternCheckbox) {
        segmentPatternCheckbox->setVisible(!isStillApp);
        if (isStillApp) {
            segmentPatternCheckbox->setChecked(false); // Deaktivieren für Still-Apps
        }
    }

    // Force File Mode für Still-Apps
    if (isStillApp && outputModeFile) {
        outputModeFile->setChecked(true);
    }

    // Update Extension wenn zu Still-App gewechselt wird
    if (isStillApp) {
        updateOutputFileExtension();
    }
}

void MainWindow::updateOutputFileExtension() {
    if (!encodingSelector) {
        qDebug() << "[Extension] encodingSelector is NULL!";
        return;
    }

    QString app = appSelector->currentText();
    bool isStillApp = (app == "rpicam-still" || app == "rpicam-jpeg");
    if (!isStillApp) {
        qDebug() << "[Extension] Not a still app:" << app;
        return; // Nur für Still-Apps relevant
    }

    QString encoding = encodingSelector->currentData().toString();
    QString newExtension = getExtensionForEncoding(encoding);
    qDebug() << "[Extension] Encoding:" << encoding << "→" << newExtension;

    QString currentFile = outputFileName->text();
    qDebug() << "[Extension] Current file:" << currentFile;

    // Nur aktualisieren wenn Feld NICHT LEER ist
    // (leeres Feld = keine Ausgabedatei gewünscht)
    if (!currentFile.isEmpty()) {
        QFileInfo fileInfo(currentFile);
        QString baseName = fileInfo.completeBaseName();
        QString path = fileInfo.path();

        QString newFileName = (path.isEmpty() || path == ".")
            ? baseName + newExtension
            : QDir(path).filePath(baseName + newExtension);

        qDebug() << "[Extension] New file:" << newFileName;
        outputFileName->setText(newFileName);
    } else {
        qDebug() << "[Extension] File is empty, skipping update";
    }
}

QString MainWindow::getExtensionForEncoding(const QString &encoding) {
    if (encoding == "jpg") return ".jpg";
    if (encoding == "png") return ".png";
    if (encoding == "bmp") return ".bmp";
    if (encoding == "rgb" || encoding == "rgb24" || encoding == "rgb48") return ".rgb";
    if (encoding == "yuv420") return ".yuv";
    return ".jpg"; // Fallback
}

void MainWindow::adjustWindowToOptimalSize() {
    if (!tabWidget) return;
    if (m_suppressResize) return;  // MCIM: inaktiver Tab soll keinen Resize auslösen

    // Get current tab
    QWidget *currentTab = tabWidget->currentWidget();
    if (!currentTab) return;

    // Process pending LayoutRequest events so the layout reflects the
    // collapsed/expanded state BEFORE we read sizeHint().
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // Force layout update to ensure size hints are accurate
    currentTab->updateGeometry();
    currentTab->adjustSize();

    // Calculate the optimal size for the current tab
    QSize tabSize = currentTab->sizeHint();

    // Also check minimum size hint which might be more accurate
    QSize minSize = currentTab->minimumSizeHint();
    int contentWidth = qMax(tabSize.width(), minSize.width());
    int contentHeight = qMax(tabSize.height(), minSize.height());

    // Add margins and chrome (menubar, window frame, etc.)
    // MCIM: Chrome stabil aus outerWindow und outerTabs berechnen —
    // unabhängig davon welcher Cam-Tab aktiv ist.
    int chrome;
    if (window() != this) {
        QMainWindow *mw = qobject_cast<QMainWindow*>(window());
        QTabWidget *outerTabs = mw ? qobject_cast<QTabWidget*>(mw->centralWidget()) : nullptr;
        if (outerTabs && outerTabs->currentWidget()) {
            // Aktueller sichtbarer Inhalt im äußeren Tab
            int visibleContentHeight = outerTabs->currentWidget()->height();
            // Gesamtfensterhöhe minus sichtbarer Inhalt = Chrome
            chrome = window()->height() - visibleContentHeight;
            if (chrome < 30) chrome = 80;
        } else {
            chrome = 80;
        }
    } else {
        chrome = menuBar()->height() + 60;
    }
    int optimalWidth = contentWidth + 30;
    int optimalHeight = contentHeight + chrome;

    // Ensure minimum width to accommodate tab bar without scrolling
    // Calculate minimum width needed for all tab labels
    int tabBarMinWidth = 0;
    if (tabWidget->tabBar()) {
        // Sum up all tab widths
        for (int i = 0; i < tabWidget->count(); ++i) {
            tabBarMinWidth += tabWidget->tabBar()->tabRect(i).width();
        }
        // Add some margin for tab bar buttons/padding
        tabBarMinWidth += 50;
    }

    // Use the larger of: optimal width or minimum tab bar width
    optimalWidth = qMax(optimalWidth, tabBarMinWidth);

    // Get screen size to avoid making window taller than screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        optimalWidth = qMin(optimalWidth, screenGeometry.width() - 50);
        optimalHeight = qMin(optimalHeight, screenGeometry.height() - 50);
    }

    // Keep current width, only adjust height when groups collapse/expand
    int finalHeight = optimalHeight;

    // MCIM: Das embedded Widget darf Qt nicht als Minimum-Constraint blockieren.
    // setMinimumHeight(0) hebt die automatische Untergrenze auf, damit window()
    // tatsächlich verkleinert werden kann.
    setMinimumHeight(0);
    if (window() != this) {
        window()->setMinimumHeight(0);
    }
    // MCIM: QTabWidget + internes QStackedWidget + alle Cam-Widgets auf 0 setzen
    // Das QStackedWidget ist der entscheidende Constraint-Halter
    if (QMainWindow *mw = qobject_cast<QMainWindow*>(window())) {
        if (QTabWidget *outerTabs = qobject_cast<QTabWidget*>(mw->centralWidget())) {
            outerTabs->setMinimumHeight(0);
            // Das interne QStackedWidget direkt finden und freigeben
            QStackedWidget *stack = outerTabs->findChild<QStackedWidget*>();
            if (stack) stack->setMinimumHeight(0);
            for (int i = 0; i < outerTabs->count(); ++i) {
                if (QWidget *w = outerTabs->widget(i)) {
                    w->setMinimumHeight(0);
                    if (w->layout()) w->layout()->activate();
                }
            }
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            outerTabs->updateGeometry();
        }
    }

    // Also reset inner tabWidget's QStackedWidget — it computes sizeHint as
    // max(all tabs), which blocks shrinking when the current tab has less content.
    tabWidget->setMinimumHeight(0);
    QStackedWidget *innerStack = tabWidget->findChild<QStackedWidget*>();
    if (innerStack) innerStack->setMinimumHeight(0);

    // Animate the resize for smooth transition
    // MCIM: window() = outerWindow; standalone: window() == this → identisch
    QPropertyAnimation *animation = new QPropertyAnimation(window(), "size");
    animation->setDuration(200);
    animation->setStartValue(window()->size());
    animation->setEndValue(QSize(window()->width(), finalHeight));  // Keep width, adjust height only
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::collapseGroupsOnly(bool collapse) {
    m_suppressResize = true;  // Resize-Callbacks während collapse blockieren
    for (CollapsibleHelper *helper : allCollapsibleHelpers) {
        if (helper && helper->isCollapsed() != collapse)
            helper->toggleGroup();
    }
    m_suppressResize = false;
    // Sofortige Layout-Neuberechnung erzwingen (nicht lazy),
    // damit Qt den korrekten kleinen sizeHint cached bevor der Resize läuft
    setMinimumHeight(0);
    if (layout()) { layout()->activate(); }
    updateGeometry();
}

void MainWindow::toggleAllGroups() {
    if (allCollapsibleHelpers.isEmpty()) {
        qDebug() << "No collapsible groups found";
        return;
    }

    // Determine whether to expand or collapse based on the state of the first group.
    bool shouldCollapse = !allCollapsibleHelpers.first()->isCollapsed();

    // Toggle all groups to the target state.
    for (CollapsibleHelper *helper : allCollapsibleHelpers) {
        if (helper && helper->isCollapsed() != shouldCollapse) {
            helper->toggleGroup();
        }
    }

    // Force all tab pages to recalculate their layout.
    // This is the key to unlocking the minimum size constraint, as Qt does not
    // automatically recalculate the size of inactive tabs.
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (QWidget *page = tabWidget->widget(i)) {
            if (page->layout()) {
                page->layout()->activate();
            }
        }
    }

    // Process events to ensure layout changes are applied.
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // Directly adjust the window size.
    adjustWindowToOptimalSize();
}

// =============================================================
// FOCUS/ZOOM FAVORITES (migrated from MainWindowDetectionActions P17)
// =============================================================

void MainWindow::rebuildFavoritesLists() {
    focusFavoritesList->clear();

    for (auto it = coupledFavorites.begin(); it != coupledFavorites.end(); ++it) {
        const QString &itemName = it.key();
        QVariantMap itemData = it.value();

        QString focusItemText = itemName;
        if (itemData["hasFocus"].toBool()) {
            int fPos = itemData["focus"].toInt();
            focusItemText = QString("%1 - F:%2").arg(itemName).arg(fPos);
        }
        if (itemData["hasZoom"].toBool()) {
            int zPos = itemData["zoom"].toInt();
            focusItemText += QString(" Z:%1").arg(zPos);
        }
        QListWidgetItem *focusItem = new QListWidgetItem(focusItemText);
        focusItem->setData(Qt::UserRole, itemName);
        focusFavoritesList->addItem(focusItem);
    }

    focusFavoritesList->setVisible(focusFavoritesList->count() > 0);
}

// =============================================================
// TIMED RECORDING (signal slot from ActionsModule, migrated P17)
// =============================================================

void MainWindow::startTimedRecording(int seconds) {
    // Negative values = sync-only request from "Start Recording" checkbox activation.
    // Set segment duration to match the recording duration (in milliseconds).
    if (seconds < 0) {
        int ms = (-seconds) * 1000;
        if (segmentDurationInput) segmentDurationInput->setText(QString::number(ms));
        return;
    }
    if (seconds == 0) return;

    qDebug() << "[RECORDING] Starting timed recording for" << seconds << "seconds";

    // Check if already running
    bool wasRunning = (process.state() == QProcess::Running);

    // Save current state
    QString savedApp = appSelector->currentText();
    QString savedTimeout = timeoutInput->text();
    QString savedTimeoutUnit = timeoutSelector->currentText();
    QString savedOutput = outputFileName->text();
    bool hadOutput = !savedOutput.isEmpty();

    qDebug() << "[RECORDING] Saved state - App:" << savedApp << "Timeout:" << savedTimeout << savedTimeoutUnit << "Output:" << savedOutput;

    // Stop current process if running and wait for it to finish
    if (wasRunning) {
        qDebug() << "[RECORDING] Stopping current process...";
        stopRpiCamApp();

        // Wait until process is really stopped before continuing
        QTimer::singleShot(1000, this, [this, seconds, savedApp, savedTimeout, savedTimeoutUnit, savedOutput, hadOutput, wasRunning]() {
            // Configure for recording
            appSelector->setCurrentText("rpicam-vid");
            updateParameterFields();

            timeoutInput->setText(QString::number(seconds));
            timeoutSelector->setCurrentText("s");

            // Generate filename with timestamp in detection_output folder
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
            QString actionsImageFolder = (m_actionsTab && m_actionsTab->module())
                ? m_actionsTab->module()->detectionAction().imageFolder : QString();
            QString recordingFolder = actionsImageFolder.isEmpty() ?
                QDir::currentPath() + "/detection_output" : actionsImageFolder;
            QString recordingPath = recordingFolder + QString("/video_%1.h264").arg(timestamp);
            outputFileName->setText(recordingPath);

            qDebug() << "[RECORDING] Recording to:" << recordingPath;

            // Start recording
            startRpiCamApp();

            // Restore settings after recording finishes
            QTimer::singleShot((seconds + 2) * 1000, this, [this, savedApp, savedTimeout, savedTimeoutUnit, savedOutput, hadOutput, wasRunning]() {
                qDebug() << "[RECORDING] Recording finished, restoring settings...";

                // Stop recording (should auto-stop due to timeout, but make sure)
                if (process.state() == QProcess::Running) {
                    stopRpiCamApp();
                }

                // Wait a moment before restarting
                QTimer::singleShot(1000, this, [this, savedApp, savedTimeout, savedTimeoutUnit, savedOutput, hadOutput, wasRunning]() {
                    // Restore settings
                    appSelector->setCurrentText(savedApp);
                    timeoutInput->setText(savedTimeout);
                    timeoutSelector->setCurrentText(savedTimeoutUnit);

                    // Restore output filename (or clear if it was empty)
                    if (hadOutput) {
                        outputFileName->setText(savedOutput);
                    } else {
                        outputFileName->clear();
                    }

                    updateParameterFields();

                    // Restart previous mode (wasRunning is always true here,
                    // since this lambda is only created inside the if (wasRunning) block)
                    qDebug() << "[RECORDING] Restarting previous mode...";
                    startRpiCamApp();

                    qDebug() << "[RECORDING] Settings restored, back to normal mode";
                });
            });
        });
    } else {
        // Not running, can start immediately
        appSelector->setCurrentText("rpicam-vid");
        updateParameterFields();

        timeoutInput->setText(QString::number(seconds));
        timeoutSelector->setCurrentText("s");

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
        QString actionsImageFolder2 = (m_actionsTab && m_actionsTab->module())
            ? m_actionsTab->module()->detectionAction().imageFolder : QString();
        QString recordingFolder = actionsImageFolder2.isEmpty() ?
            QDir::currentPath() + "/detection_output" : actionsImageFolder2;
        QString recordingPath = recordingFolder + QString("/video_%1.h264").arg(timestamp);
        outputFileName->setText(recordingPath);

        qDebug() << "[RECORDING] Recording to:" << recordingPath;
        startRpiCamApp();

        // Since nothing was running before, just stop after recording
        QTimer::singleShot((seconds + 2) * 1000, this, [this, savedApp, savedTimeout, savedTimeoutUnit, savedOutput, hadOutput]() {
            if (process.state() == QProcess::Running) {
                stopRpiCamApp();
            }

            appSelector->setCurrentText(savedApp);
            timeoutInput->setText(savedTimeout);
            timeoutSelector->setCurrentText(savedTimeoutUnit);
            if (hadOutput) {
                outputFileName->setText(savedOutput);
            } else {
                outputFileName->clear();
            }
            updateParameterFields();
        });
    }
}

// ---------------------------------------------------------------------------
// Control Socket — Live parameter updates (PR #917)
// ---------------------------------------------------------------------------

// Check whether the installed rpicam-apps build supports runtime control
// (rpicam-rt) by parsing "rpicam-vid --version" for "rpicam_rt" in the
// capabilities line.
void MainWindow::checkRpicamRtCapability()
{
    // Cache rpicam-vid --version results across all instances:
    // the binary is the same regardless of camera index, so only run once.
    static bool s_capsChecked = false;
    static bool s_hasRpicamRt = false;
    static bool s_hasPreviewBackend = false;
    static bool s_hasRoiSelection = false;
    static QString s_rpicamAppsVersion;

    if (!s_capsChecked) {
        s_capsChecked = true;

        QProcess proc;
        proc.start("rpicam-vid", {"--version"});
        if (!proc.waitForFinished(3000)) {
            qDebug() << "[Control] rpicam-vid --version timed out, assuming no ctrl support";
            appendLog(tr("[Control] Could not detect rpicam-apps version — live parameter control disabled"));
            return;
        }

        QString output = proc.readAllStandardOutput();
        if (output.isEmpty())
            output = proc.readAllStandardError();

        // Build-Version extrahieren ("rpicam-apps build: v1.5.1 123abc")
        {
            QRegularExpression re("rpicam-apps build:\\s*v?(\\S+)");
            QRegularExpressionMatch match = re.match(output);
            if (match.hasMatch()) {
                s_rpicamAppsVersion = match.captured(1);
            }
        }

        // Look for the exact capability token "rpicam_rt:1" in the
        // capabilities line, e.g.:
        //   rpicam-apps capabilites: egl:1 qt:1 drm:1 libav:1 rpicam_rt:1 roi_selection:1
        // Token match (not substring) so that "rpicam_rt:0" is treated as
        // NOT supported.
        static const QRegularExpression reRt(R"((?:^|\s)rpicam_rt:1(?:\s|$))");
        s_hasRpicamRt = reRt.match(output).hasMatch();

        // roi_selection capability (rpicam-apps feature/rt-roi branch),
        // exact token match like rpicam_rt above.
        static const QRegularExpression reRoi(R"((?:^|\s)roi_selection:1(?:\s|$))");
        s_hasRoiSelection = reRoi.match(output).hasMatch();

        if (s_hasRpicamRt) {
            qDebug() << "[Control] rpicam-apps runtime control (rpicam-rt) support detected";
        } else {
            qDebug() << "[Control] rpicam-apps does NOT support runtime control (rpicam-rt)";
            appendLog(tr("[Control] rpicam-apps build does not support control sockets — live parameter control disabled"));
        }

        // Detect --preview-backend support (rpicam-apps >= 1.13).
        // Cannot rely on capabilities line alone — egl:1/drm:1 existed before 1.13.
        // Parse the actual version string from "rpicam-apps build: v1.X.Y ..."
        {
            QRegularExpression re("rpicam-apps build:\\s*v?(\\d+)\\.(\\d+)");
            QRegularExpressionMatch match = re.match(output);
            if (match.hasMatch()) {
                int major = match.captured(1).toInt();
                int minor = match.captured(2).toInt();
                s_hasPreviewBackend = (major > 1) || (major == 1 && minor >= 13);
            }
        }
        if (s_hasPreviewBackend) {
            qDebug() << "[Control] rpicam-apps --preview-backend support detected";
        }
    }

    // Apply cached results to this instance
    m_hasRpicamRt = s_hasRpicamRt;
    m_hasPreviewBackend = s_hasPreviewBackend;
    m_hasRoiSelection = s_hasRoiSelection;
    m_rpicamAppsVersion = s_rpicamAppsVersion;
}

void MainWindow::initControlSocket()
{
    // Check whether rpicam-apps supports runtime control before showing RT indicator
    checkRpicamRtCapability();

    m_controlSocket = new ControlSocketClient(m_fixedCameraIdx >= 0 ? m_fixedCameraIdx : 0, this);

    // Position the RT + SYNC badges on the Start/Stop button
    auto positionIndicator = [this]() { repositionStartButtonBadges(); };

    // Position once initially and on resize
    QTimer::singleShot(0, this, positionIndicator);
    startStopButton->installEventFilter(this);

    // Respect the Enhanced Mode setting from GUI Setup
    updateControlSocketVisibility();

    connect(m_controlSocket, &ControlSocketClient::connected, this, [this, positionIndicator]() {
        appendLog(tr("[Control] Live parameter updates active (experimental)"));
        positionIndicator();
    });

    connect(m_controlSocket, &ControlSocketClient::disconnected, this, [this]() {
        appendLog(tr("[Control] Live parameter socket disconnected"));
    });

    connect(m_controlSocket, &ControlSocketClient::capsUpdated, this, [this](int maxFps, bool /*hasAf*/) {
        appendLog(tr("[Control] Camera caps: max %1 fps").arg(maxFps));
    });

    connect(m_controlSocket, &ControlSocketClient::errorOccurred, this, [this](const QString &msg) {
        qDebug() << "[Control] Error:" << msg;
    });
}

void MainWindow::connectControlSocket()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    // Read from [General] first, fall back to [CameraN-Tab] for migration
    settings.beginGroup("General");
    bool enabled = settings.value("Control/Enabled", QVariant()).toBool();
    settings.endGroup();
    if (!enabled && settings.value("General/Control/Enabled", QVariant()).isNull()) {
        settings.beginGroup(m_tabGroup);
        enabled = settings.value("Control/Enabled", true).toBool();
        settings.endGroup();
    }
    if (m_controlSocket && enabled && m_hasRpicamRt) {
        m_controlSocket->connectToServer();
    }
}

void MainWindow::sendSliderToSocket(const QString &key, const QString &value)
{
    if (isControlSocketActive()) {
        m_controlSocket->sendCommand(key, value);
    }
}

bool MainWindow::isControlSocketActive() const
{
    if (!m_controlSocket || !m_controlSocket->isConnected())
        return false;
    if (!m_hasRpicamRt)
        return false;
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    // Read from [General] first, fall back to [CameraN-Tab] for migration
    settings.beginGroup("General");
    bool enabled = settings.value("Control/Enabled", QVariant()).toBool();
    settings.endGroup();
    if (!enabled && settings.value("General/Control/Enabled", QVariant()).isNull()) {
        settings.beginGroup(m_tabGroup);
        enabled = settings.value("Control/Enabled", true).toBool();
        settings.endGroup();
    }
    return enabled;
}
