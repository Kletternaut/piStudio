// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// UpdateChecker.cpp - Checks GitHub Releases, downloads and installs .deb

#include "UpdateChecker.h"
#include "../../Version.h"
#include "../../utils/AppPaths.h"
#include "../../app/AppMeta.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSettings>

// GitHub release API endpoints are built from AppMeta (product-name central).

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onReplyFinished);
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------
QString UpdateChecker::currentVersion()
{
    return QString::fromLatin1(VERSION_STRING);
}

bool UpdateChecker::isGitClone()
{
    return QDir(AppPaths::base() + ".git").exists();
}

QString UpdateChecker::detectArchitecture()
{
    QProcess proc;
    proc.start(QStringLiteral("dpkg"), {QStringLiteral("--print-architecture")});
    proc.waitForFinished(3000);
    QString arch = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (arch.isEmpty())
        arch = QStringLiteral("arm64"); // fallback for RPi
    return arch;
}

// ---------------------------------------------------------------------------
// versionGreaterThan — SemVer-aware comparison incl. pre-release suffixes
// ---------------------------------------------------------------------------
// Split "0.7-beta2" into numeric part ("0.7") and pre-release suffix ("beta2").
static void splitVersionParts(const QString &version, QString &numeric, QString &prerelease)
{
    const int dash = version.indexOf(QLatin1Char('-'));
    if (dash < 0) {
        numeric = version.trimmed();
        prerelease.clear();
    } else {
        numeric = version.left(dash).trimmed();
        prerelease = version.mid(dash + 1).trimmed();
    }
}

// Rank well-known pre-release identifiers: alpha < beta < rc < anything else.
static int prereleaseRank(const QString &label)
{
    if (label.startsWith(QLatin1String("alpha"), Qt::CaseInsensitive)) return 0;
    if (label.startsWith(QLatin1String("beta"), Qt::CaseInsensitive)) return 1;
    if (label.startsWith(QLatin1String("rc"), Qt::CaseInsensitive)) return 2;
    return 3;
}

// Trailing number of a pre-release suffix ("beta2" -> 2, "beta" -> 0).
static int prereleaseNumber(const QString &prerelease)
{
    QString digits;
    for (int i = prerelease.size() - 1; i >= 0; --i) {
        if (prerelease.at(i).isDigit()) {
            digits.prepend(prerelease.at(i));
        } else {
            break;
        }
    }
    return digits.isEmpty() ? 0 : digits.toInt();
}

// Compare two pre-release suffixes: <0, 0, >0.
// No suffix (final release) beats any suffix; same rank ordered by number.
static int comparePrerelease(const QString &a, const QString &b)
{
    if (a == b) return 0;
    if (a.isEmpty()) return 1;
    if (b.isEmpty()) return -1;

    const int rankA = prereleaseRank(a);
    const int rankB = prereleaseRank(b);
    if (rankA != rankB) return rankA < rankB ? -1 : 1;

    const int numA = prereleaseNumber(a);
    const int numB = prereleaseNumber(b);
    if (numA != numB) return numA < numB ? -1 : 1;

    return QString::compare(a, b, Qt::CaseInsensitive);
}

bool UpdateChecker::versionGreaterThan(const QString &a, const QString &b)
{
    QString numericA, prereleaseA, numericB, prereleaseB;
    splitVersionParts(a, numericA, prereleaseA);
    splitVersionParts(b, numericB, prereleaseB);

    // 1) Compare numeric parts
    const QStringList partsA = numericA.split(QLatin1Char('.'));
    const QStringList partsB = numericB.split(QLatin1Char('.'));
    const int n = qMax(partsA.size(), partsB.size());
    for (int i = 0; i < n; ++i) {
        const int va = i < partsA.size() ? partsA[i].toInt() : 0;
        const int vb = i < partsB.size() ? partsB[i].toInt() : 0;
        if (va != vb) return va > vb;
    }

    // 2) Same number: pre-release ordering decides
    //    (0.7.0 > 0.7-beta, beta2 > beta, rc > beta)
    return comparePrerelease(prereleaseA, prereleaseB) > 0;
}

// ---------------------------------------------------------------------------
// checkForUpdates
// ---------------------------------------------------------------------------
void UpdateChecker::checkForUpdates()
{
    if (m_checking) return;
    m_checking = true;

    // Beta updates: user setting. When enabled, query the full release list
    // (newest first, includes pre-releases) and pick the newest entry.
    // CRITICAL: must read inside beginGroup("General") — GuiSetupDialog
    // stores the key under the "General" group, which Qt's INI format maps
    // to the [%General] section. A root-level read looks in [General],
    // which does not exist, and would silently always return false.
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup("General");
    const bool includePrereleases =
        settings.value(QStringLiteral("Updates/BetaUpdates"), false).toBool();
    settings.endGroup();

    const QString apiUrl = includePrereleases
        ? AppMeta::releasesApiUrl(false)
        : AppMeta::releasesApiUrl(true);
    m_parseList = includePrereleases;

    QNetworkRequest request((QUrl(apiUrl)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("User-Agent", AppMeta::UPDATE_UA);
    m_nam->get(request);

    qDebug() << "[UpdateChecker] Checking for updates at" << apiUrl
             << "(beta updates:" << (includePrereleases ? "on" : "off") << ")";
}

// ---------------------------------------------------------------------------
// onReplyFinished — parse release JSON
// ---------------------------------------------------------------------------
void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    // Check if this is a download reply
    if (reply != nullptr && m_downloadReply == reply) {
        return; // handled by download slots
    }

    m_checking = false;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[UpdateChecker] Network error:" << reply->errorString();
        emit errorOccurred(tr("Could not check for updates: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "[UpdateChecker] JSON parse error:" << parseError.errorString();
        emit errorOccurred(tr("Invalid response from GitHub"));
        return;
    }

    const QJsonObject release = m_parseList
        ? doc.array().first().toObject()   // full list: newest release first
        : doc.object();                    // single release from .../latest

    const QString tagName = release.value(QStringLiteral("tag_name")).toString();
    if (tagName.isEmpty()) {
        emit errorOccurred(tr("No release information found"));
        return;
    }

    // Strip leading 'v' from tag
    QString latestVersion = tagName;
    if (latestVersion.startsWith(QLatin1Char('v')) || latestVersion.startsWith(QLatin1Char('V')))
        latestVersion = latestVersion.mid(1);

    const QString curVer = currentVersion();

    if (versionGreaterThan(latestVersion, curVer)) {
        const QString releasePageUrl =
            release.value(QStringLiteral("html_url")).toString();
        const QString changelog =
            release.value(QStringLiteral("body")).toString();

        // Find matching .deb asset URL
        QString debUrl;
        const QString arch = detectArchitecture();
        const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
        for (const QJsonValue &val : assets) {
            const QJsonObject asset = val.toObject();
            const QString name = asset.value(QStringLiteral("name")).toString();
            // Match: <package>_X.Y.Z_arm64.deb (or arch variant)
            if (name.contains(QLatin1String(AppMeta::BINARY)) &&
                name.contains(arch) && name.endsWith(QStringLiteral(".deb"))) {
                debUrl = asset.value(QStringLiteral("browser_download_url")).toString();
                break;
            }
        }

        qDebug() << "[UpdateChecker] New version available:" << latestVersion
                 << "(current:" << curVer << "arch:" << arch
                 << "deb:" << (!debUrl.isEmpty() ? debUrl : "not found") << ")";
        emit updateAvailable(latestVersion, curVer, debUrl, releasePageUrl, changelog);
    } else {
        qDebug() << "[UpdateChecker] Up to date:" << curVer;
        emit upToDate(curVer);
    }
}

// ---------------------------------------------------------------------------
// downloadDeb — download .deb asset to local file
// ---------------------------------------------------------------------------
void UpdateChecker::downloadDeb(const QString &url, const QString &destPath)
{
    if (m_downloading) return;
    m_downloading = true;
    m_redirectCount = 0;

    m_downloadFile = new QFile(destPath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        qDebug() << "[UpdateChecker] Cannot open file for writing:" << destPath;
        emit downloadFinished(false, QString());
        m_downloading = false;
        return;
    }

    QUrl dlUrl(url);
    QNetworkRequest request(dlUrl);
    request.setRawHeader("User-Agent", AppMeta::UPDATE_UA);
    m_downloadReply = m_nam->get(request);

    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::onDownloadProgress);
    // NOTE: no readyRead handler — the payload is read once in
    // onDownloadFinished, after the reply has completed. Incremental
    // readyRead writes caused truncated (0-byte) .deb files.
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &UpdateChecker::onDownloadFinished);

    qDebug() << "[UpdateChecker] Downloading" << url << "->" << destPath;
}

void UpdateChecker::onDownloadProgress(qint64 received, qint64 total)
{
    emit downloadProgress(received, total);
}

void UpdateChecker::onDownloadFinished()
{
    if (!m_downloadReply) return;

    // GitHub release assets answer with a 302 to a very long Azure SAS URL.
    // Qt 5.15 sometimes fails to follow these automatically and finishes
    // with the empty 302 body (NoError + 0 bytes). Follow the redirect
    // manually; the file stays open and is written once the final reply
    // arrives. Loop protection: max 5 hops.
    const QVariant redirect =
        m_downloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirect.isValid() && m_redirectCount < 5) {
        ++m_redirectCount;
        const QUrl target = m_downloadReply->url().resolved(redirect.toUrl());
        qDebug() << "[UpdateChecker] Following redirect to"
                 << target.toString(QUrl::RemoveQuery);
        m_downloadReply->deleteLater();

        QNetworkRequest req(target);
        req.setRawHeader("User-Agent", AppMeta::UPDATE_UA);
        m_downloadReply = m_nam->get(req);
        connect(m_downloadReply, &QNetworkReply::downloadProgress,
                this, &UpdateChecker::onDownloadProgress);
        connect(m_downloadReply, &QNetworkReply::finished,
                this, &UpdateChecker::onDownloadFinished);
        return; // wait for the redirected reply
    }

    m_downloading = false;

    const bool ok = (m_downloadReply && m_downloadReply->error() == QNetworkReply::NoError);
    const QString path = m_downloadFile ? m_downloadFile->fileName() : QString();

    if (ok && m_downloadReply && m_downloadFile) {
        // The reply is complete: read the full payload now and write it in
        // one go. This is the documented Qt pattern for file downloads and
        // avoids the truncated files that incremental readyRead writes
        // caused (0-byte .deb, apt exit 100).
        const QByteArray data = m_downloadReply->readAll();
        m_downloadFile->write(data);
    }

    if (m_downloadFile) {
        m_downloadFile->close();
    }

    if (!ok) {
        qDebug() << "[UpdateChecker] Download failed:"
                 << (m_downloadReply ? m_downloadReply->errorString() : "unknown");
    } else {
        qDebug() << "[UpdateChecker] Download complete:" << path;
    }

    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;
    m_downloadFile->deleteLater();
    m_downloadFile = nullptr;

    emit downloadFinished(ok, path);
}

// ---------------------------------------------------------------------------
// installDeb — install via pkexec apt-get install -y
// ---------------------------------------------------------------------------
void UpdateChecker::installDeb(const QString &debPath)
{
    if (m_installProc) return;

    m_installProc = new QProcess(this);
    connect(m_installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &UpdateChecker::onInstallFinished);

    // Use apt-get (stable CLI, no stderr warning) instead of apt.
    // pkexec handles privilege escalation with a GUI password prompt.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("DEBIAN_FRONTEND"), QStringLiteral("noninteractive"));
    m_installProc->setProcessEnvironment(env);

    qDebug() << "[UpdateChecker] Installing:" << debPath;
    m_installProc->start(QStringLiteral("pkexec"),
                         {QStringLiteral("apt-get"), QStringLiteral("install"),
                          QStringLiteral("-y"), QStringLiteral("--allow-downgrades"),
                          debPath});
}

void UpdateChecker::onInstallFinished(int exitCode, QProcess::ExitStatus /*status*/)
{
    QString output = QString::fromUtf8(m_installProc->readAllStandardOutput());
    QString errOutput = QString::fromUtf8(m_installProc->readAllStandardError());

    bool ok = (exitCode == 0);
    QString msg = ok ? tr("Update installed successfully. Please restart %1.")
                           .arg(QLatin1String(AppMeta::NAME))
                     : tr("Installation failed (exit code %1):\n%2")
                           .arg(exitCode)
                           .arg(errOutput.isEmpty() ? output : errOutput);

    qDebug() << "[UpdateChecker] Install" << (ok ? "OK" : "FAILED")
             << "exit:" << exitCode;

    m_installProc->deleteLater();
    m_installProc = nullptr;

    emit installFinished(ok, msg);
}
