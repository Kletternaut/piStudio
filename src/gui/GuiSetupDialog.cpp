//#include "MainWindow.h"
#include "GuiSetupDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>
#include <QCoreApplication>
#include "../utils/AppPaths.h"
#include "../utils/AppPaths.h"
#include "../app/AppMeta.h"

// Free function defined in MainWindow.cpp: detects remote sessions (XRDP)
// where the Qt preview backend is required.
bool isRemoteSession();
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QTabWidget>
#include <linux/videodev2.h>
#include "V4L2Helpers.h"

GuiSetupDialog::GuiSetupDialog(const QString &tabGroup, int cameraIndex, QWidget *parent, bool globalMode)
    : QDialog(parent), m_tabGroup(tabGroup), m_cameraIndex(cameraIndex), m_globalMode(globalMode), outputPathEdit(nullptr), postProcessPathEdit(nullptr), postProcessLibsPathEdit(nullptr), tuningFilePathEdit(nullptr),
      splashScreenCheckbox(new QCheckBox(tr("Show splashscreen on startup"), this)),
      useCustomPreviewGeometryCheckbox(new QCheckBox(tr("Use custom preview geometry"), this)),
      focusTabCheckbox(new QCheckBox(tr("Focus"), this)),
      zoomTabCheckbox(new QCheckBox(tr("Zoom"), this)),
      audioTabCheckbox(new QCheckBox(tr("Audio"), this)),
      gstreamerTabCheckbox(new QCheckBox(tr("Gstreamer"), this)),
      gstTabCheckbox(new QCheckBox(tr("GST"), this)),
      odrTabCheckbox(new QCheckBox(tr("ODR"), this)),
      actionsTabCheckbox(new QCheckBox(tr("Actions"), this)),
      toolsTabCheckbox(new QCheckBox(tr("Tools"), this)),
      expertTabCheckbox(new QCheckBox(tr("Expert"), this)) {
    setWindowTitle(m_globalMode ? tr("Global Settings") : tr("Camera %1 Setup").arg(cameraIndex));

    // Set compact size depending on mode
    resize(m_globalMode ? QSize(700, 380) : QSize(700, 450));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Create TabWidget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setFocusPolicy(Qt::NoFocus);
    mainLayout->addWidget(tabWidget);

    // Group style definition
    QString groupStyle =
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 8px;"
        "    background-color: #ecf0f1;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 8px;"
        "    padding: 0 3px 0 3px;"
        "}";

    // Fixed label width for alignment
    const int labelWidth = 110;

    // ========== TAB 1: SETTINGS ==========
    if (m_globalMode) {
    QWidget *settingsTabWidget = new QWidget();
    QVBoxLayout *settingsTabLayout = new QVBoxLayout(settingsTabWidget);
    settingsTabLayout->setSpacing(8);

    // ========== General Settings Group ==========
    QGroupBox *settingsGroup = new QGroupBox(tr("General Settings"), this);
    settingsGroup->setStyleSheet(groupStyle);
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);

    splashScreenCheckbox->setToolTip(tr("Activate or deactivate the splashscreen on startup."));
    settingsLayout->addWidget(splashScreenCheckbox);

    controlSocketCheckbox = new QCheckBox(tr("Enhanced Mode (RT) — live parameter control"), this);
    controlSocketCheckbox->setToolTip(tr("Enable live parameter updates via rpicam-rt runtime control socket while streaming.\nRequires the rpicam-apps fork with runtime control + ROI selection (https://github.com/Kletternaut/rpicam-apps/tree/feature/rt-roi)"));
    settingsLayout->addWidget(controlSocketCheckbox);

    autoRestartSetDefaultsCheckbox = new QCheckBox(tr("Auto-restart on Set Defaults"), this);
    autoRestartSetDefaultsCheckbox->setToolTip(tr("Restart %1 immediately after saving startup defaults.")
                                                   .arg(QLatin1String(AppMeta::NAME)));
    autoRestartSetDefaultsCheckbox->setChecked(true);
    settingsLayout->addWidget(autoRestartSetDefaultsCheckbox);

    autoRestartResetDefaultsCheckbox = new QCheckBox(tr("Auto-restart on Reset Defaults"), this);
    autoRestartResetDefaultsCheckbox->setToolTip(tr("Restart %1 immediately after resetting startup defaults.")
                                                     .arg(QLatin1String(AppMeta::NAME)));
    autoRestartResetDefaultsCheckbox->setChecked(true);
    settingsLayout->addWidget(autoRestartResetDefaultsCheckbox);

    autoUpdateCheckCheckbox = new QCheckBox(tr("Check for updates on startup"), this);
    autoUpdateCheckCheckbox->setToolTip(tr("Automatically check GitHub Releases for new versions when %1 starts. If an update is available, the Help menu will show a notification.")
                                            .arg(QLatin1String(AppMeta::NAME)));
    autoUpdateCheckCheckbox->setChecked(true);
    settingsLayout->addWidget(autoUpdateCheckCheckbox);

    betaUpdatesCheckbox = new QCheckBox(tr("Include beta updates"), this);
    betaUpdatesCheckbox->setToolTip(tr("Also check for pre-release versions (e.g. 0.6.1-beta). Beta versions may be less stable than regular releases."));
    settingsLayout->addWidget(betaUpdatesCheckbox);

    useCustomPreviewGeometryCheckbox->setToolTip(tr("Use saved custom preview position instead of auto-calculated position. Right-click on preview field to save current position as default."));
    settingsLayout->addWidget(useCustomPreviewGeometryCheckbox);

    // Language Selector
    QHBoxLayout *languageRow = new QHBoxLayout;
    QLabel *languageLabel = new QLabel(tr("Sprache / Language:"), this);
    languageLabel->setFixedWidth(140);
    languageRow->addWidget(languageLabel);
    languageSelector = new QComboBox(this);
    languageSelector->addItem("Deutsch", "de");
    languageSelector->addItem("English", "en");
    languageSelector->setFixedWidth(150);
    languageRow->addWidget(languageSelector);
    QLabel *restartInfoLabel = new QLabel(tr("(Neustart erforderlich / Restart required)"), this);
    restartInfoLabel->setStyleSheet("color: #888; font-size: 11px; font-style: italic;");
    languageRow->addWidget(restartInfoLabel);
    languageRow->addStretch();
    settingsLayout->addLayout(languageRow);

    // Language change confirmation: show dialog in new language, restart on OK
    connect(languageSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        QString newLang = languageSelector->currentData().toString();
        QString savedLang = settings.value("Language/Selected", "de").toString();
        if (newLang == savedLang) return; // No change

        // Show dialog text in the newly selected language
        QString title, text;
        if (newLang == "de") {
            title = "Sprache geändert";
            text  = "Die Sprache wurde auf Deutsch geändert.\nDie Anwendung wird jetzt neu gestartet.";
        } else {
            title = "Language changed";
            text  = "The language has been changed to English.\nThe application will now restart.";
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(title);
        msgBox.setText(text);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);

        if (msgBox.exec() == QMessageBox::Ok) {
            // Save new language to global config (language is a global setting, not per-camera)
            QSettings globalSettings(AppPaths::globalConf(), QSettings::IniFormat);
            globalSettings.setValue("Language/Selected", newLang);
            globalSettings.sync();
            QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                    QCoreApplication::arguments());
            QApplication::quit();
        } else {
            // Revert selector to saved language (no settings change)
            int oldIndex = languageSelector->findData(savedLang);
            if (oldIndex >= 0) {
                languageSelector->blockSignals(true);
                languageSelector->setCurrentIndex(oldIndex);
                languageSelector->blockSignals(false);
            }
        }
    });

    settingsTabLayout->addWidget(settingsGroup);
    settingsTabLayout->addStretch();

    tabWidget->addTab(settingsTabWidget, tr("Settings"));
    } // m_globalMode (Settings tab only)

    // ========== TAB: PATHS (camera-specific) ==========
    if (!m_globalMode) {
    QWidget *pathsTabWidget = new QWidget();
    QVBoxLayout *pathsTabLayout = new QVBoxLayout(pathsTabWidget);
    pathsTabLayout->setSpacing(8);

    // ========== Folder Paths Group ==========
    QGroupBox *pathsGroup = new QGroupBox(tr("Folder Paths"), this);
    pathsGroup->setStyleSheet(groupStyle);
    QVBoxLayout *pathsLayout = new QVBoxLayout(pathsGroup);
    pathsLayout->setSpacing(4);

    // rpicam Config Path
    auto *rpicamConfigRow = new QHBoxLayout;
    QLabel *rpicamLabel = new QLabel(tr("rpicam Configs:"), this);
    rpicamLabel->setFixedWidth(labelWidth);
    rpicamConfigRow->addWidget(rpicamLabel);
    rpicamConfigPathEdit = new QLineEdit(this);
    QPushButton *browserpicamConfigButton = new QPushButton(tr("Browse"), this);
    browserpicamConfigButton->setFixedWidth(80);
    rpicamConfigRow->addWidget(rpicamConfigPathEdit);
    rpicamConfigRow->addWidget(browserpicamConfigButton);
    connect(browserpicamConfigButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = rpicamConfigPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = AppPaths::config();
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select rpicam Config Directory"), initialPath);
        if (!dir.isEmpty()) {
            rpicamConfigPathEdit->setText(dir);
        }
    });
    pathsLayout->addLayout(rpicamConfigRow);

    // Output Path
    auto *outputRow = new QHBoxLayout;
    QLabel *outputLabel = new QLabel(tr("Output Files:"), this);
    outputLabel->setFixedWidth(labelWidth);
    outputRow->addWidget(outputLabel);
    outputPathEdit = new QLineEdit(this);
    QPushButton *browseOutputButton = new QPushButton(tr("Browse"), this);
    browseOutputButton->setFixedWidth(80);
    outputRow->addWidget(outputPathEdit);
    outputRow->addWidget(browseOutputButton);
    connect(browseOutputButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = outputPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = AppPaths::output();
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), initialPath);
        if (!dir.isEmpty()) {
            outputPathEdit->setText(dir);
        }
    });
    pathsLayout->addLayout(outputRow);

    // Post-Process Path
    auto *postProcessRow = new QHBoxLayout;
    QLabel *postProcessLabel = new QLabel(tr("Post-Process:"), this);
    postProcessLabel->setFixedWidth(labelWidth);
    postProcessRow->addWidget(postProcessLabel);
    postProcessPathEdit = new QLineEdit(this);
    QPushButton *browsePostProcessButton = new QPushButton(tr("Browse"), this);
    browsePostProcessButton->setFixedWidth(80);
    postProcessRow->addWidget(postProcessPathEdit);
    postProcessRow->addWidget(browsePostProcessButton);
    connect(browsePostProcessButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = postProcessPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = "/home/admin/rpicam-apps/assets";
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Post-Process Directory"), initialPath);
        if (!dir.isEmpty()) {
            postProcessPathEdit->setText(dir);
        }
    });
    pathsLayout->addLayout(postProcessRow);

    // Tuning Files Path
    auto *tuningFileRow = new QHBoxLayout;
    QLabel *tuningLabel = new QLabel(tr("Tuning Files:"), this);
    tuningLabel->setFixedWidth(labelWidth);
    tuningFileRow->addWidget(tuningLabel);
    tuningFilePathEdit = new QLineEdit(this);
    QPushButton *browseTuningFileButton = new QPushButton(tr("Browse"), this);
    browseTuningFileButton->setFixedWidth(80);
    tuningFileRow->addWidget(tuningFilePathEdit);
    tuningFileRow->addWidget(browseTuningFileButton);
    connect(browseTuningFileButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = tuningFilePathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = AppPaths::tuningFileBasePath();
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Tuning File Directory"), initialPath);
        if (!dir.isEmpty()) {
            tuningFilePathEdit->setText(dir);
        }
    });
    pathsLayout->addLayout(tuningFileRow);

    // Metadata Path Row
    QHBoxLayout *metadataPathRow = new QHBoxLayout;
    QLabel *metadataLabel = new QLabel(tr("Metadata:"), this);
    metadataLabel->setFixedWidth(labelWidth);
    metadataPathRow->addWidget(metadataLabel);
    metadataPathEdit = new QLineEdit(this);
    QPushButton *browseMetadataButton = new QPushButton(tr("Browse"), this);
    browseMetadataButton->setFixedWidth(80);
    metadataPathRow->addWidget(metadataPathEdit);
    metadataPathRow->addWidget(browseMetadataButton);
    connect(browseMetadataButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = metadataPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = AppPaths::output();
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Metadata Directory"), initialPath);
        if (!dir.isEmpty()) {
            metadataPathEdit->setText(dir);
        }
    });
    pathsLayout->addLayout(metadataPathRow);

    // Tools Input Path Row
    QHBoxLayout *toolsInputPathRow = new QHBoxLayout;
    QLabel *toolsInputLabel = new QLabel(tr("Tools Input:"), this);
    toolsInputLabel->setFixedWidth(labelWidth);
    toolsInputPathRow->addWidget(toolsInputLabel);
    toolsInputPathEdit = new QLineEdit(this);
    QPushButton *browseToolsInputButton = new QPushButton(tr("Browse"), this);
    browseToolsInputButton->setFixedWidth(80);
    toolsInputPathRow->addWidget(toolsInputPathEdit);
    toolsInputPathRow->addWidget(browseToolsInputButton);
    connect(browseToolsInputButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = toolsInputPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = AppPaths::contentOutput();
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Tools Input Directory"), initialPath);
        if (!dir.isEmpty()) {
            toolsInputPathEdit->setText(dir);
        }
    });
    pathsLayout->addLayout(toolsInputPathRow);

    // Tools Output Path Row
    QHBoxLayout *toolsOutputPathRow = new QHBoxLayout;
    QLabel *toolsOutputLabel = new QLabel(tr("Tools Output:"), this);
    toolsOutputLabel->setFixedWidth(labelWidth);
    toolsOutputPathRow->addWidget(toolsOutputLabel);
    toolsOutputPathEdit = new QLineEdit(this);
    QPushButton *browseToolsOutputButton = new QPushButton(tr("Browse"), this);
    browseToolsOutputButton->setFixedWidth(80);
    toolsOutputPathRow->addWidget(toolsOutputPathEdit);
    toolsOutputPathRow->addWidget(browseToolsOutputButton);
    connect(browseToolsOutputButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = toolsOutputPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = AppPaths::contentOutput();
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Tools Output Directory"), initialPath);
        if (!dir.isEmpty()) {
            toolsOutputPathEdit->setText(dir);
        }
    });
    pathsLayout->addLayout(toolsOutputPathRow);

    pathsTabLayout->addWidget(pathsGroup);

    // ========== rpicam-apps Parameters Group ==========
    // Post-process libs and preview libs are passed as arguments to the
    // rpicam-apps and persisted in saved config files, so they live in a
    // separate group instead of the folder paths above.
    QGroupBox *rpicamParamsGroup = new QGroupBox(tr("rpicam-apps Parameters"), this);
    rpicamParamsGroup->setStyleSheet(groupStyle);
    QVBoxLayout *rpicamParamsLayout = new QVBoxLayout(rpicamParamsGroup);
    rpicamParamsLayout->setSpacing(4);

    // Post-Process Libs Path (--post-process-libs)
    auto *postProcessLibsRow = new QHBoxLayout;
    QLabel *postProcessLibsLabel = new QLabel(tr("Post-Proc Libs:"), this);
    postProcessLibsLabel->setFixedWidth(labelWidth);
    postProcessLibsRow->addWidget(postProcessLibsLabel);
    postProcessLibsStatusLabel = new QPushButton("✕", this);
    postProcessLibsStatusLabel->setFixedWidth(20);
    postProcessLibsStatusLabel->setFocusPolicy(Qt::NoFocus);
    postProcessLibsRow->addWidget(postProcessLibsStatusLabel);
    postProcessLibsPathEdit = new QLineEdit(this);
    QPushButton *browsePostProcessLibsButton = new QPushButton(tr("Browse"), this);
    browsePostProcessLibsButton->setFixedWidth(80);
    postProcessLibsRow->addWidget(postProcessLibsPathEdit);
    postProcessLibsRow->addWidget(browsePostProcessLibsButton);
    connect(browsePostProcessLibsButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = postProcessLibsPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = "/usr/local/lib";
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Post-Process Libs Directory"), initialPath);
        if (!dir.isEmpty()) {
            postProcessLibsPathEdit->setText(dir);
        }
    });
    // X-reset: clear the entry (removed from settings on Save)
    connect(postProcessLibsStatusLabel, &QPushButton::clicked, this, [this]() {
        postProcessLibsPathEdit->setText("");
    });
    rpicamParamsLayout->addLayout(postProcessLibsRow);

    // Preview Libs Path Row (--preview-libs, rpicam-apps >= 1.13).
    // The preview backend itself stays in the main window row; only the
    // custom libs path is configured here.
    auto *previewLibsRow = new QHBoxLayout;
    QLabel *previewLibsLabel = new QLabel(tr("Preview Libs:"), this);
    previewLibsLabel->setFixedWidth(labelWidth);
    previewLibsRow->addWidget(previewLibsLabel);
    previewLibsStatusLabel = new QPushButton("✕", this);
    previewLibsStatusLabel->setFixedWidth(20);
    previewLibsStatusLabel->setFocusPolicy(Qt::NoFocus);
    previewLibsRow->addWidget(previewLibsStatusLabel);
    previewLibsPathEdit = new QLineEdit(this);
    QPushButton *browsePreviewLibsButton = new QPushButton(tr("Browse"), this);
    browsePreviewLibsButton->setFixedWidth(80);
    previewLibsRow->addWidget(previewLibsPathEdit);
    previewLibsRow->addWidget(browsePreviewLibsButton);
    connect(browsePreviewLibsButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = previewLibsPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = "/usr/local/lib";
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Preview Libs Directory"), initialPath);
        if (!dir.isEmpty()) {
            previewLibsPathEdit->setText(dir);
        }
    });
    // X-reset: clear the entry (removed from settings on Save)
    connect(previewLibsStatusLabel, &QPushButton::clicked, this, [this]() {
        previewLibsPathEdit->setText("");
    });
    rpicamParamsLayout->addLayout(previewLibsRow);

    // Encoder Libs Path Row (--encoder-libs, rpicam-apps >= 1.13).
    auto *encoderLibsRow = new QHBoxLayout;
    QLabel *encoderLibsLabel = new QLabel(tr("Encoder Libs:"), this);
    encoderLibsLabel->setFixedWidth(labelWidth);
    encoderLibsRow->addWidget(encoderLibsLabel);
    encoderLibsStatusLabel = new QPushButton("✕", this);
    encoderLibsStatusLabel->setFixedWidth(20);
    encoderLibsStatusLabel->setFocusPolicy(Qt::NoFocus);
    encoderLibsRow->addWidget(encoderLibsStatusLabel);
    encoderLibsPathEdit = new QLineEdit(this);
    QPushButton *browseEncoderLibsButton = new QPushButton(tr("Browse"), this);
    browseEncoderLibsButton->setFixedWidth(80);
    encoderLibsRow->addWidget(encoderLibsPathEdit);
    encoderLibsRow->addWidget(browseEncoderLibsButton);
    connect(browseEncoderLibsButton, &QPushButton::clicked, this, [this]() {
        QString initialPath = encoderLibsPathEdit->text();
        if (initialPath.isEmpty()) {
            initialPath = "/usr/local/lib";
        }
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Encoder Libs Directory"), initialPath);
        if (!dir.isEmpty()) {
            encoderLibsPathEdit->setText(dir);
        }
    });
    // X-reset: clear the entry (removed from settings on Save)
    connect(encoderLibsStatusLabel, &QPushButton::clicked, this, [this]() {
        encoderLibsPathEdit->setText("");
    });
    rpicamParamsLayout->addLayout(encoderLibsRow);

    pathsTabLayout->addWidget(rpicamParamsGroup);
    pathsTabLayout->addStretch();

    tabWidget->addTab(pathsTabWidget, tr("Paths"));
    } // !m_globalMode (Paths tab)

    // Shared helper to build a detect handler for a given V4L2 control
    if (!m_globalMode) {
    auto buildDetectHandler = [this](unsigned int controlId, QComboBox *combo,
                                      QCheckBox *checkbox, QLabel *statusLabel,
                                      const QString &labelPrefix) {
        return [this, controlId, combo, checkbox, statusLabel, labelPrefix]() {
            QList<V4L2::AfDeviceInfo> found = V4L2::detectAfDevices(controlId);
            combo->clear();
            if (!found.isEmpty()) {
                for (const auto &inf : found) {
                    // Only add devices for this camera (or with unknown camera)
                    if (inf.cameraIndex == m_cameraIndex || inf.cameraIndex < 0)
                        combo->addItem(inf.devicePath);
                }
                int matchIdx = -1;
                for (int i = 0; i < found.size(); ++i) {
                    if (found[i].cameraIndex == m_cameraIndex) { matchIdx = i; break; }
                }
                combo->setCurrentIndex(matchIdx >= 0 ? matchIdx : 0);
                if (matchIdx >= 0) {
                    checkbox->setChecked(true);
                    statusLabel->setText(tr("Found: %1").arg(found[matchIdx].devicePath));
                    statusLabel->setStyleSheet("color: #27ae60; font-size: 11px; font-style: italic;");
                    QMessageBox::information(this, tr("V4L2 %1 Detection").arg(labelPrefix),
                        tr("Found: %1\n\n%2 hardware has been activated.").arg(found[matchIdx].devicePath).arg(labelPrefix));
                } else {
                    statusLabel->setText(tr("No %1 device for Camera %2").arg(labelPrefix).arg(m_cameraIndex));
                    statusLabel->setStyleSheet("color: #e67e22; font-size: 11px; font-style: italic;");
                    QMessageBox::warning(this, tr("V4L2 %1 Detection").arg(labelPrefix),
                        tr("No %1 device for Camera %2.\n\nPlease run detection on the other camera instance.").arg(labelPrefix).arg(m_cameraIndex));
                }
            } else {
                statusLabel->setText(tr("No %1-capable device found").arg(labelPrefix));
                statusLabel->setStyleSheet("color: #c0392b; font-size: 11px; font-style: italic;");
                QMessageBox::information(this, tr("V4L2 %1 Detection").arg(labelPrefix),
                    tr("No %1-capable V4L2 subdevice found.\n%1 hardware is not available on this system.").arg(labelPrefix));
            }
        };
    };

    // ========== TAB 3: FOCUS ==========
    QWidget *focusTab = new QWidget();
    QVBoxLayout *focusTabLayout = new QVBoxLayout(focusTab);
    focusTabLayout->setSpacing(8);

    // ---- V4L2 Focus Hardware Group ----
    QGroupBox *focusHwGroup = new QGroupBox(tr("V4L2 Focus Hardware"), this);
    focusHwGroup->setStyleSheet(groupStyle);
    QVBoxLayout *focusHwLayout = new QVBoxLayout(focusHwGroup);
    focusHwLayout->setSpacing(6);

    v4l2FocusCheckbox = new QCheckBox(tr("Enable V4L2 focus hardware"), this);
    v4l2FocusCheckbox->setToolTip(tr("Enable direct V4L2 subdevice control for focus.\nRequires a compatible driver (e.g. hocusfocus)."));
    focusHwLayout->addWidget(v4l2FocusCheckbox);

    auto *focusDeviceRow = new QHBoxLayout;
    QLabel *focusDeviceLabel = new QLabel(tr("V4L2 Subdevice:"), this);
    focusDeviceLabel->setFixedWidth(labelWidth);
    focusDeviceRow->addWidget(focusDeviceLabel);
    v4l2FocusCombo = new QComboBox(this);
    v4l2FocusCombo->setEditable(true);
    v4l2FocusCombo->setInsertPolicy(QComboBox::NoInsert);
    v4l2FocusCombo->setToolTip(tr("Path to the V4L2 subdevice for focus control"));
    v4l2FocusCombo->setFixedWidth(200);
    focusDeviceRow->addWidget(v4l2FocusCombo);
    QPushButton *focusDetectButton = new QPushButton(tr("Detect"), this);
    focusDetectButton->setFixedWidth(80);
    focusDetectButton->setToolTip(tr("Scan /dev/v4l-subdev* for focus-capable devices"));
    focusDeviceRow->addWidget(focusDetectButton);

    v4l2FocusStatusLabel = new QLabel(this);
    v4l2FocusStatusLabel->setStyleSheet("color: #555; font-size: 11px; font-style: italic;");
    connect(focusDetectButton, &QPushButton::clicked, this,
        buildDetectHandler(V4L2_CID_FOCUS_ABSOLUTE, v4l2FocusCombo, v4l2FocusCheckbox, v4l2FocusStatusLabel, tr("Focus")));

    focusDeviceRow->addStretch();
    focusHwLayout->addLayout(focusDeviceRow);
    focusHwLayout->addWidget(v4l2FocusStatusLabel);
    focusTabLayout->addWidget(focusHwGroup);

    // ---- Focus Step Sizes Group ----
    QGroupBox *focusStepsGroup = new QGroupBox(tr("Focus Step Sizes"), this);
    focusStepsGroup->setStyleSheet(groupStyle);
    QHBoxLayout *focusStepsLayout = new QHBoxLayout(focusStepsGroup);
    focusStepsLayout->setSpacing(4);

    focusStep1SpinBox = new QSpinBox(this);
    focusStep1SpinBox->setRange(10, 10000);
    focusStep1SpinBox->setValue(100);
    focusStep1SpinBox->setFixedWidth(80);
    focusStep1SpinBox->setToolTip(tr("Step size for focus movement (Near/Far buttons)"));
    focusStepsLayout->addWidget(focusStep1SpinBox);

    focusStep2SpinBox = new QSpinBox(this);
    focusStep2SpinBox->setRange(10, 10000);
    focusStep2SpinBox->setValue(300);
    focusStep2SpinBox->setFixedWidth(80);
    focusStep2SpinBox->setToolTip(tr("Step size for focus movement (Near/Far buttons)"));
    focusStepsLayout->addWidget(focusStep2SpinBox);

    focusStep3SpinBox = new QSpinBox(this);
    focusStep3SpinBox->setRange(10, 10000);
    focusStep3SpinBox->setValue(1000);
    focusStep3SpinBox->setFixedWidth(80);
    focusStep3SpinBox->setToolTip(tr("Step size for focus movement (Near/Far buttons)"));
    focusStepsLayout->addWidget(focusStep3SpinBox);

    focusStep4SpinBox = new QSpinBox(this);
    focusStep4SpinBox->setRange(10, 32767);
    focusStep4SpinBox->setValue(3000);
    focusStep4SpinBox->setFixedWidth(80);
    focusStep4SpinBox->setToolTip(tr("Step size for focus movement (Near/Far buttons)"));
    focusStepsLayout->addWidget(focusStep4SpinBox);

    focusStep5SpinBox = new QSpinBox(this);
    focusStep5SpinBox->setRange(10, 32767);
    focusStep5SpinBox->setValue(10000);
    focusStep5SpinBox->setFixedWidth(80);
    focusStep5SpinBox->setToolTip(tr("Step size for focus movement (Near/Far buttons)"));
    focusStepsLayout->addWidget(focusStep5SpinBox);

    focusStep6SpinBox = new QSpinBox(this);
    focusStep6SpinBox->setRange(10, 32767);
    focusStep6SpinBox->setValue(32767);
    focusStep6SpinBox->setFixedWidth(80);
    focusStep6SpinBox->setToolTip(tr("Step size for focus movement (Near/Far buttons)"));
    focusStepsLayout->addWidget(focusStep6SpinBox);

    focusStepsLayout->addStretch();
    focusTabLayout->addWidget(focusStepsGroup);
    focusTabLayout->addStretch();

    tabWidget->addTab(focusTab, tr("Focus"));

    // ========== TAB 4: ZOOM ==========
    QWidget *zoomTab = new QWidget();
    QVBoxLayout *zoomTabLayout = new QVBoxLayout(zoomTab);
    zoomTabLayout->setSpacing(8);

    // ---- V4L2 Zoom Hardware Group ----
    QGroupBox *zoomHwGroup = new QGroupBox(tr("V4L2 Zoom Hardware"), this);
    zoomHwGroup->setStyleSheet(groupStyle);
    QVBoxLayout *zoomHwLayout = new QVBoxLayout(zoomHwGroup);
    zoomHwLayout->setSpacing(6);

    v4l2ZoomCheckbox = new QCheckBox(tr("Enable V4L2 zoom hardware"), this);
    v4l2ZoomCheckbox->setToolTip(tr("Enable direct V4L2 subdevice control for zoom.\nRequires a compatible driver (e.g. hocusfocus)."));
    zoomHwLayout->addWidget(v4l2ZoomCheckbox);

    auto *zoomDeviceRow = new QHBoxLayout;
    QLabel *zoomDeviceLabel = new QLabel(tr("V4L2 Subdevice:"), this);
    zoomDeviceLabel->setFixedWidth(labelWidth);
    zoomDeviceRow->addWidget(zoomDeviceLabel);
    v4l2ZoomCombo = new QComboBox(this);
    v4l2ZoomCombo->setEditable(true);
    v4l2ZoomCombo->setInsertPolicy(QComboBox::NoInsert);
    v4l2ZoomCombo->setToolTip(tr("Path to the V4L2 subdevice for zoom control"));
    v4l2ZoomCombo->setFixedWidth(200);
    zoomDeviceRow->addWidget(v4l2ZoomCombo);
    QPushButton *zoomDetectButton = new QPushButton(tr("Detect"), this);
    zoomDetectButton->setFixedWidth(80);
    zoomDetectButton->setToolTip(tr("Scan /dev/v4l-subdev* for zoom-capable devices"));
    zoomDeviceRow->addWidget(zoomDetectButton);

    v4l2ZoomStatusLabel = new QLabel(this);
    v4l2ZoomStatusLabel->setStyleSheet("color: #555; font-size: 11px; font-style: italic;");
    connect(zoomDetectButton, &QPushButton::clicked, this,
        buildDetectHandler(V4L2_CID_ZOOM_ABSOLUTE, v4l2ZoomCombo, v4l2ZoomCheckbox, v4l2ZoomStatusLabel, tr("Zoom")));

    zoomDeviceRow->addStretch();
    zoomHwLayout->addLayout(zoomDeviceRow);
    zoomHwLayout->addWidget(v4l2ZoomStatusLabel);
    zoomTabLayout->addWidget(zoomHwGroup);

    // ---- Zoom Step Sizes Group ----
    QGroupBox *zoomStepsGroup = new QGroupBox(tr("Zoom Step Sizes"), this);
    zoomStepsGroup->setStyleSheet(groupStyle);
    QHBoxLayout *zoomStepsLayout = new QHBoxLayout(zoomStepsGroup);
    zoomStepsLayout->setSpacing(4);

    zoomStep1SpinBox = new QSpinBox(this);
    zoomStep1SpinBox->setRange(10, 32767);
    zoomStep1SpinBox->setValue(100);
    zoomStep1SpinBox->setFixedWidth(80);
    zoomStep1SpinBox->setToolTip(tr("Step size for zoom movement (Near/Far buttons)"));
    zoomStepsLayout->addWidget(zoomStep1SpinBox);

    zoomStep2SpinBox = new QSpinBox(this);
    zoomStep2SpinBox->setRange(10, 32767);
    zoomStep2SpinBox->setValue(300);
    zoomStep2SpinBox->setFixedWidth(80);
    zoomStep2SpinBox->setToolTip(tr("Step size for zoom movement (Near/Far buttons)"));
    zoomStepsLayout->addWidget(zoomStep2SpinBox);

    zoomStep3SpinBox = new QSpinBox(this);
    zoomStep3SpinBox->setRange(10, 32767);
    zoomStep3SpinBox->setValue(1000);
    zoomStep3SpinBox->setFixedWidth(80);
    zoomStep3SpinBox->setToolTip(tr("Step size for zoom movement (Near/Far buttons)"));
    zoomStepsLayout->addWidget(zoomStep3SpinBox);

    zoomStep4SpinBox = new QSpinBox(this);
    zoomStep4SpinBox->setRange(10, 32767);
    zoomStep4SpinBox->setValue(3000);
    zoomStep4SpinBox->setFixedWidth(80);
    zoomStep4SpinBox->setToolTip(tr("Step size for zoom movement (Near/Far buttons)"));
    zoomStepsLayout->addWidget(zoomStep4SpinBox);

    zoomStep5SpinBox = new QSpinBox(this);
    zoomStep5SpinBox->setRange(10, 32767);
    zoomStep5SpinBox->setValue(10000);
    zoomStep5SpinBox->setFixedWidth(80);
    zoomStep5SpinBox->setToolTip(tr("Step size for zoom movement (Near/Far buttons)"));
    zoomStepsLayout->addWidget(zoomStep5SpinBox);

    zoomStep6SpinBox = new QSpinBox(this);
    zoomStep6SpinBox->setRange(10, 32767);
    zoomStep6SpinBox->setValue(32767);
    zoomStep6SpinBox->setFixedWidth(80);
    zoomStep6SpinBox->setToolTip(tr("Step size for zoom movement (Near/Far buttons)"));
    zoomStepsLayout->addWidget(zoomStep6SpinBox);

    zoomStepsLayout->addStretch();
    zoomTabLayout->addWidget(zoomStepsGroup);
    zoomTabLayout->addStretch();

    tabWidget->addTab(zoomTab, tr("Zoom"));

    // ========== TAB 5: TABS ==========
    QWidget *tabsTabWidget = new QWidget();
    QVBoxLayout *tabsTabLayout = new QVBoxLayout(tabsTabWidget);
    tabsTabLayout->setSpacing(8);

    // ========== Activate Tabs Group ==========
    QGroupBox *tabsGroup = new QGroupBox(tr("Activate Tabs"), this);
    tabsGroup->setStyleSheet(groupStyle);
    QVBoxLayout *tabsLayout = new QVBoxLayout(tabsGroup);

    // Erste Zeile: Expert, Audio, Focus, Zoom, GStreamer, GST, ODR, Action, Log
    QHBoxLayout *tabsRow1 = new QHBoxLayout;

    expertTabCheckbox->setToolTip(tr("Advanced low-level parameters"));
    tabsRow1->addWidget(expertTabCheckbox);

    audioTabCheckbox->setToolTip(tr("Audio recording settings"));
    tabsRow1->addWidget(audioTabCheckbox);

    focusTabCheckbox->setToolTip(tr("Autofocus control"));
    tabsRow1->addWidget(focusTabCheckbox);

    zoomTabCheckbox->setToolTip(tr("Digital zoom control"));
    tabsRow1->addWidget(zoomTabCheckbox);

    gstreamerTabCheckbox->setToolTip(tr("Network streaming"));
    tabsRow1->addWidget(gstreamerTabCheckbox);

    gstTabCheckbox->setToolTip(tr("Stream viewer configuration"));
    tabsRow1->addWidget(gstTabCheckbox);

    odrTabCheckbox->setToolTip(tr("Object detection and recognition"));
    tabsRow1->addWidget(odrTabCheckbox);

    actionsTabCheckbox->setToolTip(tr("Detection-based actions"));
    tabsRow1->addWidget(actionsTabCheckbox);

    toolsTabCheckbox->setToolTip(tr("Utility tools (image sequence to video)"));
    tabsRow1->addWidget(toolsTabCheckbox);

    tabsLayout->addLayout(tabsRow1);

    tabsTabLayout->addWidget(tabsGroup);
    tabsTabLayout->addStretch();

    tabWidget->addTab(tabsTabWidget, tr("Tabs"));

    // ========== TAB 6: CUSTOM ==========
    QWidget *customTabWidget = new QWidget();
    QVBoxLayout *customTabLayout = new QVBoxLayout(customTabWidget);
    customTabLayout->setSpacing(8);

    // Use custom preview geometry checkbox at the top
    customTabLayout->addWidget(useCustomPreviewGeometryCheckbox);

    // Add Custom Apps and Resolutions to Custom Tab
    setupCustomAppInputs(customTabLayout);
    setupCustomResolutionInputs(customTabLayout);
    customTabLayout->addStretch();

    tabWidget->addTab(customTabWidget, tr("Custom"));
    } // !m_globalMode (Focus + Zoom + Tabs + Custom tabs)

    // Save and Cancel Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *saveButton = new QPushButton(tr("Save"), this);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::clicked, this, &GuiSetupDialog::saveGuiSettings);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    setLayout(mainLayout);

    // Hide widgets that don't belong in this mode to prevent floating orphans
    if (m_globalMode) {
        // Camera-mode widgets — not in Global Settings layout, hide them
        if (v4l2FocusCheckbox)       v4l2FocusCheckbox->hide();
        if (v4l2FocusCombo)          v4l2FocusCombo->hide();
        if (v4l2FocusStatusLabel)    v4l2FocusStatusLabel->hide();
        if (v4l2ZoomCheckbox)        v4l2ZoomCheckbox->hide();
        if (v4l2ZoomCombo)           v4l2ZoomCombo->hide();
        if (v4l2ZoomStatusLabel)     v4l2ZoomStatusLabel->hide();
        if (focusStep1SpinBox)       focusStep1SpinBox->hide();
        if (focusStep2SpinBox)       focusStep2SpinBox->hide();
        if (focusStep3SpinBox)       focusStep3SpinBox->hide();
        if (focusStep4SpinBox)       focusStep4SpinBox->hide();
        if (focusStep5SpinBox)       focusStep5SpinBox->hide();
        if (focusStep6SpinBox)       focusStep6SpinBox->hide();
        if (zoomStep1SpinBox)        zoomStep1SpinBox->hide();
        if (zoomStep2SpinBox)        zoomStep2SpinBox->hide();
        if (zoomStep3SpinBox)        zoomStep3SpinBox->hide();
        if (zoomStep4SpinBox)        zoomStep4SpinBox->hide();
        if (zoomStep5SpinBox)        zoomStep5SpinBox->hide();
        if (zoomStep6SpinBox)        zoomStep6SpinBox->hide();
        // Tab checkboxes (only in Camera Setup's Tabs tab)
        focusTabCheckbox->hide();
        zoomTabCheckbox->hide();
        audioTabCheckbox->hide();
        gstreamerTabCheckbox->hide();
        gstTabCheckbox->hide();
        odrTabCheckbox->hide();
        actionsTabCheckbox->hide();
        toolsTabCheckbox->hide();
        expertTabCheckbox->hide();
    } else {
        // Global-mode widgets — not in Camera Setup layout, hide them
        if (splashScreenCheckbox)             splashScreenCheckbox->hide();
        if (controlSocketCheckbox)            controlSocketCheckbox->hide();
        if (autoRestartSetDefaultsCheckbox)   autoRestartSetDefaultsCheckbox->hide();
        if (autoRestartResetDefaultsCheckbox) autoRestartResetDefaultsCheckbox->hide();
        if (autoUpdateCheckCheckbox)          autoUpdateCheckCheckbox->hide();
        if (betaUpdatesCheckbox)              betaUpdatesCheckbox->hide();
        if (languageSelector)                 languageSelector->hide();
    }

    // Lade bestehende Einstellungen
    loadGuiSettings();
}

GuiSetupDialog::~GuiSetupDialog() {
    // Persist the splash checkbox — same file/section as saveGuiSettings(),
    // so main.cpp reads the value from globalConf() as expected.
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup("General");
    settings.setValue("splashScreenEnabled", splashScreenCheckbox->isChecked());
    settings.endGroup();
}


void GuiSetupDialog::setEffectiveLibPaths(const QString &previewLibs, const QString &postProcessLibs, const QString &encoderLibs)
{
    m_effectivePreviewLibs = previewLibs;
    m_effectivePostProcessLibs = postProcessLibs;
    m_effectiveEncoderLibs = encoderLibs;
}

void GuiSetupDialog::saveGuiSettings() {
    // Global settings → [General], Camera settings → [CameraN-Tab]
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);

    if (m_globalMode) {
        // Global settings: Splash, Enhanced, Auto-Restart
        settings.beginGroup("General");

        settings.setValue("splashScreenEnabled", splashScreenCheckbox->isChecked());
        settings.setValue("Control/Enabled", controlSocketCheckbox->isChecked());
        settings.setValue("Defaults/AutoRestartSet", autoRestartSetDefaultsCheckbox->isChecked());
        settings.setValue("Defaults/AutoRestartReset", autoRestartResetDefaultsCheckbox->isChecked());
        settings.setValue("Updates/AutoCheck", autoUpdateCheckCheckbox->isChecked());
        settings.setValue("Updates/BetaUpdates", betaUpdatesCheckbox->isChecked());
        settings.endGroup();
    } else {
        // Camera-specific settings: Paths, Tabs, V4L2, Step Sizes, Custom Apps/Resolutions
        settings.beginGroup(m_tabGroup);

        settings.setValue("Paths/GuiOutputPath", outputPathEdit->text());
        settings.setValue("Paths/GuiPostProcessPath", postProcessPathEdit->text());
        if (postProcessLibsPathEdit->text().trimmed().isEmpty())
            settings.remove("Defaults/PostProcessLibs");
        else
            settings.setValue("Defaults/PostProcessLibs", postProcessLibsPathEdit->text());
        settings.setValue("Paths/GuiTuningFilePath", tuningFilePathEdit->text());
        settings.setValue("Paths/GuiMetadataPath", metadataPathEdit->text());
        settings.setValue("Paths/GuiRpicamConfigPath", rpicamConfigPathEdit->text());
        settings.setValue("Paths/GuiToolsInputPath", toolsInputPathEdit->text());
        settings.setValue("Paths/GuiToolsOutputPath", toolsOutputPathEdit->text());
        if (previewLibsPathEdit->text().trimmed().isEmpty())
            settings.remove("Defaults/PreviewLibs");
        else
            settings.setValue("Defaults/PreviewLibs", previewLibsPathEdit->text());
        if (encoderLibsPathEdit->text().trimmed().isEmpty())
            settings.remove("Defaults/EncoderLibs");
        else
            settings.setValue("Defaults/EncoderLibs", encoderLibsPathEdit->text());

        settings.setValue("expertTabEnabled", expertTabCheckbox->isChecked());
        settings.setValue("focusTabEnabled", focusTabCheckbox->isChecked());
        settings.setValue("zoomTabEnabled", zoomTabCheckbox->isChecked());
        settings.setValue("audioTabEnabled", audioTabCheckbox->isChecked());
        settings.setValue("gstreamerTabEnabled", gstreamerTabCheckbox->isChecked());
        settings.setValue("gstTabEnabled", gstTabCheckbox->isChecked());
        settings.setValue("odrTabEnabled", odrTabCheckbox->isChecked());
        settings.setValue("actionsTabEnabled", actionsTabCheckbox->isChecked());
        settings.setValue("toolsTabEnabled", toolsTabCheckbox->isChecked());
        settings.setValue("Preview/UseCustomGeometry", useCustomPreviewGeometryCheckbox->isChecked());

        // Save V4L2 Hardware settings
        settings.setValue("V4L2/FocusEnabled", v4l2FocusCheckbox->isChecked());
        settings.setValue("V4L2/FocusDevice", v4l2FocusCombo->currentText());
        settings.setValue("V4L2/ZoomEnabled", v4l2ZoomCheckbox->isChecked());
        settings.setValue("V4L2/ZoomDevice", v4l2ZoomCombo->currentText());

        // Save Focus & Zoom Step Sizes (6 values each)
        settings.setValue("Focus/StepSize1", focusStep1SpinBox->value());
        settings.setValue("Focus/StepSize2", focusStep2SpinBox->value());
        settings.setValue("Focus/StepSize3", focusStep3SpinBox->value());
        settings.setValue("Focus/StepSize4", focusStep4SpinBox->value());
        settings.setValue("Focus/StepSize5", focusStep5SpinBox->value());
        settings.setValue("Focus/StepSize6", focusStep6SpinBox->value());
        settings.setValue("Zoom/StepSize1", zoomStep1SpinBox->value());
        settings.setValue("Zoom/StepSize2", zoomStep2SpinBox->value());
        settings.setValue("Zoom/StepSize3", zoomStep3SpinBox->value());
        settings.setValue("Zoom/StepSize4", zoomStep4SpinBox->value());
        settings.setValue("Zoom/StepSize5", zoomStep5SpinBox->value());
        settings.setValue("Zoom/StepSize6", zoomStep6SpinBox->value());

        for (int i = 0; i < customAppInputs.size(); ++i) {
            QString key = QString("CustomApp%1").arg(i + 1);
            settings.setValue(key, customAppInputs[i]->text());
        }

        // Zuerst alle alten CustomResolution Einträge löschen
        for (int i = 1; i <= 10; ++i) {
            QString key = QString("CustomResolution%1").arg(i);
            settings.remove(key);
        }

        // Dann nur die nicht-leeren Einträge sequenziell speichern
        int resIndex = 1;
        for (int i = 0; i < customResolutionInputs.size(); ++i) {
            QString resolution = customResolutionInputs[i]->text().trimmed();
            if (!resolution.isEmpty()) {
                QString key = QString("CustomResolution%1").arg(resIndex);
                settings.setValue(key, resolution);
                resIndex++;
            }
        }

        settings.endGroup();
    }

    // Save Language Selection (global, only in Global Settings mode)
    if (m_globalMode) {
        settings.setValue("Language/Selected", languageSelector->currentData().toString());
    }

    emit settingsSaved();  // Signal aussenden, dass Settings aktualisiert wurden

    accept();
}

bool GuiSetupDialog::isSplashScreenEnabled() const {
    return splashScreenCheckbox->isChecked();
}

void GuiSetupDialog::browserpicamConfigFilePath() {
    QString initialPath = rpicamConfigPathEdit->text();
    if (initialPath.isEmpty()) {
        initialPath = AppPaths::config(); // Standardpfad, falls leer
    }

    QString dir = QFileDialog::getExistingDirectory(this, tr("Select rpicamConfig Directory"), initialPath);
    if (!dir.isEmpty()) {
        rpicamConfigPathEdit->setText(dir);
    }
}

void GuiSetupDialog::setupCustomAppInputs(QVBoxLayout *layout) {
    QGroupBox *customAppsGroup = new QGroupBox(tr("Custom Applications"), this);
    customAppsGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 8px;"
        "    background-color: #ecf0f1;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 8px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    QVBoxLayout *customAppsLayout = new QVBoxLayout(customAppsGroup);
    customAppsLayout->setSpacing(4);

    const int labelWidth = 110; // Same as other labels

    for (int i = 0; i < 3; ++i) {
        auto *appRow = new QHBoxLayout;
        QLabel *appLabel = new QLabel(QString("App %1:").arg(i + 1), this);
        appLabel->setFixedWidth(labelWidth);
        appRow->addWidget(appLabel);
        QLineEdit *input = new QLineEdit(this);
        input->setPlaceholderText(tr("e.g., rpicam-custom-app"));
        appRow->addWidget(input);
        customAppsLayout->addLayout(appRow);
        customAppInputs.append(input);
    }

    layout->addWidget(customAppsGroup);
}

void GuiSetupDialog::setupCustomResolutionInputs(QVBoxLayout *layout) {
    QGroupBox *customResGroup = new QGroupBox(tr("Custom Resolutions"), this);
    customResGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 8px;"
        "    background-color: #ecf0f1;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 8px;"
        "    padding: 0 5px;"
        "    color: #333333;"
        "}"
    );
    QVBoxLayout *customResLayout = new QVBoxLayout(customResGroup);
    customResLayout->setSpacing(4);

    const int labelWidth = 110; // Same as other labels

    for (int i = 0; i < 3; ++i) {
        QHBoxLayout *rowLayout = new QHBoxLayout;
        QLabel *label = new QLabel(QString("Resolution %1:").arg(i + 1), this);
        label->setFixedWidth(labelWidth);
        QLineEdit *input = new QLineEdit(this);
        input->setPlaceholderText(tr("e.g., 2028x1080"));
        customResolutionInputs.append(input);
        rowLayout->addWidget(label);
        rowLayout->addWidget(input);
        customResLayout->addLayout(rowLayout);
    }

    layout->addWidget(customResGroup);
}

void GuiSetupDialog::loadGuiSettings() {
    // Global settings → [General], Camera settings → [CameraN-Tab]
    // Migration: if [General] key missing, fall back to [Camera0-Tab]
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);

    if (m_globalMode) {
        settings.beginGroup("General");
        splashScreenCheckbox->setChecked(settings.value("splashScreenEnabled", true).toBool());
        controlSocketCheckbox->setChecked(settings.value("Control/Enabled", true).toBool());
        autoRestartSetDefaultsCheckbox->setChecked(settings.value("Defaults/AutoRestartSet", true).toBool());
        autoRestartResetDefaultsCheckbox->setChecked(settings.value("Defaults/AutoRestartReset", true).toBool());
        autoUpdateCheckCheckbox->setChecked(settings.value("Updates/AutoCheck", true).toBool());
        betaUpdatesCheckbox->setChecked(settings.value("Updates/BetaUpdates", false).toBool());
        settings.endGroup();
    } else {
        // Camera-specific settings from [CameraN-Tab]
        settings.beginGroup(m_tabGroup);

        outputPathEdit->setText(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiOutputPath", AppPaths::output()).toString(), AppPaths::output()));
        postProcessPathEdit->setText(settings.value("Paths/GuiPostProcessPath", "/home/admin/rpicam-apps/assets").toString());
        tuningFilePathEdit->setText(settings.value("Paths/GuiTuningFilePath", AppPaths::tuningFileBasePath()).toString());
        metadataPathEdit->setText(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiMetadataPath", AppPaths::output()).toString(), AppPaths::output()));
        rpicamConfigPathEdit->setText(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiRpicamConfigPath", AppPaths::config()).toString(), AppPaths::config()));
        toolsInputPathEdit->setText(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiToolsInputPath", AppPaths::contentOutput()).toString(), AppPaths::contentOutput()));
        toolsOutputPathEdit->setText(AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiToolsOutputPath", AppPaths::contentOutput()).toString(), AppPaths::contentOutput()));

        // Preview / post-process libs: always show the EFFECTIVE value and
        // mark its origin with the color of the X-reset button in front of
        // the field. A value from a loaded config file or an active profile
        // is temporary (red) until it is saved here; a value stored in the
        // settings is saved (green).
        auto applyLibPath = [&](const QString &key, QLineEdit *edit, QPushButton *status,
                                const QString &effective) {
            const QString saved = settings.value(key, "").toString();
            if (!effective.isEmpty() && effective != saved) {
                // Temporary: effective value differs from the saved one
                edit->setText(effective);
                status->setStyleSheet("color: red;");
                status->setToolTip(tr("Temporary: this value was loaded from a config file or profile and is not yet saved permanently. Saving the camera setup makes it permanent."));
            } else if (!saved.isEmpty()) {
                edit->setText(saved);
                status->setStyleSheet("color: green;");
                status->setToolTip(tr("Permanent: this value is saved in the settings."));
            } else {
                edit->setText("");
                status->setStyleSheet("color: black;");
                status->setToolTip(tr("Reset Auswahl"));
            }
        };
        applyLibPath("Defaults/PreviewLibs", previewLibsPathEdit, previewLibsStatusLabel, m_effectivePreviewLibs);
        applyLibPath("Defaults/PostProcessLibs", postProcessLibsPathEdit, postProcessLibsStatusLabel, m_effectivePostProcessLibs);
        applyLibPath("Defaults/EncoderLibs", encoderLibsPathEdit, encoderLibsStatusLabel, m_effectiveEncoderLibs);

        expertTabCheckbox->setChecked(settings.value("expertTabEnabled", true).toBool());
        focusTabCheckbox->setChecked(settings.value("focusTabEnabled", false).toBool());
        zoomTabCheckbox->setChecked(settings.value("zoomTabEnabled", false).toBool());
        audioTabCheckbox->setChecked(settings.value("audioTabEnabled", true).toBool());
        gstreamerTabCheckbox->setChecked(settings.value("gstreamerTabEnabled", true).toBool());
        gstTabCheckbox->setChecked(settings.value("gstTabEnabled", true).toBool());
        odrTabCheckbox->setChecked(settings.value("odrTabEnabled", false).toBool());
        actionsTabCheckbox->setChecked(settings.value("actionsTabEnabled", false).toBool());
        toolsTabCheckbox->setChecked(settings.value("toolsTabEnabled", true).toBool());
        useCustomPreviewGeometryCheckbox->setChecked(settings.value("Preview/UseCustomGeometry", false).toBool());

        // Load V4L2 Hardware settings
        v4l2FocusCheckbox->setChecked(settings.value("V4L2/FocusEnabled", false).toBool());
        QString savedFocusDevice = settings.value("V4L2/FocusDevice", "").toString();
        if (!savedFocusDevice.isEmpty())
            v4l2FocusCombo->setCurrentText(savedFocusDevice);
        v4l2ZoomCheckbox->setChecked(settings.value("V4L2/ZoomEnabled", false).toBool());
        QString savedZoomDevice = settings.value("V4L2/ZoomDevice", "").toString();
        if (!savedZoomDevice.isEmpty())
            v4l2ZoomCombo->setCurrentText(savedZoomDevice);

        // Load Focus & Zoom Step Sizes (6 values each)
        focusStep1SpinBox->setValue(settings.value("Focus/StepSize1", 100).toInt());
        focusStep2SpinBox->setValue(settings.value("Focus/StepSize2", 300).toInt());
        focusStep3SpinBox->setValue(settings.value("Focus/StepSize3", 1000).toInt());
        focusStep4SpinBox->setValue(settings.value("Focus/StepSize4", 3000).toInt());
        focusStep5SpinBox->setValue(settings.value("Focus/StepSize5", 10000).toInt());
        focusStep6SpinBox->setValue(settings.value("Focus/StepSize6", 32767).toInt());
        zoomStep1SpinBox->setValue(settings.value("Zoom/StepSize1", 100).toInt());
        zoomStep2SpinBox->setValue(settings.value("Zoom/StepSize2", 300).toInt());
        zoomStep3SpinBox->setValue(settings.value("Zoom/StepSize3", 1000).toInt());
        zoomStep4SpinBox->setValue(settings.value("Zoom/StepSize4", 3000).toInt());
        zoomStep5SpinBox->setValue(settings.value("Zoom/StepSize5", 10000).toInt());
        zoomStep6SpinBox->setValue(settings.value("Zoom/StepSize6", 32767).toInt());

        // Leere die Listen
        customAppEntries.clear();
        for (QLineEdit *input : customAppInputs) {
            input->clear();
        }

        // Lade die CustomApps in customAppEntries
        for (int i = 0; i < 3; ++i) {
            QString key = QString("CustomApp%1").arg(i + 1);
            QString value = settings.value(key, "").toString();
            if (!value.isEmpty()) { // Nur nicht-leere Werte hinzufügen
                customAppEntries.append(value);
            }
        }

        // Synchronisiere die Werte mit den QLineEdit-Feldern
        for (int i = 0; i < customAppInputs.size(); ++i) {
            if (i < customAppEntries.size()) {
                customAppInputs[i]->setText(customAppEntries[i]);
            }
        }

        // Load custom resolutions
        for (int i = 0; i < customResolutionInputs.size(); ++i) {
            customResolutionInputs[i]->clear();
        }
        int loadedCount = 0;
        for (int i = 1; i <= 10; ++i) {
            QString key = QString("CustomResolution%1").arg(i);
            QString resolution = settings.value(key, "").toString();
            if (!resolution.isEmpty() && loadedCount < customResolutionInputs.size()) {
                customResolutionInputs[loadedCount]->setText(resolution);
                loadedCount++;
            }
        }

        settings.endGroup();
    }

    // Load Language Selection (global setting, only in Global Settings mode)
    if (m_globalMode) {
        QSettings globalSettings(AppPaths::globalConf(), QSettings::IniFormat);
        QString savedLanguage = globalSettings.value("Language/Selected", "de").toString();
        int langIndex = languageSelector->findData(savedLanguage);
        if (langIndex >= 0) {
            languageSelector->setCurrentIndex(langIndex);
        }
    }
}

QStringList GuiSetupDialog::getCustomAppEntries() const {
    QStringList entries;
    for (QLineEdit *input : customAppInputs) {
        if (!input->text().isEmpty()) {
            entries.append(input->text());
        }
    }
    return entries;
}

QStringList GuiSetupDialog::getCustomResolutions() const {
    QStringList resolutions;
    QRegularExpression resFormat("^\\d+x\\d+$"); // Validiert Format WIDTHxHEIGHT
    for (QLineEdit *input : customResolutionInputs) {
        QString text = input->text().trimmed();
        if (!text.isEmpty() && resFormat.match(text).hasMatch()) {
            resolutions.append(text);
        }
    }
    return resolutions;
}
