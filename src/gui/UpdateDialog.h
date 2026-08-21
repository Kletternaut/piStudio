// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// UpdateDialog.h - Dialog showing version info, update check, download & install

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;
class QTextEdit;
class QProgressBar;
class UpdateChecker;

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget *parent = nullptr);
    ~UpdateDialog() override = default;

private slots:
    void onCheckNow();
    void onUpdateAvailable(const QString &latestVersion,
                           const QString &currentVersion,
                           const QString &debDownloadUrl,
                           const QString &releasePageUrl,
                           const QString &changelog);
    void onUpToDate(const QString &currentVersion);
    void onError(const QString &message);
    void onInstallClicked();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(bool success, const QString &filePath);
    void onInstallFinished(bool success, const QString &message);

private:
    void setStatusIcon(const QString &color, const QString &text);
    void setStateChecking();
    void setStateDone();
    void setStateInstalling();

    UpdateChecker *m_checker = nullptr;
    QLabel *m_statusIcon = nullptr;
    QLabel *m_statusText = nullptr;
    QLabel *m_versionLabel = nullptr;
    QLabel *m_gitHint = nullptr;
    QPushButton *m_checkButton = nullptr;
    QPushButton *m_installButton = nullptr;
    QPushButton *m_openPageButton = nullptr;
    QTextEdit *m_changelogView = nullptr;
    QProgressBar *m_progressBar = nullptr;

    QString m_debDownloadUrl;
    QString m_releasePageUrl;
    bool m_isGitClone = false;
};

#endif // UPDATEDIALOG_H
