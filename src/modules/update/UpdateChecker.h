// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// UpdateChecker.h - Checks GitHub Releases for newer versions, downloads .deb

#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QProcess>

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker() override = default;

    // Current version from Version.h
    static QString currentVersion();

    // True if running from a git clone (AppPaths::base()/.git exists)
    static bool isGitClone();

    // System architecture for matching .deb assets (e.g. "arm64", "armhf")
    static QString detectArchitecture();

    // Start an async check against GitHub Releases
    void checkForUpdates();

    // Download the .deb asset to a local file
    void downloadDeb(const QString &url, const QString &destPath);

    // Install a .deb via pkexec apt install -y
    void installDeb(const QString &debPath);

    // True while any request is in flight
    bool isChecking() const { return m_checking; }
    bool isDownloading() const { return m_downloading; }

signals:
    void updateAvailable(const QString &latestVersion,
                         const QString &currentVersion,
                         const QString &debDownloadUrl,
                         const QString &releasePageUrl,
                         const QString &changelog);
    void upToDate(const QString &currentVersion);
    void errorOccurred(const QString &message);
    void downloadProgress(qint64 received, qint64 total);
    void downloadFinished(bool success, const QString &filePath);
    void installFinished(bool success, const QString &message);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();
    void onInstallFinished(int exitCode, QProcess::ExitStatus status);

private:
    static bool versionGreaterThan(const QString &a, const QString &b);

    QNetworkAccessManager *m_nam = nullptr;
    bool m_checking = false;
    // True when the full release list (incl. pre-releases) was requested;
    // the reply must then be parsed as JSON array instead of a single object.
    bool m_parseList = false;

    // Download state
    QNetworkReply *m_downloadReply = nullptr;
    QFile *m_downloadFile = nullptr;
    bool m_downloading = false;

    // Manual redirect follow (GitHub release assets: 302 -> Azure SAS URL;
    // Qt 5.15 sometimes fails to follow these automatically, leaving the
    // empty 302 body). Loop protection: max 5 hops.
    int m_redirectCount = 0;

    // Install state
    QProcess *m_installProc = nullptr;
};

#endif // UPDATECHECKER_H
