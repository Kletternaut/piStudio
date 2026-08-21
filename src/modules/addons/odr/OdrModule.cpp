#include "OdrModule.h"
#include "../../../gui/CollapsibleHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QIntValidator>
#include <QMessageBox>
#include <QFile>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>
#include <QDateTime>

OdrModule::OdrModule(DetectionAction &detectionAction, QWidget *parentWidget)
    : QObject(parentWidget)
    , m_detectionAction(detectionAction)
    , m_parentWidget(parentWidget)
{
    // Ensure child process is killed before the Qt event loop exits,
    // regardless of destructor call order.
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
            this, [this]() {
        if (m_process && m_process->state() == QProcess::Running) {
            m_process->terminate();
            if (!m_process->waitForFinished(3000)) {
                m_process->kill();
                m_process->waitForFinished(1000);
            }
        }
    });
}

OdrModule::~OdrModule()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
}

void OdrModule::setup(QList<CollapsibleHelper*> &helpers,
                      std::function<void()> adjustWindowCallback)
{
    m_tab = new QWidget(m_parentWidget);
    auto *mainLayout = new QVBoxLayout(m_tab);

    // ── Control Group ────────────────────────────────────────────
    auto *controlGroup = new QGroupBox(QObject::tr("UDP Object Detection Receiver"), m_parentWidget);
    controlGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *controlLayout = new QVBoxLayout(controlGroup);

    // Port row
    auto *portLayout = new QHBoxLayout;
    portLayout->addWidget(new QLabel(QObject::tr("UDP Port:"), m_parentWidget));
    m_portInput = new QLineEdit("12347", m_parentWidget);
    m_portInput->setValidator(new QIntValidator(1, 65535, m_parentWidget));
    m_portInput->setFixedWidth(80);
    portLayout->addWidget(m_portInput);
    portLayout->addStretch();
    controlLayout->addLayout(portLayout);
    controlLayout->addSpacing(10);

    // Start/Stop button
    auto *buttonLayout = new QHBoxLayout;
    m_startButton = new QPushButton(QObject::tr("Start Receiver"), m_parentWidget);
    m_startButton->setCheckable(true);
    m_startButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    border: none;"
        "    border-radius: 4px;"
        "    outline: none;"
        "}"
        "QPushButton:hover {"
        "    background-color: #5dade2;"
        "}"
        "QPushButton:checked {"
        "    background-color: #e74c3c;"
        "}"
        "QPushButton:checked:hover {"
        "    background-color: #c0392b;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #95a5a6;"
        "}"
    );
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addStretch();
    controlLayout->addLayout(buttonLayout);

    if (adjustWindowCallback) {
        helpers.append(CollapsibleHelper::makeCollapsible(
            controlGroup, "UI/Inference/ControlGroup", adjustWindowCallback));
    } else {
        helpers.append(CollapsibleHelper::makeCollapsible(
            controlGroup, "UI/Inference/ControlGroup"));
    }
    mainLayout->addWidget(controlGroup);

    // ── Detection Results Group ──────────────────────────────────
    auto *resultsGroup = new QGroupBox(QObject::tr("Detection Results"), m_parentWidget);
    resultsGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "}"
    );
    auto *resultsLayout = new QVBoxLayout(resultsGroup);

    m_resultsList = new QListWidget(m_parentWidget);
    m_resultsList->setTextElideMode(Qt::ElideNone);
    m_resultsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_resultsList->setStyleSheet(
        "QListWidget {"
        "    font-family: 'Courier New', monospace;"
        "    font-size: 10pt;"
        "}"
    );
    m_resultsList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_resultsList->setMinimumHeight(200);
    resultsLayout->addWidget(m_resultsList, 1);

    m_reportOnlyChanges = new QCheckBox(QObject::tr("Report only changes"), m_parentWidget);
    m_reportOnlyChanges->setToolTip(QObject::tr("Show only detections when the detected object changes"));
    m_reportOnlyChanges->setChecked(false);
    resultsLayout->addWidget(m_reportOnlyChanges);

    auto *clearButton = new QPushButton(QObject::tr("Clear Results"), m_parentWidget);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        m_resultsList->clear();
        m_lastDetectedObject.clear();
    });
    resultsLayout->addWidget(clearButton);

    if (adjustWindowCallback) {
        helpers.append(CollapsibleHelper::makeCollapsible(
            resultsGroup, "UI/Inference/ResultsGroup", adjustWindowCallback));
    } else {
        helpers.append(CollapsibleHelper::makeCollapsible(
            resultsGroup, "UI/Inference/ResultsGroup"));
    }
    resultsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(resultsGroup, 100);
    mainLayout->addStretch(1);

    // ── Signal connections ───────────────────────────────────────
    connect(m_startButton, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            m_startButton->setText(QObject::tr("Stop Receiver"));
            startReceiver();
        } else {
            m_startButton->setText(QObject::tr("Start Receiver"));
            stopReceiver();
        }
    });
}

// ── Private slots ────────────────────────────────────────────────

void OdrModule::startReceiver()
{
    if (m_process && m_process->state() == QProcess::Running) {
        QMessageBox::information(m_parentWidget, QObject::tr("Already Running"),
                                 QObject::tr("Receiver is already running."));
        return;
    }

    if (!m_process) {
        m_process = new QProcess(this);

        connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
            QString output = m_process->readAllStandardOutput();
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            QRegularExpression detectLineRegex(
                "(?:DETECTION:\\s*)?(\\w+)(?:\\s+id=\\d+)?\\s*\\(\\d+(?:\\.\\d+)?%\\)");
            for (const QString &line : lines) {
                if (line.contains("DETECTION:") || detectLineRegex.match(line).hasMatch()) {
                    addDetectionToList(line);
                }
            }
        });

        connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
            QString error = m_process->readAllStandardError();
            qDebug() << "Inference receiver error:" << error;
        });

        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "Receiver stopped. Exit code:" << exitCode << "Status:" << exitStatus;
            m_startButton->setChecked(false);
            m_startButton->setText(QObject::tr("Start Receiver"));
            if (exitCode != 0 && exitCode != 15 && exitStatus != QProcess::NormalExit) {
                QMessageBox::warning(m_parentWidget, QObject::tr("Receiver Error"),
                    QObject::tr("Receiver stopped unexpectedly with exit code: %1").arg(exitCode));
            }
        });
    }

    QString port    = m_portInput->text();
    QString program = QCoreApplication::applicationDirPath() + "/udp_object_detection";

    if (!QFile::exists(program)) {
        QMessageBox::critical(m_parentWidget, QObject::tr("Error"),
            QObject::tr("udp_object_detection binary not found at: %1").arg(program));
        return;
    }

    m_process->start(program, QStringList() << port);

    if (!m_process->waitForStarted(3000)) {
        QMessageBox::critical(m_parentWidget, QObject::tr("Error"),
                              QObject::tr("Failed to start receiver process."));
        m_startButton->setChecked(false);
        m_startButton->setText(QObject::tr("Start Receiver"));
        return;
    }

    m_resultsList->addItem(QObject::tr("Receiver started on port %1").arg(port));
}

void OdrModule::stopReceiver()
{
    if (!m_process || m_process->state() != QProcess::Running) {
        QMessageBox::information(m_parentWidget, QObject::tr("Not Running"),
                                 QObject::tr("No receiver process running."));
        m_startButton->setChecked(false);
        m_startButton->setText(QObject::tr("Start Receiver"));
        return;
    }

    m_process->terminate();
    if (!m_process->waitForFinished(3000)) {
        m_process->kill();
    }
    m_resultsList->addItem(QObject::tr("Receiver stopped"));
}

void OdrModule::addDetectionToList(const QString &detection)
{
    QString objectName;
    int confidence = 0;

    QRegularExpression regex(
        "(?:\\[.*?\\]\\s*)?(?:DETECTION:\\s*)?(\\w+)(?:\\s+id=\\d+)?\\s*\\((\\d+(?:\\.\\d+)?)%\\)");
    QRegularExpressionMatch match = regex.match(detection);
    if (match.hasMatch()) {
        objectName  = match.captured(1).trimmed();
        confidence  = qRound(match.captured(2).toDouble());
    } else {
        QRegularExpression simpleRegex(
            "(?:\\[.*?\\]\\s*)?(?:DETECTION:\\s*)?(\\w+)");
        QRegularExpressionMatch simpleMatch = simpleRegex.match(detection);
        if (simpleMatch.hasMatch()) {
            objectName = simpleMatch.captured(1).trimmed();
        }
    }

    // Note: confidence/cooldown/object-filter gating is handled exclusively in
    // ActionsModule::executeDetection(). OdrModule only applies the reportOnlyChanges
    // filter (UI list + emit), nothing else.
    if (m_reportOnlyChanges && m_reportOnlyChanges->isChecked()) {
        if (objectName == m_lastDetectedObject && !m_lastDetectedObject.isEmpty()) {
            // Same object as last time — silently discard (no log, no emit, no list entry)
            return;
        }
        m_lastDetectedObject = objectName;
    }

    qDebug() << "[OdrModule] Detection:" << objectName << "Confidence:" << confidence;

    emit detectionToExecute(objectName, confidence, detection);

    if (m_resultsList->count() >= 100) {
        delete m_resultsList->takeItem(0);
    }
    m_resultsList->addItem(detection);
    m_resultsList->scrollToBottom();
}


