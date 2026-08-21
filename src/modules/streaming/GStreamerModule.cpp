#include "GStreamerModule.h"
#include "../../gui/CollapsibleHelper.h"
#include "../../utils/AppPaths.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDebug>

GStreamerModule::GStreamerModule(const QString &tabGroup, QObject *parent)
    : QObject(parent), m_tabGroup(tabGroup)
{
}

void GStreamerModule::setup(QList<CollapsibleHelper*> &helpers,
                             std::function<void()> adjustWindowCallback,
                             QComboBox *resolutionSelector)
{
    m_resolutionSelector = resolutionSelector;
    m_tab = new QWidget;
    auto *tabLayout = new QVBoxLayout(m_tab);

    // Note: Tab is enabled/disabled via Output Mode radio buttons in MainWindow.
    m_tab->setEnabled(false); // Default: disabled (File mode is default)

    // =============================================================
    // STREAMING TARGET GROUP
    // =============================================================
    auto *targetGroup = new QGroupBox(tr("Streaming Target"), m_tab);
    targetGroup->setStyleSheet(
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
        "}"
    );
    auto *targetLayout = new QGridLayout(targetGroup);

    targetLayout->addWidget(new QLabel(tr("Target:"), m_tab), 0, 0);
    m_targetSelector = new QComboBox(m_tab);
    m_targetSelector->addItems({
        tr("UDP Streaming"),
        tr("RTSP Server"),
        tr("File Output"),
        tr("Multicast")
    });
    m_targetSelector->setToolTip(tr("Select streaming target type"));
    targetLayout->addWidget(m_targetSelector, 0, 1);

    targetLayout->addWidget(new QLabel(tr("Host:"), m_tab), 1, 0);
    m_hostEdit = new QLineEdit("192.168.0.30", m_tab);
    m_hostEdit->setToolTip(tr("Target IP address for streaming"));
    targetLayout->addWidget(m_hostEdit, 1, 1);

    targetLayout->addWidget(new QLabel(tr("Port:"), m_tab), 1, 2);
    m_portSpinBox = new QSpinBox(m_tab);
    m_portSpinBox->setRange(1000, 65535);
    m_portSpinBox->setValue(8554);
    m_portSpinBox->setToolTip(tr("Target port for streaming"));
    targetLayout->addWidget(m_portSpinBox, 1, 3);

    helpers.append(CollapsibleHelper::makeCollapsible(
        targetGroup, "UI/GStreamer/TargetGroup", adjustWindowCallback));
    tabLayout->addWidget(targetGroup);

    // =============================================================
    // PIPELINE CONFIGURATION GROUP
    // =============================================================
    auto *pipelineGroup = new QGroupBox(tr("Pipeline Configuration"), m_tab);
    pipelineGroup->setStyleSheet(
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
        "}"
    );
    auto *pipelineLayout = new QGridLayout(pipelineGroup);

    pipelineLayout->addWidget(new QLabel(tr("Container:"), m_tab), 0, 0);
    m_formatSelector = new QComboBox(m_tab);
    m_formatSelector->addItems({"mpegts", "mp4", "rtp", "raw"});
    m_formatSelector->setCurrentText("mpegts");
    m_formatSelector->setToolTip(tr("GStreamer container format"));
    pipelineLayout->addWidget(m_formatSelector, 0, 1);

    pipelineLayout->addWidget(new QLabel(tr("Parser:"), m_tab), 0, 2);
    m_parserSelector = new QComboBox(m_tab);
    m_parserSelector->addItems({"h264parse", "h265parse", "auto"});
    m_parserSelector->setCurrentText("h264parse");
    m_parserSelector->setToolTip(tr("Video stream parser"));
    pipelineLayout->addWidget(m_parserSelector, 0, 3);

    pipelineLayout->addWidget(new QLabel(tr("Payload:"), m_tab), 1, 0);
    m_payloadSelector = new QComboBox(m_tab);
    m_payloadSelector->addItems({"rtph264pay", "rtph265pay", "rtpjpegpay"});
    m_payloadSelector->setCurrentText("rtph264pay");
    m_payloadSelector->setToolTip(tr("RTP payload format"));
    pipelineLayout->addWidget(m_payloadSelector, 1, 1);

    pipelineLayout->addWidget(new QLabel(tr("Config Interval:"), m_tab), 1, 2);
    m_configInterval = new QSpinBox(m_tab);
    m_configInterval->setRange(1, 60);
    m_configInterval->setValue(1);
    m_configInterval->setToolTip(tr("SPS/PPS insertion interval"));
    pipelineLayout->addWidget(m_configInterval, 1, 3);

    pipelineLayout->addWidget(new QLabel(tr("Payload Type:"), m_tab), 2, 0);
    m_payloadType = new QSpinBox(m_tab);
    m_payloadType->setRange(96, 127);
    m_payloadType->setValue(96);
    m_payloadType->setToolTip(tr("RTP payload type number"));
    pipelineLayout->addWidget(m_payloadType, 2, 1);

    m_syncCheckBox = new QCheckBox(tr("Sync"), m_tab);
    m_syncCheckBox->setChecked(false);
    m_syncCheckBox->setToolTip(tr("Enable/disable pipeline synchronization"));
    pipelineLayout->addWidget(m_syncCheckBox, 2, 2);

    pipelineLayout->addWidget(new QLabel(tr("Width:"), m_tab), 3, 0);
    m_streamWidthInput = new QLineEdit(m_tab);
    m_streamWidthInput->setPlaceholderText(tr("e.g. 1920"));
    pipelineLayout->addWidget(m_streamWidthInput, 3, 1);

    pipelineLayout->addWidget(new QLabel(tr("Height:"), m_tab), 3, 2);
    m_streamHeightInput = new QLineEdit(m_tab);
    m_streamHeightInput->setPlaceholderText(tr("e.g. 1080"));
    pipelineLayout->addWidget(m_streamHeightInput, 3, 3);

    // Aspect-Ratio-Berechnung: Width → Height
    connect(m_streamWidthInput, &QLineEdit::textChanged,
            this, [this](const QString &text) {
        if (!m_resolutionSelector) return;
        bool ok;
        int width = text.toInt(&ok);
        if (!ok || width <= 0) return;
        QString camSize = m_resolutionSelector->currentText();
        QStringList parts = camSize.split('x');
        if (parts.size() == 2) {
            int cw = parts[0].toInt(), ch = parts[1].toInt();
            if (cw > 0 && ch > 0) {
                int height = qRound(width / ((double)cw / ch));
                m_streamHeightInput->blockSignals(true);
                m_streamHeightInput->setText(QString::number(height));
                m_streamHeightInput->blockSignals(false);
            }
        }
    });

    // Aspect-Ratio-Berechnung: Height → Width
    connect(m_streamHeightInput, &QLineEdit::textChanged,
            this, [this](const QString &text) {
        if (!m_resolutionSelector) return;
        bool ok;
        int height = text.toInt(&ok);
        if (!ok || height <= 0) return;
        QString camSize = m_resolutionSelector->currentText();
        QStringList parts = camSize.split('x');
        if (parts.size() == 2) {
            int cw = parts[0].toInt(), ch = parts[1].toInt();
            if (cw > 0 && ch > 0) {
                int width = qRound(height * ((double)cw / ch));
                m_streamWidthInput->blockSignals(true);
                m_streamWidthInput->setText(QString::number(width));
                m_streamWidthInput->blockSignals(false);
            }
        }
    });

    helpers.append(CollapsibleHelper::makeCollapsible(
        pipelineGroup, "UI/GStreamer/PipelineGroup", adjustWindowCallback));
    tabLayout->addWidget(pipelineGroup);

    tabLayout->addStretch();
}

QString GStreamerModule::buildPipeline() const
{
    QString target       = m_targetSelector   ? m_targetSelector->currentText()   : "UDP Streaming";
    QString host         = m_hostEdit         ? m_hostEdit->text()                : "192.168.0.30";
    int     port         = m_portSpinBox      ? m_portSpinBox->value()            : 8554;
    QString format       = m_formatSelector   ? m_formatSelector->currentText()   : "mpegts";
    QString parser       = m_parserSelector   ? m_parserSelector->currentText()   : "h264parse";
    QString payload      = m_payloadSelector  ? m_payloadSelector->currentText()  : "rtph264pay";
    int     configIntvl  = m_configInterval   ? m_configInterval->value()         : 1;
    int     payloadType  = m_payloadType      ? m_payloadType->value()            : 96;
    bool    sync         = m_syncCheckBox     ? m_syncCheckBox->isChecked()       : false;

    QString pipeline = "gst-launch-1.0 fdsrc fd=0 ! ";

    if (format == "mpegts") {
        pipeline += "tsdemux ! ";
    }

    if (parser != "auto") {
        pipeline += parser + " ! ";
    }

    if (target == "UDP Streaming" || target == "RTSP Server" || target == "Multicast") {
        pipeline += QString("%1 config-interval=%2 pt=%3 ! ")
                        .arg(payload).arg(configIntvl).arg(payloadType);
    }

    if (target == "UDP Streaming") {
        pipeline += QString("udpsink host=%1 port=%2 sync=%3")
                        .arg(host).arg(port).arg(sync ? "true" : "false");
    } else if (target == "RTSP Server") {
        pipeline += QString("udpsink host=%1 port=%2 sync=false").arg(host).arg(port);
    } else if (target == "Multicast") {
        pipeline += QString("udpsink host=%1 port=%2 auto-multicast=true sync=false").arg(host).arg(port);
    } else if (target == "File Output") {
        pipeline += "filesink location=/tmp/gstreamer_output.mp4";
    }

    qDebug().noquote() << "[GStreamerModule] Generated pipeline:" << pipeline;
    return pipeline;
}

void GStreamerModule::saveSettings(QSettings &settings)
{
    settings.setValue("GStreamer/Target",       m_targetSelector   ? m_targetSelector->currentText()  : "UDP Streaming");
    settings.setValue("GStreamer/Host",         m_hostEdit         ? m_hostEdit->text()               : "192.168.0.30");
    settings.setValue("GStreamer/Port",         m_portSpinBox      ? m_portSpinBox->value()           : 8554);
    settings.setValue("GStreamer/Format",       m_formatSelector   ? m_formatSelector->currentText()  : "mpegts");
    settings.setValue("GStreamer/Parser",       m_parserSelector   ? m_parserSelector->currentText()  : "h264parse");
    settings.setValue("GStreamer/Payload",      m_payloadSelector  ? m_payloadSelector->currentText() : "rtph264pay");
    settings.setValue("GStreamer/ConfigIntvl",  m_configInterval   ? m_configInterval->value()        : 1);
    settings.setValue("GStreamer/PayloadType",  m_payloadType      ? m_payloadType->value()           : 96);
    settings.setValue("GStreamer/Sync",         m_syncCheckBox     ? m_syncCheckBox->isChecked()      : false);
    settings.setValue("GStreamer/StreamWidth",  m_streamWidthInput  ? m_streamWidthInput->text()      : "");
    settings.setValue("GStreamer/StreamHeight", m_streamHeightInput ? m_streamHeightInput->text()     : "");
}

void GStreamerModule::loadSettings(QSettings &settings)
{
    if (!settings.contains("GStreamer/Target")) return;
    if (m_targetSelector)    m_targetSelector->setCurrentText(settings.value("GStreamer/Target",       "UDP Streaming").toString());
    if (m_hostEdit)          m_hostEdit->setText(                settings.value("GStreamer/Host",         "192.168.0.30").toString());
    if (m_portSpinBox)       m_portSpinBox->setValue(            settings.value("GStreamer/Port",         8554).toInt());
    if (m_formatSelector)    m_formatSelector->setCurrentText(   settings.value("GStreamer/Format",       "mpegts").toString());
    if (m_parserSelector)    m_parserSelector->setCurrentText(   settings.value("GStreamer/Parser",       "h264parse").toString());
    if (m_payloadSelector)   m_payloadSelector->setCurrentText(  settings.value("GStreamer/Payload",      "rtph264pay").toString());
    if (m_configInterval)    m_configInterval->setValue(         settings.value("GStreamer/ConfigIntvl",  1).toInt());
    if (m_payloadType)       m_payloadType->setValue(            settings.value("GStreamer/PayloadType",  96).toInt());
    if (m_syncCheckBox)      m_syncCheckBox->setChecked(         settings.value("GStreamer/Sync",         false).toBool());
    if (m_streamWidthInput)  m_streamWidthInput->setText(        settings.value("GStreamer/StreamWidth",  "").toString());
    if (m_streamHeightInput) m_streamHeightInput->setText(       settings.value("GStreamer/StreamHeight", "").toString());
}
