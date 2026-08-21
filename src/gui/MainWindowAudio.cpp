#include "MainWindow.h"
#include "CollapsibleHelper.h"
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

// =============================================================================
// Audio Tab Methods
// =============================================================================

void MainWindow::onAudioToggled(bool checked) {
    // Wenn Audio aktiviert wird, stelle automatisch auf libav Codec um
    if (checked && codecSelector) {
        int libavIndex = codecSelector->findText("libav");
        if (libavIndex != -1 && codecSelector->currentText() != "libav") {
            codecSelector->setCurrentIndex(libavIndex);
            appendLog("Codec automatically switched to 'libav' for audio recording");
        }
    }
    updateAudioControlsState();

    // Update auto-naming wenn aktiviert
    if (autoNamingCheckbox && autoNamingCheckbox->isChecked() && appSelector && codecSelector) {
        QString app = appSelector->currentText();
        if (app == "rpicam-vid" || app == "rpicam-focus" || app == "rpicam-focus008") {
            QString codec = codecSelector->currentText();
            QString baseName = "video";

            // Füge "_audio" hinzu wenn Audio aktiviert ist und Codec libav ist
            if (checked && codec == "libav") {
                baseName += "_audio";
            }

            QString extension;
            if (codec == "mjpeg") {
                extension = ".mjpeg";
            } else if (codec == "yuv420") {
                extension = ".avi";
            } else {
                extension = ".mp4";
            }

            QString fileName = QDir(guiOutputFilePath).filePath(baseName + extension);
            outputFileName->setText(fileName);
        }
    }
}

void MainWindow::updateAudioControlsState() {
    if (!enableAudioCheckBox || !audioCodecSelector || !audioBitrateSpinBox ||
        !audioSourceSelector || !audioDeviceEdit || !audioChannelsSpinBox ||
        !audioSampleRateSelector || !audioAvSyncSpinBox) {
        return;
    }

    bool enabled = enableAudioCheckBox->isChecked();

    // Enable/disable all audio controls based on checkbox state
    audioCodecSelector->setEnabled(enabled);
    audioBitrateSpinBox->setEnabled(enabled);
    audioSourceSelector->setEnabled(enabled);
    audioDeviceEdit->setEnabled(enabled);
    audioChannelsSpinBox->setEnabled(enabled);
    audioSampleRateSelector->setEnabled(enabled);
    audioAvSyncSpinBox->setEnabled(enabled);
}

void MainWindow::resetAudioToDefaults() {
    if (!enableAudioCheckBox || !audioCodecSelector || !audioBitrateSpinBox ||
        !audioSourceSelector || !audioDeviceEdit || !audioChannelsSpinBox ||
        !audioSampleRateSelector || !audioAvSyncSpinBox) {
        return;
    }

    enableAudioCheckBox->setChecked(false);
    audioCodecSelector->setCurrentText("aac");
    audioBitrateSpinBox->setValue(128);
    audioSourceSelector->setCurrentText("pulse");
    audioDeviceEdit->clear();
    audioChannelsSpinBox->setValue(2);
    audioSampleRateSelector->setCurrentText("44100");
    audioAvSyncSpinBox->setValue(0);

    updateAudioControlsState();
    updateAudioResetButtonColor();
}

void MainWindow::loadAudioSettings() {
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);

    if (!enableAudioCheckBox || !audioCodecSelector || !audioBitrateSpinBox ||
        !audioSourceSelector || !audioDeviceEdit || !audioChannelsSpinBox ||
        !audioSampleRateSelector || !audioAvSyncSpinBox) {
        return;
    }

    // Audio startet immer deaktiviert, aber andere Einstellungen werden geladen
    enableAudioCheckBox->setChecked(false);
    audioCodecSelector->setCurrentText(settings.value("Audio/Codec", "aac").toString());
    audioBitrateSpinBox->setValue(settings.value("Audio/Bitrate", 128).toInt());
    audioSourceSelector->setCurrentText(settings.value("Audio/Source", "pulse").toString());
    audioDeviceEdit->setText(settings.value("Audio/Device", "").toString());
    audioChannelsSpinBox->setValue(settings.value("Audio/Channels", 2).toInt());
    audioSampleRateSelector->setCurrentText(settings.value("Audio/SampleRate", "44100").toString());
    audioAvSyncSpinBox->setValue(settings.value("Audio/AvSync", 0).toInt());
    settings.endGroup();

    updateAudioControlsState();
}

void MainWindow::saveAudioSettings() {
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);

    if (!enableAudioCheckBox || !audioCodecSelector || !audioBitrateSpinBox ||
        !audioSourceSelector || !audioDeviceEdit || !audioChannelsSpinBox ||
        !audioSampleRateSelector || !audioAvSyncSpinBox) {
        return;
    }

    settings.setValue("Audio/Enabled", enableAudioCheckBox->isChecked());
    settings.setValue("Audio/Codec", audioCodecSelector->currentText());
    settings.setValue("Audio/Bitrate", audioBitrateSpinBox->value());
    settings.setValue("Audio/Source", audioSourceSelector->currentText());
    settings.setValue("Audio/Device", audioDeviceEdit->text());
    settings.setValue("Audio/Channels", audioChannelsSpinBox->value());
    settings.setValue("Audio/SampleRate", audioSampleRateSelector->currentText());
    settings.setValue("Audio/AvSync", audioAvSyncSpinBox->value());
    settings.endGroup();
}

void MainWindow::updateAudioResetButtonColor() {
    if (!audioResetButton || !enableAudioCheckBox || !audioCodecSelector ||
        !audioBitrateSpinBox || !audioSourceSelector || !audioDeviceEdit ||
        !audioChannelsSpinBox || !audioSampleRateSelector || !audioAvSyncSpinBox) {
        return;
    }

    // Prüfe ob irgendein Wert vom Standard abweicht
    bool isDefault = (enableAudioCheckBox->isChecked() == false &&
                      audioCodecSelector->currentText() == "aac" &&
                      audioBitrateSpinBox->value() == 128 &&
                      audioSourceSelector->currentText() == "pulse" &&
                      audioDeviceEdit->text().isEmpty() &&
                      audioChannelsSpinBox->value() == 2 &&
                      audioSampleRateSelector->currentText() == "44100" &&
                      audioAvSyncSpinBox->value() == 0);

    if (isDefault) {
        audioResetButton->setStyleSheet("color: black;");
    } else {
        audioResetButton->setStyleSheet("color: red;");
    }
}
