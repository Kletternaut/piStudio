#include "MainWindow.h"
#include "CollapsibleHelper.h"
#include "DonationDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTabWidget>
#include <QFrame>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
void MainWindow::updateAppSelector() {
    appSelector->clear();
    appSelector->addItems({"rpicam-vid", "rpicam-jpeg", "rpicam-still", "rpicam-raw", "rpicam-hello"});
    appSelector->addItems(customAppEntries);
}

void MainWindow::updateTimeoutSelector() {
    QString currentValue = timeoutSelector->currentText();

    // Sortiere die Einträge numerisch
    QList<int> numericValues;
    for (const QString &entry : customTimeoutEntries) {
        bool ok;
        int value = entry.toInt(&ok);
        if (ok) {
            numericValues.append(value);
        }
    }
    std::sort(numericValues.begin(), numericValues.end());

    // Zurück zu Strings konvertieren
    customTimeoutEntries.clear();
    for (int value : numericValues) {
        customTimeoutEntries.append(QString::number(value));
    }

    timeoutSelector->clear();
    timeoutSelector->addItems(customTimeoutEntries);

    // Versuche den vorherigen Wert wiederherzustellen
    int idx = timeoutSelector->findText(currentValue);
    if (idx >= 0) {
        timeoutSelector->setCurrentIndex(idx);
    } else if (timeoutSelector->count() > 0) {
        timeoutSelector->setCurrentIndex(0);
    }
}

void MainWindow::saveTimeoutEntries() {
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);

    // Lösche alte Einträge
    for (int i = 0; i < 10; ++i) {
        QString key = QString("CustomTimeout%1").arg(i + 1);
        settings.remove(key);
    }

    // Speichere aktuelle Einträge
    for (int i = 0; i < customTimeoutEntries.size() && i < 10; ++i) {
        QString key = QString("CustomTimeout%1").arg(i + 1);
        settings.setValue(key, customTimeoutEntries[i]);
    }
    settings.endGroup();
}

void MainWindow::updateTimelapseSelector() {
    QString currentValue = timelapseInput ? timelapseInput->currentText() : "";

    // Sortiere die Einträge numerisch
    QList<int> numericValues;
    for (const QString &entry : customTimelapseEntries) {
        bool ok;
        int value = entry.toInt(&ok);
        if (ok) {
            numericValues.append(value);
        }
    }
    std::sort(numericValues.begin(), numericValues.end());

    // Zurück zu Strings konvertieren
    customTimelapseEntries.clear();
    for (int value : numericValues) {
        customTimelapseEntries.append(QString::number(value));
    }

    timelapseInput->clear();
    if (!customTimelapseEntries.isEmpty()) {
        timelapseInput->addItem(""); // Leerer Eintrag an erster Stelle
        timelapseInput->addItems(customTimelapseEntries);
    } else {
        timelapseInput->addItem(""); // Nur leerer Eintrag
    }

    // Versuche den vorherigen Wert wiederherzustellen
    int idx = timelapseInput->findText(currentValue);
    if (idx >= 0) {
        timelapseInput->setCurrentIndex(idx);
    } else {
        timelapseInput->setCurrentIndex(0); // Leerer Eintrag
    }
}

void MainWindow::saveTimelapseEntries() {
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);

    // Lösche alte Einträge
    for (int i = 0; i < 10; ++i) {
        QString key = QString("CustomTimelapse%1").arg(i + 1);
        settings.remove(key);
    }

    // Speichere aktuelle Einträge (nur wenn Liste nicht leer)
    if (!customTimelapseEntries.isEmpty()) {
        for (int i = 0; i < customTimelapseEntries.size() && i < 10; ++i) {
            QString key = QString("CustomTimelapse%1").arg(i + 1);
            settings.setValue(key, customTimelapseEntries[i]);
        }
    }
    settings.endGroup();
}

void MainWindow::updatePostProcessFileDropdown() {
    if (!postProcessFileSelector) return;
    if (guiPostProcessFilePath.isEmpty()) return;
    QDir dir(guiPostProcessFilePath);
    if (!dir.exists()) return;
    QString previousSelection = postProcessFileSelector->currentText();
    postProcessFileSelector->clear();
    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &file : jsonFiles) {
        postProcessFileSelector->addItem(file);
    }
    int idx = postProcessFileSelector->findText(previousSelection);
    if (idx >= 0) {
        postProcessFileSelector->setCurrentIndex(idx);
    } else {
        postProcessFileSelector->setCurrentIndex(-1);
        postProcessFileSelector->setCurrentText("");
    }
}

void MainWindow::updateTuningFileDropdown() {
    if (!tuningFileSelector) return;
    if (guiTuningFilePath.isEmpty()) return;
    QDir dir(guiTuningFilePath);
    if (!dir.exists()) return;
    QString previousSelection = tuningFileSelector->currentText();
    tuningFileSelector->clear();
    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &file : jsonFiles) {
        tuningFileSelector->addItem(file);
    }
    int idx = tuningFileSelector->findText(previousSelection);
    if (idx >= 0) {
        tuningFileSelector->setCurrentIndex(idx);
    } else {
        tuningFileSelector->setCurrentIndex(-1);
        tuningFileSelector->setCurrentText("");
    }
}

void MainWindow::updateOutputFileNameForTimelapse() {
    QString name = outputFileName->text();
    if (appSelector->currentText() == "rpicam-still" && !timelapseInput->currentText().isEmpty() && timelapseInput->currentText() != "0") {
        if (!name.contains("_%04d.jpg")) {
            if (name.endsWith(".jpg", Qt::CaseInsensitive)) {
                name.chop(4);
            }
            name += "_%04d.jpg";
            outputFileName->setText(name);
        }
    } else {
        if (name.endsWith("_%04d.jpg")) {
            name.chop(9);
            name += ".jpg";
            outputFileName->setText(name);
        }
    }
}

void MainWindow::showDonationDialog() {
    DonationDialog dialog(this);
    dialog.exec();
}

void MainWindow::updateFilterResetButtonColor() {
    if (!filterResetButton) return;
    if (isInitializing) return;

    bool isDefault = true;
    if (m_actionsTab && m_actionsTab->module()) {
        isDefault = m_actionsTab->module()->detectionAction().filteredObjects.isEmpty();
    }

    filterResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
}

void MainWindow::updateActionsResetButtonColor() {
    if (!actionsResetButton) return;
    if (isInitializing) return;

    if (m_actionsTab && m_actionsTab->module()) {
        const auto &a = m_actionsTab->module()->detectionAction();
        bool anyActive = a.playSound || a.saveImage || a.showNotification
                      || a.runScript || a.startRecording || a.sendTelegram
                      || a.sendTelegramImage || a.sendTelegramVideo;
        actionsResetButton->setStyleSheet(anyActive ? "color: red;" : "color: black;");
    }
}

void MainWindow::updateFilterSettingsResetButtonColor() {
    if (!filterSettingsResetButton) return;
    if (isInitializing) return;

    bool isDefault = true;
    if (m_actionsTab && m_actionsTab->module()) {
        const auto &a = m_actionsTab->module()->detectionAction();
        // Default cooldown: 30, default confidence: 70
        isDefault = (a.cooldownSeconds == 30 && a.minConfidence == 70);
    }

    filterSettingsResetButton->setStyleSheet(isDefault ? "color: black;" : "color: red;");
}

void MainWindow::updateGlobalResetButtonColor() {
    if (!globalResetButton) return;

    // Während der Initialisierung keine Farb-Updates durchführen
    if (isInitializing) {
        qDebug() << "Skipping global reset button color update during initialization";
        return;
    }

    // Check all settings for default values
    bool hasNonDefaultValues = false;
    QStringList nonDefaultReasons;

    // Preview/Overlay Check - use getDefaultBoxInput() to respect custom geometry setting
    QString currentBox = BoxInput->text();
    QString expectedBox = getDefaultBoxInput();
    if (currentBox != expectedBox) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("BoxInput: '%1' != '%2'").arg(currentBox, expectedBox);
    }

    // Output File Check
    if (!outputFileName->text().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("outputFileName: '%1'").arg(outputFileName->text());
    }
    if (autoNamingCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "autoNamingCheckbox: checked";
    }
    if (timestampCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "timestampCheckbox: checked";
    }

    // Timeout Check
    if (timeoutSelector->currentText() != "0") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Timeout: '%1'").arg(timeoutSelector->currentText());
    }

    // Timelapse Check
    if (!timelapseInput->currentText().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("timelapse: '%1'").arg(timelapseInput->currentText());
    }

    // Sync Check
    if (syncSelector && syncSelector->currentText() != "off") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Sync: '%1'").arg(syncSelector->currentText());
    }

    // HDR Check
    if (hdrSelector->currentText() != "off") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("HDR: '%1'").arg(hdrSelector->currentText());
    }

    // Denoise Check
    if (denoiseSelector->currentText() != "auto") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Denoise: '%1'").arg(denoiseSelector->currentText());
    }

    // Flicker Period Check
    if (flickerPeriodSelector) {
        QString flickerData = flickerPeriodSelector->currentData().toString();
        bool flickerActive = (!flickerData.isEmpty() && flickerData != "off") || (flickerData.isEmpty() && !flickerPeriodSelector->currentText().trimmed().isEmpty());
        if (flickerActive) {
            hasNonDefaultValues = true;
            nonDefaultReasons << QString("Flicker: %1").arg(flickerPeriodSelector->currentText().trimmed());
        }
    }

    // Metadata File Check
    if ((metadataAutoNamingCheckbox && metadataAutoNamingCheckbox->isChecked()) ||
        (metadataFileEdit && !metadataFileEdit->text().trimmed().isEmpty())) {
        hasNonDefaultValues = true;
        if (metadataAutoNamingCheckbox && metadataAutoNamingCheckbox->isChecked()) {
            nonDefaultReasons << QString("Metadata: Auto-naming");
        } else {
            nonDefaultReasons << QString("Metadata: '%1'").arg(metadataFileEdit->text().trimmed());
        }
    }

    // Post-Process File Check
    if (!postProcessFileSelector->currentText().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("PostProcess: '%1'").arg(postProcessFileSelector->currentText());
    }

    // Tuning File Check
    if (!tuningFileSelector->currentText().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("TuningFile: '%1'").arg(tuningFileSelector->currentText());
    }

    // Codec Check
    if (codecSelector->currentText() != "h264" && !codecSelector->currentText().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Codec: '%1'").arg(codecSelector->currentText());
    }

    // Encoding Check (für Still-Apps)
    if (encodingSelector && encodingSelector->currentIndex() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Encoding: '%1'").arg(encodingSelector->currentText());
    }

    // Still Tab Checks
    if (autofocusOnCaptureCheckbox && autofocusOnCaptureCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Autofocus on Capture: checked";
    }
    if (zslCheckbox && zslCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "ZSL: checked";
    }
    if (immediateCheckbox && immediateCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Immediate: checked";
    }
    if (framestartSpinBox && framestartSpinBox->value() > 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Framestart: %1").arg(framestartSpinBox->value());
    }
    if (thumbLineEdit && !thumbLineEdit->text().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Thumb: '%1'").arg(thumbLineEdit->text());
    }
    if (restartSpinBox && restartSpinBox->value() > 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Restart: %1").arg(restartSpinBox->value());
    }
    if (exifLineEdit && !exifLineEdit->text().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("EXIF: '%1'").arg(exifLineEdit->text());
    }
    if (latestLineEdit && !latestLineEdit->text().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Latest: '%1'").arg(latestLineEdit->text());
    }
    if (rawCheckbox && rawCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "RAW: checked";
    }

    // Profile Check
    if (profileSelector && profileSelector->currentIndex() != -1) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Profile: '%1'").arg(profileSelector->currentText());
    }

    // Level Check
    if (levelSelector && levelSelector->currentIndex() != -1) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Level: '%1'").arg(levelSelector->currentText());
    }

    // Inline Headers Check
    if (inlineHeadersCheckbox && inlineHeadersCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Inline Headers: checked";
    }

    // Bitrate Check
    if (bitrateSpinBox && bitrateSpinBox->value() > 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Bitrate: %1").arg(bitrateSpinBox->value());
    }

    // Quality Check
    if (qualitySpinBox && qualitySpinBox->value() > 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Quality: %1").arg(qualitySpinBox->value());
    }

    // Intra Period Check
    if (intraSpinBox && intraSpinBox->value() > 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Intra: %1").arg(intraSpinBox->value());
    }

    // Frames Check
    if (framesSpinBox && framesSpinBox->value() > 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Frames: %1").arg(framesSpinBox->value());
    }

    // Flush Check
    if (flushCheckbox && flushCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Flush: checked";
    }

    // Save PTS Check
    if (savePtsInput && !savePtsInput->text().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Save PTS: '%1'").arg(savePtsInput->text());
    }

    // Recording Options Checks
    if (signalRecordingCheckbox && signalRecordingCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Signal Recording: checked";
    }
    if (keypressRecordingCheckbox && keypressRecordingCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Keypress Recording: checked";
    }
    if (initialStateComboBox && initialStateComboBox->currentText() != "pause") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Initial State: '%1'").arg(initialStateComboBox->currentText());
    }
    if (splitFilesCheckbox && splitFilesCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Split Files: checked";
    }
    if (segmentDurationInput && !segmentDurationInput->text().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Segment Duration: '%1'").arg(segmentDurationInput->text());
    }
    if (circularBufferInput && !circularBufferInput->text().isEmpty()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Circular Buffer: '%1'").arg(circularBufferInput->text());
    }

    // AWB Check
    if (awbSelector->currentText() != "auto") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("AWB: '%1'").arg(awbSelector->currentText());
    }

    // Metering Check
    if (meteringSelector->currentText() != "Select option:") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Metering: '%1'").arg(meteringSelector->currentText());
    }

    // Low Resolution Check
    if (loresComboBox->isLoresEnabled()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "LowRes: enabled";
    }

    // Slider Checks
    if (sharpnessSlider->value() != 10) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Sharpness: %1").arg(sharpnessSlider->value());
    }
    if (evSlider->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("EV: %1").arg(evSlider->value());
    }
    if (gainSlider->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Gain: %1").arg(gainSlider->value());
    }
    if (awbGainRedSlider->value() != 15) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("AWB Gain Red: %1").arg(awbGainRedSlider->value());
    }
    if (awbGainBlueSlider->value() != 12) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("AWB Gain Blue: %1").arg(awbGainBlueSlider->value());
    }
    if (brightnessSlider->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Brightness: %1").arg(brightnessSlider->value());
    }
    if (contrastSlider->value() != 10) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Contrast: %1").arg(contrastSlider->value());
    }
    if (saturationSlider->value() != 10) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Saturation: %1").arg(saturationSlider->value());
    }

    // Autofocus Parameter Checks
    if (autofocusModeSelector && autofocusModeSelector->currentText() != "auto") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("AF Mode: '%1'").arg(autofocusModeSelector->currentText());
    }
    if (autofocusRangeSelector && autofocusRangeSelector->currentText() != "normal") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("AF Range: '%1'").arg(autofocusRangeSelector->currentText());
    }
    if (autofocusSpeedSelector && autofocusSpeedSelector->currentText() != "normal") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("AF Speed: '%1'").arg(autofocusSpeedSelector->currentText());
    }
    if (autofocusWindowInput && autofocusWindowInput->text() != "0.333,0.333,0.333,0.333") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("AF Window: '%1'").arg(autofocusWindowInput->text());
    }
    if (lensPositionSlider && lensPositionSlider->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Lens Position: %1").arg(lensPositionSlider->value());
    }

    // Geometry Checkboxes Check
    if ((hflipCheckbox && hflipCheckbox->isChecked()) ||
        (vflipCheckbox && vflipCheckbox->isChecked()) ||
        (rotationCheckbox && rotationCheckbox->isChecked())) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "Geometry: transforms active";
    }

    // Info-Text Checkboxes Check
    if ((infoTextFrameCheckbox && infoTextFrameCheckbox->isChecked()) ||
        (infoTextFpsCheckbox && infoTextFpsCheckbox->isChecked()) ||
        (infoTextExpCheckbox && infoTextExpCheckbox->isChecked()) ||
        (infoTextAgCheckbox && infoTextAgCheckbox->isChecked()) ||
        (infoTextDgCheckbox && infoTextDgCheckbox->isChecked()) ||
        (infoTextRgCheckbox && infoTextRgCheckbox->isChecked()) ||
        (infoTextBgCheckbox && infoTextBgCheckbox->isChecked()) ||
        (infoTextFocusCheckbox && infoTextFocusCheckbox->isChecked()) ||
        (infoTextAelockCheckbox && infoTextAelockCheckbox->isChecked()) ||
        (infoTextLpCheckbox && infoTextLpCheckbox->isChecked()) ||
        (infoTextAfstateCheckbox && infoTextAfstateCheckbox->isChecked())) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "InfoText: options active";
    }

    // ROI Check
    if (!roiInput->text().isEmpty() && roiInput->text() != "0.0,0.0,1.0,1.0") {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("ROI: '%1'").arg(roiInput->text());
    }

    // x2 Checkbox Check
    if (doubleSizeCheckbox->isChecked()) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "x2: checked";
    }

    // Actions Checkboxes Check (P12: via Plugin-API wenn verfuegbar)
    if (m_actionsTab && m_actionsTab->module()) {
        const auto &a = m_actionsTab->module()->detectionAction();
        if (a.playSound || a.saveImage || a.showNotification || a.runScript
                || a.startRecording || a.sendTelegram || a.sendTelegramImage || a.sendTelegramVideo) {
            hasNonDefaultValues = true;
            nonDefaultReasons << "Actions: enabled (via tab)";
        }
        if (!a.filteredObjects.isEmpty()) {
            hasNonDefaultValues = true;
            nonDefaultReasons << "Detection Filter: has selections";
        }
        // Default cooldown: 30, default confidence: 70
        if (a.cooldownSeconds != 30 || a.minConfidence != 70) {
            hasNonDefaultValues = true;
            nonDefaultReasons << QString("Filter Settings: cooldown=%1, confidence=%2")
                                     .arg(a.cooldownSeconds).arg(a.minConfidence);
        }
    } // end if m_actionsTab block

    // Expert Tab Checks (default: all values = 0 or auto)
    if (viewfinderModeSelector && viewfinderModeSelector->currentIndex() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Viewfinder Mode: index %1").arg(viewfinderModeSelector->currentIndex());
    }
    if (viewfinderWidthSpinBox && viewfinderWidthSpinBox->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Viewfinder Width: %1").arg(viewfinderWidthSpinBox->value());
    }
    if (viewfinderHeightSpinBox && viewfinderHeightSpinBox->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Viewfinder Height: %1").arg(viewfinderHeightSpinBox->value());
    }
    if (bufferCountSpinBox && bufferCountSpinBox->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Buffer Count: %1").arg(bufferCountSpinBox->value());
    }
    if (viewfinderBufferCountSpinBox && viewfinderBufferCountSpinBox->value() != 0) {
        hasNonDefaultValues = true;
        nonDefaultReasons << QString("Viewfinder Buffer Count: %1").arg(viewfinderBufferCountSpinBox->value());
    }

    // Lib paths (configured in the camera setup dialog): these have no widget
    // in the main window, so their state is tracked against the persisted
    // value. Changed + saved = green, changed but only temporary
    // (loaded from config file / profile) = red.
    bool libChangedSaved = false;
    bool libChangedTemp = false;
    {
        QSettings libSettings(AppPaths::globalConf(), QSettings::IniFormat);
        libSettings.beginGroup(m_tabGroup);
        auto checkLib = [&](const QString &active, const QString &saved) {
            if (active.isEmpty() && saved.isEmpty()) return; // default state
            if (active == saved) { libChangedSaved = true; }
            else { libChangedTemp = true; }
        };
        checkLib(m_previewLibsPath, libSettings.value("Defaults/PreviewLibs", "").toString());
        checkLib(m_postProcessLibsPath, libSettings.value("Defaults/PostProcessLibs", "").toString());
        checkLib(m_encoderLibsPath, libSettings.value("Defaults/EncoderLibs", "").toString());
        libSettings.endGroup();
    }

    // Sync start/stop toggle
    if (m_syncStartStop) {
        hasNonDefaultValues = true;
        nonDefaultReasons << "SyncStartStop: enabled";
    }

    // Button-Farbe entsprechend setzen
    if (hasNonDefaultValues || libChangedTemp) {
        globalResetButton->setStyleSheet("color: red;");
        qDebug() << "Global reset button set to RED. Reasons:" << nonDefaultReasons;
    } else if (libChangedSaved) {
        // Values differ from the hardcoded defaults, but they are saved in
        // the settings — nothing temporary is active.
        globalResetButton->setStyleSheet("color: green;");
        qDebug() << "Global reset button set to GREEN - lib paths changed but saved";
    } else {
        globalResetButton->setStyleSheet("color: black;");
        qDebug() << "Global reset button set to BLACK - all values are default";
    }
}
