// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
//
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// LogWindow.cpp - Standalone log window implementation.

#include "LogWindow.h"
#include "CollapsibleHelper.h"
#include "../utils/AppPaths.h"
#include "../utils/DebugLogger.h"
#include "../app/AppMeta.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>

LogWindow::LogWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QApplication::translate("LogWindow", "%1 - Log")
                       .arg(QLatin1String(AppMeta::NAME)));
    setWindowIcon(QIcon(QLatin1String(AppMeta::ICON_RESOURCE)));

    auto *mainLayout = new QVBoxLayout(this);

    // --- Debug Output Options group ---
    auto *debugOptionsGroup = new QGroupBox(QApplication::translate("LogWindow", "Debug Output Options"));
    debugOptionsGroup->setStyleSheet(
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
    auto *debugOptionsLayout = new QHBoxLayout(debugOptionsGroup);

    // Show process output
    auto *processOutputLabel = new QLabel(QApplication::translate("LogWindow", "Show process output"));
    m_processOutputCheckbox = new QCheckBox();
    m_processOutputCheckbox->setToolTip(QApplication::translate("LogWindow",
        "Display stdout/stderr from rpicam-apps process (including metadata with --metadata -)"));
    QObject::connect(m_processOutputCheckbox, &QCheckBox::toggled, [](bool checked) {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.setValue("Debug/ShowProcessOutput", checked);
    });

    // "Log to:" label
    auto *logToLabel = new QLabel(QApplication::translate("LogWindow", "Log to:"));

    // Log to window checkbox
    m_debugToLogWindowCheckbox = new QCheckBox(QApplication::translate("LogWindow", "window"));
    m_debugToLogWindowCheckbox->setToolTip(QApplication::translate("LogWindow", "Display debug messages in this window"));
    QObject::connect(m_debugToLogWindowCheckbox, &QCheckBox::toggled, [](bool checked) {
        DebugLogger::setLogToWidget(checked);
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.setValue("Debug/LogToWindow", checked);
    });

    // Log to console checkbox
    m_standardLoggingCheckbox = new QCheckBox(QApplication::translate("LogWindow", "console"));
    m_standardLoggingCheckbox->setToolTip(QApplication::translate("LogWindow", "Write debug messages to terminal/console"));
    QObject::connect(m_standardLoggingCheckbox, &QCheckBox::toggled, [](bool checked) {
        DebugLogger::setStandardLogging(checked);
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.setValue("Debug/LogToConsole", checked);
    });

    // Log to file checkbox
    m_debugToFileCheckbox = new QCheckBox("debug.log");
    m_debugToFileCheckbox->setToolTip(QApplication::translate("LogWindow", "Write debug messages to debug.log file"));
    QObject::connect(m_debugToFileCheckbox, &QCheckBox::toggled, [](bool checked) {
        DebugLogger::setLogToFile(checked);
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.setValue("Debug/LogToFile", checked);
    });

    debugOptionsLayout->addWidget(processOutputLabel);
    debugOptionsLayout->addWidget(m_processOutputCheckbox);
    debugOptionsLayout->addSpacing(20);
    debugOptionsLayout->addWidget(logToLabel);
    debugOptionsLayout->addWidget(m_debugToLogWindowCheckbox);
    debugOptionsLayout->addWidget(m_standardLoggingCheckbox);
    debugOptionsLayout->addWidget(m_debugToFileCheckbox);
    debugOptionsLayout->addStretch();

    // Load debug settings from global config
    QSettings debugSettings(AppPaths::globalConf(), QSettings::IniFormat);
    bool logToWindow = debugSettings.value("Debug/LogToWindow", false).toBool();
    bool logToConsole = debugSettings.value("Debug/LogToConsole", false).toBool();
    bool logToFile = debugSettings.value("Debug/LogToFile", false).toBool();
    bool showProcessOutput = debugSettings.value("Debug/ShowProcessOutput", false).toBool();

    DebugLogger::setLogToWidget(logToWindow);
    DebugLogger::setLogToFile(logToFile);
    DebugLogger::setStandardLogging(logToConsole);

    m_debugToLogWindowCheckbox->setChecked(logToWindow);
    m_standardLoggingCheckbox->setChecked(logToConsole);
    m_debugToFileCheckbox->setChecked(logToFile);
    m_processOutputCheckbox->setChecked(showProcessOutput);

    CollapsibleHelper::makeCollapsible(debugOptionsGroup, "UI/SharedLog/OptionsGroup");

    mainLayout->addWidget(debugOptionsGroup);

    // --- Debug Messages group ---
    auto *debugMessagesGroup = new QGroupBox(QApplication::translate("LogWindow", "Debug Messages"));
    debugMessagesGroup->setStyleSheet(
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
    auto *debugMessagesLayout = new QVBoxLayout(debugMessagesGroup);

    m_logWidget = new QTextEdit();
    m_logWidget->setReadOnly(true);
    m_logWidget->setStyleSheet(
        "QTextEdit {"
        "    font-family: 'Courier New', monospace;"
        "    font-size: 10pt;"
        "}"
    );
    m_logWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_logWidget->setMinimumHeight(200);
    debugMessagesLayout->addWidget(m_logWidget, 1);

    auto *clearLogButton = new QPushButton(QApplication::translate("LogWindow", "Clear Log"));
    connect(clearLogButton, &QPushButton::clicked, this, &LogWindow::clearLog);
    debugMessagesLayout->addWidget(clearLogButton);

    CollapsibleHelper::makeCollapsible(debugMessagesGroup, "UI/SharedLog/MessagesGroup");

    debugMessagesGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(debugMessagesGroup, 100);
    mainLayout->addStretch(1);

    // Restore window geometry
    QSettings geomSettings(AppPaths::globalConf(), QSettings::IniFormat);
    QByteArray savedGeometry = geomSettings.value("Geometry/LogWindow").toByteArray();
    if (!savedGeometry.isEmpty()) {
        restoreGeometry(savedGeometry);
    } else {
        resize(700, 500);
    }
}

LogWindow::~LogWindow()
{
    // Save window geometry on destruction
    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    s.setValue("Geometry/LogWindow", saveGeometry());
}

void LogWindow::closeEvent(QCloseEvent *event)
{
    // Save geometry before hiding
    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    s.setValue("Geometry/LogWindow", saveGeometry());

    hide();
    emit visibilityChanged(false);
    event->ignore(); // Don't close, just hide
}

void LogWindow::clearLog()
{
    m_logWidget->clear();
    if (m_debugToFileCheckbox->isChecked()) {
        DebugLogger::clearLogFile();
    }
}
