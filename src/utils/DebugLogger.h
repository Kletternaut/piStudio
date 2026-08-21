#ifndef DEBUGLOGGER_H
#define DEBUGLOGGER_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QtGlobal>
#include <QTextEdit>
#include <functional>

class DebugLogger {
public:
    static void initialize(const QString &filePath);
    static void shutdown();
    static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    
    // Neue Funktionen für flexible Debug-Ausgabe
    static void setLogToFile(bool enabled);
    static void setLogToWidget(bool enabled);
    static void setStandardLogging(bool enabled);
    static void setLogWidget(QTextEdit* widget);
    
    static bool isLogToFileEnabled() { return logToFileEnabled; }
    static bool isLogToWidgetEnabled() { return logToWidgetEnabled; }
    static bool isStandardLoggingEnabled() { return standardLoggingEnabled; }
    static void clearLogFile();

private:
    static QFile debugLogFile;
    static QTextEdit* logWidget;
    static bool logToFileEnabled;
    static bool logToWidgetEnabled;
    static bool standardLoggingEnabled;
};

#endif // DEBUGLOGGER_H