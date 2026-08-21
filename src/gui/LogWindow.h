// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
//
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// LogWindow.h - Standalone log window (was formerly a tab in the outer QTabWidget).

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QWidget>
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>

class LogWindow : public QWidget {
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow() override;

    // Returns the shared log text widget for process output and debug messages
    QTextEdit *logWidget() const { return m_logWidget; }

public slots:
    void clearLog();

signals:
    void visibilityChanged(bool visible);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QTextEdit *m_logWidget = nullptr;
    QCheckBox *m_processOutputCheckbox = nullptr;
    QCheckBox *m_debugToLogWindowCheckbox = nullptr;
    QCheckBox *m_standardLoggingCheckbox = nullptr;
    QCheckBox *m_debugToFileCheckbox = nullptr;
};

#endif // LOGWINDOW_H
