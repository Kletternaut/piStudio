#include "ActionsModule.h"
#include "../../gui/CollapsibleHelper.h"
#include "../../gui/CheckableComboBox.h"
#include "../../utils/AppPaths.h"
#include "../../app/AppMeta.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QIntValidator>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QUrl>
#include <QCoreApplication>
#include <QDebug>
#include <csignal>
#include <unistd.h>

// ============================================================
// Construction
// ============================================================

ActionsModule::ActionsModule(const QString &tabGroup, QObject *parent)
    : QObject(parent)
    , m_tabGroup(tabGroup)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

// ============================================================
// SETUP — equivalent of MainWindow::setupDetectionActionsUI()
// ============================================================

void ActionsModule::setup(QList<CollapsibleHelper *> &helpers,
                           std::function<void()> adjustWindowCallback,
                           const CameraInterface &cameraIface,
                           std::function<void()> onStateChanged)
{
    m_isInitializing = true;
    m_camera = cameraIface;
    m_onStateChanged = onStateChanged;

    m_tab = new QWidget;
    auto *mainLayout = new QVBoxLayout(m_tab);

    const QString groupStyle =
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
        "}";

    // ==========================================================
    // GROUP 1: Object Filter
    // ==========================================================
    auto *filterGroup = new QGroupBox(tr("Object Filter"), m_tab);
    filterGroup->setStyleSheet(groupStyle);
    auto *filterLayout = new QGridLayout(filterGroup);
    filterLayout->setColumnMinimumWidth(0, 150);
    filterLayout->setColumnStretch(3, 1);

    filterLayout->addWidget(new QLabel(tr("Filter Objects:"), m_tab), 0, 0);
    m_filterComboBox = new CheckableComboBox(m_tab);
    m_filterComboBox->addCheckableItem(tr("All Objects"), "*");
    QStringList commonObjects = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
        "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe"
    };
    for (const QString &obj : commonObjects) {
        m_filterComboBox->addCheckableItem(obj, obj);
    }
    m_filterComboBox->setToolTip(
        tr("Select objects to trigger actions. 'All Objects' or empty = all objects trigger actions"));
    filterLayout->addWidget(m_filterComboBox, 0, 1, 1, 2);

    filterLayout->addWidget(new QLabel(tr("Custom Object:"), m_tab), 1, 0);
    m_customObjectInput = new QLineEdit(m_tab);
    m_customObjectInput->setPlaceholderText(tr("Enter custom object name..."));
    m_customObjectInput->setToolTip(tr("Add a custom object name to the filter list"));
    filterLayout->addWidget(m_customObjectInput, 1, 1);

    m_addCustomObjButton = new QPushButton(tr("Add"), m_tab);
    m_addCustomObjButton->setToolTip(tr("Add custom object to filter"));
    connect(m_addCustomObjButton, &QPushButton::clicked,
            this, &ActionsModule::addCustomObject);
    filterLayout->addWidget(m_addCustomObjButton, 1, 2);

    m_filterResetButton = new QPushButton("x", m_tab);
    m_filterResetButton->setFixedWidth(20);
    m_filterResetButton->setToolTip(tr("Reset object filter to defaults"));
    connect(m_filterComboBox, &CheckableComboBox::checkedItemsChanged, this, [this]() {
        updateConfig();
        updateFilterResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    connect(m_filterResetButton, &QPushButton::clicked, this, [this]() {
        if (m_filterComboBox) m_filterComboBox->clearCheckedItems();
        updateFilterResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    filterLayout->addWidget(m_filterResetButton, 1, 4, Qt::AlignRight);

    helpers.append(CollapsibleHelper::makeCollapsible(
        filterGroup, "UI/Inference/FilterGroup", adjustWindowCallback));
    mainLayout->addWidget(filterGroup);

    // ==========================================================
    // GROUP 2: Detection Actions
    // ==========================================================
    auto *actionsGroup = new QGroupBox(tr("Detection Actions"), m_tab);
    actionsGroup->setStyleSheet(groupStyle);
    auto *actionsLayout = new QGridLayout(actionsGroup);
    actionsLayout->setColumnMinimumWidth(0, 150);
    actionsLayout->setColumnMinimumWidth(1, 400);
    actionsLayout->setColumnStretch(3, 1);

    m_actionsResetButton = new QPushButton("x", m_tab);
    m_actionsResetButton->setFixedWidth(20);
    m_actionsResetButton->setToolTip(
        tr("Reset all action checkboxes to defaults (paths and tokens are preserved)"));

    int row = 0;

    // Play Sound
    m_playSoundCheckbox = new QCheckBox(tr("Play Sound"), m_tab);
    connect(m_playSoundCheckbox, &QCheckBox::toggled, this, [this]() {
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_playSoundCheckbox, row, 0);
    m_soundFileInput = new QLineEdit(m_tab);
    m_soundFileInput->setPlaceholderText(tr("Path to sound file..."));
    connect(m_soundFileInput, &QLineEdit::textChanged, this, &ActionsModule::updateConfig);
    actionsLayout->addWidget(m_soundFileInput, row, 1);
    auto *soundBrowse = new QPushButton(tr("Browse..."), m_tab);
    connect(soundBrowse, &QPushButton::clicked, this, &ActionsModule::browseActionSoundFile);
    actionsLayout->addWidget(soundBrowse, row, 2);
    row++;

    // Save Image
    m_saveImageCheckbox = new QCheckBox(tr("Save Image"), m_tab);
    connect(m_saveImageCheckbox, &QCheckBox::toggled, this, [this]() {
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_saveImageCheckbox, row, 0);
    m_imageFolderInput = new QLineEdit(m_tab);
    m_imageFolderInput->setPlaceholderText(tr("Default: output path from Setup"));
    connect(m_imageFolderInput, &QLineEdit::textChanged, this, &ActionsModule::updateConfig);
    actionsLayout->addWidget(m_imageFolderInput, row, 1);
    auto *imgBrowse = new QPushButton(tr("Browse..."), m_tab);
    connect(imgBrowse, &QPushButton::clicked, this, &ActionsModule::browseActionImageFolder);
    actionsLayout->addWidget(imgBrowse, row, 2);
    row++;

    // Show Notification
    m_showNotificationCheckbox = new QCheckBox(tr("Show Notification"), m_tab);
    connect(m_showNotificationCheckbox, &QCheckBox::toggled, this, [this]() {
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_showNotificationCheckbox, row, 0, 1, 3);
    row++;

    // Run Script
    m_runScriptCheckbox = new QCheckBox(tr("Run Script"), m_tab);
    connect(m_runScriptCheckbox, &QCheckBox::toggled, this, [this]() {
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_runScriptCheckbox, row, 0);
    m_scriptPathInput = new QLineEdit(m_tab);
    m_scriptPathInput->setPlaceholderText(tr("Path to script/program..."));
    connect(m_scriptPathInput, &QLineEdit::textChanged, this, &ActionsModule::updateConfig);
    actionsLayout->addWidget(m_scriptPathInput, row, 1);
    auto *scriptBrowse = new QPushButton(tr("Browse..."), m_tab);
    connect(scriptBrowse, &QPushButton::clicked, this, &ActionsModule::browseActionScriptPath);
    actionsLayout->addWidget(scriptBrowse, row, 2);
    row++;

    // Start Recording
    m_startRecordingCheckbox = new QCheckBox(tr("Start Recording"), m_tab);
    connect(m_startRecordingCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
        if (checked && !m_isInitializing) {
            emit setCodecMjpegRequested();
            emit enableSignalRecordingRequested();
            // Sync duration to segment duration (seconds+2)*1000 ms
            bool ok = false;
            int seconds = m_recordingDurationInput ? m_recordingDurationInput->text().toInt(&ok) : 30;
            if (ok) emit startTimedRecordingRequested(-(seconds + 2)); // -N → nur sync, kein Start
        }
    });
    actionsLayout->addWidget(m_startRecordingCheckbox, row, 0);

    auto *durationWidget = new QWidget(m_tab);
    auto *durationLayout = new QHBoxLayout(durationWidget);
    durationLayout->setContentsMargins(0, 0, 0, 0);
    durationLayout->addWidget(new QLabel(tr("Duration:"), m_tab));
    m_recordingDurationInput = new QLineEdit("30", m_tab);
    m_recordingDurationInput->setFixedWidth(50);
    m_recordingDurationInput->setValidator(new QIntValidator(1, 3600, m_tab));
    connect(m_recordingDurationInput, &QLineEdit::textChanged, this, &ActionsModule::updateConfig);
    durationLayout->addWidget(m_recordingDurationInput);
    durationLayout->addWidget(new QLabel(tr("seconds"), m_tab));
    durationLayout->addStretch();
    actionsLayout->addWidget(durationWidget, row, 1, 1, 2);
    row++;

    // Send Telegram Message
    m_sendTelegramCheckbox = new QCheckBox(tr("Send Telegram Message"), m_tab);
    connect(m_sendTelegramCheckbox, &QCheckBox::toggled, this, [this]() {
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_sendTelegramCheckbox, row, 0, 1, 3);
    row++;

    actionsLayout->addWidget(new QLabel(tr("  Bot Token:"), m_tab), row, 0);
    m_telegramBotTokenInput = new QLineEdit(m_tab);
    m_telegramBotTokenInput->setPlaceholderText("123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11");
    m_telegramBotTokenInput->setEchoMode(QLineEdit::Password);
    connect(m_telegramBotTokenInput, &QLineEdit::textChanged, this, &ActionsModule::updateConfig);
    actionsLayout->addWidget(m_telegramBotTokenInput, row, 1, 1, 2);
    row++;

    actionsLayout->addWidget(new QLabel(tr("  Chat ID:"), m_tab), row, 0);
    m_telegramChatIdInput = new QLineEdit(m_tab);
    m_telegramChatIdInput->setPlaceholderText("@channelname or 123456789");
    connect(m_telegramChatIdInput, &QLineEdit::textChanged, this, &ActionsModule::updateConfig);
    actionsLayout->addWidget(m_telegramChatIdInput, row, 1, 1, 2);
    row++;

    // Telegram Image
    m_sendTelegramImageCheckbox = new QCheckBox(tr("  Send captured images to Telegram"), m_tab);
    connect(m_sendTelegramImageCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            if (m_sendTelegramCheckbox && !m_sendTelegramCheckbox->isChecked()) {
                m_sendTelegramCheckbox->setChecked(true);
            }
            if (m_saveImageCheckbox && !m_saveImageCheckbox->isChecked()) {
                m_saveImageCheckbox->setChecked(true);
            }
        }
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_sendTelegramImageCheckbox, row, 0, 1, 3);
    row++;

    // Telegram Video
    m_sendTelegramVideoCheckbox = new QCheckBox(
        tr("  Send recorded videos to Telegram (with ffmpeg conversion)"), m_tab);
    connect(m_sendTelegramVideoCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && !m_isInitializing) {
            if (m_sendTelegramCheckbox && !m_sendTelegramCheckbox->isChecked()) {
                m_sendTelegramCheckbox->setChecked(true);
            }
            if (m_startRecordingCheckbox && !m_startRecordingCheckbox->isChecked()) {
                m_startRecordingCheckbox->setChecked(true);
            }
            emit setCodecMjpegRequested();
            emit enableSignalRecordingRequested();
            emit enableSplitFilesRequested();
            emit setOutputModeFileRequested();
            emit enableAutoNamingRequested();
            emit enableSegmentPatternRequested();
            qDebug() << "[ActionsModule] Telegram Video auto-configuration emitted";
        }
        updateConfig(); updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_sendTelegramVideoCheckbox, row, 0, 1, 3);
    row++;

    // Test Telegram Button
    auto *telegramTestBtn = new QPushButton(tr("Test Telegram"), m_tab);
    connect(telegramTestBtn, &QPushButton::clicked, this, [this]() {
        if (m_action.telegramBotToken.isEmpty()) {
            QMessageBox::warning(m_tab, tr("Telegram Test"), tr("Bitte Bot Token eingeben!"));
            return;
        }
        if (m_action.telegramChatId.isEmpty()) {
            QMessageBox::warning(m_tab, tr("Telegram Test"), tr("Bitte Chat ID eingeben!"));
            return;
        }
        sendTelegramMessage(QString("Test-Nachricht von %1\n")
                                .arg(QLatin1String(AppMeta::NAME))
                            + QDateTime::currentDateTime().toString());
    });
    actionsLayout->addWidget(telegramTestBtn, row, 0);

    connect(m_actionsResetButton, &QPushButton::clicked, this, [this]() {
        if (m_playSoundCheckbox)          m_playSoundCheckbox->setChecked(false);
        if (m_saveImageCheckbox)          m_saveImageCheckbox->setChecked(false);
        if (m_showNotificationCheckbox)   m_showNotificationCheckbox->setChecked(false);
        if (m_runScriptCheckbox)          m_runScriptCheckbox->setChecked(false);
        if (m_startRecordingCheckbox)     m_startRecordingCheckbox->setChecked(false);
        if (m_sendTelegramCheckbox)       m_sendTelegramCheckbox->setChecked(false);
        if (m_sendTelegramImageCheckbox)  m_sendTelegramImageCheckbox->setChecked(false);
        if (m_sendTelegramVideoCheckbox)  m_sendTelegramVideoCheckbox->setChecked(false);
        updateActionsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
    });
    actionsLayout->addWidget(m_actionsResetButton, row, 4);

    helpers.append(CollapsibleHelper::makeCollapsible(
        actionsGroup, "UI/Inference/ActionsGroup", adjustWindowCallback));
    mainLayout->addWidget(actionsGroup);

    // ==========================================================
    // GROUP 3: Filter Settings
    // ==========================================================
    auto *settingsGroup = new QGroupBox(tr("Filter Settings"), m_tab);
    settingsGroup->setStyleSheet(groupStyle);
    auto *settingsLayout = new QGridLayout(settingsGroup);
    settingsLayout->setColumnMinimumWidth(0, 150);
    settingsLayout->setColumnStretch(3, 1);

    m_filterSettingsResetButton = new QPushButton("x", m_tab);
    m_filterSettingsResetButton->setFixedWidth(20);
    m_filterSettingsResetButton->setToolTip(tr("Reset cooldown and confidence to defaults"));

    int settingsRow = 0;

    // Cooldown
    settingsLayout->addWidget(new QLabel(tr("Global Cooldown:"), m_tab), settingsRow, 0);
    m_cooldownSlider = new QSlider(Qt::Horizontal, m_tab);
    m_cooldownSlider->setRange(0, 300);
    m_cooldownSlider->setValue(30);
    m_cooldownSlider->setTickPosition(QSlider::TicksBelow);
    m_cooldownSlider->setTickInterval(30);
    m_cooldownSlider->setToolTip(
        tr("Minimum time between consecutive actions to prevent flooding"));
    settingsLayout->addWidget(m_cooldownSlider, settingsRow, 1);
    m_cooldownInput = new QLineEdit("30", m_tab);
    m_cooldownInput->setFixedWidth(50);
    m_cooldownInput->setValidator(new QIntValidator(0, 300, m_tab));
    settingsLayout->addWidget(m_cooldownInput, settingsRow, 2);
    settingsLayout->addWidget(new QLabel(tr("sec"), m_tab), settingsRow, 3);
    settingsRow++;

    connect(m_cooldownSlider, &QSlider::valueChanged, this, [this](int value) {
        m_cooldownInput->setText(QString::number(value));
        updateConfig(); updateFilterSettingsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
        emit actionsStateChanged();
    });
    connect(m_cooldownInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool ok; int value = text.toInt(&ok);
        if (ok && value >= 0 && value <= 300) m_cooldownSlider->setValue(value);
    });

    // Min Confidence
    settingsLayout->addWidget(new QLabel(tr("Min. Confidence:"), m_tab), settingsRow, 0);
    m_confidenceSlider = new QSlider(Qt::Horizontal, m_tab);
    m_confidenceSlider->setRange(0, 100);
    m_confidenceSlider->setValue(70);
    m_confidenceSlider->setTickPosition(QSlider::TicksBelow);
    m_confidenceSlider->setTickInterval(10);
    m_confidenceSlider->setToolTip(
        tr("Minimum confidence threshold for triggering actions"));
    settingsLayout->addWidget(m_confidenceSlider, settingsRow, 1);
    m_confidenceInput = new QLineEdit("70", m_tab);
    m_confidenceInput->setFixedWidth(50);
    m_confidenceInput->setValidator(new QIntValidator(0, 100, m_tab));
    settingsLayout->addWidget(m_confidenceInput, settingsRow, 2);
    settingsLayout->addWidget(new QLabel("%", m_tab), settingsRow, 3);
    settingsRow++;

    connect(m_confidenceSlider, &QSlider::valueChanged, this, [this](int value) {
        m_confidenceInput->setText(QString::number(value));
        updateConfig(); updateFilterSettingsResetButtonColor();
        if (m_onStateChanged) m_onStateChanged();
        emit actionsStateChanged();
    });
    connect(m_confidenceInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool ok; int value = text.toInt(&ok);
        if (ok && value >= 0 && value <= 100) m_confidenceSlider->setValue(value);
    });

    connect(m_filterSettingsResetButton, &QPushButton::clicked, this, [this]() {
        if (m_cooldownSlider)    m_cooldownSlider->setValue(30);
        if (m_cooldownInput)     m_cooldownInput->setText("30");
        if (m_confidenceSlider)  m_confidenceSlider->setValue(70);
        if (m_confidenceInput)   m_confidenceInput->setText("70");
        // Button direkt schwarz setzen — Werte sind jetzt die echten Defaults
        if (m_filterSettingsResetButton) m_filterSettingsResetButton->setStyleSheet("");
        if (m_onStateChanged) m_onStateChanged();
    });
    settingsLayout->addWidget(m_filterSettingsResetButton, settingsRow, 4, Qt::AlignRight);

    helpers.append(CollapsibleHelper::makeCollapsible(
        settingsGroup, "UI/Inference/SettingsGroup", adjustWindowCallback));
    mainLayout->addWidget(settingsGroup);

    mainLayout->addStretch();

    // Load saved settings (at end of setup, after all widgets created)
    QSettings initSettings(AppPaths::globalConf(), QSettings::IniFormat);
    initSettings.beginGroup(m_tabGroup);
    loadSettings(initSettings);

    m_isInitializing = false;
    qDebug() << "[ActionsModule] Setup complete";
}

// ============================================================
// RESET
// ============================================================

void ActionsModule::resetActionCheckboxes()
{
    if (m_playSoundCheckbox)         m_playSoundCheckbox->setChecked(false);
    if (m_saveImageCheckbox)         m_saveImageCheckbox->setChecked(false);
    if (m_showNotificationCheckbox)  m_showNotificationCheckbox->setChecked(false);
    if (m_runScriptCheckbox)         m_runScriptCheckbox->setChecked(false);
    if (m_startRecordingCheckbox)    m_startRecordingCheckbox->setChecked(false);
    if (m_sendTelegramCheckbox)      m_sendTelegramCheckbox->setChecked(false);
    if (m_sendTelegramImageCheckbox) m_sendTelegramImageCheckbox->setChecked(false);
    if (m_sendTelegramVideoCheckbox) m_sendTelegramVideoCheckbox->setChecked(false);
    qDebug() << "[ActionsModule] All action checkboxes reset to unchecked on startup";
}

void ActionsModule::resetFilterSettings(int cooldown, int confidence)
{
    if (m_cooldownSlider)   m_cooldownSlider->setValue(cooldown);
    if (m_cooldownInput)    m_cooldownInput->setText(QString::number(cooldown));
    if (m_confidenceSlider) m_confidenceSlider->setValue(confidence);
    if (m_confidenceInput)  m_confidenceInput->setText(QString::number(confidence));
    if (m_filterComboBox)   m_filterComboBox->clearCheckedItems();
    updateFilterSettingsResetButtonColor();
}

// ============================================================
// SETTINGS
// ============================================================

void ActionsModule::saveSettings(QSettings &settings)
{
    if (!m_playSoundCheckbox) return;

    QStringList checkedObjects = m_filterComboBox ? m_filterComboBox->getCheckedItems() : QStringList();
    settings.setValue("DetectionActions/FilteredObjects",   checkedObjects);
    settings.setValue("DetectionActions/PlaySound",         m_playSoundCheckbox->isChecked());
    settings.setValue("DetectionActions/SoundFile",         m_soundFileInput->text());
    settings.setValue("DetectionActions/SaveImage",         m_saveImageCheckbox->isChecked());
    settings.setValue("DetectionActions/ImageFolder",       m_imageFolderInput->text());
    settings.setValue("DetectionActions/ShowNotification",  m_showNotificationCheckbox->isChecked());
    settings.setValue("DetectionActions/RunScript",         m_runScriptCheckbox->isChecked());
    settings.setValue("DetectionActions/ScriptPath",        m_scriptPathInput->text());
    settings.setValue("DetectionActions/StartRecording",    m_startRecordingCheckbox->isChecked());
    settings.setValue("DetectionActions/RecordingDuration", m_recordingDurationInput->text().toInt());
    settings.setValue("DetectionActions/SendTelegram",      m_sendTelegramCheckbox->isChecked());
    settings.setValue("DetectionActions/TelegramBotToken",  m_telegramBotTokenInput->text());
    settings.setValue("DetectionActions/TelegramChatId",    m_telegramChatIdInput->text());
    settings.setValue("DetectionActions/SendTelegramImage", m_sendTelegramImageCheckbox->isChecked());
    settings.setValue("DetectionActions/SendTelegramVideo", m_sendTelegramVideoCheckbox->isChecked());
    settings.setValue("DetectionActions/Cooldown",          m_cooldownInput->text().toInt());
    settings.setValue("DetectionActions/MinConfidence",     m_confidenceInput->text().toInt());
    settings.sync();
}

void ActionsModule::loadSettings(QSettings &settings)
{
    if (!m_filterComboBox) return;

    // Load custom objects (list items are preserved, but none are checked on startup)
    QStringList customObjs = settings.value("DetectionActions/CustomObjects").toStringList();
    for (const QString &obj : customObjs) {
        if (!obj.isEmpty()) {
            m_filterComboBox->addCheckableItem(obj, obj);
        }
    }
    // NOTE: FilteredObjects (checked items) are intentionally NOT restored on startup.
    // Filter selections must always start empty to prevent unintended filtering.

    // NOTE: Action checkboxes are intentionally NOT restored on startup.
    // They must always start unchecked to prevent unintended triggers.
    // Only persistent config values (paths, tokens, durations) are loaded.
    if (m_soundFileInput) {
        m_soundFileInput->setText(settings.value("DetectionActions/SoundFile").toString());
    }
    if (m_imageFolderInput) {
        m_imageFolderInput->setText(settings.value("DetectionActions/ImageFolder").toString());
    }
    if (m_scriptPathInput) {
        m_scriptPathInput->setText(settings.value("DetectionActions/ScriptPath").toString());
    }
    if (m_recordingDurationInput) {
        m_recordingDurationInput->setText(settings.value("DetectionActions/RecordingDuration", 30).toString());
    }
    if (m_telegramBotTokenInput) {
        m_telegramBotTokenInput->setText(settings.value("DetectionActions/TelegramBotToken").toString());
    }
    if (m_telegramChatIdInput) {
        m_telegramChatIdInput->setText(settings.value("DetectionActions/TelegramChatId").toString());
    }

    int cooldown   = settings.value("DetectionActions/Cooldown",       30).toInt();
    int confidence = settings.value("DetectionActions/MinConfidence",    0).toInt();
    if (m_cooldownInput) { m_cooldownInput->setText(QString::number(cooldown)); }
    if (m_cooldownSlider) { m_cooldownSlider->setValue(cooldown); }
    if (m_confidenceInput) { m_confidenceInput->setText(QString::number(confidence)); }
    if (m_confidenceSlider) { m_confidenceSlider->setValue(confidence); }

    // Sync m_action struct
    updateConfig();
}

// ============================================================
// BROWSE SLOTS
// ============================================================

void ActionsModule::browseActionSoundFile()
{
    QString startDir = m_soundFileInput ? m_soundFileInput->text() : QString();
    if (!startDir.isEmpty() && QFileInfo(startDir).exists()) {
        startDir = QFileInfo(startDir).absolutePath();
    } else {
        startDir = QDir::currentPath();
    }

    QString file = QFileDialog::getOpenFileName(
        m_tab, tr("Select Sound File"), startDir,
        tr("Sound Files (*.wav *.mp3 *.ogg);;All Files (*)"));
    if (!file.isEmpty() && m_soundFileInput) {
        m_soundFileInput->setText(file);
        updateConfig();
    }
}

void ActionsModule::browseActionImageFolder()
{
    QString startDir = m_imageFolderInput ? m_imageFolderInput->text() : QString();
    if (startDir.isEmpty() || !QFileInfo(startDir).exists()) {
        startDir = QDir::currentPath();
    }

    QString folder = QFileDialog::getExistingDirectory(
        m_tab, tr("Select Image Folder"), startDir, QFileDialog::ShowDirsOnly);
    if (!folder.isEmpty() && m_imageFolderInput) {
        m_imageFolderInput->setText(folder);
        updateConfig();
    }
}

void ActionsModule::browseActionScriptPath()
{
    QString startDir = m_scriptPathInput ? m_scriptPathInput->text() : QString();
    if (!startDir.isEmpty() && QFileInfo(startDir).exists()) {
        startDir = QFileInfo(startDir).absolutePath();
    } else {
        startDir = QDir::currentPath();
    }

    QString file = QFileDialog::getOpenFileName(
        m_tab, tr("Select Script/Program"), startDir,
        tr("Executables (*.sh *.py *.pl);;All Files (*)"));
    if (!file.isEmpty() && m_scriptPathInput) {
        m_scriptPathInput->setText(file);
        updateConfig();
    }
}

// ============================================================
// ADD CUSTOM OBJECT
// ============================================================

void ActionsModule::addCustomObject()
{
    if (!m_customObjectInput || !m_filterComboBox) return;
    QString obj = m_customObjectInput->text().trimmed();
    if (obj.isEmpty()) {
        QMessageBox::warning(m_tab, tr("Empty Object"), tr("Please enter an object name."));
        return;
    }
    for (int i = 0; i < m_filterComboBox->count(); ++i) {
        if (m_filterComboBox->itemText(i).toLower() == obj.toLower()) {
            QMessageBox::information(m_tab, tr("Already Exists"),
                QString(tr("Object '%1' is already in the list.")).arg(obj));
            return;
        }
    }
    m_filterComboBox->addCheckableItem(obj, obj);
    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    s.beginGroup(m_tabGroup);
    QStringList customObjs = s.value("DetectionActions/CustomObjects").toStringList();
    if (!customObjs.contains(obj, Qt::CaseInsensitive)) {
        customObjs.append(obj);
        s.setValue("DetectionActions/CustomObjects", customObjs);
        s.sync();
    }
    m_customObjectInput->clear();
    qDebug() << "[ActionsModule] Added custom object:" << obj;
}

// ============================================================
// UPDATE CONFIG — sync m_action from widgets + save
// ============================================================

void ActionsModule::updateConfig()
{
    if (!m_playSoundCheckbox) return;

    m_action.playSound         = m_playSoundCheckbox->isChecked();
    m_action.soundFile         = m_soundFileInput->text();
    m_action.saveImage         = m_saveImageCheckbox->isChecked();
    m_action.imageFolder       = m_imageFolderInput->text();
    m_action.showNotification  = m_showNotificationCheckbox->isChecked();
    m_action.runScript         = m_runScriptCheckbox->isChecked();
    m_action.scriptPath        = m_scriptPathInput->text();
    m_action.startRecording    = m_startRecordingCheckbox->isChecked();
    m_action.recordingDuration = m_recordingDurationInput->text().toInt();
    m_action.sendTelegram      = m_sendTelegramCheckbox->isChecked();
    m_action.telegramBotToken  = m_telegramBotTokenInput->text();
    m_action.telegramChatId    = m_telegramChatIdInput->text();
    m_action.sendTelegramImage = m_sendTelegramImageCheckbox ? m_sendTelegramImageCheckbox->isChecked() : false;
    m_action.sendTelegramVideo = m_sendTelegramVideoCheckbox ? m_sendTelegramVideoCheckbox->isChecked() : false;
    m_action.cooldownSeconds   = m_cooldownInput->text().toInt();
    m_action.minConfidence     = m_confidenceInput->text().toInt();

    m_action.filteredObjects.clear();
    if (m_filterComboBox) {
        for (const QString &o : m_filterComboBox->getCheckedItems()) {
            m_action.filteredObjects.insert(o);
        }
    }

    if (m_isInitializing) return;

    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    s.beginGroup(m_tabGroup);
    saveSettings(s);
}

// ============================================================
// RESET BUTTON COLORS
// ============================================================

void ActionsModule::updateActionsResetButtonColor()
{
    if (!m_actionsResetButton) return;
    bool anyChecked = (m_playSoundCheckbox         && m_playSoundCheckbox->isChecked())
                   || (m_saveImageCheckbox          && m_saveImageCheckbox->isChecked())
                   || (m_showNotificationCheckbox   && m_showNotificationCheckbox->isChecked())
                   || (m_runScriptCheckbox          && m_runScriptCheckbox->isChecked())
                   || (m_startRecordingCheckbox     && m_startRecordingCheckbox->isChecked())
                   || (m_sendTelegramCheckbox       && m_sendTelegramCheckbox->isChecked())
                   || (m_sendTelegramImageCheckbox  && m_sendTelegramImageCheckbox->isChecked())
                   || (m_sendTelegramVideoCheckbox  && m_sendTelegramVideoCheckbox->isChecked());
    m_actionsResetButton->setStyleSheet(anyChecked
        ? "QPushButton { color: red; font-weight: bold; }" : "");
}

void ActionsModule::updateFilterResetButtonColor()
{
    if (!m_filterResetButton || !m_filterComboBox) return;
    bool hasFilter = !m_filterComboBox->getCheckedItems().isEmpty();
    m_filterResetButton->setStyleSheet(hasFilter
        ? "QPushButton { color: red; font-weight: bold; }" : "");
}

void ActionsModule::updateFilterSettingsResetButtonColor()
{
    if (!m_filterSettingsResetButton) return;
    int cooldown   = m_cooldownInput   ? m_cooldownInput->text().toInt()   : 30;
    int confidence = m_confidenceInput ? m_confidenceInput->text().toInt() : 70;
    bool nonDefault = (cooldown != 30 || confidence != 70);
    m_filterSettingsResetButton->setStyleSheet(nonDefault
        ? "QPushButton { color: red; font-weight: bold; }" : "");
}

// ============================================================
// CAPTURE CURRENT FRAME — reads from CameraInterface::getBoxCoords
// ============================================================

void ActionsModule::captureCurrentFrame(const QString &filename)
{
    qDebug() << "[ActionsModule] captureCurrentFrame:" << filename;

    QFileInfo fileInfo(filename);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists() && !dir.mkpath(".")) {
        qDebug() << "[ActionsModule] ERROR: Cannot create dir:" << dir.absolutePath();
        return;
    }

    // If BoxInput coords are unavailable, capture via rpicam-jpeg
    QString boxCoords;
    if (m_camera.getBoxCoords) {
        boxCoords = m_camera.getBoxCoords();
    }

    if (boxCoords.isEmpty()) {
        qDebug() << "[ActionsModule] BoxInput empty — capturing via rpicam-jpeg";
        QStringList args;
        args << "-o" << filename << "-n" << "--immediate";
        bool ok = QProcess::startDetached("rpicam-jpeg", args);
        qDebug() << (ok ? "[ActionsModule] rpicam-jpeg started" : "[ActionsModule] ERROR: rpicam-jpeg failed to start");
        return;
    }
    QStringList coords = boxCoords.split(',');
    if (coords.size() != 4) {
        qDebug() << "[ActionsModule] ERROR: Invalid BoxInput format:" << boxCoords;
        return;
    }
    bool ok;
    int x = coords[0].toInt(&ok);      if (!ok) return;
    int y = coords[1].toInt(&ok);      if (!ok) return;
    int width  = coords[2].toInt(&ok); if (!ok) return;
    int height = coords[3].toInt(&ok); if (!ok) return;

    int menuBarHeight = (height > 600) ? 0 : 30;
    y += menuBarHeight;
    width  += 2;

    QRect previewRect(x, y, width, height);
    previewRect.translate(0, 2);
    previewRect.setWidth(previewRect.width() - 2);
    previewRect.setHeight(previewRect.height() + 6);

    QScreen *screen = QGuiApplication::screenAt(previewRect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QPixmap screenshot = screen->grabWindow(0);
    QRect cropRect = previewRect;
    cropRect.moveTo(previewRect.topLeft() - screen->geometry().topLeft());
    QPixmap cropped = screenshot.copy(cropRect);

    if (cropped.save(filename, "JPG", 95)) {
        qDebug() << "[ActionsModule] Screenshot saved:" << filename;
    } else {
        qDebug() << "[ActionsModule] ERROR: Failed to save screenshot:" << filename;
    }
}

// ============================================================
// EXECUTE DETECTION ACTIONS
// ============================================================

void ActionsModule::executeDetection(const QString &object,
                                       int confidence,
                                       const QString &fullDetection)
{
    Q_UNUSED(fullDetection)
    qDebug() << "[ActionsModule] executeDetection — object:" << object
             << "confidence:" << confidence;

    // Cooldown check
    if (m_action.cooldownSeconds > 0) {
        qint64 elapsedMs = m_lastActionTime.isNull()
                         ? m_action.cooldownSeconds * 1000LL
                         : m_lastActionTime.msecsTo(QDateTime::currentDateTime());
        if (elapsedMs < m_action.cooldownSeconds * 1000LL) {
            qDebug() << "[ActionsModule] Cooldown active — skipping";
            return;
        }
    }

    // Min confidence filter
    if (confidence > 0 && confidence < m_action.minConfidence) {
        qDebug() << "[ActionsModule] Confidence" << confidence
                 << "below threshold" << m_action.minConfidence << "— skipping";
        return;
    }

    // Object filter
    if (!m_action.filteredObjects.isEmpty()
            && !m_action.filteredObjects.contains("*")
            && !m_action.filteredObjects.contains(object)) {
        qDebug() << "[ActionsModule] Object" << object << "not in filter — skipping";
        return;
    }

    m_lastActionTime = QDateTime::currentDateTime();

    // ---- Play Sound ----
    if (m_action.playSound && !m_action.soundFile.isEmpty()) {
        if (QFile::exists(m_action.soundFile)) {
            QStringList players = {"aplay", "ffplay", "paplay"};
            QString player;
            for (const QString &p : players) {
                QProcess testProc;
                testProc.start("which", QStringList() << p);
                testProc.waitForFinished(1000);
                if (testProc.exitCode() == 0) { player = p; break; }
            }
            if (!player.isEmpty()) {
                QStringList args;
                if (player == "ffplay") {
                    args << "-nodisp" << "-autoexit" << m_action.soundFile;
                } else {
                    args << m_action.soundFile;
                }
                QProcess::startDetached(player, args);
            }
        }
    }

    // ---- Save Image ----
    if (m_action.saveImage) {
        QString folder;
        if (m_action.imageFolder.isEmpty()) {
            // Fall back to the output path configured in Setup dialog
            // IMPORTANT: key is stored under beginGroup(m_tabGroup)
            QSettings pathSettings(AppPaths::globalConf(), QSettings::IniFormat);
            pathSettings.beginGroup(m_tabGroup);
            folder = AppPaths::sanitizeIfSystemPath(pathSettings.value("Paths/GuiOutputPath", AppPaths::contentOutput()).toString(), AppPaths::contentOutput());
            pathSettings.endGroup();
        } else {
            folder = m_action.imageFolder;
        }
        folder = QDir::cleanPath(folder);
        qDebug() << "[ActionsModule] Image folder:" << folder;
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
        QString filename  = QString("%1/%2_%3_%4.jpg")
                                .arg(folder, object)
                                .arg(confidence).arg(timestamp);
        captureCurrentFrame(filename);

        if (m_action.sendTelegramImage && m_action.sendTelegram
                && !m_action.telegramBotToken.isEmpty()
                && !m_action.telegramChatId.isEmpty()) {
            QTimer::singleShot(500, this, [this, filename, object, confidence]() {
                if (QFile::exists(filename)) {
                    QString caption = QString("Object Detection\n\nObject: %1\nConfidence: %2%%\nTime: %3")
                        .arg(object).arg(confidence)
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
                    sendTelegramPhoto(filename, caption);
                }
            });
        }
    }

    // ---- Show Notification ----
    if (m_action.showNotification) {
        QString msg = QString("%1 detected (%2% confidence)").arg(object).arg(confidence);
        QSystemTrayIcon *tray = m_tab ? m_tab->findChild<QSystemTrayIcon*>() : nullptr;
        if (tray && tray->isVisible()) {
            tray->showMessage(tr("Object Detected"), msg, QSystemTrayIcon::Information, 5000);
        } else {
            QTimer::singleShot(0, this, [this, msg]() {
                QMessageBox msgBox(m_tab);
                msgBox.setWindowTitle(tr("Object Detected"));
                msgBox.setText(msg);
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();
            });
        }
    }

    // ---- Run Script ----
    if (m_action.runScript && !m_action.scriptPath.isEmpty()) {
        if (QFile::exists(m_action.scriptPath)) {
            QStringList args;
            args << object << QString::number(confidence)
                 << QDateTime::currentDateTime().toString(Qt::ISODate);
            QProcess::startDetached(m_action.scriptPath, args);
        }
    }

    // ---- Start Recording (via SIGUSR1 or timed recording) ----
    if (m_action.startRecording) {
        bool vidRunning = m_camera.isVidRunning && m_camera.isVidRunning();
        bool signalOk   = m_camera.isSignalRecordingEnabled && m_camera.isSignalRecordingEnabled();
        qint64 pid      = (m_camera.getProcessId && vidRunning) ? m_camera.getProcessId() : 0;
        QString curApp  = m_camera.getCurrentApp ? m_camera.getCurrentApp() : QString();

        if (vidRunning && pid > 0 && curApp == "rpicam-vid") {
            if (signalOk) {
                qDebug() << "[ActionsModule] Sending SIGUSR1 to PID:" << pid;
                if (kill(static_cast<pid_t>(pid), SIGUSR1) == 0) {
                    qDebug() << "[ActionsModule] SIGUSR1 sent — scheduling stop after"
                             << m_action.recordingDuration << "s";

                    // Nach Aufnahmedauer: SIGUSR1 Stop senden, dann (optional) Video per Telegram
                    bool doTelegram = m_action.sendTelegramVideo
                                   && m_action.sendTelegram
                                   && !m_action.telegramBotToken.isEmpty()
                                   && !m_action.telegramChatId.isEmpty();

                    QTimer::singleShot(m_action.recordingDuration * 1000,
                                       this, [this, pid, object, confidence, doTelegram]() {
                        bool stillRunning = m_camera.isVidRunning && m_camera.isVidRunning();
                        qint64 curPid     = m_camera.getProcessId ? m_camera.getProcessId() : 0;
                        if (stillRunning && curPid == pid) {
                            kill(static_cast<pid_t>(pid), SIGUSR1);
                            qDebug() << "[ActionsModule] Stop SIGUSR1 sent";
                        }

                        if (!doTelegram) return;

                        // 10s warten bis rpicam-vid das Segment auf die Platte geschrieben hat
                        qDebug() << "[ActionsModule] Waiting 10s for video file write...";
                        QTimer::singleShot(10000, this, [this, object, confidence]() {
                            // Konfigurierten Output-Pfad holen (z.B. "/path/to/video.mjpeg")
                            QString configuredOutput = m_camera.getOutputFileName
                                                     ? m_camera.getOutputFileName() : QString();
                            qDebug() << "[ActionsModule][Telegram] Configured output:" << configuredOutput;

                            if (configuredOutput.isEmpty()) {
                                qDebug() << "[ActionsModule][Telegram] ERROR: No output file configured";
                                return;
                            }

                            // Mit --split erzeugt rpicam-vid: base0001.ext, base0002.ext, ...
                            QFileInfo outInfo(configuredOutput);
                            QString folder    = outInfo.absolutePath();
                            QString baseName  = outInfo.completeBaseName(); // ohne Extension
                            QString extension = outInfo.suffix();           // z.B. "mjpeg"

                            qDebug() << "[ActionsModule][Telegram] Folder:" << folder
                                     << "BaseName:" << baseName << "Ext:" << extension;

                            QDir dir(folder);
                            QStringList filters;
                            filters << baseName + "*." + extension;
                            QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);
                            qDebug() << "[ActionsModule][Telegram] Found" << files.size()
                                     << "file(s) matching" << filters;

                            if (files.isEmpty()) {
                                qDebug() << "[ActionsModule][Telegram] No matching video file found in:"
                                         << folder;
                                return;
                            }

                            QString latestVideo = files.first().absoluteFilePath();
                            qint64  fileSize    = QFileInfo(latestVideo).size();
                            qDebug() << "[ActionsModule][Telegram] Latest video:" << latestVideo
                                     << "Size:" << fileSize << "bytes";

                            if (fileSize == 0) {
                                qDebug() << "[ActionsModule][Telegram] ERROR: File has 0 bytes";
                                return;
                            }

                            QString caption = QString("Object Detection Video\n\nObject: %1\nConfidence: %2%%\nDuration: %3s\nTime: %4")
                                                  .arg(object).arg(confidence)
                                                  .arg(m_action.recordingDuration)
                                                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
                            sendTelegramVideo(latestVideo, caption);
                        });
                    });
                }
            }
        } else {
            // Kein Prozess läuft oder falscher App-Typ → Timed Recording anfordern
            emit startTimedRecordingRequested(m_action.recordingDuration);
        }
    }
}

// ============================================================
// TELEGRAM HELPERS
// ============================================================

void ActionsModule::sendTelegramMessage(const QString &message)
{
    if (m_action.telegramBotToken.isEmpty() || m_action.telegramChatId.isEmpty()) return;

    QString urlStr = QString("https://api.telegram.org/bot%1/sendMessage")
                         .arg(m_action.telegramBotToken);
    QUrl reqUrl(urlStr);
    QNetworkRequest req(reqUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("chat_id",    m_action.telegramChatId);
    params.addQueryItem("text",       message);
    params.addQueryItem("parse_mode", "HTML");

    QNetworkReply *reply = m_networkManager->post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[ActionsModule] Telegram message error:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

void ActionsModule::sendTelegramPhoto(const QString &filePath, const QString &caption)
{
    if (m_action.telegramBotToken.isEmpty() || m_action.telegramChatId.isEmpty()) return;

    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "[ActionsModule] Cannot open photo:" << filePath;
        delete file;
        return;
    }

    QString urlStr = QString("https://api.telegram.org/bot%1/sendPhoto")
                         .arg(m_action.telegramBotToken);

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart chatPart;
    chatPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"chat_id\""));
    chatPart.setBody(m_action.telegramChatId.toUtf8());
    multiPart->append(chatPart);

    QHttpPart captionPart;
    captionPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant("form-data; name=\"caption\""));
    captionPart.setBody(caption.toUtf8());
    multiPart->append(captionPart);

    QHttpPart photoPart;
    photoPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QString("form-data; name=\"photo\"; filename=\"%1\"")
                                     .arg(QFileInfo(filePath).fileName())));
    photoPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    file->setParent(multiPart);
    photoPart.setBodyDevice(file);
    multiPart->append(photoPart);

    QUrl photoUrl(urlStr);
    QNetworkRequest req(photoUrl);
    QNetworkReply *reply = m_networkManager->post(req, multiPart);
    multiPart->setParent(reply);
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[ActionsModule] Telegram photo error:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

void ActionsModule::sendTelegramVideo(const QString &filePath, const QString &caption)
{
    if (m_action.telegramBotToken.isEmpty() || m_action.telegramChatId.isEmpty()) return;

    // Convert to MP4 with ffmpeg first (Telegram doesn't accept H.264 raw)
    QString outputPath = QFileInfo(filePath).absolutePath()
                       + "/telegram_" + QFileInfo(filePath).baseName() + ".mp4";

    auto *ffconvertProcess = new QProcess(this);
    ffconvertProcess->setProgram("ffmpeg");
    ffconvertProcess->setArguments({"-y", "-i", filePath,
                                    "-c:v", "libx264", "-preset", "fast",
                                    "-crf", "28", "-movflags", "+faststart",
                                    outputPath});

    connect(ffconvertProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, outputPath, caption, filePath, ffconvertProcess]
                  (int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if (exitCode != 0 || !QFile::exists(outputPath)) {
            qDebug() << "[ActionsModule] ffmpeg conversion failed";
            ffconvertProcess->deleteLater();
            return;
        }

        QString urlStr = QString("https://api.telegram.org/bot%1/sendVideo")
                             .arg(m_action.telegramBotToken);

        QFile *file = new QFile(outputPath);
        if (!file->open(QIODevice::ReadOnly)) {
            qDebug() << "[ActionsModule] Cannot open converted video:" << outputPath;
            delete file;
            ffconvertProcess->deleteLater();
            return;
        }

        auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        QHttpPart chatPart;
        chatPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant("form-data; name=\"chat_id\""));
        chatPart.setBody(m_action.telegramChatId.toUtf8());
        multiPart->append(chatPart);

        QHttpPart captionPart;
        captionPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                              QVariant("form-data; name=\"caption\""));
        captionPart.setBody(caption.toUtf8());
        multiPart->append(captionPart);

        QHttpPart videoPart;
        videoPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            QVariant(QString("form-data; name=\"video\"; filename=\"%1\"")
                                         .arg(QFileInfo(outputPath).fileName())));
        videoPart.setHeader(QNetworkRequest::ContentTypeHeader,
                            QVariant("video/mp4"));
        file->setParent(multiPart);
        videoPart.setBodyDevice(file);
        multiPart->append(videoPart);

        QUrl videoUrl(urlStr);
        QNetworkRequest req(videoUrl);
        QNetworkReply *reply = m_networkManager->post(req, multiPart);
        multiPart->setParent(reply);
        connect(reply, &QNetworkReply::finished, this, [reply, outputPath, ffconvertProcess]() {
            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "[ActionsModule] Telegram video error:" << reply->errorString();
            }
            QFile::remove(outputPath);
            reply->deleteLater();
            ffconvertProcess->deleteLater();
        });
    });

    ffconvertProcess->start();
}
