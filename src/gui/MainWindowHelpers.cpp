#include "MainWindow.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <csignal>
#include <algorithm>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include "Defaults.h"
void MainWindow::parseListCamerasOutput(const QString &output) {
    cameraSelector->clear();
    resolutionSelector->clear();
    framerateSelector->clear();
    cameraDetails.clear();
    cameraModes.clear(); // Clear viewfinder modes
    m_resolutionFps.clear();
    m_cameraFormats.clear();
    m_formatData.clear();

    // Log-Ausgabe beim Start der Kamera-Erkennung
    appendLog(tr("=== Camera Detection Started ==="));

    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    QStringList detectedCameras;
    QStringList detectedModes;

    // Kontext über die aktuell gelesene Kamera / das aktuelle Pixelformat.
    // Keine hardkodierten Werte: Bit-Tiefe und Packing kommen aus dem
    // Pixelformat-Namen der Format-Zeile ('SRGGB10_CSI2P' : ...).
    int  currentBitDepth      = 0;    // aus Pixelformat abgeleitet
    int  cameraFallbackDepth  = 0;    // aus Kamera-Zeile ("12-bit RGGB")
    bool currentPacked        = false; // Format enthält "_CSI2P"
    QString currentFormatName;        // z. B. "SRGGB10_CSI2P"
    // MCIM: Bei festem Kamera-Index (Tab-Instanz) werden nur Modi der
    // eigenen Kamera übernommen – sonst vermischen sich die Modi mehrerer
    // Kameras. Ohne festen Index (Legacy-Fenster mit Kamera-Auswahl) bleibt
    // das bisherige Verhalten (alle Kameras, Umschalten über --camera).
    bool currentCameraActive = (m_fixedCameraIdx < 0);
    // Fingerprint der eigenen Kamera (Modell + Pixel-Array-Größe) für die
    // Erkennung eines Kameratyp-Wechsels (→ Defaults-Reset).
    QString ownCameraFingerprint;

    // Leitet die Bit-Tiefe aus dem Pixelformat-Namen ab (erste Ziffernfolge,
    // z. B. "SRGGB8" -> 8, "SRGGB10_CSI2P" -> 10, "R10_CSI2P" -> 10).
    // Funktioniert generisch für Bayer- und Mono-Formate aller Sensoren.
    auto bitDepthFromFormat = [](const QString &formatName) -> int {
        static const QRegularExpression re(R"(\d+)");
        QRegularExpressionMatch m = re.match(formatName);
        return m.hasMatch() ? m.captured(0).toInt() : 0;
    };

    for (const QString &line : lines) {
        // Kamera-Zeile: "0 : imx477 [4056x3040 12-bit RGGB] (/base/...)"
        static const QRegularExpression regexCamera(R"(^(\d+)\s*:\s*(.+))");
        QRegularExpressionMatch matchCamera = regexCamera.match(line);
        if (matchCamera.hasMatch()) {
            QString cameraIndex = matchCamera.captured(1);
            QString cameraDescription = matchCamera.captured(2);

            // MCIM: nur die eigene Kamera dieses Tabs verarbeiten
            bool ownCamera = (m_fixedCameraIdx < 0) ||
                             (cameraIndex.toInt() == m_fixedCameraIdx);
            if (!ownCamera) {
                currentCameraActive = false;
                continue;
            }
            currentCameraActive = true;

            // Fingerprint: Modellname + Pixel-Array-Größe
            // (z. B. "imx477 4056x3040") – ohne Pfad, damit ein Wechsel des
            // Anschlusses nicht fälschlich als Kamerawechsel zählt.
            if (m_fixedCameraIdx >= 0) {
                static const QRegularExpression regexModel(R"(^([^\[]+))");
                static const QRegularExpression regexSize(R"(\[(\d+x\d+))");
                QRegularExpressionMatch mModel = regexModel.match(cameraDescription);
                QRegularExpressionMatch mSize = regexSize.match(cameraDescription);
                if (mModel.hasMatch() && mSize.hasMatch()) {
                    ownCameraFingerprint = mModel.captured(1).trimmed() + " " + mSize.captured(1);
                }
            }

            cameraSelector->addItem(cameraIndex);
            cameraDetails[cameraIndex] = cameraDescription;

            // Bit-Tiefe aus der Kamera-Beschreibung nur als Fallback merken
            static const QRegularExpression regexBitDepth(R"((\d+)-bit)");
            QRegularExpressionMatch matchBitDepth = regexBitDepth.match(cameraDescription);
            cameraFallbackDepth = matchBitDepth.hasMatch() ? matchBitDepth.captured(1).toInt() : 0;
            currentBitDepth = cameraFallbackDepth;
            currentPacked = false;

            detectedCameras.append(QString("Camera %1: %2").arg(cameraIndex, cameraDescription));
            continue;
        }

        // Zeilen fremder Kameras überspringen (MCIM mit festem Index)
        if (!currentCameraActive) {
            continue;
        }

        // Format-Zeile: "'SRGGB10_CSI2P' : 1332x990 [...]"
        // Achtung: Das Format steht in der Regel AUF DERSELBEN Zeile wie der
        // erste Modus ("    Modes: 'SRGGB10_CSI2P' : 1332x990 [...]") oder
        // allein ("           'SRGGB12_CSI2P' :"). Daher: kein Zeilenanker,
        // kein continue – der Modus-Match läuft danach trotzdem weiter.
        static const QRegularExpression regexFormat(R"('([A-Za-z0-9_]+)'\s*:)");
        QRegularExpressionMatch matchFormat = regexFormat.match(line);
        if (matchFormat.hasMatch()) {
            QString formatName = matchFormat.captured(1);
            currentFormatName = formatName;
            currentBitDepth = bitDepthFromFormat(formatName);
            if (currentBitDepth == 0) {
                currentBitDepth = cameraFallbackDepth;
            }
            currentPacked = formatName.contains("CSI2P");
            if (!m_cameraFormats.contains(formatName)) {
                m_cameraFormats.append(formatName);
            }
        }

        // Modus-Zeile: "1332x990 [120.50 fps - (696, 528)/2664x1980 crop]"
        static const QRegularExpression regexMode(R"((\d+x\d+)\s*\[\s*([\d.]+)\s*fps)");
        QRegularExpressionMatch matchMode = regexMode.match(line);
        if (matchMode.hasMatch()) {
            QString resolution = matchMode.captured(1);
            // Normalisieren ("120.5" -> "120.50") für konsistente Anzeige
            QString framerate = QString::number(matchMode.captured(2).toDouble(), 'f', 2);

            if (resolutionSelector->findText(resolution) == -1) {
                resolutionSelector->addItem(resolution);
            }

            // Framerates pro Auflösung sammeln (über alle Pixelformate hinweg)
            QStringList &fpsList = m_resolutionFps[resolution];
            if (!fpsList.contains(framerate)) {
                fpsList.append(framerate);
            }

            // Strukturierte Daten pro Format für den Format-Filter
            if (!currentFormatName.isEmpty()) {
                QStringList &formatFps = m_formatData[currentFormatName][resolution];
                if (!formatFps.contains(framerate)) {
                    formatFps.append(framerate);
                }
            }

            // Sammle Modi-Info für Log
            QString modeInfo = QString("  Mode: %1 @ %2 fps").arg(resolution, framerate);
            if (!detectedModes.contains(modeInfo)) {
                detectedModes.append(modeInfo);
            }

            // Modus für --mode / --viewfinder-mode (Format W:H:Bit-Tiefe:Packing).
            // rpicam-apps wählt den Sensormodus anhand von Bit-Tiefe + Packing,
            // daher sind beide Informationen generisch ableitbar.
            QStringList resParts = resolution.split("x");
            if (resParts.size() == 2) {
                int depth = currentBitDepth > 0 ? currentBitDepth : 12;
                QString packing = currentPacked ? "P" : "U";
                QString modeString = QString("%1:%2:%3:%4")
                                         .arg(resParts[0], resParts[1])
                                         .arg(depth)
                                         .arg(packing);
                // Pixelformat im Anzeigetext – so lässt sich per Filter
                // zwischen SRGGB8 / SRGGB10_CSI2P / SRGGB12_CSI2P ... umschalten.
                QString formatTag = currentFormatName.isEmpty()
                                        ? QString("%1-bit").arg(depth)
                                        : currentFormatName;
                QString displayString = QString("%1 @ %2 fps (%3)")
                                            .arg(resolution, framerate)
                                            .arg(formatTag);
                cameraModes[displayString] = modeString;
            }
        }
    }

    // Framerates pro Auflösung aufsteigend sortieren
    for (auto it = m_resolutionFps.begin(); it != m_resolutionFps.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
                  [](const QString &a, const QString &b) {
                      return a.toDouble() < b.toDouble();
                  });
    }

    // Speichere die Kamera-Resolutions für später (Custom Resolution Management)
    cameraResolutions.clear();
    for (int i = 0; i < resolutionSelector->count(); ++i) {
        cameraResolutions.append(resolutionSelector->itemText(i));
    }

    // Kamera-Info-Feld mit den erkannten Kameras befüllen
    if (cameraInfo) {
        cameraInfo->setPlainText(detectedCameras.join("\n"));
    }

    // Detaillierte Log-Ausgabe
    if (cameraSelector->count() == 0) {
        appendLog(tr("No cameras found."));
    } else {
        appendLog(QString(tr("%1 camera(s) detected:")).arg(cameraSelector->count()));
        for (const QString &camera : detectedCameras) {
            appendLog(QString("  • %1").arg(camera));
        }

        if (!detectedModes.isEmpty()) {
            appendLog(QString(tr("%1 video mode(s) detected:")).arg(detectedModes.count()));
            for (const QString &mode : detectedModes) {
                appendLog(mode);
            }
        }

        appendLog(QString(tr("Total resolutions available: %1")).arg(resolutionSelector->count()));
        appendLog(QString(tr("Total framerates available: %1")).arg(framerateSelector->count()));
    }

    appendLog(tr("=== Camera Detection Complete ==="));

    // MCIM: Kameratyp-Wechsel erkennen. Weicht der gespeicherte Fingerprint
    // von der erkannten Kamera ab, werden die kameraspezifischen Defaults
    // dieses Tabs automatisch zurückgesetzt (alte Auflösung/Framerate passen
    // sonst nicht zur neuen Kamera). Erst danach werden die Startup-Defaults
    // geladen – daher reicht das Entfernen der Keys aus.
    if (m_fixedCameraIdx >= 0 && !ownCameraFingerprint.isEmpty()) {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        QString storedFp = settings.value("Camera/Fingerprint").toString();
        if (!storedFp.isEmpty() && storedFp != ownCameraFingerprint) {
            settings.remove("Defaults/Width");
            settings.remove("Defaults/Height");
            settings.remove("Defaults/Framerate");
            settings.remove("Defaults/ViewfinderMode");
            appendLog(tr("Camera type changed (%1 → %2) – camera-specific defaults reset.")
                          .arg(storedFp, ownCameraFingerprint));
        }
        settings.setValue("Camera/Fingerprint", ownCameraFingerprint);
        settings.endGroup();
    }

    // Set first camera as default selection if cameras are available
    if (cameraSelector->count() > 0) {
        cameraSelector->setCurrentIndex(0);
        updateCameraInfo(0);
    }

    // Default-Auflösung setzen und Framerates aktualisieren.
    // (setCurrentIndex(0) feuert evtl. kein Signal, wenn bereits Index 0
    // aktiv ist – daher zusätzlich explizit aufrufen.)
    if (resolutionSelector->count() > 0) {
        resolutionSelector->setCurrentIndex(0);
        updateFramerateOptions(resolutionSelector->currentText());
    }

    // Format-Filter (globale Zeile) mit den erkannten Pixelformaten befüllen.
    // Anzeige als Ziffern (Platzersparnis): 0 = Auto, 1..n = verfügbare Modi.
    // Die Zuordnung steht dynamisch im Tooltip.
    if (formatSelector) {
        QString currentFormat = formatSelector->currentData().toString();
        formatSelector->blockSignals(true);
        formatSelector->clear();
        formatSelector->addItem("0", QString());
        for (int i = 0; i < m_cameraFormats.size(); ++i) {
            formatSelector->addItem(QString::number(i + 1), m_cameraFormats.at(i));
        }
        int idx = formatSelector->findData(currentFormat);
        formatSelector->setCurrentIndex(idx >= 0 ? idx : 0);
        formatSelector->blockSignals(false);

        // Wiederhergestellten Filter sofort anwenden (Signal war blockiert)
        if (idx > 0) {
            applyFormatFilter();
        }

        // Tooltip dynamisch mit der Ziffern-Zuordnung befüllen
        QStringList tipLines;
        tipLines << tr("Pixel format filter (digit = mode):");
        tipLines << QString("0 = %1").arg(tr("Auto"));
        for (int i = 0; i < m_cameraFormats.size(); ++i) {
            tipLines << QString("%1 = %2").arg(i + 1).arg(m_cameraFormats.at(i));
        }
        formatSelector->setToolTip(tipLines.join("\n"));
    }

    // Update viewfinder mode dropdown if Expert Tab is enabled
    updateViewfinderModes();
}

void MainWindow::updateViewfinderModes() {
    // Only update if Expert Tab is enabled and viewfinderModeSelector exists
    if (!viewfinderModeSelector) {
        return;
    }

    // Aktiver Format-Filter (leer = alle Formate anzeigen)
    QString filterFormat;
    if (formatSelector && formatSelector->currentIndex() > 0) {
        filterFormat = formatSelector->currentData().toString();
    }

    // Save current selection
    QString currentSelection = viewfinderModeSelector->currentData().toString();

    // Clear and rebuild
    viewfinderModeSelector->clear();
    viewfinderModeSelector->addItem(tr("Auto (use capture resolution)"), "");

    // Add detected modes (gefiltert nach Pixelformat)
    QMapIterator<QString, QString> i(cameraModes);
    while (i.hasNext()) {
        i.next();
        QString displayText = i.key(); // e.g. "1332x990 @ 120.50 fps (SRGGB10_CSI2P)"
        QString modeValue = i.value();  // e.g. "1332:990:10:P"
        if (!filterFormat.isEmpty() &&
            !displayText.contains("(" + filterFormat + ")")) {
            continue;
        }
        viewfinderModeSelector->addItem(displayText, modeValue);
    }

    // Restore previous selection if it exists
    if (!currentSelection.isEmpty()) {
        int index = viewfinderModeSelector->findData(currentSelection);
        if (index != -1) {
            viewfinderModeSelector->setCurrentIndex(index);
        }
    }
}
