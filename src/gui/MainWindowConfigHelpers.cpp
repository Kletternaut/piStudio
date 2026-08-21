#include "MainWindow.h"
#include "../app/AppMeta.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// Free function from MainWindow.cpp – detects remote (XRDP) sessions
extern bool isRemoteSession();
#include <QProcess>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QDialog>
#include <csignal>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include "Defaults.h"
void MainWindow::saveConfigurationToFile(const QString &filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        // Speichere die Kamera, falls gesetzt
        if (!cameraSelector->currentText().isEmpty()) {
            out << "camera=" << cameraSelector->currentText() << "\n";
        }

        // Save preview mode (backward-compatible: old and new format)
        QString previewValue = previewSelector->currentData().toString();
        if (previewValue == "--qt-preview") {
            out << "qt-preview=\n"; // Legacy format
        } else if (previewValue == "--fullscreen") {
            out << "fullscreen=1\n";
        } else if (previewValue == "--nopreview") {
            out << "nopreview=1\n";
        } else if (previewValue == "wayland-egl" || previewValue == "egl" ||
                   previewValue == "drm" || previewValue == "qt") {
            out << "preview-backend=" << previewValue << "\n"; // New format
        }

        // Save preview-libs path
        if (!m_previewLibsPath.isEmpty()) {
            out << "preview-libs=" << m_previewLibsPath << "\n";
        }

        // Save encoder-libs path
        if (!m_encoderLibsPath.isEmpty()) {
            out << "encoder-libs=" << m_encoderLibsPath << "\n";
        }

        // Speichere Vorschaufenstermaße (preview=x,y,w,h)
        if (!BoxInput->text().isEmpty()) {
            out << "preview=" << BoxInput->text() << "\n";
        }

        QString width = resolutionSelector->currentText().split("x").value(0);
        QString height = resolutionSelector->currentText().split("x").value(1);
        if (!width.isEmpty() && !height.isEmpty()) {
            out << "width=" << width << "\n";
            out << "height=" << height << "\n";
        }

        // 0 oder leer = Auto: kein framerate-Eintrag
        QString currentFramerate = framerateSelector->currentText().trimmed();
        if (!currentFramerate.isEmpty() && currentFramerate != "0") {
            out << "framerate=" << currentFramerate << "\n";
        }

        if (!timeoutSelector->currentText().isEmpty()) {
            out << "timeout=" << timeoutSelector->currentText() << "\n";
        }

        if (awbSelector->currentText() != "auto") {
            out << "awb=" << awbSelector->currentText() << "\n";
        }

        // Metering configuration
        if (meteringSelector->currentText() != "centre" && meteringSelector->currentText() != "Select option:") {
            if (meteringSelector->currentText() == "custom") {
                QString customValue = meteringCustomInput->text().trimmed();
                if (!customValue.isEmpty()) {
                    out << "metering=" << customValue << "\n";
                }
            } else {
                out << "metering=" << meteringSelector->currentText() << "\n";
            }
        }

        // Low Resolution configuration (using new custom widget)
        if (loresComboBox->isLoresEnabled()) {
            if (loresComboBox->isParEnabled()) {
                out << "lores-par=1\n";
            }

            if (loresComboBox->getHeight() > 0) {
                out << "lores-height=" << loresComboBox->getHeight() << "\n";
            }
            if (loresComboBox->getWidth() > 0) {
                out << "lores-width=" << loresComboBox->getWidth() << "\n";
            }
        }

        double sharpness = sharpnessInput->text().toDouble();
        if (sharpness != DEFAULT_SHARPNESS) {
            out << "sharpness=" << QString::number(sharpness, 'f', 1) << "\n";
        }

        double ev = evInput->text().toDouble();
        if (ev != DEFAULT_EV) {
            out << "ev=" << QString::number(ev, 'f', 1) << "\n";
        }

        double gain = gainInput->text().toDouble();
        if (gain != DEFAULT_GAIN) {
            out << "gain=" << QString::number(gain, 'f', 1) << "\n";
        }

        double awbGainRed = awbGainRedInput->text().toDouble();
        double awbGainBlue = awbGainBlueInput->text().toDouble();
        if (awbGainRed != DEFAULT_AWB_GAIN_RED || awbGainBlue != DEFAULT_AWB_GAIN_BLUE) {
            out << "awbgains=" << QString::number(awbGainRed, 'f', 1) << "," << QString::number(awbGainBlue, 'f', 1) << "\n";
        }

        // CCM (Colour Correction Matrix)
        if (ccmInput && !ccmInput->text().isEmpty()) {
            out << "ccm=" << ccmInput->text().trimmed() << "\n";
        }

        double brightness = brightnessInput->text().toDouble();
        if (brightness != DEFAULT_BRIGHTNESS) {
            out << "brightness=" << QString::number(brightness, 'f', 1) << "\n";
        }

        double contrast = contrastInput->text().toDouble();
        if (contrast != DEFAULT_CONTRAST) {
            out << "contrast=" << QString::number(contrast, 'f', 1) << "\n";
        }

        double saturation = saturationInput->text().toDouble();
        if (saturation != DEFAULT_SATURATION) {
            out << "saturation=" << QString::number(saturation, 'f', 1) << "\n";
        }

        // Speichere Video-Parameter, wenn die App rpicam-vid oder rpicam-raw ist
        if (appSelector->currentText() == "rpicam-vid" || appSelector->currentText() == "rpicam-raw") {
            QString codec = codecSelector->currentText();
            // Speichere Codec nur wenn es nicht der Standard (h264) ist
            if (!codec.isEmpty() && codec != "h264") {
                out << "codec=" << codec << "\n";

                // LibAV-spezifische Optionen speichern (nur bei libav)
                if (codec == "libav") {
                    if (libavFormatSelector && !libavFormatSelector->currentText().isEmpty()) {
                        out << "libav-format=" << libavFormatSelector->currentText() << "\n";
                    }
                    if (libavVideoCodecSelector && !libavVideoCodecSelector->currentText().isEmpty()) {
                        out << "libav-video-codec=" << libavVideoCodecSelector->currentText() << "\n";
                    }
                    if (libavCodecOptsSelector && !libavCodecOptsSelector->currentText().trimmed().isEmpty()) {
                        out << "libav-video-codec-opts=" << libavCodecOptsSelector->currentText().trimmed() << "\n";
                    }
                    if (lowLatencyCheckbox && lowLatencyCheckbox->isChecked()) {
                        out << "low-latency=1\n";
                    }

                    // Save audio settings for libav
                    if (enableAudioCheckBox && enableAudioCheckBox->isChecked()) {
                        out << "libav-audio=1\n";
                        if (audioCodecSelector && !audioCodecSelector->currentText().isEmpty()) {
                            out << "audio-codec=" << audioCodecSelector->currentText() << "\n";
                        }
                        if (audioBitrateSpinBox && audioBitrateSpinBox->value() > 0) {
                            out << "audio-bitrate=" << audioBitrateSpinBox->value() << "\n";
                        }
                        if (audioSourceSelector && !audioSourceSelector->currentText().isEmpty()) {
                            out << "audio-source=" << audioSourceSelector->currentText() << "\n";
                        }
                        if (audioDeviceEdit && !audioDeviceEdit->text().trimmed().isEmpty()) {
                            out << "audio-device=" << audioDeviceEdit->text().trimmed() << "\n";
                        }
                        if (audioChannelsSpinBox && audioChannelsSpinBox->value() > 0) {
                            out << "audio-channels=" << audioChannelsSpinBox->value() << "\n";
                        }
                        if (audioSampleRateSelector && !audioSampleRateSelector->currentText().isEmpty()) {
                            out << "audio-samplerate=" << audioSampleRateSelector->currentText() << "\n";
                        }
                        if (audioAvSyncSpinBox && audioAvSyncSpinBox->value() != 0) {
                            out << "av-sync=" << audioAvSyncSpinBox->value() << "\n";
                        }
                    }
                }
            }

            // Expert Parameters (if Expert Tab is enabled)
            if (viewfinderModeSelector) {
                QString vfMode = viewfinderModeSelector->currentData().toString();
                if (!vfMode.isEmpty()) {
                    out << "viewfinder-mode=" << vfMode << "\n";
                }
            }
            if (viewfinderWidthSpinBox && viewfinderWidthSpinBox->value() > 0) {
                out << "viewfinder-width=" << viewfinderWidthSpinBox->value() << "\n";
            }
            if (viewfinderHeightSpinBox && viewfinderHeightSpinBox->value() > 0) {
                out << "viewfinder-height=" << viewfinderHeightSpinBox->value() << "\n";
            }
            if (bufferCountSpinBox && bufferCountSpinBox->value() > 0) {
                out << "buffer-count=" << bufferCountSpinBox->value() << "\n";
            }
            if (viewfinderBufferCountSpinBox && viewfinderBufferCountSpinBox->value() > 0) {
                out << "viewfinder-buffer-count=" << viewfinderBufferCountSpinBox->value() << "\n";
            }

            // H264-spezifische Optionen (nur bei h264)
            if (codec == "h264") {
                // Profile
                if (profileSelector && profileSelector->currentIndex() != -1) {
                    out << "profile=" << profileSelector->currentText() << "\n";
                }

                // Level
                if (levelSelector && levelSelector->currentIndex() != -1) {
                    out << "level=" << levelSelector->currentText() << "\n";
                }
            }

            // Inline Headers: für alle Codecs außer yuv420
            if (codec != "yuv420") {
                if (inlineHeadersCheckbox && inlineHeadersCheckbox->isChecked()) {
                    out << "inline=1\n";
                }
            }

            // Sync (Multi-Camera)
            if (syncSelector && syncSelector->currentText() != "off") {
                out << "sync=" << syncSelector->currentText() << "\n";
            }

            // Bitrate
            if (bitrateSpinBox && bitrateSpinBox->value() > 0) {
                out << "bitrate=" << bitrateSpinBox->value() << "\n";
            }

            // Quality
            if (qualitySpinBox && qualitySpinBox->value() > 0) {
                out << "quality=" << qualitySpinBox->value() << "\n";
            }

            // Intra Period
            if (intraSpinBox && intraSpinBox->value() > 0) {
                out << "intra=" << intraSpinBox->value() << "\n";
            }

            // Frames
            if (framesSpinBox && framesSpinBox->value() > 0) {
                out << "frames=" << framesSpinBox->value() << "\n";
            }

            // Flush
            if (flushCheckbox && flushCheckbox->isChecked()) {
                out << "flush=1\n";
            }

            // Save PTS
            if (savePtsInput && !savePtsInput->text().trimmed().isEmpty()) {
                out << "save-pts=" << savePtsInput->text().trimmed() << "\n";
            }

            // Segmentation Settings
            // Note: Save individual settings even if main segmentation checkbox is not checked
            // This allows partial configuration (e.g., only split or only segment)

            // Segment Duration (rpicam-vid parameter: --segment <milliseconds>)
            if (segmentDurationInput && !segmentDurationInput->text().trimmed().isEmpty()) {
                QString segmentValue = segmentDurationInput->text().trimmed();
                if (segmentValue != "0") {
                    out << "segment=" << segmentValue << "\n";
                }
            }

            // Split Files
            if (splitFilesCheckbox && splitFilesCheckbox->isChecked()) {
                out << "split=1\n";
            }

            // Signal Recording
            if (signalRecordingCheckbox && signalRecordingCheckbox->isChecked()) {
                out << "signal=1\n";
            }

            // Keypress Recording
            if (keypressRecordingCheckbox && keypressRecordingCheckbox->isChecked()) {
                out << "keypress=1\n";
            }

            // Circular Buffer
            if (circularBufferInput && !circularBufferInput->text().trimmed().isEmpty()) {
                QString circularValue = circularBufferInput->text().trimmed();
                if (circularValue != "0") {
                    out << "circular=" << circularValue << "\n";
                }
            }

            // Initial State (pause/record) - nur speichern wenn signal ODER keypress aktiv ist
            // (--initial funktioniert NUR mit --signal oder --keypress!)
            bool hasSignal = (signalRecordingCheckbox && signalRecordingCheckbox->isChecked());
            bool hasKeypress = (keypressRecordingCheckbox && keypressRecordingCheckbox->isChecked());

            if (initialStateComboBox && (hasSignal || hasKeypress) &&
                initialStateComboBox->currentText() == "pause") {
                out << "initial=pause\n";
            }
        }

        // Speichere Encoding für rpicam-still/jpeg (nur wenn nicht Standard "jpg")
        if ((appSelector->currentText() == "rpicam-still" || appSelector->currentText() == "rpicam-jpeg")
            && encodingSelector) {
            QString encoding = encodingSelector->currentData().toString();
            if (!encoding.isEmpty() && encoding != "jpg") {
                out << "encoding=" << encoding << "\n";
            }
        }

        // Speichere den vollständigen Pfad der Post-Process-Datei, falls vorhanden
        QString postProcessFile = postProcessFileSelector->currentText();
        if (!postProcessFile.isEmpty() && postProcessFile.trimmed() != "") {
            QString postProcessFilePath;
            if (QFileInfo(postProcessFile).isAbsolute()) {
                postProcessFilePath = postProcessFile;
            } else {
                postProcessFilePath = QDir(guiPostProcessFilePath).filePath(postProcessFile);
            }
            out << "post-process-file=" << postProcessFilePath << "\n";
        }

        // Save the post-process libs path (configured in the camera setup dialog)
        if (!m_postProcessLibsPath.isEmpty()) {
            out << "post-process-libs=" << m_postProcessLibsPath << "\n";
        }

        // Speichere den vollständigen Pfad der Tuning-Datei, falls vorhanden
        QString tuningFile = tuningFileSelector->currentText();
        if (!tuningFile.isEmpty() && tuningFile.trimmed() != "") {
            QString tuningFilePath;
            if (QFileInfo(tuningFile).isAbsolute()) {
                tuningFilePath = tuningFile;
            } else {
                tuningFilePath = QDir(guiTuningFilePath).filePath(tuningFile);
            }
            out << "tuning-file=" << tuningFilePath << "\n";
        }

        // Speichere Autofocus Mode (nur wenn nicht Standard-Wert "auto")
        QString autofocusMode = autofocusModeSelector->currentText();
        if (!autofocusMode.isEmpty() && autofocusMode != "auto") {
            out << "autofocus-mode=" << autofocusMode << "\n";
        }

        // Speichere Autofocus Range, falls nicht normal
        QString autofocusRange = autofocusRangeSelector->currentText();
        if (!autofocusRange.isEmpty() && autofocusRange != "normal") {
            out << "autofocus-range=" << autofocusRange << "\n";
        }

        // Speichere Autofocus Speed, falls nicht normal
        QString autofocusSpeed = autofocusSpeedSelector->currentText();
        if (!autofocusSpeed.isEmpty() && autofocusSpeed != "normal") {
            out << "autofocus-speed=" << autofocusSpeed << "\n";
        }

        // Speichere Autofocus Window, falls nicht default
        QString autofocusWindow = autofocusWindowInput->text();
        if (!autofocusWindow.isEmpty() && autofocusWindow != "0.333,0.333,0.333,0.333") {
            out << "autofocus-window=" << autofocusWindow << "\n";
        }

        // Speichere Lens Position, falls nicht 0.0
        double lensPosition = lensPositionInput->text().toDouble();
        if (lensPosition != DEFAULT_LENS_POSITION) {
            out << "lens-position=" << QString::number(lensPosition, 'f', 2) << "\n";
        }

        // Speichere das Output-File, falls vorhanden
        QString outputFile = outputFileName->text();
        if (!outputFile.isEmpty()) {
            // Add %04d pattern if segmentPatternCheckbox is checked
            if (segmentPatternCheckbox && segmentPatternCheckbox->isChecked()) {
                QFileInfo fileInfo(outputFile);
                QString baseName = fileInfo.completeBaseName();
                QString suffix = fileInfo.suffix();
                QString directory = fileInfo.path();
                if (directory == ".") {
                    outputFile = baseName + "_%04d." + suffix;
                } else {
                    outputFile = QDir(directory).filePath(baseName + "_%04d." + suffix);
                }
            }

            QString outputFilePath;
            if (QDir::isAbsolutePath(outputFile)) {
                // Wenn der Pfad bereits absolut ist, verwende ihn direkt
                outputFilePath = QDir::cleanPath(outputFile);
            } else {
                // Wenn der Pfad relativ ist, füge das Basisverzeichnis hinzu
                outputFilePath = QDir::cleanPath("/home/admin/rpicam-output/" + outputFile);
            }
            out << "output=" << outputFilePath << "\n";
        }

        // Speichere den Timelapse-Wert, falls gesetzt (nur wenn nicht leer!)
        QString timelapseValue = timelapseInput->currentText();
        if (!timelapseValue.isEmpty()) {
            out << "timelapse=" << timelapseValue << "\n";
            qDebug() << "[DEBUG] Timelapse saved as:" << timelapseValue;
        }

        // Speichere Geometry-Parameter von Checkboxen
        if (hflipCheckbox && hflipCheckbox->isChecked()) {
            out << "hflip=1\n";
        }
        if (vflipCheckbox && vflipCheckbox->isChecked()) {
            out << "vflip=1\n";
        }
        if (rotationCheckbox && rotationCheckbox->isChecked()) {
            out << "rotation=180\n";
        }

        // Save shutter as raw µs (input field shows unit-appropriate number)
        int shutterUs = parseShutterInput();
        if (shutterUs > 0) {
            out << "shutter=" << shutterUs << "\n";
        }

        if (hdrSelector && hdrSelector->currentText() != "off") {
            out << "hdr=" << hdrSelector->currentText() << "\n";
        }

        if (denoiseSelector && denoiseSelector->currentText() != "auto") {
            out << "denoise=" << denoiseSelector->currentText() << "\n";
        }

        // Flicker Period configuration
        if (flickerPeriodSelector) {
            QString flickerData = flickerPeriodSelector->currentData().toString();
            if (!flickerData.isEmpty() && flickerData != "off") {
                out << "flicker-period=" << flickerData << "\n";
            } else if (flickerData.isEmpty()) {
                QString customValue = flickerPeriodSelector->currentText().trimmed();
                if (!customValue.isEmpty()) {
                    out << "flicker-period=" << customValue << "\n";
                }
            }
        }

        // Metadata configuration
        bool metadataEnabled = false;
        if (metadataAutoNamingCheckbox && metadataAutoNamingCheckbox->isChecked()) {
            // For auto-naming: save the output base filename so metadata can be generated from it
            QString outputBase = outputFileName->text().trimmed();
            if (!outputBase.isEmpty()) {
                out << "metadata=" << outputBase << "\n";
                metadataEnabled = true;
            }
        } else if (metadataFileEdit && !metadataFileEdit->text().trimmed().isEmpty()) {
            out << "metadata=" << metadataFileEdit->text().trimmed() << "\n";
            metadataEnabled = true;
        }
        // Only save metadata-format if metadata is actually enabled
        if (metadataEnabled && metadataFormatSelector && !metadataFormatSelector->currentText().trimmed().isEmpty()) {
            out << "metadata-format=" << metadataFormatSelector->currentText().trimmed() << "\n";
        }

        // ROI configuration
        QString roiValue = roiInput ? roiInput->text().trimmed() : "";
        if (!roiValue.isEmpty() && roiValue != "0.0,0.0,1.0,1.0") {
            out << "roi=" << roiValue << "\n";
        }

        // Still-Tab Parameters
        if (autofocusOnCaptureCheckbox && autofocusOnCaptureCheckbox->isChecked()) {
            out << "autofocus-on-capture=1\n";
        }
        if (zslCheckbox && zslCheckbox->isChecked()) {
            out << "zsl=1\n";
        }
        if (immediateCheckbox && immediateCheckbox->isChecked()) {
            out << "immediate=1\n";
        }
        if (framestartSpinBox && framestartSpinBox->value() > 0) {
            out << "framestart=" << framestartSpinBox->value() << "\n";
        }
        if (thumbLineEdit && !thumbLineEdit->text().trimmed().isEmpty()) {
            out << "thumb=" << thumbLineEdit->text().trimmed() << "\n";
        }
        if (restartSpinBox && restartSpinBox->value() > 0) {
            out << "restart=" << restartSpinBox->value() << "\n";
        }
        if (exifLineEdit && !exifLineEdit->text().trimmed().isEmpty()) {
            out << "exif=" << exifLineEdit->text().trimmed() << "\n";
        }
        if (latestLineEdit && !latestLineEdit->text().trimmed().isEmpty()) {
            out << "latest=" << latestLineEdit->text().trimmed() << "\n";
        }
        if (rawCheckbox && rawCheckbox->isChecked()) {
            out << "raw=1\n";
        }

        // Speichere Info-Text Parameter von Checkboxen mit beschreibenden Labels
        QStringList infoTextParts;

        if (infoTextFrameCheckbox && infoTextFrameCheckbox->isChecked()) {
            infoTextParts << "No.:%frame";
        }
        if (infoTextFpsCheckbox && infoTextFpsCheckbox->isChecked()) {
            infoTextParts << "(%fps fps)";
        }
        if (infoTextExpCheckbox && infoTextExpCheckbox->isChecked()) {
            infoTextParts << "exp %exp";
        }
        if (infoTextAgCheckbox && infoTextAgCheckbox->isChecked()) {
            infoTextParts << "ag %ag";
        }
        if (infoTextDgCheckbox && infoTextDgCheckbox->isChecked()) {
            infoTextParts << "dg %dg";
        }
        if (infoTextRgCheckbox && infoTextRgCheckbox->isChecked()) {
            infoTextParts << "rg %rg";
        }
        if (infoTextBgCheckbox && infoTextBgCheckbox->isChecked()) {
            infoTextParts << "bg %bg";
        }
        if (infoTextFocusCheckbox && infoTextFocusCheckbox->isChecked()) {
            infoTextParts << "focus %focus";
        }
        if (infoTextAelockCheckbox && infoTextAelockCheckbox->isChecked()) {
            infoTextParts << "aelock %aelock";
        }
        if (infoTextLpCheckbox && infoTextLpCheckbox->isChecked()) {
            infoTextParts << "lp %lp";
        }
        if (infoTextAfstateCheckbox && infoTextAfstateCheckbox->isChecked()) {
            infoTextParts << "af %afstate";
        }

        if (!infoTextParts.isEmpty()) {
            QString infoTextString = infoTextParts.join(" ");
            out << "info-text=" << infoTextString << "\n";
        }

        // Recording Options
        if (signalRecordingCheckbox && signalRecordingCheckbox->isChecked()) {
            out << "signal=1\n";

            // Initial state
            if (initialStateComboBox && !initialStateComboBox->currentText().isEmpty()) {
                out << "initial=" << initialStateComboBox->currentText() << "\n";
            }

            // Split files
            if (splitFilesCheckbox && splitFilesCheckbox->isChecked()) {
                out << "split=1\n";
                // Note: The %04d pattern is embedded in the output filename, not a separate parameter
            }

            // Segment duration
            if (segmentDurationInput && !segmentDurationInput->text().trimmed().isEmpty()) {
                out << "segment=" << segmentDurationInput->text().trimmed() << "\n";
            }

            // Circular buffer
            if (circularBufferInput && !circularBufferInput->text().trimmed().isEmpty()) {
                out << "circular=" << circularBufferInput->text().trimmed() << "\n";
            }
        }

        // Keypress recording
        if (keypressRecordingCheckbox && keypressRecordingCheckbox->isChecked()) {
            out << "keypress=1\n";
        }

        file.close();
        appendLog(tr("Configuration saved to ") + filePath);

        // Save Detection Action Settings via ActionsPlugin (P22: replaces stale struct write)
        QSettings guiSettings(AppPaths::globalConf(), QSettings::IniFormat);
        guiSettings.beginGroup(m_tabGroup);
        if (m_actionsTab) {
            m_actionsTab->saveSettings(guiSettings);
        }
        guiSettings.endGroup();
        guiSettings.sync();
        appendLog(tr("Detection actions saved to %1")
                      .arg(QLatin1String(AppMeta::CONFIG_FILE)));
    } else {
        appendLog(tr("Failed to save configuration to ") + filePath);
    }
}

void MainWindow::loadConfigurationFromFile(const QString &filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        QString width, height; // Temporäre Variablen für Breite und Höhe
        QString framerate; // Temporäre Variable für Framerate (wird nach Auflösung gesetzt)

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("#") || line.isEmpty()) {
                continue; // Skip comments and empty lines
            }

            QStringList parts = line.split("=", Qt::KeepEmptyParts);
            if (parts.size() != 2) {
                continue; // Skip invalid lines
            }

            QString key = parts[0].trimmed();
            QString value = parts[1].trimmed();

            if (key == "camera") {
                cameraSelector->setCurrentText(value);
            } else if (key == "qt-preview") {
                previewSelector->setCurrentIndex(previewSelector->findData("--qt-preview"));
            } else if (key == "fullscreen") {
                previewSelector->setCurrentIndex(previewSelector->findData("--fullscreen"));
            } else if (key == "nopreview") {
                previewSelector->setCurrentIndex(previewSelector->findData("--nopreview"));
            } else if (key == "preview-backend") {
                int idx = previewSelector->findData(value);
                if (idx >= 0) previewSelector->setCurrentIndex(idx);
            } else if (key == "preview-libs") {
                m_previewLibsPath = value;
            } else if (key == "encoder-libs") {
                m_encoderLibsPath = value;
            } else if (key == "timeout") {
                timeoutSelector->setCurrentText(value);
            } else if (key == "preview") {
                BoxInput->setText(value);
            } else if (key == "width") {
                width = value; // Store the width
            } else if (key == "height") {
                height = value; // Store the height
            } else if (key == "framerate") {
                framerate = value; // Speichere Framerate (wird nach Auflösung gesetzt)
            } else if (key == "awb") {
                awbSelector->setCurrentText(value);
            } else if (key == "metering") {
                // Check if it's one of the predefined options
                if (value == "centre" || value == "spot" || value == "average") {
                    meteringSelector->setCurrentText(value);
                } else {
                    // It's a custom value
                    meteringSelector->setCurrentText("custom");
                    meteringCustomInput->setText(value);
                    meteringCustomInput->setVisible(true);
                }
            } else if (key == "meteringCustom") {
                meteringCustomInput->setText(value);
                meteringCustomInput->setVisible(true);
            } else if (key == "lores-par") {
                if (value == "1") {
                    loresComboBox->setParEnabled(true);
                }
            } else if (key == "lores-height") {
                loresComboBox->setHeight(value.toInt());
            } else if (key == "lores-width") {
                loresComboBox->setWidth(value.toInt());
            } else if (key == "lores-mode") {
                // Legacy support - convert old dropdown modes to new format
                if (value.contains("PAR") || value != "Select option:") {
                    loresComboBox->setLoresEnabled(true);
                    if (value.contains("PAR")) {
                        loresComboBox->setParEnabled(true);
                    }
                }
            } else if (key == "lores") {
                // Legacy support for old format
                if (value == "320x240" || value == "640x480" || value == "800x600" || value == "1280x720") {
                    QStringList dimensions = value.split("x");
                    if (dimensions.size() == 2) {
                        loresComboBox->setLoresEnabled(true);
                        loresComboBox->setWidth(dimensions[0].toInt());
                        loresComboBox->setHeight(dimensions[1].toInt());
                    }
                } else {
                    // It's a custom value - enable lores
                    loresComboBox->setLoresEnabled(true);
                }
            } else if (key == "loresCustom") {
                // Legacy support - enable lores
                loresComboBox->setLoresEnabled(true);
            } else if (key == "sharpness") {
                sharpnessInput->setText(value);
                sharpnessSlider->setValue(static_cast<int>(value.toDouble() * 10));
            } else if (key == "ev") {
                evInput->setText(value);
                evSlider->setValue(static_cast<int>(value.toDouble() * 10));
            } else if (key == "gain") {
                gainInput->setText(value);
                gainSlider->setValue(static_cast<int>(value.toDouble() * 10));
            } else if (key == "awbgains") {
                QStringList gains = value.split(",");
                if (gains.size() == 2) {
                    double redGain = gains[0].toDouble();
                    double blueGain = gains[1].toDouble();
                    awbGainRedInput->setText(QString::number(redGain, 'f', 1));
                    awbGainRedSlider->setValue(static_cast<int>(redGain * 10));
                    awbGainBlueInput->setText(QString::number(blueGain, 'f', 1));
                    awbGainBlueSlider->setValue(static_cast<int>(blueGain * 10));
                }
            } else if (key == "ccm") {
                if (ccmInput) ccmInput->setText(value);
            } else if (key == "brightness") {
                brightnessInput->setText(value);
                brightnessSlider->setValue(static_cast<int>(value.toDouble() * 10));
            } else if (key == "contrast") {
                contrastInput->setText(value);
                contrastSlider->setValue(static_cast<int>(value.toDouble() * 10));
            } else if (key == "saturation") {
                saturationInput->setText(value);
                saturationSlider->setValue(static_cast<int>(value.toDouble() * 10));
            } else if (key == "post-process-file") {
                postProcessFileSelector->setCurrentText(value);
            } else if (key == "post-process-libs") {
                m_postProcessLibsPath = value;
            } else if (key == "tuning-file") {
                tuningFileSelector->setCurrentText(value);
            } else if (key == "autofocus-mode") {
                if (autofocusModeSelector) autofocusModeSelector->setCurrentText(value);
            } else if (key == "autofocus-range") {
                if (autofocusRangeSelector) autofocusRangeSelector->setCurrentText(value);
            } else if (key == "autofocus-speed") {
                if (autofocusSpeedSelector) autofocusSpeedSelector->setCurrentText(value);
            } else if (key == "autofocus-window") {
                if (autofocusWindowInput) autofocusWindowInput->setText(value);
            } else if (key == "lens-position") {
                if (lensPositionInput) {
                    lensPositionInput->setText(value);
                    lensPositionSlider->setValue(static_cast<int>(value.toDouble() * 100));
                }
            } else if (key == "output") {
                // Check if output contains %04d pattern and remove it for display
                QString outputValue = value;
                if (outputValue.contains("_%04d")) {
                    if (segmentPatternCheckbox) {
                        segmentPatternCheckbox->setChecked(true);
                    }
                    // Remove %04d from filename for display in the text field
                    outputValue.replace("_%04d", "");
                }
                outputFileName->setText(outputValue);
            } else if (key == "timelapse") {
                timelapseInput->setCurrentText(value);
            } else if (key == "shutter") {
                if (shutterValueInput) {
                    bool ok;
                    int us = value.toInt(&ok);
                    int maxUs = shutterMaxUs();
                    if (ok && us >= 100 && shutterSlider) {
                        double logRange = std::log10(static_cast<double>(maxUs)) - 2.0;
                        if (logRange <= 0.0) logRange = 1.0;
                        double logPos = (std::log10(static_cast<double>(us)) - 2.0) * 1000.0 / logRange;
                        int pos = static_cast<int>(std::round(logPos));
                        if (pos < 1) pos = 1;
                        if (pos > 1000) pos = 1000;
                        shutterSlider->setValue(pos);
                        updateShutterDisplay(us);
                    } else if (ok && us == 0) {
                        updateShutterDisplay(0);
                    }
                }
            } else if (key == "hdr") {
                if (hdrSelector) hdrSelector->setCurrentText(value);
            } else if (key == "denoise") {
                if (denoiseSelector) denoiseSelector->setCurrentText(value);
            } else if (key == "flicker-period") {
                if (flickerPeriodSelector) {
                    // Find item by data role value
                    int idx = flickerPeriodSelector->findData(value);
                    if (idx >= 0) {
                        flickerPeriodSelector->setCurrentIndex(idx);
                    } else {
                        // Custom value - directly set in editable combo
                        flickerPeriodSelector->setCurrentText(value);
                    }
                }
            } else if (key == "metadata") {
                // If metadata value matches output base, enable auto-naming
                QString outputBase = outputFileName ? outputFileName->text().trimmed() : "";
                if (!outputBase.isEmpty() && value == outputBase) {
                    // Auto-naming mode
                    if (metadataAutoNamingCheckbox) metadataAutoNamingCheckbox->setChecked(true);
                    if (metadataFileEdit) metadataFileEdit->clear();
                } else {
                    // Explicit filename mode
                    if (metadataAutoNamingCheckbox) metadataAutoNamingCheckbox->setChecked(false);
                    if (metadataFileEdit) metadataFileEdit->setText(value);
                }
            } else if (key == "metadata-format") {
                if (metadataFormatSelector) metadataFormatSelector->setCurrentText(value);
            } else if (key == "roi") {
                if (roiInput) roiInput->setText(value);
            } else if (key == "autofocus-on-capture") {
                if (autofocusOnCaptureCheckbox && value == "1") {
                    autofocusOnCaptureCheckbox->setChecked(true);
                }
            } else if (key == "zsl") {
                if (zslCheckbox && value == "1") {
                    zslCheckbox->setChecked(true);
                }
            } else if (key == "immediate") {
                if (immediateCheckbox && value == "1") {
                    immediateCheckbox->setChecked(true);
                }
            } else if (key == "framestart") {
                if (framestartSpinBox) {
                    framestartSpinBox->setValue(value.toInt());
                }
            } else if (key == "thumb") {
                if (thumbLineEdit) thumbLineEdit->setText(value);
            } else if (key == "restart") {
                if (restartSpinBox) {
                    restartSpinBox->setValue(value.toInt());
                }
            } else if (key == "exif") {
                if (exifLineEdit) exifLineEdit->setText(value);
            } else if (key == "latest") {
                if (latestLineEdit) latestLineEdit->setText(value);
            } else if (key == "raw") {
                if (rawCheckbox && value == "1") {
                    rawCheckbox->setChecked(true);
                }
            } else if (key == "hflip") {
                if (hflipCheckbox && value == "1") {
                    hflipCheckbox->setChecked(true);
                }
            } else if (key == "vflip") {
                if (vflipCheckbox && value == "1") {
                    vflipCheckbox->setChecked(true);
                }
            } else if (key == "rotation") {
                if (rotationCheckbox && value == "180") {
                    rotationCheckbox->setChecked(true);
                }
            } else if (key == "info-text") {
                // Lade Info-Text Parameter von Checkboxen (unterstützt sowohl Labels als auch direkte Parameter)
                QString infoText = value.trimmed();

                // Setze alle Checkboxen zurück
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

                // Parse die Textfelder und setze entsprechende Checkboxen
                if (infoText.contains("%frame") && infoTextFrameCheckbox) {
                    infoTextFrameCheckbox->setChecked(true);
                }
                if (infoText.contains("%fps") && infoTextFpsCheckbox) {
                    infoTextFpsCheckbox->setChecked(true);
                }
                if (infoText.contains("%exp") && infoTextExpCheckbox) {
                    infoTextExpCheckbox->setChecked(true);
                }
                if (infoText.contains("%ag") && infoTextAgCheckbox) {
                    infoTextAgCheckbox->setChecked(true);
                }
                if (infoText.contains("%dg") && infoTextDgCheckbox) {
                    infoTextDgCheckbox->setChecked(true);
                }
                if (infoText.contains("%rg") && infoTextRgCheckbox) {
                    infoTextRgCheckbox->setChecked(true);
                }
                if (infoText.contains("%bg") && infoTextBgCheckbox) {
                    infoTextBgCheckbox->setChecked(true);
                }
                if (infoText.contains("%focus") && infoTextFocusCheckbox) {
                    infoTextFocusCheckbox->setChecked(true);
                }
                if (infoText.contains("%aelock") && infoTextAelockCheckbox) {
                    infoTextAelockCheckbox->setChecked(true);
                }
                if (infoText.contains("%lp") && infoTextLpCheckbox) {
                    infoTextLpCheckbox->setChecked(true);
                }
                if (infoText.contains("%afstate") && infoTextAfstateCheckbox) {
                    infoTextAfstateCheckbox->setChecked(true);
                }
            } else if (key == "detection-filter") {
                // Detection filter is now managed by ActionsPlugin (P18) - ignored in legacy parser
            } else if (key == "action-play-sound" || key == "action-sound-file" ||
                       key == "action-save-image"  || key == "action-image-folder") {
                // Actions are now managed by ActionsPlugin (P18) - ignored in legacy parser
            } else if (key == "signal") {
                if (signalRecordingCheckbox) signalRecordingCheckbox->setChecked(value == "1");
            } else if (key == "initial") {
                if (initialStateComboBox) initialStateComboBox->setCurrentText(value);
            } else if (key == "segment") {
                // Lade Segment Duration (Millisekunden)
                if (segmentDurationInput) {
                    segmentDurationInput->setText(value);
                }
            } else if (key == "split") {
                if (splitFilesCheckbox) splitFilesCheckbox->setChecked(value == "1");
            } else if (key == "circular") {
                if (circularBufferInput) circularBufferInput->setText(value);
            } else if (key == "keypress") {
                if (keypressRecordingCheckbox) keypressRecordingCheckbox->setChecked(value == "1");
            } else if (key == "codec") {
                if (codecSelector) codecSelector->setCurrentText(value);
            } else if (key == "encoding") {
                if (encodingSelector) {
                    // Finde den Index mit dem passenden data-Wert
                    for (int i = 0; i < encodingSelector->count(); i++) {
                        if (encodingSelector->itemData(i).toString() == value) {
                            encodingSelector->setCurrentIndex(i);
                            break;
                        }
                    }
                }
            } else if (key == "libav-format") {
                if (libavFormatSelector) libavFormatSelector->setCurrentText(value);
            } else if (key == "libav-video-codec") {
                if (libavVideoCodecSelector) libavVideoCodecSelector->setCurrentText(value);
            } else if (key == "libav-video-codec-opts") {
                if (libavCodecOptsSelector) libavCodecOptsSelector->setCurrentText(value);
            } else if (key == "low-latency") {
                if (lowLatencyCheckbox) lowLatencyCheckbox->setChecked(value == "1");
            } else if (key == "sync") {
                if (syncSelector) syncSelector->setCurrentText(value);
            } else if (key == "viewfinder-mode") {
                if (viewfinderModeSelector) {
                    int index = viewfinderModeSelector->findData(value);
                    if (index != -1) viewfinderModeSelector->setCurrentIndex(index);
                }
            } else if (key == "viewfinder-width") {
                if (viewfinderWidthSpinBox) viewfinderWidthSpinBox->setValue(value.toInt());
            } else if (key == "viewfinder-height") {
                if (viewfinderHeightSpinBox) viewfinderHeightSpinBox->setValue(value.toInt());
            } else if (key == "buffer-count") {
                if (bufferCountSpinBox) bufferCountSpinBox->setValue(value.toInt());
            } else if (key == "viewfinder-buffer-count") {
                if (viewfinderBufferCountSpinBox) viewfinderBufferCountSpinBox->setValue(value.toInt());
            } else if (key == "profile") {
                if (profileSelector) profileSelector->setCurrentText(value);
            } else if (key == "level") {
                if (levelSelector) levelSelector->setCurrentText(value);
            } else if (key == "inline") {
                if (inlineHeadersCheckbox) inlineHeadersCheckbox->setChecked(value == "1");
            } else if (key == "bitrate") {
                if (bitrateSpinBox) bitrateSpinBox->setValue(value.toInt());
            } else if (key == "quality") {
                if (qualitySpinBox) qualitySpinBox->setValue(value.toInt());
            } else if (key == "intra") {
                if (intraSpinBox) intraSpinBox->setValue(value.toInt());
            } else if (key == "frames") {
                if (framesSpinBox) framesSpinBox->setValue(value.toInt());
            } else if (key == "flush") {
                if (flushCheckbox) flushCheckbox->setChecked(value == "1");
            } else if (key == "save-pts") {
                if (savePtsInput) savePtsInput->setText(value);
            } else if (key == "libav-audio") {
                if (enableAudioCheckBox) enableAudioCheckBox->setChecked(value != "0");
            } else if (key == "audio-codec") {
                if (audioCodecSelector) audioCodecSelector->setCurrentText(value);
            } else if (key == "audio-bitrate") {
                if (audioBitrateSpinBox) audioBitrateSpinBox->setValue(value.toInt());
            } else if (key == "audio-source") {
                if (audioSourceSelector) audioSourceSelector->setCurrentText(value);
            } else if (key == "audio-device") {
                if (audioDeviceEdit) audioDeviceEdit->setText(value);
            } else if (key == "audio-channels") {
                if (audioChannelsSpinBox) audioChannelsSpinBox->setValue(value.toInt());
            } else if (key == "audio-samplerate") {
                if (audioSampleRateSelector) audioSampleRateSelector->setCurrentText(value);
            } else if (key == "av-sync") {
                if (audioAvSyncSpinBox) audioAvSyncSpinBox->setValue(value.toInt());
            }
        }

        // Setze die Auflösung, falls Breite und Höhe vorhanden sind
        if (!width.isEmpty() && !height.isEmpty()) {
            QString resolution = width + "x" + height;
            if (resolutionSelector->findText(resolution) == -1) {
                resolutionSelector->addItem(resolution); // Füge die Auflösung hinzu, falls sie nicht existiert
            }
            resolutionSelector->setCurrentText(resolution); // Setze die aktuelle Auflösung
        }

        // Setze die Framerate nach der Auflösung (damit updateFramerateOptions sie nicht überschreibt)
        if (!framerate.isEmpty()) {
            // Framerate aus der Config direkt übernehmen (editierbares Feld).
            framerateSelector->setCurrentText(framerate);
        }

        file.close();
        appendLog(tr("Configuration loaded from ") + filePath);

        // Load Detection Action Settings from the global config
        QSettings guiSettings(AppPaths::globalConf(), QSettings::IniFormat);
        guiSettings.beginGroup(m_tabGroup);

        // P12/P18: ActionsPlugin verwaltet alle Action-Widgets intern
        if (m_actionsTab && m_actionsTab->module()) {
            m_actionsTab->loadSettings(guiSettings);
            appendLog(tr("Detection actions loaded via ActionsTab"));
        }
        guiSettings.endGroup();

        // Refresh the GUI
        cameraSelector->update();
        previewSelector->update();
        resolutionSelector->update();
        framerateSelector->update();
        postProcessFileSelector->update();
    } else {
        appendLog(tr("Failed to load configuration from ") + filePath);
    }
}

// Remove all user-defined startup defaults, restoring hardcoded rpicam-apps defaults.
void MainWindow::resetStartupDefaults()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    // Use allKeys() instead of childKeys() because QSettings treats "/" as a group
    // separator — Defaults/Codec is seen as subgroup "Defaults" containing key "Codec",
    // so childKeys() at Camera0-Tab level does not return it.
    QStringList keys = settings.allKeys();
    for (const QString &key : keys) {
        if (key.startsWith("Defaults/"))
            settings.remove(key);
    }
    settings.endGroup();
    settings.sync();

    // Check if auto-restart is enabled for reset defaults
    // Read from [General] first, fall back to [CameraN-Tab] for migration
    settings.beginGroup("General");
    bool autoRestart = settings.value("Defaults/AutoRestartReset", QVariant()).toBool();
    settings.endGroup();
    if (!autoRestart && settings.value("General/Defaults/AutoRestartReset", QVariant()).isNull()) {
        settings.beginGroup(m_tabGroup);
        autoRestart = settings.value("Defaults/AutoRestartReset", true).toBool();
        settings.endGroup();
    }

    if (autoRestart) {
        // Show brief notification before restart
        QDialog *notify = new QDialog(this, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        notify->setWindowModality(Qt::ApplicationModal);
        notify->setFixedSize(400, 80);
        QVBoxLayout *layout = new QVBoxLayout(notify);
        QLabel *label = new QLabel(tr("Defaults cleared. Restarting..."), notify);
        label->setAlignment(Qt::AlignCenter);
        QFont font = label->font();
        font.setPointSize(12);
        label->setFont(font);
        layout->addWidget(label);
        notify->show();

        QTimer::singleShot(1000, [notify]() {
            notify->close();
            QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
            QApplication::quit();
        });
    }
}

// Save all current UI widget values into the given QSettings group.
// Shared by "Set Defaults" (group = camera tab group) and by profile
// snapshots (group = Profiles/<name>/Cam<N>). Camera-specific values
// (resolution, framerate, viewfinder mode, ...) must not leak into the
// other camera tab.
// includePreviewBox=false: skip the preview position (see MainWindow.h).
void MainWindow::saveWidgetValuesToGroup(const QString &group, bool includePreviewBox)
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);

    settings.beginGroup(group);

    // --- App ---
    settings.setValue("Defaults/App", appSelector->currentText());

    // --- Output settings (mode, filename, naming checkboxes) ---
    QString outputMode = QStringLiteral("file");
    if (outputModeGroup) {
        QAbstractButton *checked = outputModeGroup->checkedButton();
        if (checked == outputModeGStreamer) outputMode = QStringLiteral("gstreamer");
        else if (checked == outputModeTCP) outputMode = QStringLiteral("tcp");
        else if (checked == outputModeUDP) outputMode = QStringLiteral("udp");
    }
    settings.setValue("Defaults/OutputMode", outputMode);
    if (outputFileName && !outputFileName->text().trimmed().isEmpty())
        settings.setValue("Defaults/OutputFileName", outputFileName->text().trimmed());
    else
        settings.remove("Defaults/OutputFileName");
    if (autoNamingCheckbox)
        settings.setValue("Defaults/AutoNaming", autoNamingCheckbox->isChecked() ? "1" : "0");
    if (timestampCheckbox)
        settings.setValue("Defaults/Timestamp", timestampCheckbox->isChecked() ? "1" : "0");
    if (segmentPatternCheckbox)
        settings.setValue("Defaults/SegmentPattern", segmentPatternCheckbox->isChecked() ? "1" : "0");

    // --- Sync start/stop toggle (shared across camera tabs) ---
    settings.setValue("Defaults/SyncStartStop", m_syncStartStop ? "1" : "0");

    // --- Preview ---
    settings.setValue("Defaults/Preview", previewSelector->currentData().toString());
    if (includePreviewBox) {
        // Only profiles persist the preview position. "Set Defaults" must not,
        // because the auto-calculated position becomes stale after the main
        // window has been moved (then it re-pins the old coordinates).
        if (!BoxInput->text().isEmpty()) {
            settings.setValue("Defaults/Box", BoxInput->text());
        } else {
            settings.remove("Defaults/Box");
        }
    } else {
        settings.remove("Defaults/Box"); // clean up any legacy value
    }

    // --- Resolution ---
    QStringList resParts = resolutionSelector->currentText().split("x");
    if (resParts.size() == 2) {
        settings.setValue("Defaults/Width", resParts[0]);
        settings.setValue("Defaults/Height", resParts[1]);
    }

    // --- Framerate ---
    if (!framerateSelector->currentText().isEmpty()) {
        settings.setValue("Defaults/Framerate", framerateSelector->currentText());
    } else {
        settings.remove("Defaults/Framerate");
    }

    // --- Timeout ---
    if (!timeoutSelector->currentText().isEmpty()) {
        settings.setValue("Defaults/Timeout", timeoutSelector->currentText());
    } else {
        settings.remove("Defaults/Timeout");
    }

    // --- Timelapse ---
    if (!timelapseInput->currentText().isEmpty()) {
        settings.setValue("Defaults/Timelapse", timelapseInput->currentText());
    } else {
        settings.remove("Defaults/Timelapse");
    }

    // --- AWB ---
    settings.setValue("Defaults/AWB", awbSelector->currentText());

    // --- Metering ---
    if (meteringSelector->currentText() == "custom") {
        settings.setValue("Defaults/Metering", meteringCustomInput->text().trimmed());
    } else if (meteringSelector->currentText() != "Select option:") {
        settings.setValue("Defaults/Metering", meteringSelector->currentText());
    } else {
        settings.remove("Defaults/Metering");
    }

    // --- LoRes ---
    settings.setValue("Defaults/LoresPar", loresComboBox->isParEnabled() ? "1" : "0");
    if (loresComboBox->getWidth() > 0)
        settings.setValue("Defaults/LoresWidth", loresComboBox->getWidth());
    else
        settings.remove("Defaults/LoresWidth");
    if (loresComboBox->getHeight() > 0)
        settings.setValue("Defaults/LoresHeight", loresComboBox->getHeight());
    else
        settings.remove("Defaults/LoresHeight");

    // --- Image controls ---
    settings.setValue("Defaults/Sharpness", sharpnessInput->text());
    settings.setValue("Defaults/EV", evInput->text());
    settings.setValue("Defaults/Gain", gainInput->text());
    settings.setValue("Defaults/AWBGains", awbGainRedInput->text() + "," + awbGainBlueInput->text());
    if (ccmInput && !ccmInput->text().isEmpty())
        settings.setValue("Defaults/CCM", ccmInput->text().trimmed());
    else
        settings.remove("Defaults/CCM");
    if (!m_previewLibsPath.isEmpty())
        settings.setValue("Defaults/PreviewLibs", m_previewLibsPath);
    else
        settings.remove("Defaults/PreviewLibs");
    if (!m_encoderLibsPath.isEmpty())
        settings.setValue("Defaults/EncoderLibs", m_encoderLibsPath);
    else
        settings.remove("Defaults/EncoderLibs");
    settings.setValue("Defaults/Brightness", brightnessInput->text());
    settings.setValue("Defaults/Contrast", contrastInput->text());
    settings.setValue("Defaults/Saturation", saturationInput->text());

    // --- Codec (video params) ---
    QString codec = codecSelector->currentText();
    settings.setValue("Defaults/Codec", codec);

    // --- LibAV options ---
    if (libavFormatSelector)
        settings.setValue("Defaults/LibavFormat", libavFormatSelector->currentText());
    if (libavVideoCodecSelector)
        settings.setValue("Defaults/LibavVideoCodec", libavVideoCodecSelector->currentText());
    if (libavCodecOptsSelector)
        settings.setValue("Defaults/LibavCodecOpts", libavCodecOptsSelector->currentText().trimmed());
    if (lowLatencyCheckbox)
        settings.setValue("Defaults/LowLatency", lowLatencyCheckbox->isChecked() ? "1" : "0");

    // --- H.264 options ---
    if (profileSelector)
        settings.setValue("Defaults/Profile", profileSelector->currentText());
    if (levelSelector)
        settings.setValue("Defaults/Level", levelSelector->currentText());
    if (inlineHeadersCheckbox)
        settings.setValue("Defaults/Inline", inlineHeadersCheckbox->isChecked() ? "1" : "0");

    // --- Sync ---
    if (syncSelector)
        settings.setValue("Defaults/Sync", syncSelector->currentText());

    // --- Bitrate / Quality / Intra / Frames ---
    if (bitrateSpinBox)
        settings.setValue("Defaults/Bitrate", bitrateSpinBox->value());
    if (qualitySpinBox)
        settings.setValue("Defaults/Quality", qualitySpinBox->value());
    if (intraSpinBox)
        settings.setValue("Defaults/Intra", intraSpinBox->value());
    if (framesSpinBox)
        settings.setValue("Defaults/Frames", framesSpinBox->value());

    // --- Flush ---
    if (flushCheckbox)
        settings.setValue("Defaults/Flush", flushCheckbox->isChecked() ? "1" : "0");

    // --- Save PTS ---
    if (savePtsInput && !savePtsInput->text().trimmed().isEmpty())
        settings.setValue("Defaults/SavePts", savePtsInput->text().trimmed());
    else
        settings.remove("Defaults/SavePts");

    // --- Segmentation ---
    if (segmentDurationInput && !segmentDurationInput->text().trimmed().isEmpty())
        settings.setValue("Defaults/Segment", segmentDurationInput->text().trimmed());
    else
        settings.remove("Defaults/Segment");
    if (splitFilesCheckbox)
        settings.setValue("Defaults/Split", splitFilesCheckbox->isChecked() ? "1" : "0");
    if (signalRecordingCheckbox)
        settings.setValue("Defaults/Signal", signalRecordingCheckbox->isChecked() ? "1" : "0");
    if (keypressRecordingCheckbox)
        settings.setValue("Defaults/Keypress", keypressRecordingCheckbox->isChecked() ? "1" : "0");
    if (circularBufferInput && !circularBufferInput->text().trimmed().isEmpty())
        settings.setValue("Defaults/Circular", circularBufferInput->text().trimmed());
    else
        settings.remove("Defaults/Circular");
    if (initialStateComboBox)
        settings.setValue("Defaults/Initial", initialStateComboBox->currentText());

    // --- Expert ---
    if (viewfinderModeSelector)
        settings.setValue("Defaults/ViewfinderMode", viewfinderModeSelector->currentData().toString());
    if (viewfinderWidthSpinBox)
        settings.setValue("Defaults/ViewfinderWidth", viewfinderWidthSpinBox->value());
    if (viewfinderHeightSpinBox)
        settings.setValue("Defaults/ViewfinderHeight", viewfinderHeightSpinBox->value());
    if (bufferCountSpinBox)
        settings.setValue("Defaults/BufferCount", bufferCountSpinBox->value());
    if (viewfinderBufferCountSpinBox)
        settings.setValue("Defaults/ViewfinderBufferCount", viewfinderBufferCountSpinBox->value());

    // --- Encoding (still/jpeg) ---
    if (encodingSelector)
        settings.setValue("Defaults/Encoding", encodingSelector->currentData().toString());

    // --- LibAV audio ---
    if (enableAudioCheckBox)
        settings.setValue("Defaults/LibavAudio", enableAudioCheckBox->isChecked() ? "1" : "0");
    if (audioCodecSelector)
        settings.setValue("Defaults/AudioCodec", audioCodecSelector->currentText());
    if (audioBitrateSpinBox)
        settings.setValue("Defaults/AudioBitrate", audioBitrateSpinBox->value());
    if (audioSourceSelector)
        settings.setValue("Defaults/AudioSource", audioSourceSelector->currentText());
    if (audioDeviceEdit)
        settings.setValue("Defaults/AudioDevice", audioDeviceEdit->text().trimmed());
    if (audioChannelsSpinBox)
        settings.setValue("Defaults/AudioChannels", audioChannelsSpinBox->value());
    if (audioSampleRateSelector)
        settings.setValue("Defaults/AudioSampleRate", audioSampleRateSelector->currentText());
    if (audioAvSyncSpinBox)
        settings.setValue("Defaults/AvSync", audioAvSyncSpinBox->value());

    // --- Geometry ---
    if (hflipCheckbox)
        settings.setValue("Defaults/Hflip", hflipCheckbox->isChecked() ? "1" : "0");
    if (vflipCheckbox)
        settings.setValue("Defaults/Vflip", vflipCheckbox->isChecked() ? "1" : "0");
    if (rotationCheckbox)
        settings.setValue("Defaults/Rotation", rotationCheckbox->isChecked() ? "1" : "0");
    if (doubleSizeCheckbox)
        settings.setValue("Defaults/DoubleSize", doubleSizeCheckbox->isChecked() ? "1" : "0");

    // --- Shutter ---
    int shutterUs = parseShutterInput();
    if (shutterUs > 0)
        settings.setValue("Defaults/Shutter", shutterUs);
    else
        settings.remove("Defaults/Shutter");

    // --- HDR / Denoise ---
    if (hdrSelector)
        settings.setValue("Defaults/HDR", hdrSelector->currentText());
    if (denoiseSelector)
        settings.setValue("Defaults/Denoise", denoiseSelector->currentText());

    // --- Flicker ---
    if (flickerPeriodSelector) {
        QString fd = flickerPeriodSelector->currentData().toString();
        if (fd.isEmpty())
            fd = flickerPeriodSelector->currentText().trimmed();
        settings.setValue("Defaults/FlickerPeriod", fd);
    }

    // --- ROI ---
    if (roiInput && !roiInput->text().trimmed().isEmpty())
        settings.setValue("Defaults/ROI", roiInput->text().trimmed());
    else
        settings.remove("Defaults/ROI");

    // --- Still tab ---
    if (autofocusOnCaptureCheckbox)
        settings.setValue("Defaults/AutofocusOnCapture", autofocusOnCaptureCheckbox->isChecked() ? "1" : "0");
    if (zslCheckbox)
        settings.setValue("Defaults/ZSL", zslCheckbox->isChecked() ? "1" : "0");
    if (immediateCheckbox)
        settings.setValue("Defaults/Immediate", immediateCheckbox->isChecked() ? "1" : "0");
    if (framestartSpinBox)
        settings.setValue("Defaults/Framestart", framestartSpinBox->value());
    if (thumbLineEdit)
        settings.setValue("Defaults/Thumb", thumbLineEdit->text().trimmed());
    if (restartSpinBox)
        settings.setValue("Defaults/Restart", restartSpinBox->value());
    if (exifLineEdit)
        settings.setValue("Defaults/Exif", exifLineEdit->text().trimmed());
    if (latestLineEdit)
        settings.setValue("Defaults/Latest", latestLineEdit->text().trimmed());
    if (rawCheckbox)
        settings.setValue("Defaults/Raw", rawCheckbox->isChecked() ? "1" : "0");

    // --- Post-process ---
    if (!postProcessFileSelector->currentText().trimmed().isEmpty())
        settings.setValue("Defaults/PostProcessFile", postProcessFileSelector->currentText().trimmed());
    else
        settings.remove("Defaults/PostProcessFile");
    if (!m_postProcessLibsPath.isEmpty())
        settings.setValue("Defaults/PostProcessLibs", m_postProcessLibsPath);
    else
        settings.remove("Defaults/PostProcessLibs");

    // --- Tuning file ---
    if (!tuningFileSelector->currentText().trimmed().isEmpty())
        settings.setValue("Defaults/TuningFile", tuningFileSelector->currentText().trimmed());
    else
        settings.remove("Defaults/TuningFile");

    // --- Autofocus ---
    if (autofocusModeSelector)
        settings.setValue("Defaults/AutofocusMode", autofocusModeSelector->currentText());
    if (autofocusRangeSelector)
        settings.setValue("Defaults/AutofocusRange", autofocusRangeSelector->currentText());
    if (autofocusSpeedSelector)
        settings.setValue("Defaults/AutofocusSpeed", autofocusSpeedSelector->currentText());
    if (autofocusWindowInput && !autofocusWindowInput->text().trimmed().isEmpty())
        settings.setValue("Defaults/AutofocusWindow", autofocusWindowInput->text().trimmed());
    else
        settings.remove("Defaults/AutofocusWindow");

    // --- Lens position ---
    if (lensPositionInput)
        settings.setValue("Defaults/LensPosition", lensPositionInput->text());

    // --- Info text ---
    QStringList infoParts;
    if (infoTextFrameCheckbox && infoTextFrameCheckbox->isChecked())   infoParts << "%frame";
    if (infoTextFpsCheckbox && infoTextFpsCheckbox->isChecked())       infoParts << "%fps";
    if (infoTextExpCheckbox && infoTextExpCheckbox->isChecked())       infoParts << "%exp";
    if (infoTextAgCheckbox && infoTextAgCheckbox->isChecked())         infoParts << "%ag";
    if (infoTextDgCheckbox && infoTextDgCheckbox->isChecked())         infoParts << "%dg";
    if (infoTextRgCheckbox && infoTextRgCheckbox->isChecked())         infoParts << "%rg";
    if (infoTextBgCheckbox && infoTextBgCheckbox->isChecked())         infoParts << "%bg";
    if (infoTextFocusCheckbox && infoTextFocusCheckbox->isChecked())   infoParts << "%focus";
    if (infoTextAelockCheckbox && infoTextAelockCheckbox->isChecked()) infoParts << "%aelock";
    if (infoTextLpCheckbox && infoTextLpCheckbox->isChecked())         infoParts << "%lp";
    if (infoTextAfstateCheckbox && infoTextAfstateCheckbox->isChecked()) infoParts << "%afstate";
    if (!infoParts.isEmpty())
        settings.setValue("Defaults/InfoText", infoParts.join(" "));
    else
        settings.remove("Defaults/InfoText");

    settings.endGroup();
    settings.sync();
}

// Save all current UI widget values as startup defaults in QSettings.
// These are applied on next launch before isInitializing becomes false.
void MainWindow::saveStartupDefaults()
{
    saveWidgetValuesToGroup(m_tabGroup, /*includePreviewBox=*/false);

    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);

    // Check if auto-restart is enabled for set defaults
    // Read from [General] first, fall back to [CameraN-Tab] for migration
    settings.beginGroup("General");
    bool autoRestart = settings.value("Defaults/AutoRestartSet", QVariant()).toBool();
    settings.endGroup();
    if (!autoRestart && settings.value("General/Defaults/AutoRestartSet", QVariant()).isNull()) {
        settings.beginGroup(m_tabGroup);
        autoRestart = settings.value("Defaults/AutoRestartSet", true).toBool();
        settings.endGroup();
    }

    if (autoRestart) {
        // Show brief notification before restart
        QDialog *notify = new QDialog(this, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        notify->setWindowModality(Qt::ApplicationModal);
        notify->setFixedSize(400, 80);
        QVBoxLayout *layout = new QVBoxLayout(notify);
        QLabel *label = new QLabel(tr("Defaults saved. Restarting..."), notify);
        label->setAlignment(Qt::AlignCenter);
        QFont font = label->font();
        font.setPointSize(12);
        label->setFont(font);
        layout->addWidget(label);
        notify->show();

        QTimer::singleShot(1000, [notify]() {
            notify->close();
            QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
            QApplication::quit();
        });
    } else {
        appendLog(tr("Startup defaults saved. These will be applied on next launch."));
    }
}

// Load startup defaults and apply them to the UI widgets.
// Priority: an active profile that includes this camera wins over the
// regular startup defaults ("Set Defaults"). Otherwise the camera's own
// defaults group is used.
void MainWindow::loadStartupDefaults()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);

    const QString active = settings.value("Profiles/Active").toString();
    if (!active.isEmpty()) {
        const QString camGroup = QString("Profiles/%1/Cam%2").arg(active).arg(m_fixedCameraIdx);
        settings.beginGroup(camGroup);
        const bool inScope = !settings.allKeys().isEmpty();
        settings.endGroup();
        if (inScope) {
            loadWidgetValuesFromGroup(camGroup);
            return;
        }
    }

    loadWidgetValuesFromGroup(m_tabGroup, /*includePreviewBox=*/false);
}

// Load widget values from the given QSettings group into the UI widgets.
// Only keys that actually exist in QSettings are applied — missing keys
// keep their hardcoded rpicam-apps defaults.
// includePreviewBox=false: ignore any Defaults/Box key (see MainWindow.h).
void MainWindow::loadWidgetValuesFromGroup(const QString &group, bool includePreviewBox)
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(group);

    // Helper: only apply if the key exists
    auto applyIfSet = [&](const QString &key, auto setter) {
        if (settings.contains(key))
            setter(settings.value(key).toString());
    };

    // --- App ---
    applyIfSet("Defaults/App", [this](const QString &v) {
        int idx = appSelector->findText(v);
        if (idx >= 0) appSelector->setCurrentIndex(idx);
    });

    // --- Output settings (mode FIRST, so the saved filename below wins over
    //     the automatic tcp:// / udp:// URLs the mode toggle writes) ---
    applyIfSet("Defaults/OutputMode", [this](const QString &v) {
        if (!outputModeGroup) return;
        QRadioButton *target = outputModeFile;
        if (v == "gstreamer") target = outputModeGStreamer;
        else if (v == "tcp") target = outputModeTCP;
        else if (v == "udp") target = outputModeUDP;
        if (target) target->setChecked(true);
    });
    applyIfSet("Defaults/OutputFileName", [this](const QString &v) {
        if (outputFileName) outputFileName->setText(v);
    });
    // AN/TS/segment pattern: unlike the other keys these are ALWAYS applied.
    // When the loaded group has no saved value, they fall back to the
    // hardcoded default (unchecked) — otherwise a previously active profile
    // would leave them enabled ("sticky" state) after switching to None.
    auto applyCheckboxDefault = [&](const QString &key, QCheckBox *box) {
        if (!box) return;
        box->setChecked(settings.contains(key)
                            ? settings.value(key).toString() == "1"
                            : false);
    };
    applyCheckboxDefault("Defaults/AutoNaming", autoNamingCheckbox);
    applyCheckboxDefault("Defaults/Timestamp", timestampCheckbox);
    applyCheckboxDefault("Defaults/SegmentPattern", segmentPatternCheckbox);

    // Sync start/stop: like the checkboxes above this is ALWAYS applied —
    // otherwise a previously active profile would leave it enabled
    // ("sticky" state) after switching to a group without the key.
    m_syncStartStop = settings.contains("Defaults/SyncStartStop")
                          ? settings.value("Defaults/SyncStartStop").toString() == "1"
                          : false;

    // --- Preview ---
    applyIfSet("Defaults/Preview", [this](const QString &v) {
        // Never apply a remote-only preview default on a local session.
        // --qt-preview / qt are required for XRDP but break local display.
        if (!isRemoteSession() && (v == "--qt-preview" || v == "qt"))
            return;
        int idx = previewSelector->findData(v);
        if (idx >= 0) previewSelector->setCurrentIndex(idx);
    });
    if (includePreviewBox) {
        applyIfSet("Defaults/Box", [this](const QString &v) { BoxInput->setText(v); });
    }

    // --- Resolution (deferred until after Width+Height are both read) ---
    QString defWidth, defHeight;
    if (settings.contains("Defaults/Width"))  defWidth  = settings.value("Defaults/Width").toString();
    if (settings.contains("Defaults/Height")) defHeight = settings.value("Defaults/Height").toString();
    if (!defWidth.isEmpty() && !defHeight.isEmpty()) {
        QString res = defWidth + "x" + defHeight;
        if (resolutionSelector->findText(res) == -1)
            resolutionSelector->addItem(res);
        resolutionSelector->setCurrentText(res);
    }

    // --- Framerate ---
    applyIfSet("Defaults/Framerate", [this](const QString &v) { framerateSelector->setCurrentText(v); });

    // --- Timeout ---
    applyIfSet("Defaults/Timeout", [this](const QString &v) { timeoutSelector->setCurrentText(v); });

    // --- Timelapse ---
    applyIfSet("Defaults/Timelapse", [this](const QString &v) { timelapseInput->setCurrentText(v); });

    // --- AWB ---
    applyIfSet("Defaults/AWB", [this](const QString &v) { awbSelector->setCurrentText(v); });

    // --- Metering ---
    applyIfSet("Defaults/Metering", [this](const QString &v) {
        if (v == "Select option:") return; // skip placeholder
        if (v == "centre" || v == "spot" || v == "average") {
            meteringSelector->setCurrentText(v);
        } else {
            meteringSelector->setCurrentText("custom");
            meteringCustomInput->setText(v);
            meteringCustomInput->setVisible(true);
        }
    });

    // --- LoRes ---
    applyIfSet("Defaults/LoresPar", [this](const QString &v) { loresComboBox->setParEnabled(v == "1"); });
    applyIfSet("Defaults/LoresWidth", [this](const QString &v) { loresComboBox->setWidth(v.toInt()); });
    applyIfSet("Defaults/LoresHeight", [this](const QString &v) { loresComboBox->setHeight(v.toInt()); });

    // --- Image controls ---
    applyIfSet("Defaults/Sharpness", [this](const QString &v) {
        sharpnessInput->setText(v);
        sharpnessSlider->setValue(static_cast<int>(v.toDouble() * 10));
    });
    applyIfSet("Defaults/EV", [this](const QString &v) {
        evInput->setText(v);
        evSlider->setValue(static_cast<int>(v.toDouble() * 10));
    });
    applyIfSet("Defaults/Gain", [this](const QString &v) {
        gainInput->setText(v);
        gainSlider->setValue(static_cast<int>(v.toDouble() * 10));
    });
    applyIfSet("Defaults/AWBGains", [this](const QString &v) {
        QStringList gains = v.split(",");
        if (gains.size() == 2) {
            double rg = gains[0].toDouble();
            double bg = gains[1].toDouble();
            awbGainRedInput->setText(QString::number(rg, 'f', 1));
            awbGainRedSlider->setValue(static_cast<int>(rg * 10));
            awbGainBlueInput->setText(QString::number(bg, 'f', 1));
            awbGainBlueSlider->setValue(static_cast<int>(bg * 10));
        }
    });
    applyIfSet("Defaults/CCM", [this](const QString &v) {
        if (ccmInput) ccmInput->setText(v);
    });
    applyIfSet("Defaults/PreviewLibs", [this](const QString &v) {
        m_previewLibsPath = v;
    });
    applyIfSet("Defaults/EncoderLibs", [this](const QString &v) {
        m_encoderLibsPath = v;
    });
    applyIfSet("Defaults/Brightness", [this](const QString &v) {
        brightnessInput->setText(v);
        brightnessSlider->setValue(static_cast<int>(v.toDouble() * 10));
    });
    applyIfSet("Defaults/Contrast", [this](const QString &v) {
        contrastInput->setText(v);
        contrastSlider->setValue(static_cast<int>(v.toDouble() * 10));
    });
    applyIfSet("Defaults/Saturation", [this](const QString &v) {
        saturationInput->setText(v);
        saturationSlider->setValue(static_cast<int>(v.toDouble() * 10));
    });

    // --- Codec ---
    applyIfSet("Defaults/Codec", [this](const QString &v) {
        if (codecSelector) codecSelector->setCurrentText(v);
    });

    // --- LibAV ---
    applyIfSet("Defaults/LibavFormat", [this](const QString &v) { if (libavFormatSelector) libavFormatSelector->setCurrentText(v); });
    applyIfSet("Defaults/LibavVideoCodec", [this](const QString &v) { if (libavVideoCodecSelector) libavVideoCodecSelector->setCurrentText(v); });
    applyIfSet("Defaults/LibavCodecOpts", [this](const QString &v) { if (libavCodecOptsSelector) libavCodecOptsSelector->setCurrentText(v); });
    applyIfSet("Defaults/LowLatency", [this](const QString &v) { if (lowLatencyCheckbox) lowLatencyCheckbox->setChecked(v == "1"); });

    // --- H.264 ---
    applyIfSet("Defaults/Profile", [this](const QString &v) { if (profileSelector) profileSelector->setCurrentText(v); });
    applyIfSet("Defaults/Level", [this](const QString &v) { if (levelSelector) levelSelector->setCurrentText(v); });
    applyIfSet("Defaults/Inline", [this](const QString &v) { if (inlineHeadersCheckbox) inlineHeadersCheckbox->setChecked(v == "1"); });

    // --- Sync ---
    applyIfSet("Defaults/Sync", [this](const QString &v) { if (syncSelector) syncSelector->setCurrentText(v); });

    // --- Bitrate / Quality / Intra / Frames ---
    applyIfSet("Defaults/Bitrate", [this](const QString &v) { if (bitrateSpinBox) bitrateSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/Quality", [this](const QString &v) { if (qualitySpinBox) qualitySpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/Intra", [this](const QString &v) { if (intraSpinBox) intraSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/Frames", [this](const QString &v) { if (framesSpinBox) framesSpinBox->setValue(v.toInt()); });

    // --- Flush ---
    applyIfSet("Defaults/Flush", [this](const QString &v) { if (flushCheckbox) flushCheckbox->setChecked(v == "1"); });

    // --- Save PTS ---
    applyIfSet("Defaults/SavePts", [this](const QString &v) { if (savePtsInput) savePtsInput->setText(v); });

    // --- Segmentation ---
    applyIfSet("Defaults/Segment", [this](const QString &v) { if (segmentDurationInput) segmentDurationInput->setText(v); });
    applyIfSet("Defaults/Split", [this](const QString &v) { if (splitFilesCheckbox) splitFilesCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/Signal", [this](const QString &v) { if (signalRecordingCheckbox) signalRecordingCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/Keypress", [this](const QString &v) { if (keypressRecordingCheckbox) keypressRecordingCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/Circular", [this](const QString &v) { if (circularBufferInput) circularBufferInput->setText(v); });
    applyIfSet("Defaults/Initial", [this](const QString &v) { if (initialStateComboBox) initialStateComboBox->setCurrentText(v); });

    // --- Expert ---
    applyIfSet("Defaults/ViewfinderMode", [this](const QString &v) {
        if (viewfinderModeSelector) {
            int idx = viewfinderModeSelector->findData(v);
            if (idx >= 0) viewfinderModeSelector->setCurrentIndex(idx);
        }
    });
    applyIfSet("Defaults/ViewfinderWidth", [this](const QString &v) { if (viewfinderWidthSpinBox) viewfinderWidthSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/ViewfinderHeight", [this](const QString &v) { if (viewfinderHeightSpinBox) viewfinderHeightSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/BufferCount", [this](const QString &v) { if (bufferCountSpinBox) bufferCountSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/ViewfinderBufferCount", [this](const QString &v) { if (viewfinderBufferCountSpinBox) viewfinderBufferCountSpinBox->setValue(v.toInt()); });

    // --- Encoding ---
    applyIfSet("Defaults/Encoding", [this](const QString &v) {
        if (encodingSelector) {
            for (int i = 0; i < encodingSelector->count(); i++) {
                if (encodingSelector->itemData(i).toString() == v) {
                    encodingSelector->setCurrentIndex(i);
                    break;
                }
            }
        }
    });

    // --- LibAV Audio ---
    applyIfSet("Defaults/LibavAudio", [this](const QString &v) { if (enableAudioCheckBox) enableAudioCheckBox->setChecked(v == "1"); });
    applyIfSet("Defaults/AudioCodec", [this](const QString &v) { if (audioCodecSelector) audioCodecSelector->setCurrentText(v); });
    applyIfSet("Defaults/AudioBitrate", [this](const QString &v) { if (audioBitrateSpinBox) audioBitrateSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/AudioSource", [this](const QString &v) { if (audioSourceSelector) audioSourceSelector->setCurrentText(v); });
    applyIfSet("Defaults/AudioDevice", [this](const QString &v) { if (audioDeviceEdit) audioDeviceEdit->setText(v); });
    applyIfSet("Defaults/AudioChannels", [this](const QString &v) { if (audioChannelsSpinBox) audioChannelsSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/AudioSampleRate", [this](const QString &v) { if (audioSampleRateSelector) audioSampleRateSelector->setCurrentText(v); });
    applyIfSet("Defaults/AvSync", [this](const QString &v) { if (audioAvSyncSpinBox) audioAvSyncSpinBox->setValue(v.toInt()); });

    // --- Geometry ---
    applyIfSet("Defaults/Hflip", [this](const QString &v) { if (hflipCheckbox) hflipCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/Vflip", [this](const QString &v) { if (vflipCheckbox) vflipCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/Rotation", [this](const QString &v) { if (rotationCheckbox) rotationCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/DoubleSize", [this](const QString &v) { if (doubleSizeCheckbox) doubleSizeCheckbox->setChecked(v == "1"); });

    // --- Shutter ---
    applyIfSet("Defaults/Shutter", [this](const QString &v) {
        if (shutterValueInput && shutterSlider) {
            bool ok;
            int us = v.toInt(&ok);
            int maxUs = shutterMaxUs();
            if (ok && us >= 100) {
                double logRange = std::log10(static_cast<double>(maxUs)) - 2.0;
                if (logRange <= 0.0) logRange = 1.0;
                double logPos = (std::log10(static_cast<double>(us)) - 2.0) * 1000.0 / logRange;
                int pos = static_cast<int>(std::round(logPos));
                if (pos < 1) pos = 1;
                if (pos > 1000) pos = 1000;
                shutterSlider->setValue(pos);
                updateShutterDisplay(us);
            } else if (ok && us == 0) {
                updateShutterDisplay(0);
            }
        }
    });

    // --- HDR / Denoise ---
    applyIfSet("Defaults/HDR", [this](const QString &v) { if (hdrSelector) hdrSelector->setCurrentText(v); });
    applyIfSet("Defaults/Denoise", [this](const QString &v) { if (denoiseSelector) denoiseSelector->setCurrentText(v); });

    // --- Flicker ---
    applyIfSet("Defaults/FlickerPeriod", [this](const QString &v) {
        if (flickerPeriodSelector) {
            int idx = flickerPeriodSelector->findData(v);
            if (idx >= 0)
                flickerPeriodSelector->setCurrentIndex(idx);
            else
                flickerPeriodSelector->setCurrentText(v);
        }
    });

    // --- ROI ---
    applyIfSet("Defaults/ROI", [this](const QString &v) { if (roiInput) roiInput->setText(v); });

    // --- Still tab ---
    applyIfSet("Defaults/AutofocusOnCapture", [this](const QString &v) { if (autofocusOnCaptureCheckbox) autofocusOnCaptureCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/ZSL", [this](const QString &v) { if (zslCheckbox) zslCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/Immediate", [this](const QString &v) { if (immediateCheckbox) immediateCheckbox->setChecked(v == "1"); });
    applyIfSet("Defaults/Framestart", [this](const QString &v) { if (framestartSpinBox) framestartSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/Thumb", [this](const QString &v) { if (thumbLineEdit) thumbLineEdit->setText(v); });
    applyIfSet("Defaults/Restart", [this](const QString &v) { if (restartSpinBox) restartSpinBox->setValue(v.toInt()); });
    applyIfSet("Defaults/Exif", [this](const QString &v) { if (exifLineEdit) exifLineEdit->setText(v); });
    applyIfSet("Defaults/Latest", [this](const QString &v) { if (latestLineEdit) latestLineEdit->setText(v); });
    applyIfSet("Defaults/Raw", [this](const QString &v) { if (rawCheckbox) rawCheckbox->setChecked(v == "1"); });

    // --- Post-process ---
    applyIfSet("Defaults/PostProcessFile", [this](const QString &v) { postProcessFileSelector->setCurrentText(v); });
    applyIfSet("Defaults/PostProcessLibs", [this](const QString &v) {
        // Legacy values from the old dropdown could be relative (subdirectory
        // name or "."); now only absolute paths are valid (full path configured
        // in the camera setup dialog).
        if (QDir::isAbsolutePath(v)) m_postProcessLibsPath = v;
    });

    // --- Tuning file ---
    applyIfSet("Defaults/TuningFile", [this](const QString &v) { tuningFileSelector->setCurrentText(v); });

    // --- Autofocus ---
    applyIfSet("Defaults/AutofocusMode", [this](const QString &v) { if (autofocusModeSelector) autofocusModeSelector->setCurrentText(v); });
    applyIfSet("Defaults/AutofocusRange", [this](const QString &v) { if (autofocusRangeSelector) autofocusRangeSelector->setCurrentText(v); });
    applyIfSet("Defaults/AutofocusSpeed", [this](const QString &v) { if (autofocusSpeedSelector) autofocusSpeedSelector->setCurrentText(v); });
    applyIfSet("Defaults/AutofocusWindow", [this](const QString &v) { if (autofocusWindowInput) autofocusWindowInput->setText(v); });

    // --- Lens position ---
    applyIfSet("Defaults/LensPosition", [this](const QString &v) {
        if (lensPositionInput) {
            lensPositionInput->setText(v);
            lensPositionSlider->setValue(static_cast<int>(v.toDouble() * 100));
        }
    });

    // --- Info text ---
    applyIfSet("Defaults/InfoText", [this](const QString &v) {
        // Reset all first
        if (infoTextFrameCheckbox)   infoTextFrameCheckbox->setChecked(false);
        if (infoTextFpsCheckbox)     infoTextFpsCheckbox->setChecked(false);
        if (infoTextExpCheckbox)     infoTextExpCheckbox->setChecked(false);
        if (infoTextAgCheckbox)      infoTextAgCheckbox->setChecked(false);
        if (infoTextDgCheckbox)      infoTextDgCheckbox->setChecked(false);
        if (infoTextRgCheckbox)      infoTextRgCheckbox->setChecked(false);
        if (infoTextBgCheckbox)      infoTextBgCheckbox->setChecked(false);
        if (infoTextFocusCheckbox)   infoTextFocusCheckbox->setChecked(false);
        if (infoTextAelockCheckbox)  infoTextAelockCheckbox->setChecked(false);
        if (infoTextLpCheckbox)      infoTextLpCheckbox->setChecked(false);
        if (infoTextAfstateCheckbox) infoTextAfstateCheckbox->setChecked(false);
        // Set from saved string
        if (v.contains("%frame")   && infoTextFrameCheckbox)   infoTextFrameCheckbox->setChecked(true);
        if (v.contains("%fps")     && infoTextFpsCheckbox)     infoTextFpsCheckbox->setChecked(true);
        if (v.contains("%exp")     && infoTextExpCheckbox)     infoTextExpCheckbox->setChecked(true);
        if (v.contains("%ag")      && infoTextAgCheckbox)      infoTextAgCheckbox->setChecked(true);
        if (v.contains("%dg")      && infoTextDgCheckbox)      infoTextDgCheckbox->setChecked(true);
        if (v.contains("%rg")      && infoTextRgCheckbox)      infoTextRgCheckbox->setChecked(true);
        if (v.contains("%bg")      && infoTextBgCheckbox)      infoTextBgCheckbox->setChecked(true);
        if (v.contains("%focus")   && infoTextFocusCheckbox)   infoTextFocusCheckbox->setChecked(true);
        if (v.contains("%aelock")  && infoTextAelockCheckbox)  infoTextAelockCheckbox->setChecked(true);
        if (v.contains("%lp")      && infoTextLpCheckbox)      infoTextLpCheckbox->setChecked(true);
        if (v.contains("%afstate") && infoTextAfstateCheckbox) infoTextAfstateCheckbox->setChecked(true);
    });

    // Sync flag is shared across camera tabs: if either tab has it enabled
    // (e.g. stored via Set Defaults or a profile for one tab only), enable
    // it for both. OR-merge is order-independent.
    if (m_sibling && (m_syncStartStop || m_sibling->m_syncStartStop)) {
        m_syncStartStop = true;
        m_sibling->m_syncStartStop = true;
    }
    updateSyncBadgeStyle();
    if (m_sibling) m_sibling->updateSyncBadgeStyle();

    settings.endGroup();
}

// ---------------------------------------------------------------------------
// Profile snapshots — same mechanism as "Set Defaults", but stored in a
// dedicated profile group (Profiles/<name>/Cam<N>).
// ---------------------------------------------------------------------------
void MainWindow::saveProfileSnapshot(const QString &profileName)
{
    saveWidgetValuesToGroup(QString("Profiles/%1/Cam%2").arg(profileName).arg(m_fixedCameraIdx));
}

void MainWindow::loadProfileValues(const QString &profileName)
{
    loadWidgetValuesFromGroup(QString("Profiles/%1/Cam%2").arg(profileName).arg(m_fixedCameraIdx));
}
