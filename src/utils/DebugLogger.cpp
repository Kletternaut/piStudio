#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QApplication>
#include <QDir>
#include "DebugLogger.h"
#include "AppPaths.h"
#include "../app/AppMeta.h"

QFile DebugLogger::debugLogFile;
QTextEdit* DebugLogger::logWidget = nullptr;
bool DebugLogger::logToFileEnabled = false;
bool DebugLogger::logToWidgetEnabled = false;
bool DebugLogger::standardLoggingEnabled = false;

void DebugLogger::initialize(const QString &filePath) {
    Q_UNUSED(filePath);

    // Determine writable path for debug.log:
    // - System install: ~/.config/<config-dir>/debug.log (dir from AppMeta)
    // - Dev build: <project-root>/debug.log
    QString fullPath;
    if (AppPaths::isSystemInstall()) {
        QDir().mkpath(QDir::homePath() + "/" + AppMeta::CONFIG_DIR);
        fullPath = QDir::homePath() + "/" + AppMeta::CONFIG_DIR + "/debug.log";
    } else {
        fullPath = AppPaths::base() + "debug.log";
    }
    debugLogFile.setFileName(fullPath);

    if (debugLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        debugLogFile.resize(0); // Clear file on startup
        qInstallMessageHandler(customMessageHandler);
    } else {
        qDebug() << "Failed to open debug log file:" << fullPath;
    }
}

void DebugLogger::setLogToFile(bool enabled) {
    logToFileEnabled = enabled;
}

void DebugLogger::clearLogFile() {
    if (debugLogFile.isOpen()) {
        debugLogFile.resize(0);
        debugLogFile.seek(0);
    }
}

void DebugLogger::setLogToWidget(bool enabled) {
    logToWidgetEnabled = enabled;
}

void DebugLogger::setStandardLogging(bool enabled) {
    standardLoggingEnabled = enabled;
}

void DebugLogger::setLogWidget(QTextEdit* widget) {
    logWidget = widget;
}

void DebugLogger::shutdown() {
    if (debugLogFile.isOpen()) {
        debugLogFile.close();
    }
    qInstallMessageHandler(nullptr); // Restore default handler
}

void DebugLogger::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString logType;

    switch (type) {
    case QtDebugMsg:
        logType = "DEBUG";
        break;
    case QtInfoMsg:
        logType = "INFO";
        break;
    case QtWarningMsg:
        logType = "WARNING";
        break;
    case QtCriticalMsg:
        logType = "CRITICAL";
        break;
    case QtFatalMsg:
        logType = "FATAL";
        break;
    }

    QString formattedMsg = QString("[%1] [%2] %3").arg(timeStamp, logType, msg);

    // Write to file
    if (logToFileEnabled && debugLogFile.isOpen()) {
        QTextStream out(&debugLogFile);
        out << formattedMsg << "\n";
        out.flush();
    }

    // Write to log widget (thread-safe via QMetaObject::invokeMethod)
    if (logToWidgetEnabled && logWidget) {
        QMetaObject::invokeMethod(logWidget, [formattedMsg]() {
            if (logWidget) {
                logWidget->append(formattedMsg);
            }
        }, Qt::QueuedConnection);
    }

    // Standard logging (console)
    if (standardLoggingEnabled) {
        (void)fprintf(stderr, "%s\n", formattedMsg.toLocal8Bit().constData());
        (void)fflush(stderr);
    }
}
