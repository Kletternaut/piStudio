#include "GstLaunchModule.h"
#include "../../gui/CollapsibleHelper.h"
#include "../../utils/AppPaths.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QSettings>
#include <QTimer>
#include <QSet>
#include <QDebug>

// ============================================================
// Construction / Destruction
// ============================================================

GstLaunchModule::GstLaunchModule(const QString &tabGroup, QObject *parent)
    : QObject(parent), m_tabGroup(tabGroup)
{
}

GstLaunchModule::~GstLaunchModule()
{
    // Stop all running stream viewers
    for (auto &tab : m_streamViewerTabs) {
        if (tab.process && tab.process->state() != QProcess::NotRunning) {
            tab.process->terminate();
            tab.process->waitForFinished(1000);
        }
    }
    // Stop all running test sources
    for (auto &tab : m_testSourceTabs) {
        if (tab.process && tab.process->state() != QProcess::NotRunning) {
            tab.process->terminate();
            tab.process->waitForFinished(1000);
        }
    }
}

// ============================================================
// SETUP — equivalent of MainWindow::setupGstLaunchTab()
// ============================================================

void GstLaunchModule::setup(QList<CollapsibleHelper *> &helpers,
                             std::function<void()> adjustWindowCallback)
{
    m_tab = new QWidget;
    auto *mainLayout = new QVBoxLayout(m_tab);

    const QString groupStyle =
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
        "    padding: 0 5px;"
        "    color: #333333;"
        "}";

    const QString tabStyle =
        "QTabWidget::pane {"
        "    border: 2px solid #cccccc;"
        "    background-color: white;"
        "}"
        "QTabBar::tab {"
        "    background-color: #f0f0f0;"
        "    color: #333333;"
        "    padding: 4px 12px;"
        "    margin-right: 1px;"
        "    border: 2px solid #cccccc;"
        "    border-bottom: none;"
        "    border-top-left-radius: 3px;"
        "    border-top-right-radius: 3px;"
        "    min-width: 60px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: white;"
        "    color: #333333;"
        "    border-bottom-color: white;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background-color: #e8e8e8;"
        "}";

    const QString plusBtnStyle =
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 16px;"
        "    border: none;"
        "    border-radius: 4px;"
        "    outline: none;"
        "}"
        "QPushButton:hover {"
        "    background-color: #5dade2;"
        "}"
        "QPushButton:focus {"
        "    outline: none;"
        "}";

    // ==========================================================
    // STREAM VIEWER GROUP
    // ==========================================================
    auto *viewerGroup = new QGroupBox(tr("Stream Viewer"), m_tab);
    viewerGroup->setStyleSheet(groupStyle);
    auto *viewerGroupLayout = new QVBoxLayout(viewerGroup);

    m_streamViewerTabWidget = new QTabWidget(m_tab);
    m_streamViewerTabWidget->setStyleSheet(tabStyle);
    m_streamViewerTabWidget->setTabsClosable(false);
    m_streamViewerTabWidget->setUsesScrollButtons(true);

    // First stream tab init
    addStreamViewerTab();

    // "+" button in corner
    auto *cornerContainer = new QWidget(m_tab);
    auto *cornerLayout = new QHBoxLayout(cornerContainer);
    cornerLayout->setContentsMargins(5, 2, 5, 2);
    cornerLayout->setSpacing(0);
    auto *addTabButton = new QPushButton("+", cornerContainer);
    addTabButton->setFixedSize(28, 28);
    addTabButton->setStyleSheet(plusBtnStyle);
    addTabButton->setToolTip(tr("Add new stream viewer"));
    cornerLayout->addWidget(addTabButton);
    cornerContainer->setLayout(cornerLayout);
    m_streamViewerTabWidget->setCornerWidget(cornerContainer, Qt::TopRightCorner);

    connect(addTabButton, &QPushButton::clicked, this, [this]() {
        addStreamViewerTab();
        if (m_streamViewerTabWidget->count() > 1) {
            m_streamViewerTabWidget->setTabsClosable(true);
        }
    });

    connect(m_streamViewerTabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int widgetIndex) {
        if (m_streamViewerTabWidget->count() <= 1) {
            QMessageBox::information(m_tab, tr("Info"),
                tr("Cannot close the last stream viewer tab."));
            return;
        }
        QWidget *tabWidget = m_streamViewerTabWidget->widget(widgetIndex);
        int tabIndexToRemove = -1;
        for (auto it = m_streamViewerTabs.begin(); it != m_streamViewerTabs.end(); ++it) {
            if (it.value().widget == tabWidget) {
                tabIndexToRemove = it.key();
                auto &tab = it.value();
                if (tab.process && tab.process->state() != QProcess::NotRunning) {
                    tab.process->terminate();
                    if (!tab.process->waitForFinished(2000)) {
                        tab.process->kill();
                    }
                }
                break;
            }
        }
        if (tabIndexToRemove >= 0) {
            m_streamViewerTabs.remove(tabIndexToRemove);
        }
        m_streamViewerTabWidget->removeTab(widgetIndex);
        for (int i = 0; i < m_streamViewerTabWidget->count(); ++i) {
            QWidget *w = m_streamViewerTabWidget->widget(i);
            for (auto it = m_streamViewerTabs.begin(); it != m_streamViewerTabs.end(); ++it) {
                if (it.value().widget == w && it.value().nameEdit) {
                    QString name = it.value().nameEdit->text().trimmed();
                    if (name.isEmpty()) name = tr("Stream %1").arg(i + 1);
                    m_streamViewerTabWidget->setTabText(i, name);
                    break;
                }
            }
        }
        if (m_streamViewerTabWidget->count() == 1) {
            m_streamViewerTabWidget->setTabsClosable(false);
        }
    });

    viewerGroupLayout->addWidget(m_streamViewerTabWidget);
    helpers.append(CollapsibleHelper::makeCollapsible(
        viewerGroup, "UI/GST/ViewerGroup", adjustWindowCallback));
    mainLayout->addWidget(viewerGroup);

    // ==========================================================
    // TEST SOURCE GROUP
    // ==========================================================
    auto *testGroup = new QGroupBox(tr("Test Source Sender"), m_tab);
    testGroup->setStyleSheet(groupStyle);
    auto *testGroupLayout = new QVBoxLayout(testGroup);

    m_testSourceTabWidget = new QTabWidget(m_tab);
    m_testSourceTabWidget->setStyleSheet(tabStyle);
    m_testSourceTabWidget->setTabsClosable(false);
    m_testSourceTabWidget->setUsesScrollButtons(true);

    addTestSourceTab();

    auto *testCornerContainer = new QWidget(m_tab);
    auto *testCornerLayout = new QHBoxLayout(testCornerContainer);
    testCornerLayout->setContentsMargins(5, 2, 5, 2);
    testCornerLayout->setSpacing(0);
    auto *addTestTabButton = new QPushButton("+", testCornerContainer);
    addTestTabButton->setFixedSize(28, 28);
    addTestTabButton->setStyleSheet(plusBtnStyle);
    addTestTabButton->setToolTip(tr("Add new test source sender"));
    testCornerLayout->addWidget(addTestTabButton);
    testCornerContainer->setLayout(testCornerLayout);
    m_testSourceTabWidget->setCornerWidget(testCornerContainer, Qt::TopRightCorner);

    connect(addTestTabButton, &QPushButton::clicked, this, [this]() {
        addTestSourceTab();
        if (m_testSourceTabWidget->count() > 1) {
            m_testSourceTabWidget->setTabsClosable(true);
        }
    });

    connect(m_testSourceTabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int widgetIndex) {
        if (m_testSourceTabWidget->count() <= 1) {
            QMessageBox::information(m_tab, tr("Info"),
                tr("Cannot close the last test source tab."));
            return;
        }
        QWidget *tabWidget = m_testSourceTabWidget->widget(widgetIndex);
        int tabIndexToRemove = -1;
        for (auto it = m_testSourceTabs.begin(); it != m_testSourceTabs.end(); ++it) {
            if (it.value().widget == tabWidget) {
                tabIndexToRemove = it.key();
                auto &tab = it.value();
                if (tab.process && tab.process->state() != QProcess::NotRunning) {
                    tab.process->terminate();
                    if (!tab.process->waitForFinished(2000)) {
                        tab.process->kill();
                    }
                }
                break;
            }
        }
        if (tabIndexToRemove >= 0) {
            m_testSourceTabs.remove(tabIndexToRemove);
        }
        m_testSourceTabWidget->removeTab(widgetIndex);
        for (int i = 0; i < m_testSourceTabWidget->count(); ++i) {
            m_testSourceTabWidget->setTabText(i, tr("Sender %1").arg(i + 1));
        }
        if (m_testSourceTabWidget->count() == 1) {
            m_testSourceTabWidget->setTabsClosable(false);
        }
    });

    testGroupLayout->addWidget(m_testSourceTabWidget);
    helpers.append(CollapsibleHelper::makeCollapsible(
        testGroup, "UI/GST/TestGroup", adjustWindowCallback));
    mainLayout->addWidget(testGroup);

    // ==========================================================
    // RECORDING & OPTIONS GROUP
    // ==========================================================
    auto *optionsGroup = new QGroupBox(tr("Recording & Options"), m_tab);
    optionsGroup->setStyleSheet(groupStyle);
    auto *optionsLayout = new QGridLayout(optionsGroup);

    m_recordCheckBox = new QCheckBox(tr("Record to File"), m_tab);
    m_recordCheckBox->setToolTip(tr("Save stream to file"));
    optionsLayout->addWidget(m_recordCheckBox, 0, 0);

    m_recordFileEdit = new QLineEdit(m_tab);
    m_recordFileEdit->setPlaceholderText(tr("/path/to/output.mkv"));
    m_recordFileEdit->setEnabled(false);
    optionsLayout->addWidget(m_recordFileEdit, 0, 1);

    m_recordBrowseButton = new QPushButton(tr("Browse..."), m_tab);
    m_recordBrowseButton->setEnabled(false);
    optionsLayout->addWidget(m_recordBrowseButton, 0, 2);

    connect(m_recordCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_recordFileEdit->setEnabled(checked);
        m_recordBrowseButton->setEnabled(checked);
    });
    connect(m_recordBrowseButton, &QPushButton::clicked,
            this, &GstLaunchModule::browseRecordFile);

    m_eosCheckBox = new QCheckBox(tr("EOS on Shutdown"), m_tab);
    m_eosCheckBox->setToolTip(tr("Force EOS event on shutdown for proper file finalization (-e)"));
    optionsLayout->addWidget(m_eosCheckBox, 1, 0);

    m_savePipelineButton = new QPushButton(tr("Save Pipeline Config"), m_tab);
    m_savePipelineButton->setToolTip(tr("Save current pipeline configuration"));
    optionsLayout->addWidget(m_savePipelineButton, 1, 2);

    connect(m_savePipelineButton, &QPushButton::clicked,
            this, &GstLaunchModule::saveSettings);

    helpers.append(CollapsibleHelper::makeCollapsible(
        optionsGroup, "UI/GST/OptionsGroup", adjustWindowCallback));
    mainLayout->addWidget(optionsGroup);

    // Debug Output group
    auto *debugGroup = new QGroupBox(tr("Debug Output"), m_tab);
    debugGroup->setStyleSheet(groupStyle);
    auto *debugGroupLayout = new QGridLayout(debugGroup);

    m_gstDebugLogCheckBox = new QCheckBox(tr("GST debug"), m_tab);
    m_gstDebugLogCheckBox->setToolTip(tr("When checked, GST process output is forwarded to the main log (requires Debug Level > 0)"));
    debugGroupLayout->addWidget(m_gstDebugLogCheckBox, 0, 0);

    auto *debugSpacer = new QWidget(m_tab);
    debugSpacer->setFixedWidth(50);
    debugGroupLayout->addWidget(debugSpacer, 0, 1);

    auto *debugLevelLayout = new QHBoxLayout;
    debugLevelLayout->addWidget(new QLabel(tr("Debug Level"), m_tab));
    m_debugLevelSpinBox = new QSpinBox(m_tab);
    m_debugLevelSpinBox->setRange(0, 9);
    m_debugLevelSpinBox->setValue(0);
    m_debugLevelSpinBox->setToolTip(tr("GStreamer debug level\n0=none, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG, 5=LOG, 6=TRACE, 7+=MEMDUMP\nOnly levels >= 4 produce visible output on a running pipeline"));
    debugLevelLayout->addWidget(m_debugLevelSpinBox);
    debugGroupLayout->addLayout(debugLevelLayout, 0, 2);
    debugGroupLayout->setColumnStretch(3, 1);

    helpers.append(CollapsibleHelper::makeCollapsible(
        debugGroup, "UI/GST/DebugOutputGroup", adjustWindowCallback));
    mainLayout->addWidget(debugGroup);

    mainLayout->addStretch();

    // Load saved settings
    loadSettings();
}

// ============================================================
// SETTINGS
// ============================================================

void GstLaunchModule::saveSettings()
{
    if (!m_streamViewerPortSpinBox) return;

    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    settings.setValue("GST/Port",           m_streamViewerPortSpinBox    ? m_streamViewerPortSpinBox->value()            : 8554);
    settings.setValue("GST/Codec",          m_streamViewerCodecSelector  ? m_streamViewerCodecSelector->currentText()    : "H264");
    settings.setValue("GST/Payload",        m_streamViewerPayloadSpinBox ? m_streamViewerPayloadSpinBox->value()         : 96);
    settings.setValue("GST/Sync",           m_streamViewerSyncCheckBox   ? m_streamViewerSyncCheckBox->isChecked()       : false);
    settings.setValue("GST/TestPattern",    m_testSourceSelector         ? m_testSourceSelector->currentText()           : "SMPTE Color Bars");
    settings.setValue("GST/TestHost",       m_testHostEdit               ? m_testHostEdit->text()                        : "127.0.0.1");
    settings.setValue("GST/TestPort",       m_testPortSpinBox            ? m_testPortSpinBox->value()                   : 8554);
    settings.setValue("GST/RecordEnabled",  m_recordCheckBox             ? m_recordCheckBox->isChecked()                : false);
    settings.setValue("GST/RecordFile",     m_recordFileEdit             ? m_recordFileEdit->text()                     : "");
    settings.setValue("GST/EOS",            m_eosCheckBox                ? m_eosCheckBox->isChecked()                   : false);
    settings.setValue("GST/DebugLevel",     m_debugLevelSpinBox          ? m_debugLevelSpinBox->value()                 : 0);

    if (m_savePipelineButton) {
        // Show confirmation only on explicit user save
        QMessageBox::information(m_tab, tr("Saved"), tr("Pipeline configuration saved successfully!"));
    }
}

void GstLaunchModule::loadSettings()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    if (!settings.contains("GST/Port")) return;

    if (m_streamViewerPortSpinBox)    m_streamViewerPortSpinBox->setValue(          settings.value("GST/Port",          8554).toInt());
    if (m_streamViewerCodecSelector)  m_streamViewerCodecSelector->setCurrentText(  settings.value("GST/Codec",         "H264").toString());
    if (m_streamViewerPayloadSpinBox) m_streamViewerPayloadSpinBox->setValue(        settings.value("GST/Payload",       96).toInt());
    if (m_streamViewerSyncCheckBox)   m_streamViewerSyncCheckBox->setChecked(        settings.value("GST/Sync",          false).toBool());

    if (m_testSourceSelector) {
        QString p = settings.value("GST/TestPattern", "SMPTE Color Bars").toString();
        int idx = m_testSourceSelector->findText(p);
        if (idx != -1) m_testSourceSelector->setCurrentIndex(idx);
    }
    if (m_testHostEdit)      m_testHostEdit->setText(        settings.value("GST/TestHost",     "127.0.0.1").toString());
    if (m_testPortSpinBox)   m_testPortSpinBox->setValue(    settings.value("GST/TestPort",     8554).toInt());
    if (m_recordCheckBox)    m_recordCheckBox->setChecked(   settings.value("GST/RecordEnabled",false).toBool());
    if (m_recordFileEdit)    m_recordFileEdit->setText(      settings.value("GST/RecordFile",   "").toString());
    if (m_eosCheckBox)              m_eosCheckBox->setChecked(      settings.value("GST/EOS",          false).toBool());
    if (m_gstDebugLogCheckBox)      m_gstDebugLogCheckBox->setChecked(settings.value("GST/DebugLog",    false).toBool());
    if (m_debugLevelSpinBox)        m_debugLevelSpinBox->setValue(  settings.value("GST/DebugLevel",   0).toInt());
}

// ============================================================
// BROWSE RECORD FILE
// ============================================================

void GstLaunchModule::browseRecordFile()
{
    QString file = QFileDialog::getSaveFileName(
        m_tab,
        tr("Select Output File"),
        QString(),
        tr("Video Files (*.mkv *.mp4 *.avi);;All Files (*)")
    );
    if (!file.isEmpty() && m_recordFileEdit) {
        m_recordFileEdit->setText(file);
    }
}

// ============================================================
// STREAM VIEWER — addStreamViewerTab()
// Ported from MainWindow::addStreamViewerTab()
// ============================================================

void GstLaunchModule::addStreamViewerTab()
{
    int tabIndex = m_streamViewerTabCounter++;

    auto *tabWidget = new QWidget;
    auto *tabLayout = new QGridLayout(tabWidget);

    StreamViewerTab tab;
    tab.widget = tabWidget;

    // Find first available port
    int nextPort = 8554;
    QSet<int> usedPorts;
    for (const auto &existingTab : m_streamViewerTabs) {
        if (existingTab.portSpinBox) {
            usedPorts.insert(existingTab.portSpinBox->value());
        }
    }
    while (usedPorts.contains(nextPort)) nextPort++;

    // Row 0: Viewer Name
    auto *nameLabel = new QLabel(tr("Viewer Name:"));
    nameLabel->setMinimumWidth(120);
    tabLayout->addWidget(nameLabel, 0, 0);
    tab.nameEdit = new QLineEdit;
    tab.nameEdit->setMaximumWidth(150);
    tab.nameEdit->setText(QString("Stream %1").arg(m_streamViewerTabWidget->count() + 1));
    tab.nameEdit->setPlaceholderText(tr("Enter viewer name"));
    tab.nameEdit->setToolTip(tr("Custom name for this viewer tab"));
    tabLayout->addWidget(tab.nameEdit, 0, 1);

    // Row 0: Codec
    auto *codecLabel = new QLabel(tr("Codec:"));
    codecLabel->setMinimumWidth(120);
    tabLayout->addWidget(codecLabel, 0, 2);
    tab.codecSelector = new QComboBox;
    tab.codecSelector->setFixedWidth(120);
    tab.codecSelector->addItems({"H264", "H265", "MJPEG"});
    tab.codecSelector->setCurrentText("H264");
    tab.codecSelector->setToolTip(tr("Expected video codec"));
    tabLayout->addWidget(tab.codecSelector, 0, 3);

    // Row 1: UDP Port
    auto *portLabel = new QLabel(tr("UDP Port:"));
    portLabel->setMinimumWidth(120);
    tabLayout->addWidget(portLabel, 1, 0);
    tab.portSpinBox = new QSpinBox;
    tab.portSpinBox->setMaximumWidth(150);
    tab.portSpinBox->setRange(1000, 65535);
    tab.portSpinBox->setValue(nextPort);
    tab.portSpinBox->setToolTip(tr("UDP port to receive stream from"));
    tabLayout->addWidget(tab.portSpinBox, 1, 1);

    // Row 1: RTP Payload
    auto *payloadLabel = new QLabel(tr("RTP Payload:"));
    payloadLabel->setMinimumWidth(120);
    tabLayout->addWidget(payloadLabel, 1, 2);
    tab.payloadSpinBox = new QSpinBox;
    tab.payloadSpinBox->setFixedWidth(120);
    tab.payloadSpinBox->setRange(96, 127);
    tab.payloadSpinBox->setValue(96);
    tab.payloadSpinBox->setToolTip(tr("RTP payload type"));
    tabLayout->addWidget(tab.payloadSpinBox, 1, 3);

    // Row 2: Preview Size
    auto *previewLabel = new QLabel(tr("Preview Size:"));
    previewLabel->setMinimumWidth(120);
    tabLayout->addWidget(previewLabel, 2, 0);
    tab.previewSizeSelector = new QComboBox;

    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    QString size1 = settings.value("StreamViewer/PreviewSize1", "1920x1080").toString();
    QString size2 = settings.value("StreamViewer/PreviewSize2", "1280x720").toString();
    QString size3 = settings.value("StreamViewer/PreviewSize3", "640x480").toString();
    QString size4 = settings.value("StreamViewer/PreviewSize4", "800x600").toString();
    tab.previewSizeSelector->addItem(size1);
    tab.previewSizeSelector->addItem(size2);
    tab.previewSizeSelector->addItem(size3);
    tab.previewSizeSelector->addItem(size4);
    tab.previewSizeSelector->setCurrentIndex(0);
    tab.previewSizeSelector->setEditable(true);
    tab.previewSizeSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    tab.previewSizeSelector->setMinimumContentsLength(10);
    tab.previewSizeSelector->setMaximumWidth(150);
    tab.previewSizeSelector->setToolTip(tr("Preview window size (4 editable entries)"));

    connect(tab.previewSizeSelector->lineEdit(), &QLineEdit::editingFinished,
            this, [this, tabIndex]() {
        if (!m_streamViewerTabs.contains(tabIndex)) return;
        auto &t = m_streamViewerTabs[tabIndex];
        QString text = t.previewSizeSelector->currentText().trimmed();
        int currentIdx = t.previewSizeSelector->currentIndex();
        if (!text.isEmpty() && text.contains('x')) {
            bool isNew = true;
            for (int i = 0; i < t.previewSizeSelector->count(); i++) {
                if (t.previewSizeSelector->itemText(i) == text) { isNew = false; break; }
            }
            if (isNew && currentIdx >= 0 && currentIdx < 4) {
                t.previewSizeSelector->setItemText(currentIdx, text);
                QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
                s.beginGroup(m_tabGroup);
                s.setValue(QString("StreamViewer/PreviewSize%1").arg(currentIdx + 1), text);
            }
        }
    });
    tabLayout->addWidget(tab.previewSizeSelector, 2, 1);

    // Row 2: Sync
    auto *syncLabel = new QLabel(tr("Sync:"));
    syncLabel->setMinimumWidth(120);
    tabLayout->addWidget(syncLabel, 2, 2);
    tab.syncCheckBox = new QCheckBox;
    tab.syncCheckBox->setChecked(false);
    tab.syncCheckBox->setToolTip(tr("Enable/disable synchronization"));
    tabLayout->addWidget(tab.syncCheckBox, 2, 3);

    // Row 3: Start/Stop Button
    const QString startBtnStyle =
        "QPushButton {"
        "    background-color: #3498db; color: white; font-weight: bold;"
        "    padding: 8px 16px; border: none; border-radius: 4px; outline: none;"
        "}"
        "QPushButton:hover { background-color: #5dade2; }"
        "QPushButton:checked { background-color: #e74c3c; }"
        "QPushButton:checked:hover { background-color: #c0392b; }"
        "QPushButton:disabled { background-color: #95a5a6; }";

    auto *buttonLayout = new QHBoxLayout;
    tab.startButton = new QPushButton(tr("Start Viewer"));
    tab.startButton->setCheckable(true);
    tab.startButton->setStyleSheet(startBtnStyle);
    tab.startButton->setToolTip(tr("Start/Stop stream viewer"));
    buttonLayout->addWidget(tab.startButton);
    buttonLayout->addStretch();
    tabLayout->addLayout(buttonLayout, 3, 0, 1, 4);

    connect(tab.nameEdit, &QLineEdit::textChanged, this, [this, tabIndex](const QString &text) {
        if (!m_streamViewerTabs.contains(tabIndex)) return;
        for (int i = 0; i < m_streamViewerTabWidget->count(); ++i) {
            if (m_streamViewerTabWidget->widget(i) == m_streamViewerTabs[tabIndex].widget) {
                QString name = text.trimmed().isEmpty()
                             ? QString("Stream %1").arg(tabIndex + 1)
                             : text.trimmed();
                m_streamViewerTabWidget->setTabText(i, name);
                break;
            }
        }
    });

    connect(tab.startButton, &QPushButton::clicked, this, [this, tabIndex](bool checked) {
        if (!m_streamViewerTabs.contains(tabIndex)) return;
        auto &t = m_streamViewerTabs[tabIndex];
        if (checked) {
            t.startButton->setText(tr("Stop Viewer"));
            startStreamViewerForTab(tabIndex);
        } else {
            t.startButton->setText(tr("Start Viewer"));
            stopStreamViewerForTab(tabIndex);
        }
    });

    m_streamViewerTabs[tabIndex] = tab;

    QString tabName = QString("Stream %1").arg(m_streamViewerTabWidget->count() + 1);
    int newTabIndex = m_streamViewerTabWidget->addTab(tabWidget, tabName);
    m_streamViewerTabWidget->setCurrentIndex(newTabIndex);

    // Legacy pointers for first tab
    if (tabIndex == 0) {
        m_streamViewerPortSpinBox    = tab.portSpinBox;
        m_streamViewerCodecSelector  = tab.codecSelector;
        m_streamViewerPayloadSpinBox = tab.payloadSpinBox;
        m_streamViewerSyncCheckBox   = tab.syncCheckBox;
        m_streamViewerStartButton    = tab.startButton;
        m_streamViewerProcess        = nullptr;
    }
}

// ============================================================
// STREAM VIEWER — startStreamViewer() (legacy, delegates to ForTab)
// ============================================================

void GstLaunchModule::startStreamViewer()
{
    if (!m_streamViewerPortSpinBox || !m_streamViewerCodecSelector ||
        !m_streamViewerPayloadSpinBox || !m_streamViewerSyncCheckBox ||
        !m_streamViewerStartButton) {
        return;
    }
    if (m_streamViewerProcess && m_streamViewerProcess->state() != QProcess::NotRunning) {
        stopStreamViewer();
    }
    startStreamViewerForTab(0);
}

void GstLaunchModule::stopStreamViewer()
{
    if (!m_streamViewerProcess || !m_streamViewerStartButton) return;
    if (m_streamViewerProcess->state() != QProcess::NotRunning) {
        m_streamViewerProcess->terminate();
        if (!m_streamViewerProcess->waitForFinished(2000)) {
            m_streamViewerProcess->kill();
        }
    }
    m_streamViewerProcess->deleteLater();
    m_streamViewerProcess = nullptr;
    if (m_streamViewerPortSpinBox)    m_streamViewerPortSpinBox->setEnabled(true);
    if (m_streamViewerCodecSelector)  m_streamViewerCodecSelector->setEnabled(true);
    if (m_streamViewerPayloadSpinBox) m_streamViewerPayloadSpinBox->setEnabled(true);
    if (m_streamViewerSyncCheckBox)   m_streamViewerSyncCheckBox->setEnabled(true);
}

// ============================================================
// STREAM VIEWER — startStreamViewerForTab()
// ============================================================

void GstLaunchModule::startStreamViewerForTab(int tabIndex)
{
    if (!m_streamViewerTabs.contains(tabIndex)) return;
    auto &tab = m_streamViewerTabs[tabIndex];

    if (!tab.portSpinBox || !tab.codecSelector || !tab.payloadSpinBox ||
        !tab.syncCheckBox || !tab.startButton) {
        return;
    }

    if (tab.process && tab.process->state() != QProcess::NotRunning) {
        stopStreamViewerForTab(tabIndex);
    }

    int      port    = tab.portSpinBox->value();
    QString  codec   = tab.codecSelector->currentText();
    int      payload = tab.payloadSpinBox->value();
    bool     sync    = tab.syncCheckBox->isChecked();

    int width = 1920, height = 1080;
    if (tab.previewSizeSelector) {
        QStringList parts = tab.previewSizeSelector->currentText().split('x');
        if (parts.size() == 2) { width = parts[0].toInt(); height = parts[1].toInt(); }
    }

    QString depay, parse, decoder;
    if (codec == "H264")       { depay = "rtph264depay"; parse = "h264parse"; decoder = "avdec_h264"; }
    else if (codec == "H265")  { depay = "rtph265depay"; parse = "h265parse"; decoder = "avdec_h265"; }
    else if (codec == "MJPEG") { depay = "rtpjpegdepay"; parse = "jpegparse"; decoder = "jpegdec"; }

    QString pipeline = QString(
        "udpsrc port=%1 "
        "caps=\"application/x-rtp, media=video, encoding-name=%2, payload=%3\" "
        "! %4 ! %5 ! %6"
    ).arg(port).arg(codec.toUpper()).arg(payload).arg(depay).arg(parse).arg(decoder);

    QString streamName = tab.nameEdit ? tab.nameEdit->text().trimmed() : QString();
    if (streamName.isEmpty()) streamName = QString("Stream %1").arg(tabIndex + 1);

    pipeline += QString(" ! videoconvert ! videoscale ! video/x-raw,width=%1,height=%2 ! autovideosink sync=%3")
                    .arg(width).arg(height).arg(sync ? "true" : "false");

    QString command = "gst-launch-1.0";
    if (m_eosCheckBox && m_eosCheckBox->isChecked()) command += " -e";
    if (m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
        command += QString(" --gst-debug-level=%1").arg(m_debugLevelSpinBox->value());
    }
    command += " " + pipeline;

    tab.process = new QProcess(this);

    if (m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
        connect(tab.process, &QProcess::readyReadStandardOutput,
                this, [this, tabIndex]() {
            if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
                    m_streamViewerTabs.contains(tabIndex) &&
                    m_streamViewerTabs[tabIndex].process) {
                QString out = QString::fromLocal8Bit(
                    m_streamViewerTabs[tabIndex].process->readAllStandardOutput());
                if (!out.trimmed().isEmpty())
                    emit debugLog(QString("[GST Stream %1] %2").arg(tabIndex+1).arg(out.trimmed()));
            }
        });
        connect(tab.process, &QProcess::readyReadStandardError,
                this, [this, tabIndex]() {
            if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
                    m_streamViewerTabs.contains(tabIndex) &&
                    m_streamViewerTabs[tabIndex].process) {
                QString err = QString::fromLocal8Bit(
                    m_streamViewerTabs[tabIndex].process->readAllStandardError());
                if (!err.trimmed().isEmpty())
                    emit debugLog(QString("[GST Stream %1] %2").arg(tabIndex+1).arg(err.trimmed()));
            }
        });
    }

    connect(tab.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, tabIndex](int exitCode, QProcess::ExitStatus exitStatus) {
        if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
                m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
            emit debugLog(QString("[GST Stream %1] finished (Exit Code: %2, Status: %3)")
                              .arg(tabIndex+1).arg(exitCode)
                              .arg(exitStatus == QProcess::NormalExit ? "Normal" : "Crashed"));
        }
        stopStreamViewerForTab(tabIndex);
    });

    connect(tab.process, &QProcess::errorOccurred,
            this, [this, tabIndex](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked()) {
                emit debugLog(QString("[GST Stream %1] ERROR: Failed to start. Make sure GStreamer is installed.").arg(tabIndex+1));
            }
            QMessageBox::warning(m_tab, tr("Stream Viewer Error"),
                QString(tr("Failed to start Stream %1. Make sure GStreamer is installed."))
                    .arg(tabIndex+1));
            stopStreamViewerForTab(tabIndex);
        }
    });

    if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
            m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
        emit debugLog(QString("[GST Stream %1] Starting...").arg(tabIndex+1));
        emit debugLog(QString("[GST Stream %1] Port: %2, Codec: %3").arg(tabIndex+1).arg(port).arg(codec));
    }

    // Remove GST_TRACERS from child environment to prevent
    // GstShark (hailo-tappas-core) from creating gstshark_* trace directories
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("GST_TRACERS");
    tab.process->setProcessEnvironment(env);

    tab.process->start("bash", QStringList() << "-c" << command);

    // Try to rename window via xdotool after startup
    QTimer::singleShot(800, this, [this, tabIndex, streamName]() {
        if (m_streamViewerTabs.contains(tabIndex) && m_streamViewerTabs[tabIndex].process) {
            QString cmd = QString(
                "WINDOW=$(xdotool search --name 'gst-launch-1.0' 2>/dev/null | tail -1); "
                "if [ -z \"$WINDOW\" ]; then "
                "  WINDOW=$(xdotool search --name 'OpenGL renderer' 2>/dev/null | tail -1); "
                "fi; "
                "if [ -n \"$WINDOW\" ]; then "
                "  xdotool set_window --name '%1' $WINDOW 2>/dev/null; "
                "fi"
            ).arg(streamName);
            QProcess::execute("bash", QStringList() << "-c" << cmd);
        }
    });
    QTimer::singleShot(2000, this, [this, tabIndex, streamName]() {
        if (m_streamViewerTabs.contains(tabIndex) && m_streamViewerTabs[tabIndex].process) {
            QString cmd = QString(
                "WINDOW=$(xdotool search --name 'gst-launch-1.0' 2>/dev/null | tail -1); "
                "if [ -z \"$WINDOW\" ]; then "
                "  WINDOW=$(xdotool search --name 'OpenGL renderer' 2>/dev/null | tail -1); "
                "fi; "
                "if [ -n \"$WINDOW\" ]; then "
                "  xdotool set_window --name '%1' $WINDOW 2>/dev/null; "
                "fi"
            ).arg(streamName);
            QProcess::execute("bash", QStringList() << "-c" << cmd);
        }
    });

    tab.portSpinBox->setEnabled(false);
    tab.codecSelector->setEnabled(false);
    tab.payloadSpinBox->setEnabled(false);
    tab.syncCheckBox->setEnabled(false);
    if (tab.previewSizeSelector) tab.previewSizeSelector->setEnabled(false);

    if (tabIndex == 0) m_streamViewerProcess = tab.process;
}

// ============================================================
// STREAM VIEWER — stopStreamViewerForTab()
// ============================================================

void GstLaunchModule::stopStreamViewerForTab(int tabIndex)
{
    if (!m_streamViewerTabs.contains(tabIndex)) return;
    auto &tab = m_streamViewerTabs[tabIndex];
    if (!tab.process || !tab.startButton) return;

    if (tab.process->state() != QProcess::NotRunning) {
        tab.process->terminate();
        if (!tab.process->waitForFinished(2000)) {
            tab.process->kill();
        }
    }
    tab.process->deleteLater();
    tab.process = nullptr;

    if (tab.portSpinBox)         tab.portSpinBox->setEnabled(true);
    if (tab.codecSelector)       tab.codecSelector->setEnabled(true);
    if (tab.payloadSpinBox)      tab.payloadSpinBox->setEnabled(true);
    if (tab.syncCheckBox)        tab.syncCheckBox->setEnabled(true);
    if (tab.previewSizeSelector) tab.previewSizeSelector->setEnabled(true);
    if (tab.startButton) {
        tab.startButton->setChecked(false);
        tab.startButton->setText(tr("Start Viewer"));
    }
    if (tabIndex == 0) m_streamViewerProcess = nullptr;
}

// ============================================================
// TEST SOURCE — addTestSourceTab()
// ============================================================

void GstLaunchModule::addTestSourceTab()
{
    int tabIndex = m_testSourceTabCounter++;

    auto *tabWidget = new QWidget;
    auto *tabLayout = new QGridLayout(tabWidget);

    TestSourceTab tab;
    tab.widget = tabWidget;

    const QString startBtnStyle =
        "QPushButton {"
        "    background-color: #3498db; color: white; font-weight: bold;"
        "    padding: 8px 16px; border: none; border-radius: 4px; outline: none;"
        "}"
        "QPushButton:hover { background-color: #5dade2; }"
        "QPushButton:checked { background-color: #e74c3c; }"
        "QPushButton:checked:hover { background-color: #c0392b; }"
        "QPushButton:disabled { background-color: #95a5a6; }";

    auto *patternLabel = new QLabel(tr("Pattern:"));
    patternLabel->setMinimumWidth(120);
    tabLayout->addWidget(patternLabel, 0, 0);
    tab.sourceSelector = new QComboBox;
    tab.sourceSelector->addItems({
        "SMPTE Color Bars", "Snow (Random)", "Black", "White", "Checkers", "Circular"
    });
    tab.sourceSelector->setToolTip(tr("Sends H264 test pattern via UDP."));
    tabLayout->addWidget(tab.sourceSelector, 0, 1);

    auto *hostLabel = new QLabel(tr("Target Host:"));
    hostLabel->setMinimumWidth(120);
    tabLayout->addWidget(hostLabel, 0, 2);
    tab.hostEdit = new QLineEdit("127.0.0.1");
    tab.hostEdit->setToolTip(tr("Target IP address (use 127.0.0.1 for local testing)"));
    tab.hostEdit->setFixedWidth(120);
    tabLayout->addWidget(tab.hostEdit, 0, 3);

    auto *portLabel = new QLabel(tr("Target Port:"));
    portLabel->setMinimumWidth(120);
    tabLayout->addWidget(portLabel, 1, 0);
    tab.portSpinBox = new QSpinBox;
    tab.portSpinBox->setRange(1000, 65535);
    tab.portSpinBox->setValue(8554 + tabIndex);
    tab.portSpinBox->setToolTip(tr("Target UDP port (must match receiver port)"));
    tabLayout->addWidget(tab.portSpinBox, 1, 1);

    auto *buttonLayout = new QHBoxLayout;
    tab.startButton = new QPushButton(tr("Start Sender"));
    tab.startButton->setCheckable(true);
    tab.startButton->setStyleSheet(startBtnStyle);
    tab.startButton->setToolTip(tr("Start/Stop test source sender"));
    buttonLayout->addWidget(tab.startButton);
    buttonLayout->addStretch();
    tabLayout->addLayout(buttonLayout, 2, 0, 1, 4);

    connect(tab.startButton, &QPushButton::clicked, this, [this, tabIndex](bool checked) {
        if (!m_testSourceTabs.contains(tabIndex)) return;
        auto &t = m_testSourceTabs[tabIndex];
        if (checked) {
            t.startButton->setText(tr("Stop Sender"));
            startGstTestForTab(tabIndex);
        } else {
            t.startButton->setText(tr("Start Sender"));
            stopGstTestForTab(tabIndex);
        }
    });

    m_testSourceTabs[tabIndex] = tab;

    QString tabName = QString("Sender %1").arg(m_testSourceTabWidget->count() + 1);
    int newTabIndex = m_testSourceTabWidget->addTab(tabWidget, tabName);
    m_testSourceTabWidget->setCurrentIndex(newTabIndex);

    // Legacy pointers for first tab
    if (tabIndex == 0) {
        m_testSourceSelector = tab.sourceSelector;
        m_testHostEdit       = tab.hostEdit;
        m_testPortSpinBox    = tab.portSpinBox;
        m_testStartButton    = tab.startButton;
        m_testProcess        = nullptr;
    }
}

// ============================================================
// TEST SOURCE — startGstTest() / stopGstTest() (legacy)
// ============================================================

void GstLaunchModule::startGstTest()
{
    if (!m_testSourceSelector || !m_testHostEdit || !m_testPortSpinBox || !m_testStartButton) {
        return;
    }
    if (m_testProcess && m_testProcess->state() != QProcess::NotRunning) {
        stopGstTest();
    }
    startGstTestForTab(0);
}

void GstLaunchModule::stopGstTest()
{
    if (!m_testProcess || !m_testStartButton) return;
    if (m_testProcess->state() != QProcess::NotRunning) {
        m_testProcess->terminate();
        if (!m_testProcess->waitForFinished(2000)) {
            m_testProcess->kill();
        }
    }
    m_testProcess->deleteLater();
    m_testProcess = nullptr;
    if (m_testSourceSelector) m_testSourceSelector->setEnabled(true);
    if (m_testHostEdit)       m_testHostEdit->setEnabled(true);
    if (m_testPortSpinBox)    m_testPortSpinBox->setEnabled(true);
}

// ============================================================
// TEST SOURCE — startGstTestForTab()
// ============================================================

static QString patternToGst(const QString &pattern)
{
    if (pattern == "SMPTE Color Bars") return "smpte";
    if (pattern == "Snow (Random)")    return "snow";
    if (pattern == "Black")            return "black";
    if (pattern == "White")            return "white";
    if (pattern == "Checkers")         return "checkers-1";
    if (pattern == "Circular")         return "circular";
    return "smpte";
}

void GstLaunchModule::startGstTestForTab(int tabIndex)
{
    if (!m_testSourceTabs.contains(tabIndex)) return;
    auto &tab = m_testSourceTabs[tabIndex];
    if (!tab.sourceSelector || !tab.hostEdit || !tab.portSpinBox || !tab.startButton) return;

    if (tab.process && tab.process->state() != QProcess::NotRunning) {
        stopGstTestForTab(tabIndex);
    }

    QString pattern    = tab.sourceSelector->currentText();
    QString host       = tab.hostEdit->text();
    int     port       = tab.portSpinBox->value();
    QString patternNum = patternToGst(pattern);

    QString pipeline = QString(
        "videotestsrc pattern=%1 ! "
        "video/x-raw,width=1280,height=720,framerate=30/1 ! "
        "x264enc tune=zerolatency bitrate=2000 speed-preset=ultrafast key-int-max=30 ! "
        "video/x-h264,profile=baseline ! "
        "rtph264pay pt=96 config-interval=1 ! "
        "udpsink host=%2 port=%3"
    ).arg(patternNum).arg(host).arg(port);

    QString command = "gst-launch-1.0";
    if (m_eosCheckBox && m_eosCheckBox->isChecked()) command += " -e";
    if (m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
        command += QString(" --gst-debug-level=%1").arg(m_debugLevelSpinBox->value());
    }
    command += " " + pipeline;

    tab.process = new QProcess(this);

    if (m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
        connect(tab.process, &QProcess::readyReadStandardOutput,
                this, [this, tabIndex]() {
            if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
                    m_testSourceTabs.contains(tabIndex) &&
                    m_testSourceTabs[tabIndex].process) {
                QString out = QString::fromLocal8Bit(
                    m_testSourceTabs[tabIndex].process->readAllStandardOutput());
                if (!out.trimmed().isEmpty())
                    emit debugLog(QString("[GST Sender %1] %2").arg(tabIndex+1).arg(out.trimmed()));
            }
        });
        connect(tab.process, &QProcess::readyReadStandardError,
                this, [this, tabIndex]() {
            if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
                    m_testSourceTabs.contains(tabIndex) &&
                    m_testSourceTabs[tabIndex].process) {
                QString err = QString::fromLocal8Bit(
                    m_testSourceTabs[tabIndex].process->readAllStandardError());
                if (!err.trimmed().isEmpty())
                    emit debugLog(QString("[GST Sender %1] %2").arg(tabIndex+1).arg(err.trimmed()));
            }
        });
    }

    connect(tab.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, tabIndex](int exitCode, QProcess::ExitStatus exitStatus) {
        if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
                m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
            emit debugLog(QString("[GST Sender %1] finished (Exit Code: %2, Status: %3)")
                              .arg(tabIndex+1).arg(exitCode)
                              .arg(exitStatus == QProcess::NormalExit ? "Normal" : "Crashed"));
        }
        stopGstTestForTab(tabIndex);
    });

    connect(tab.process, &QProcess::errorOccurred,
            this, [this, tabIndex](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked()) {
                emit debugLog(QString("[GST Sender %1] ERROR: Failed to start. Make sure GStreamer is installed.").arg(tabIndex+1));
            }
            QMessageBox::warning(m_tab, tr("Test Source Error"),
                QString(tr("Failed to start Sender %1. Make sure GStreamer is installed."))
                    .arg(tabIndex+1));
            stopGstTestForTab(tabIndex);
        }
    });

    if (m_gstDebugLogCheckBox && m_gstDebugLogCheckBox->isChecked() &&
            m_debugLevelSpinBox && m_debugLevelSpinBox->value() > 0) {
        emit debugLog(QString("[GST Sender %1] Starting...").arg(tabIndex+1));
        emit debugLog(QString("[GST Sender %1] Pattern: %2, Target: %3:%4").arg(tabIndex+1).arg(pattern).arg(host).arg(port));
    }

    // Remove GST_TRACERS from child environment to prevent
    // GstShark (hailo-tappas-core) from creating gstshark_* trace directories
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("GST_TRACERS");
    tab.process->setProcessEnvironment(env);

    tab.process->start("bash", QStringList() << "-c" << command);

    tab.sourceSelector->setEnabled(false);
    tab.hostEdit->setEnabled(false);
    tab.portSpinBox->setEnabled(false);

    if (tabIndex == 0) m_testProcess = tab.process;
}

// ============================================================
// TEST SOURCE — stopGstTestForTab()
// ============================================================

void GstLaunchModule::stopGstTestForTab(int tabIndex)
{
    if (!m_testSourceTabs.contains(tabIndex)) return;
    auto &tab = m_testSourceTabs[tabIndex];
    if (!tab.process || !tab.startButton) return;

    if (tab.process->state() != QProcess::NotRunning) {
        tab.process->terminate();
        if (!tab.process->waitForFinished(2000)) {
            tab.process->kill();
        }
    }
    tab.process->deleteLater();
    tab.process = nullptr;

    if (tab.sourceSelector) tab.sourceSelector->setEnabled(true);
    if (tab.hostEdit)       tab.hostEdit->setEnabled(true);
    if (tab.portSpinBox)    tab.portSpinBox->setEnabled(true);
    if (tab.startButton) {
        tab.startButton->setChecked(false);
        tab.startButton->setText(tr("Start Sender"));
    }
    if (tabIndex == 0) m_testProcess = nullptr;
}
