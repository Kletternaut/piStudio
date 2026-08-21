#include "MainWindow.h"
#include "../modules/streaming/GStreamerModule.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <csignal>
#include <QFile>
#include <QTextStream>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include "Defaults.h"
void MainWindow::startRpiCamApp() {
    QString app = appSelector->currentText();
    QString camera = cameraSelector->currentText();
    QString resolution = resolutionSelector->currentText();
    QString framerate = framerateSelector->currentText();
    QString timeout = timeoutSelector->currentText();
    QString outputFile = outputFileName->text();
    QString path = guiOutputFilePath; // Initialisiere path mit guiOutputFilePath
    QString timelapse = timelapseInput->currentText();
    if (timestampCheckbox->isChecked() && !outputFile.isEmpty()) {
        QDateTime now = QDateTime::currentDateTime();
        QString timestamp = now.toString("yyyy-MM-dd_HH-mm-ss");
        QFileInfo fileInfo(outputFile);
        QString baseName = fileInfo.completeBaseName();
        QString suffix = fileInfo.suffix();
        QString directory = fileInfo.path();
        if (directory.isEmpty() || directory == ".") {
            directory = guiOutputFilePath;
        }

        // FÜR STILL-APPS: Hole Extension aus Encoding-Selector
        bool isStillApp = (app == "rpicam-still" || app == "rpicam-jpeg");
        if (isStillApp && encodingSelector) {
            QString encoding = encodingSelector->currentData().toString();
            QString ext = getExtensionForEncoding(encoding);
            // Entferne den führenden Punkt
            if (ext.startsWith(".")) suffix = ext.mid(1);
        }

        // Füge "_audio" hinzu, wenn Audio aktiviert ist
        QString audioSuffix = "";
        if (enableAudioCheckBox && enableAudioCheckBox->isChecked()) {
            audioSuffix = "_audio";
        }

        outputFile = QDir(directory).filePath(baseName + audioSuffix + "_" + timestamp + "." + suffix);
        appendLog(tr("Output file with timestamp: ") + outputFile);
    }

    // Insert %04d pattern if segmentPatternCheckbox is checked
    if (segmentPatternCheckbox && segmentPatternCheckbox->isChecked() && !outputFile.isEmpty()) {
        QFileInfo fileInfo(outputFile);
        QString baseName = fileInfo.completeBaseName();
        QString suffix = fileInfo.suffix();
        QString directory = fileInfo.path();
        if (directory.isEmpty() || directory == ".") {
            directory = guiOutputFilePath;
        }
        outputFile = QDir(directory).filePath(baseName + "_%04d." + suffix);
        appendLog(tr("Output file with segment pattern: ") + outputFile);
    }

    QStringList resolutionParts = resolution.split("x");
    QString width = resolutionParts.value(0);
    QString height = resolutionParts.value(1);
    QStringList arguments;
    arguments << "--camera" << camera
              << "--width" << width
              << "--height" << height;
    // 0 oder leer = Auto: kein --framerate übergeben
    if (!framerate.isEmpty() && framerate != "0") {
        arguments << "--framerate" << framerate;
    }
    QString previewValue = previewSelector->currentData().toString();
    if (!previewValue.isEmpty()) {
        // New --preview-backend format (rpicam-apps >= 1.13): data is just the backend name
        if (previewValue == "wayland-egl" || previewValue == "egl" ||
            previewValue == "drm" || previewValue == "qt") {
            arguments << "--preview-backend" << previewValue;
        } else {
            arguments << previewValue; // Legacy: --fullscreen, --qt-preview, --nopreview
        }
    }
    // --preview-libs (custom .so path, rpicam-apps >= 1.13)
    if (m_hasPreviewBackend && !m_previewLibsPath.isEmpty()) {
        arguments << "--preview-libs" << m_previewLibsPath;
    }
    // --encoder-libs (custom .so path, rpicam-apps >= 1.13)
    if (m_hasPreviewBackend && !m_encoderLibsPath.isEmpty()) {
        arguments << "--encoder-libs" << m_encoderLibsPath;
    }
    if (!timeout.isEmpty()) {
        bool ok;
        int timeoutMs = timeout.toInt(&ok);
        if (ok) {
            arguments << "-t" << QString::number(timeoutMs);
        } else {
            appendLog(tr("Invalid timeout value. Using default."));
            arguments << "-t" << "0";
        }
    } else {
        arguments << "-t" << "0";
    }
    if (!timelapse.isEmpty()) {
        bool ok;
        int timelapseMs = timelapse.toInt(&ok);
        if (ok) {
            arguments << "--timelapse" << QString::number(timelapseMs);
        } else {
            appendLog(tr("Invalid timelapse value. Skipping."));
        }
    }

    // Note: Das _%04d Pattern wird bereits weiter oben im Dateinamen eingefügt (Zeile ~56)
    // Der --segment Parameter mit Millisekunden wird später hinzugefügt (Zeile ~650)

    // Preview window position (must be before -o output)
    // Recalculate only if BoxInput is still in auto-mode (overlayResetButton not red = not manually overridden)
    // and not using saved custom geometry. This ensures sibling-aware offset is applied at start time.
    {
        QSettings previewSettings(AppPaths::globalConf(), QSettings::IniFormat);
        previewSettings.beginGroup(m_tabGroup);
        bool useCustom = previewSettings.value("Preview/UseCustomGeometry", false).toBool();
        previewSettings.endGroup();
        bool isManuallySet = overlayResetButton && overlayResetButton->styleSheet().contains("red");
        if (!useCustom && !isManuallySet) {
            BoxInput->setText(calculateBoxInput(+30));
        }
    }
    QString Box = BoxInput->text();
    if (!Box.isEmpty()) {
        arguments << "--preview" << Box;
    }

    // Check if signal recording is enabled (needed later)
    // IMPORTANT: Check isEnabled() to prevent disabled-but-checked state after global reset
    bool signalRecordingEnabled = (signalRecordingCheckbox &&
                                   signalRecordingCheckbox->isEnabled() &&
                                   signalRecordingCheckbox->isChecked());

    // Output file handling - check if GStreamer mode is enabled
    bool gstreamerMode = (outputModeGStreamer && outputModeGStreamer->isChecked());

    if (gstreamerMode) {
        // GStreamer mode: output to stdout (pipe)
        arguments << "-o" << "-";
    } else {
        // File mode: output to file as usual
        // Check if output is a network stream (tcp:// or udp://)
        bool isNetworkStream = outputFile.startsWith("tcp://", Qt::CaseInsensitive) ||
                               outputFile.startsWith("udp://", Qt::CaseInsensitive);

        if (!outputFile.isEmpty() && !QFileInfo(outputFile).isAbsolute() && !isNetworkStream) {
            outputFile = QDir(path).filePath(outputFile); // Kombiniere mit guiOutputFilePath
        }
        if (!outputFile.isEmpty()) {
            arguments << "-o" << outputFile;
        }
    }

    // Encoding Parameter (nur für rpicam-still / rpicam-jpeg)
    bool isStillApp = (app == "rpicam-still" || app == "rpicam-jpeg");
    if (isStillApp && encodingSelector) {
        QString encoding = encodingSelector->currentData().toString();
        if (!encoding.isEmpty()) {
            arguments << "--encoding" << encoding;
        }
    }

    QString postProcessFile = postProcessFileSelector->currentText();
    if (!postProcessFile.isEmpty()) {
        QString postProcessFilePath;
        if (QFileInfo(postProcessFile).isAbsolute()) {
            postProcessFilePath = postProcessFile;
        } else {
            postProcessFilePath = QDir(guiPostProcessFilePath).filePath(postProcessFile); // <-- dynamischer Pfad aus Setup
        }
        arguments << "--post-process-file" << postProcessFilePath;
    }

    // Post-process libs path (full path, configured in the camera setup dialog)
    if (!m_postProcessLibsPath.isEmpty()) {
        arguments << "--post-process-libs" << m_postProcessLibsPath;
    }

    QString tuningFile = tuningFileSelector->currentText();
    if (!tuningFile.isEmpty()) {
        QString tuningFilePath;
        if (QFileInfo(tuningFile).isAbsolute()) {
            tuningFilePath = tuningFile;
        } else {
            tuningFilePath = QDir(guiTuningFilePath).filePath(tuningFile); // <-- dynamischer Pfad aus Setup
        }
        arguments << "--tuning-file" << tuningFilePath;
    }

    // Autofocus Mode Parameter (nur wenn nicht Standard-Wert "auto")
    QString autofocusMode = autofocusModeSelector->currentText();
    if (!autofocusMode.isEmpty() && autofocusMode != "auto") {
        arguments << "--autofocus-mode" << autofocusMode;
    }

    // Autofocus Range Parameter
    QString autofocusRange = autofocusRangeSelector->currentText();
    if (!autofocusRange.isEmpty() && autofocusRange != "normal") {
        arguments << "--autofocus-range" << autofocusRange;
    }

    // Autofocus Speed Parameter
    QString autofocusSpeed = autofocusSpeedSelector->currentText();
    if (!autofocusSpeed.isEmpty() && autofocusSpeed != "normal") {
        arguments << "--autofocus-speed" << autofocusSpeed;
    }

    // Autofocus Window Parameter
    QString autofocusWindow = autofocusWindowInput->text();
    if (!autofocusWindow.isEmpty() && autofocusWindow != "0.333,0.333,0.333,0.333") {
        arguments << "--autofocus-window" << autofocusWindow;
    }

    // Lens Position Parameter
    double lensPosition = lensPositionInput->text().toDouble();
    if (lensPosition != DEFAULT_LENS_POSITION) {
        arguments << "--lens-position" << QString::number(lensPosition, 'f', 2);
    }

    // Note: --preview for position is already added earlier (before -o)
    // Don't add it again here to avoid duplication

    if (app == "rpicam-vid") {
        // Codec (nur wenn nicht h264 Standard)
        QString codec = codecSelector->currentText();
        if (!codec.isEmpty() && codec != "h264") {
            arguments << "--codec" << codec;

            // LibAV-spezifische Optionen
            if (codec == "libav") {
                // Format aus UI verwenden
                if (libavFormatSelector && !libavFormatSelector->currentText().isEmpty()) {
                    arguments << "--libav-format" << libavFormatSelector->currentText();
                } else {
                    arguments << "--libav-format" << "mpegts"; // Fallback
                }

                // Video Codec aus UI verwenden
                if (libavVideoCodecSelector && !libavVideoCodecSelector->currentText().isEmpty()) {
                    arguments << "--libav-video-codec" << libavVideoCodecSelector->currentText();
                } else {
                    arguments << "--libav-video-codec" << "h264_v4l2m2m"; // Fallback
                }

                // LibAV Codec Options: Profile aus Dropdown + zusätzliche Optionen kombinieren
                QString codecOpts = "";
                if (profileSelector && profileSelector->currentIndex() != -1) {
                    codecOpts = "profile=" + profileSelector->currentText();
                }
                if (libavCodecOptsSelector && !libavCodecOptsSelector->currentText().trimmed().isEmpty()) {
                    QString additionalOpts = libavCodecOptsSelector->currentText().trimmed();
                    if (!codecOpts.isEmpty() && !additionalOpts.isEmpty()) {
                        codecOpts += ";" + additionalOpts;
                    } else if (!additionalOpts.isEmpty()) {
                        codecOpts = additionalOpts;
                    }
                }
                if (!codecOpts.isEmpty()) {
                    arguments << "--libav-video-codec-opts" << codecOpts;
                }

                // Low Latency
                if (lowLatencyCheckbox && lowLatencyCheckbox->isChecked()) {
                    arguments << "--low-latency";
                }

                // EXPERT PARAMETERS (if Expert Tab is enabled)
                // Viewfinder Mode
                if (viewfinderModeSelector) {
                    QString vfMode = viewfinderModeSelector->currentData().toString();
                    if (!vfMode.isEmpty()) {
                        arguments << "--viewfinder-mode" << vfMode;
                    }
                }

                // Viewfinder Width
                if (viewfinderWidthSpinBox && viewfinderWidthSpinBox->value() > 0) {
                    arguments << "--viewfinder-width" << QString::number(viewfinderWidthSpinBox->value());
                }

                // Viewfinder Height
                if (viewfinderHeightSpinBox && viewfinderHeightSpinBox->value() > 0) {
                    arguments << "--viewfinder-height" << QString::number(viewfinderHeightSpinBox->value());
                }

                // Buffer Count
                if (bufferCountSpinBox && bufferCountSpinBox->value() > 0) {
                    arguments << "--buffer-count" << QString::number(bufferCountSpinBox->value());
                }

                // Viewfinder Buffer Count
                if (viewfinderBufferCountSpinBox && viewfinderBufferCountSpinBox->value() > 0) {
                    arguments << "--viewfinder-buffer-count" << QString::number(viewfinderBufferCountSpinBox->value());
                }

                // AUDIO PARAMETERS - nur bei LibAV
                if (enableAudioCheckBox && enableAudioCheckBox->isChecked()) {
                    arguments << "--libav-audio";

                    // Audio Codec
                    if (audioCodecSelector && !audioCodecSelector->currentText().isEmpty()) {
                        arguments << "--audio-codec" << audioCodecSelector->currentText();
                    }

                    // Audio Bitrate
                    if (audioBitrateSpinBox && audioBitrateSpinBox->value() > 0) {
                        arguments << "--audio-bitrate" << QString::number(audioBitrateSpinBox->value());
                    }

                    // Audio Source
                    if (audioSourceSelector && !audioSourceSelector->currentText().isEmpty()) {
                        arguments << "--audio-source" << audioSourceSelector->currentText();
                    }

                    // Audio Device
                    if (audioDeviceEdit && !audioDeviceEdit->text().trimmed().isEmpty()) {
                        arguments << "--audio-device" << audioDeviceEdit->text().trimmed();
                    }

                    // Audio Channels
                    if (audioChannelsSpinBox && audioChannelsSpinBox->value() > 0) {
                        arguments << "--audio-channels" << QString::number(audioChannelsSpinBox->value());
                    }

                    // Audio Sample Rate
                    if (audioSampleRateSelector && !audioSampleRateSelector->currentText().isEmpty()) {
                        arguments << "--audio-samplerate" << audioSampleRateSelector->currentText();
                    }

                    // AV Sync
                    if (audioAvSyncSpinBox && audioAvSyncSpinBox->value() != 0) {
                        arguments << "--av-sync" << QString::number(audioAvSyncSpinBox->value());
                    }
                }
            }
        }

        // H264-spezifische Optionen (nur bei h264)
        if (codec == "h264") {
            // Profile
            if (profileSelector && profileSelector->currentIndex() != -1) {
                arguments << "--profile" << profileSelector->currentText();
            }

            // Level
            if (levelSelector && levelSelector->currentIndex() != -1) {
                arguments << "--level" << levelSelector->currentText();
            }
        }

        // Inline Headers: für alle Codecs außer yuv420
        if (codec != "yuv420") {
            if (inlineHeadersCheckbox && inlineHeadersCheckbox->isChecked()) {
                arguments << "--inline";
            }
        }

        // Sync (Multi-Camera)
        if (syncSelector && syncSelector->currentText() != "off") {
            arguments << "--sync" << syncSelector->currentText();
        }

        // Bitrate
        if (bitrateSpinBox && bitrateSpinBox->value() > 0) {
            arguments << "--bitrate" << QString::number(bitrateSpinBox->value());
        }

        // Quality (MJPEG)
        if (qualitySpinBox && qualitySpinBox->value() > 0) {
            arguments << "--quality" << QString::number(qualitySpinBox->value());
        }

        // Intra Period (Keyframe Interval)
        if (intraSpinBox && intraSpinBox->value() > 0) {
            arguments << "--intra" << QString::number(intraSpinBox->value());
        }

        // Frames Limit
        if (framesSpinBox && framesSpinBox->value() > 0) {
            arguments << "--frames" << QString::number(framesSpinBox->value());
        }

        // Flush
        if (flushCheckbox && flushCheckbox->isChecked()) {
            arguments << "--flush";
        }

        // Save PTS
        if (savePtsInput && !savePtsInput->text().trimmed().isEmpty()) {
            arguments << "--save-pts" << savePtsInput->text().trimmed();
        }
    }
    QString awb = awbSelector->currentText();
    if (awb != "auto") { // Nur hinzufügen, wenn der Wert nicht "auto" ist
        arguments << "--awb" << awb;
    }

    // Metering parameter
    QString metering = meteringSelector->currentText();
    if (metering != "centre" && metering != "Select option:") { // Nur hinzufügen, wenn der Wert nicht "centre" oder "Select option:" ist
        if (metering == "custom") {
            QString customValue = meteringCustomInput->text().trimmed();
            if (!customValue.isEmpty()) {
                arguments << "--metering" << customValue;
            }
        } else {
            arguments << "--metering" << metering;
        }
    }

    // Low Resolution parameter (using new custom widget)
    if (loresComboBox->isLoresEnabled()) {
        int loresWidth = loresComboBox->getWidth();
        int loresHeight = loresComboBox->getHeight();
        bool usePar = loresComboBox->isParEnabled();

        // Add parameters if values are valid
        if (loresWidth > 0) {
            arguments << "--lores-width" << QString::number(loresWidth);
        }
        if (loresHeight > 0) {
            arguments << "--lores-height" << QString::number(loresHeight);
        }

        // Add lores-par parameter if PAR is enabled
        if (usePar) {
            arguments << "--lores-par";
        }
    }

    double sharpness = sharpnessInput->text().toDouble();
    if (sharpness != 1.0) { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--sharpness" << QString::number(sharpness, 'f', 1);
    }
    double ev = evInput->text().toDouble();
    if (ev != 0.0) { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--ev" << QString::number(ev, 'f', 1);
    }
    double gain = gainInput->text().toDouble();
    if (gain != 0.0) { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--gain" << QString::number(gain, 'f', 1);
    }

    // AWB Gains (Red and Blue) — always send explicit gains when CCM is set,
    // because rpicam-apps silently ignores --ccm without --awbgains.
    double awbGainRed = awbGainRedInput->text().toDouble();
    double awbGainBlue = awbGainBlueInput->text().toDouble();
    bool hasCcm = (ccmInput && !ccmInput->text().isEmpty());
    if (hasCcm || awbGainRed != DEFAULT_AWB_GAIN_RED || awbGainBlue != DEFAULT_AWB_GAIN_BLUE) {
        arguments << "--awbgains" << QString("%1,%2").arg(awbGainRed, 0, 'f', 1).arg(awbGainBlue, 0, 'f', 1);
    }

    // CCM (Colour Correction Matrix) — only valid with explicit AWB gains, rpicam-apps >= 1.13
    if (m_hasPreviewBackend && hasCcm) {
        QString ccm = ccmInput->text().trimmed();
        // Validate: must have exactly 9 comma-separated values
        QStringList parts = ccm.split(',');
        if (parts.size() == 9) {
            arguments << "--ccm" << ccm;
        } else {
            appendLog(tr("CCM: Invalid matrix (need 9 comma-separated values, got %1). Skipping --ccm.").arg(parts.size()));
        }
    }

    double brightness = brightnessInput->text().toDouble();
    if (brightness != 0.0) { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--brightness" << QString::number(brightness, 'f', 1);
    }
    double contrast = contrastInput->text().toDouble();
    if (contrast != 1.0) { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--contrast" << QString::number(contrast, 'f', 1);
    }
    double saturation = saturationInput->text().toDouble();
    if (saturation != 1.0) { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--saturation" << QString::number(saturation, 'f', 1);
    }
    int shutterUs = parseShutterInput();
    if (shutterUs > 0) {
        arguments << "--shutter" << QString::number(shutterUs);
    }

    // HDR-Parameter
    QString hdrValue = hdrSelector ? hdrSelector->currentText() : "";
    if (!hdrValue.isEmpty() && hdrValue != "off") { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--hdr" << hdrValue;
    }

    // Denoise-Parameter
    QString denoiseValue = denoiseSelector ? denoiseSelector->currentText() : "";
    if (!denoiseValue.isEmpty() && denoiseValue != "auto") { // Nur hinzufügen, wenn nicht Standardwert
        arguments << "--denoise" << denoiseValue;
    }

    // Flicker Period Parameter
    if (flickerPeriodSelector) {
        QString flickerData = flickerPeriodSelector->currentData().toString();
        if (!flickerData.isEmpty() && flickerData != "off") {
            // Predefined value (10000us or 8333us)
            arguments << "--flicker-period" << flickerData;
        } else if (flickerData.isEmpty()) {
            // Custom value typed by user (no data role)
            QString customValue = flickerPeriodSelector->currentText().trimmed();
            if (!customValue.isEmpty()) {
                arguments << "--flicker-period" << customValue;
            }
        }
    }

    // Metadata Parameters
    bool metadataAutoNaming = metadataAutoNamingCheckbox ? metadataAutoNamingCheckbox->isChecked() : false;
    QString metadataFile = metadataFileEdit ? metadataFileEdit->text().trimmed() : "";

    if (metadataAutoNaming || !metadataFile.isEmpty()) {
        QString metadataPath;
        QString metadataFormat = metadataFormatSelector ? metadataFormatSelector->currentText().trimmed() : "json";
        QString extension = (metadataFormat == "txt") ? ".txt" : ".json";

        if (metadataAutoNaming) {
            // Auto-generate metadata filename from output filename
            QFileInfo outputInfo(outputFile);
            QString baseName = outputInfo.completeBaseName();
            metadataPath = guiMetadataPath + "/" + baseName + extension;
        } else if (metadataFile == "-") {
            // stdout
            metadataPath = "-";
        } else {
            // Use provided filename
            QFileInfo metadataInfo(metadataFile);
            if (metadataInfo.isAbsolute()) {
                // Absolute path provided
                metadataPath = metadataFile;
            } else {
                // Relative path - prepend guiMetadataPath
                metadataPath = guiMetadataPath + "/" + metadataFile;
            }
        }

        arguments << "--metadata" << metadataPath;
        arguments << "--metadata-format" << metadataFormat;
    }

    // ROI-Parameter
    QString roiValue = roiInput ? roiInput->text().trimmed() : "";
    if (!roiValue.isEmpty() && roiValue != "0.0,0.0,1.0,1.0") { // Nur hinzufügen, wenn nicht Standardwert
        // Validiere ROI-Format (x,y,width,height mit Dezimalwerten 0.0-1.0)
        QStringList roiParts = roiValue.split(",");
        if (roiParts.size() == 4) {
            bool allValid = true;
            for (const QString &part : roiParts) {
                bool ok;
                double value = part.toDouble(&ok);
                if (!ok || value < 0.0 || value > 1.0) {
                    allValid = false;
                    break;
                }
            }
            if (allValid) {
                arguments << "--roi" << roiValue;
            }
        }
    }

    // Still-Tab Parameter (nur für rpicam-still/jpeg)
    if (isStillApp) {
        if (autofocusOnCaptureCheckbox && autofocusOnCaptureCheckbox->isChecked()) {
            arguments << "--autofocus-on-capture";
        }
        if (zslCheckbox && zslCheckbox->isChecked()) {
            arguments << "--zsl";
        }
        if (immediateCheckbox && immediateCheckbox->isChecked()) {
            arguments << "--immediate";
        }
        if (framestartSpinBox && framestartSpinBox->value() > 0) {
            arguments << "--framestart" << QString::number(framestartSpinBox->value());
        }
        if (thumbLineEdit && !thumbLineEdit->text().trimmed().isEmpty()) {
            arguments << "--thumb" << thumbLineEdit->text().trimmed();
        }
        if (restartSpinBox && restartSpinBox->value() > 0) {
            arguments << "--restart" << QString::number(restartSpinBox->value());
        }
        if (exifLineEdit && !exifLineEdit->text().trimmed().isEmpty()) {
            arguments << "--exif" << exifLineEdit->text().trimmed();
        }
        if (latestLineEdit && !latestLineEdit->text().trimmed().isEmpty()) {
            arguments << "--latest" << latestLineEdit->text().trimmed();
        }
        if (rawCheckbox && rawCheckbox->isChecked()) {
            arguments << "--raw";
        }
    }

    // Geometry-Parameter von Checkboxen
    if (hflipCheckbox && hflipCheckbox->isChecked()) {
        arguments << "--hflip";
    }
    if (vflipCheckbox && vflipCheckbox->isChecked()) {
        arguments << "--vflip";
    }
    if (rotationCheckbox && rotationCheckbox->isChecked()) {
        arguments << "--rotation" << "180";
    }

    // Info-Text Parameter von Checkboxen mit beschreibenden Labels
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
        arguments << "--info-text" << infoTextString;
    }

    // Recording Options (signal-based recording)
    // IMPORTANT: Check isEnabled() to prevent disabled-but-checked state after global reset
    bool keypressEnabled = (keypressRecordingCheckbox &&
                           keypressRecordingCheckbox->isEnabled() &&
                           keypressRecordingCheckbox->isChecked());

    if (signalRecordingEnabled) {
        arguments << "--signal";
    }

    if (keypressEnabled) {
        arguments << "--keypress";
    }

    // Initial state (only works with --signal or --keypress!)
    if ((signalRecordingEnabled || keypressEnabled) && initialStateComboBox) {
        QString initialState = initialStateComboBox->currentText();
        if (!initialState.isEmpty() && initialState != "record") {
            arguments << "--initial" << initialState;
        }
    }

    // Segment duration (works independently or with --signal)
    if (segmentDurationInput) {
        QString segmentValue = segmentDurationInput->text().trimmed();
        if (!segmentValue.isEmpty()) {
            bool ok;
            int segmentMs = segmentValue.toInt(&ok);
            if (ok && segmentMs > 0) {
                arguments << "--segment" << QString::number(segmentMs);
            }
        }
    }

    // Split files (works with --segment or --signal) - ONLY for rpicam-vid
    // IMPORTANT: Check isEnabled() to prevent disabled-but-checked state after global reset
    if (app == "rpicam-vid" && splitFilesCheckbox &&
        splitFilesCheckbox->isEnabled() && splitFilesCheckbox->isChecked()) {
        arguments << "--split";
    }

    // Circular buffer (typically used with --signal)
    if (circularBufferInput) {
        QString circularValue = circularBufferInput->text().trimmed();
        if (!circularValue.isEmpty()) {
            bool ok;
            int circularMb = circularValue.toInt(&ok);
            if (ok && circularMb > 0) {
                arguments << "--circular" << QString::number(circularMb);

                // Add --inline for circular buffer (prevents warning and ensures proper operation)
                if (app == "rpicam-vid" || app == "rpicam-raw") {
                    arguments << "--inline";
                    appendLog(tr("[Circular Buffer] Added --inline flag for optimal operation"));
                }
            }
        }
    }

    // GStreamer mode was already checked at the top (line ~91)
    // Use the existing gstreamerMode variable
    if (gstreamerMode) {
        // Build GStreamer pipeline
        QString gstPipeline = buildGStreamerPipeline();

        // Combine rpicam-vid command with GStreamer pipeline
        // Escape arguments properly for shell execution
        QStringList escapedArgs;
        for (const QString &arg : arguments) {
            // Quote arguments that contain special shell characters
            if (arg.contains(' ') || arg.contains('(') || arg.contains(')') ||
                arg.contains('%') || arg.contains(':') || arg.contains('|')) {
                escapedArgs << ("'" + arg + "'");
            } else {
                escapedArgs << arg;
            }
        }
        QString rpicamCommand = app + " " + escapedArgs.join(" ");
        QString fullCommand = rpicamCommand + " | " + gstPipeline;

        qDebug().noquote() << "[GStreamer Mode] Full command:" << fullCommand;
        appendLog(tr("[GStreamer Mode] Starting streaming..."));
        appendLog(tr("Command: ") + fullCommand);

        // Execute via bash to support piping
        // Use 'setsid' to create a new process group for proper cleanup
        // Redirect stderr to suppress V4L2 event subscription errors
        QStringList bashArgs;
        bashArgs << "-c" << ("setsid " + fullCommand + " 2>/dev/null");

        // Remove GST_TRACERS from child environment to prevent
        // GstShark (hailo-tappas-core) from creating gstshark_* trace directories
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove("GST_TRACERS");
        process.setProcessEnvironment(env);

        process.start("/bin/bash", bashArgs);
    } else {
        // File mode: execute rpicam command directly
        QString fullCommand = app + " " + arguments.join(" ");
        qDebug().noquote() << "Full command:" << fullCommand;
        process.start(app, arguments);
    }
    updateButtonVisibility();

    // Attempt control socket connection (delayed — socket file may not exist immediately)
    QTimer::singleShot(500, this, [this]() {
        connectControlSocket();
    });
}

// --- Ab hier die ausgelagerten Funktionen ---

QString MainWindow::buildGStreamerPipeline() {
    if (m_gstreamerTab && m_gstreamerTab->module()) {
        return m_gstreamerTab->module()->buildPipeline();
    }
    return QString();
}

void MainWindow::stopRpiCamApp() {
    // Disconnect control socket before stopping process
    if (m_controlSocket) {
        m_controlSocket->disconnectFromServer();
    }

    if (process.state() == QProcess::Running) {
        qint64 pid = process.processId();
        qDebug() << "[Stop] Terminating process with PID:" << pid;

        // Kill process tree: find all descendant PIDs and kill them
        // This works even with setsid because we traverse the process tree
        QProcess killCmd;
        QString killScript = QString(
            "kill_tree() {"
            "  local pid=$1; local sig=$2;"
            "  for child in $(pgrep -P $pid 2>/dev/null); do"
            "    kill_tree $child $sig;"
            "  done;"
            "  kill $sig $pid 2>/dev/null;"
            "};"
            "kill_tree %1 -INT"
        ).arg(pid);

        killCmd.start("/bin/bash", QStringList() << "-c" << killScript);
        killCmd.waitForFinished(2000);

        qDebug() << "[Stop] Sent SIGINT to process tree, waiting for graceful shutdown...";

        // Wait up to 10 seconds for graceful shutdown and file finalization
        if (!process.waitForFinished(10000)) {
            qDebug() << "[Stop] Process didn't terminate after SIGINT, trying SIGTERM...";

            killScript = QString(
                "kill_tree() {"
                "  local pid=$1; local sig=$2;"
                "  for child in $(pgrep -P $pid 2>/dev/null); do"
                "    kill_tree $child $sig;"
                "  done;"
                "  kill $sig $pid 2>/dev/null;"
                "};"
                "kill_tree %1 -TERM"
            ).arg(pid);

            killCmd.start("/bin/bash", QStringList() << "-c" << killScript);
            killCmd.waitForFinished(2000);

            if (!process.waitForFinished(5000)) {
                qDebug() << "[Stop] Process didn't terminate gracefully, forcing with SIGKILL";

                killScript = QString(
                    "kill_tree() {"
                    "  local pid=$1; local sig=$2;"
                    "  for child in $(pgrep -P $pid 2>/dev/null); do"
                    "    kill_tree $child $sig;"
                    "  done;"
                    "  kill $sig $pid 2>/dev/null;"
                    "};"
                    "kill_tree %1 -KILL"
                ).arg(pid);

                killCmd.start("/bin/bash", QStringList() << "-c" << killScript);
                killCmd.waitForFinished(2000);
                process.kill();
                process.waitForFinished();
            }
        }
        appendLog(tr("Process terminated."));
    } else {
        appendLog(tr("No running process to terminate."));
    }
    updateButtonVisibility();
}
