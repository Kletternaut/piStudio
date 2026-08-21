// CameraInstanceManager.cpp
// MCIM Phase 1.7
// Kamera-Erkennung + Erzeugung von CameraInstance-Objekten

#include "CameraInstanceManager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

// ---------------------------------------------------------------------------
// Konstruktor / Destruktor
// ---------------------------------------------------------------------------
CameraInstanceManager::CameraInstanceManager(ResourceBroker *broker,
                                             QWidget *parent)
    : QObject(parent)
    , m_broker(broker)
{
    m_tabWidget = new QTabWidget(qobject_cast<QWidget *>(parent));
    m_tabWidget->setTabPosition(QTabWidget::North);
}

CameraInstanceManager::~CameraInstanceManager()
{
    if (m_detectionProcess) {
        m_detectionProcess->kill();
        m_detectionProcess->waitForFinished(1000);
    }
}

// ---------------------------------------------------------------------------
// initialize() – startet rpicam-vid --list-cameras
// ---------------------------------------------------------------------------
void CameraInstanceManager::initialize()
{
    m_detectionOutput.clear();

    m_detectionProcess = new QProcess(this);
    connect(m_detectionProcess, &QProcess::readyReadStandardOutput,
            this, [this]() {
        m_detectionOutput +=
            QString::fromLocal8Bit(
                m_detectionProcess->readAllStandardOutput());
    });
    connect(m_detectionProcess, &QProcess::readyReadStandardError,
            this, [this]() {
        // Auch stderr erfassen (rpicam-vid schreibt hier manchmal)
        m_detectionOutput +=
            QString::fromLocal8Bit(
                m_detectionProcess->readAllStandardError());
    });
    connect(m_detectionProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CameraInstanceManager::onDetectionFinished);

    qDebug() << "[CameraInstanceManager] Starte Kamera-Erkennung…";
    m_detectionProcess->start("rpicam-vid",
                              QStringList() << "--list-cameras");
}

// ---------------------------------------------------------------------------
// Slot: Kamera-Erkennung abgeschlossen
// ---------------------------------------------------------------------------
void CameraInstanceManager::onDetectionFinished(int exitCode,
                                                QProcess::ExitStatus /*status*/)
{
    qDebug() << "[CameraInstanceManager] Erkennung beendet (exit" << exitCode << ")";
    parseListCamerasOutput(m_detectionOutput);

    if (m_instances.isEmpty()) {
        // Fallback: mindestens eine Kamera (Index 0)
        qDebug() << "[CameraInstanceManager] Keine Kamera erkannt,"
                    " erzeuge Fallback-Instanz für Index 0";
        createInstance(0);
    }

    emit cameraDetectionFinished(m_instances.size());
    m_detectionProcess->deleteLater();
    m_detectionProcess = nullptr;
}

// ---------------------------------------------------------------------------
// parseListCamerasOutput – sucht nach Zeilen "N : ..."
// ---------------------------------------------------------------------------
void CameraInstanceManager::parseListCamerasOutput(const QString &output)
{
    // Beispiel-Ausgabe von rpicam-apps (--list-cameras):
    //   Available cameras
    //   -----------------
    //   0 : imx708 [4608x2592] (/base/soc/...)
    //   1 : ov64a40 [9248x6944] (/base/soc/...)

    // Regex: Zeile beginnt mit Zahl(en) gefolgt von ' : '
    static const QRegularExpression re(R"(^\s*(\d+)\s*:)",
                                       QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = re.globalMatch(output);

    QList<int> found;
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        int idx = m.captured(1).toInt();
        if (!found.contains(idx)) {
            found.append(idx);
        }
    }
    std::sort(found.begin(), found.end());

    qDebug() << "[CameraInstanceManager] Erkannte Kamera-Indizes:" << found;

    for (int idx : found) {
        createInstance(idx);
    }
}

// ---------------------------------------------------------------------------
// createInstance – erzeugt CameraInstance + Tab
// ---------------------------------------------------------------------------
void CameraInstanceManager::createInstance(int cameraIndex)
{
    auto *inst = new CameraInstance(cameraIndex, m_broker,
                                    m_tabWidget);

    // Log-Nachrichten weiterleiten
    connect(inst, &CameraInstance::logMessage,
            this, &CameraInstanceManager::logMessage);

    // Tab-Text: "Camera 0", "Camera 1", ...
    // CameraInstance ist selbst ein QWidget – direkt als Tab einfügen
    m_tabWidget->addTab(inst, tr("Camera %1").arg(cameraIndex));

    m_instances.append(inst);
    qDebug() << "[CameraInstanceManager] CameraInstance" << cameraIndex
             << "erstellt und als Tab eingefügt";
}

// ---------------------------------------------------------------------------
// Instanz-Zugriff
// ---------------------------------------------------------------------------
CameraInstance *CameraInstanceManager::instance(int cameraIndex) const
{
    for (CameraInstance *inst : m_instances) {
        if (inst->cameraIndex() == cameraIndex) {
            return inst;
        }
    }
    return nullptr;
}

CameraInstance *CameraInstanceManager::activeInstance() const
{
    int idx = m_tabWidget ? m_tabWidget->currentIndex() : -1;
    if (idx >= 0 && idx < m_instances.size()) {
        return m_instances[idx];
    }
    return nullptr;
}
