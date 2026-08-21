#pragma once

#include <QString>
#include <QSet>

// Shared struct for detection action configuration.
// Kept as a direct MainWindow member so MainWindowHelpers.cpp can access it
// without modification (Phase 1-3 constraint).
struct DetectionAction {
    // String fields first (largest alignment)
    QString soundFile;
    QString imageFolder;
    QString scriptPath;
    QString telegramBotToken;
    QString telegramChatId;
    QSet<QString> filteredObjects;
    // Integer fields
    int recordingDuration = 30;
    int cooldownSeconds = 30;
    int minConfidence = 0;
    // Bool fields last (smallest alignment)
    bool playSound = false;
    bool saveImage = false;
    bool showNotification = false;
    bool runScript = false;
    bool startRecording = false;
    bool sendTelegram = false;
    bool sendTelegramImage = false;
    bool sendTelegramVideo = false;
};
