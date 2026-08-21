// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// UpdateDialog.cpp - Dialog showing version info, update check, download & install

#include "UpdateDialog.h"
#include "../modules/update/UpdateChecker.h"
#include "../app/AppMeta.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QPixmap>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QFrame>
#include <QMessageBox>
#include <QProcess>
#include <QDate>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
UpdateDialog::UpdateDialog(QWidget *parent)
    : QDialog(parent)
{
    m_isGitClone = UpdateChecker::isGitClone();

    setWindowTitle(tr("Check for Updates"));
    // No fixed size: the dialog grows when the update-available block
    // (multi-line status, changelog, git hint) appears. A fixed height
    // made the status block overlap the logo.
    setMinimumSize(m_isGitClone ? 420 : 460, m_isGitClone ? 340 : 380);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // --- Header: logo + app info (matching About dialog style) ---
    auto *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(15);

    auto *logoLabel = new QLabel();
    QPixmap logoPixmap(QLatin1String(AppMeta::LOGO_RESOURCE));
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    headerLayout->addWidget(logoLabel, 0, Qt::AlignTop);

    auto *headerTextWidget = new QWidget();
    headerTextWidget->setMinimumSize(250, 90);
    auto *headerTextLayout = new QVBoxLayout(headerTextWidget);
    headerTextLayout->setSpacing(2);
    headerTextLayout->setContentsMargins(0, 0, 0, 0);

    // TEMPORARILY DISABLED: "piStudio" heading in the header (user request).
    // Re-enable by removing this comment block.
    /*
    auto *titleLabel = new QLabel(QStringLiteral("<h3 style='margin:0;'>%1</h3>")
                                      .arg(QLatin1String(AppMeta::NAME)));
    headerTextLayout->addWidget(titleLabel);
    */

    m_versionLabel = new QLabel(
        QStringLiteral("<p style='margin:2px 0;'>%1 <b>%2</b></p>")
            .arg(tr("Version"), UpdateChecker::currentVersion()));
    m_versionLabel->setAlignment(Qt::AlignLeft);
    headerTextLayout->addWidget(m_versionLabel);

    auto *descLabel = new QLabel(
        tr("<p style='margin:2px 0;'><i>Camera App for Raspberry Pi OS</i></p>"));
    descLabel->setWordWrap(true);
    headerTextLayout->addWidget(descLabel);

    auto *copyrightLabel = new QLabel(tr(
        "<p style='margin: 2px 0;'>Copyright © 2025-%1 <b>Kletternaut</b></p>"
        "<p style='margin: 2px 0;'><a href='%2'>%3</a></p>")
        .arg(QDate::currentDate().year())
        .arg(AppMeta::repoUrl(), AppMeta::repoUrl()));
    copyrightLabel->setAlignment(Qt::AlignLeft);
    copyrightLabel->setOpenExternalLinks(true);
    headerTextLayout->addWidget(copyrightLabel);

    // One extra line of visual separation: the install-type line sat
    // directly on the URL above it.
    headerTextLayout->addSpacing(10);

    // Install type (last line of the header) - drives the update flow
    // (git clone hint vs. .deb download).
    auto *installLabel = new QLabel(m_isGitClone
        ? tr("<p style='margin:2px 0;'><i>Git clone &mdash; build from source</i></p>")
        : tr("<p style='margin:2px 0;'><i>System install &mdash; apt update</i></p>"));
    installLabel->setWordWrap(true);
    headerTextLayout->addWidget(installLabel);

    headerTextLayout->addStretch();
    headerLayout->addWidget(headerTextWidget, 1);
    mainLayout->addLayout(headerLayout);

    // --- Status line (icon + text) ---
    auto *statusLayout = new QHBoxLayout;
    m_statusIcon = new QLabel();
    m_statusIcon->setFixedWidth(24);
    m_statusIcon->setAlignment(Qt::AlignCenter);
    m_statusIcon->setStyleSheet("font-size: 16pt; font-weight: bold;");
    statusLayout->addWidget(m_statusIcon);

    m_statusText = new QLabel(tr("Click \"Check Now\" to look for updates."));
    m_statusText->setWordWrap(true);
    m_statusText->setStyleSheet("font-size: 11pt;");
    statusLayout->addWidget(m_statusText, 1);
    mainLayout->addLayout(statusLayout);

    // --- Git clone hint (hidden for .deb installs) ---
    m_gitHint = new QLabel();
    m_gitHint->setWordWrap(true);
    m_gitHint->setVisible(false);
    // Selectable so the build command can be copied by the user
    m_gitHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_gitHint->setStyleSheet(
        "QLabel { background-color: #fff3cd; border: 1px solid #ffc107; "
        "border-radius: 4px; padding: 6px; font-size: 10pt; }");
    mainLayout->addWidget(m_gitHint);

    // --- Progress bar (hidden until download starts) ---
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat(tr("%p% — %v / %m KB"));
    mainLayout->addWidget(m_progressBar);

    // --- Changelog (hidden until update found) ---
    m_changelogView = new QTextEdit();
    m_changelogView->setReadOnly(true);
    m_changelogView->setVisible(false);
    m_changelogView->setMaximumHeight(140);
    m_changelogView->setStyleSheet(
        "QTextEdit { background-color: #f5f5f5; border: 1px solid #ddd; "
        "border-radius: 4px; padding: 6px; }");
    mainLayout->addWidget(m_changelogView);

    mainLayout->addStretch();

    // --- Separator ---
    auto *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    // --- Buttons ---
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_installButton = new QPushButton(tr("Install Update"));
    m_installButton->setVisible(false);
    m_installButton->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; padding: 6px 16px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #2ecc71; }"
        "QPushButton:disabled { background-color: #95a5a6; }");
    connect(m_installButton, &QPushButton::clicked,
            this, &UpdateDialog::onInstallClicked);
    buttonLayout->addWidget(m_installButton);

    m_openPageButton = new QPushButton(tr("Open Release Page"));
    m_openPageButton->setVisible(false);
    m_openPageButton->setStyleSheet(
        "QPushButton { padding: 6px 16px; border-radius: 4px; }");
    connect(m_openPageButton, &QPushButton::clicked, this, [this]() {
        if (!m_releasePageUrl.isEmpty())
            QDesktopServices::openUrl(QUrl(m_releasePageUrl));
    });
    buttonLayout->addWidget(m_openPageButton);

    m_checkButton = new QPushButton(tr("Check Now"));
    m_checkButton->setStyleSheet(
        "QPushButton { padding: 6px 20px; border-radius: 4px; }");
    connect(m_checkButton, &QPushButton::clicked, this, &UpdateDialog::onCheckNow);
    buttonLayout->addWidget(m_checkButton);

    auto *closeButton = new QPushButton(tr("Close"));
    closeButton->setStyleSheet(
        "QPushButton { padding: 6px 20px; border-radius: 4px; }");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // --- UpdateChecker ---
    m_checker = new UpdateChecker(this);
    connect(m_checker, &UpdateChecker::updateAvailable,
            this, &UpdateDialog::onUpdateAvailable);
    connect(m_checker, &UpdateChecker::upToDate,
            this, &UpdateDialog::onUpToDate);
    connect(m_checker, &UpdateChecker::errorOccurred,
            this, &UpdateDialog::onError);
    connect(m_checker, &UpdateChecker::downloadProgress,
            this, &UpdateDialog::onDownloadProgress);
    connect(m_checker, &UpdateChecker::downloadFinished,
            this, &UpdateDialog::onDownloadFinished);
    connect(m_checker, &UpdateChecker::installFinished,
            this, &UpdateDialog::onInstallFinished);

    // Auto-check on open
    QMetaObject::invokeMethod(this, "onCheckNow", Qt::QueuedConnection);
}

// ---------------------------------------------------------------------------
// Slots — Check
// ---------------------------------------------------------------------------
void UpdateDialog::onCheckNow()
{
    if (m_checker->isChecking()) return;
    setStateChecking();
    m_checker->checkForUpdates();
}

void UpdateDialog::onUpdateAvailable(const QString &latestVersion,
                                     const QString &currentVersion,
                                     const QString &debDownloadUrl,
                                     const QString &releasePageUrl,
                                     const QString &changelog)
{
    setStateDone();
    setStatusIcon("#e74c3c", "!");

    m_debDownloadUrl = debDownloadUrl;
    m_releasePageUrl = releasePageUrl;

    if (m_isGitClone) {
        // Git clone: show build instructions
        m_statusText->setText(
            tr("<b>Update available!</b><br>"
               "Installed: %1 &nbsp;&rarr;&nbsp; Latest: <b>%2</b>")
                .arg(currentVersion, latestVersion));
        m_gitHint->setText(
            tr("<b>Build from source:</b><br>"
               "<code>git pull &amp;&amp; cmake -S . -B build &amp;&amp; cmake --build build -j$(nproc)</code>"));
        m_gitHint->setVisible(true);
        m_openPageButton->setVisible(true);
        m_installButton->setVisible(false);
    } else {
        // System install: show install button
        m_statusText->setText(
            tr("<b>Update available!</b><br>"
               "Installed: %1 &nbsp;&rarr;&nbsp; Latest: <b>%2</b>")
                .arg(currentVersion, latestVersion));
        m_gitHint->setVisible(false);

        if (!m_debDownloadUrl.isEmpty()) {
            m_installButton->setVisible(true);
            m_openPageButton->setVisible(false);
        } else {
            m_installButton->setVisible(false);
            m_openPageButton->setVisible(true);
            m_statusText->setText(m_statusText->text() +
                tr("<br><i>No .deb asset found for %1</i>")
                    .arg(UpdateChecker::detectArchitecture()));
        }
    }

    m_checkButton->setText(tr("Check Again"));

    if (!changelog.isEmpty()) {
        m_changelogView->setPlainText(changelog);
        m_changelogView->setVisible(true);
    }

    // Grow the dialog to fit the now-visible update block
    adjustSize();
}

void UpdateDialog::onUpToDate(const QString &currentVersion)
{
    setStateDone();
    setStatusIcon("#27ae60", "OK");
    m_statusText->setText(
        tr("You are running the latest version (%1).").arg(currentVersion));
    m_installButton->setVisible(false);
    m_openPageButton->setVisible(false);
    m_changelogView->setVisible(false);
    m_gitHint->setVisible(false);
    m_progressBar->setVisible(false);
    m_checkButton->setText(tr("Check Again"));
    adjustSize();
}

void UpdateDialog::onError(const QString &message)
{
    setStateDone();
    setStatusIcon("#f39c12", "?");
    m_statusText->setText(message);
    m_installButton->setVisible(false);
    m_openPageButton->setVisible(false);
    m_changelogView->setVisible(false);
    m_gitHint->setVisible(false);
    m_progressBar->setVisible(false);
    m_checkButton->setText(tr("Retry"));
    adjustSize();
}

// ---------------------------------------------------------------------------
// Slots — Download & Install
// ---------------------------------------------------------------------------
void UpdateDialog::onInstallClicked()
{
    if (m_debDownloadUrl.isEmpty()) return;

    setStateInstalling();
    m_statusText->setText(tr("Downloading update..."));

    QString destPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                       + QStringLiteral("/%1_update.deb").arg(QLatin1String(AppMeta::BINARY));
    // Remove stale file
    QFile::remove(destPath);

    m_checker->downloadDeb(m_debDownloadUrl, destPath);
}

void UpdateDialog::onDownloadProgress(qint64 received, qint64 total)
{
    m_progressBar->setVisible(true);
    if (total > 0) {
        m_progressBar->setMaximum(static_cast<int>(total / 1024));
        m_progressBar->setValue(static_cast<int>(received / 1024));
    }
}

void UpdateDialog::onDownloadFinished(bool success, const QString &filePath)
{
    m_progressBar->setVisible(false);

    if (!success) {
        setStateDone();
        setStatusIcon("#e74c3c", "!");
        m_statusText->setText(tr("Download failed. Check your internet connection."));
        m_installButton->setEnabled(true);
        m_checkButton->setText(tr("Retry"));
        return;
    }

    m_statusText->setText(tr("Installing update... (you may be asked for your password)"));
    m_checker->installDeb(filePath);
}

void UpdateDialog::onInstallFinished(bool success, const QString &message)
{
    setStateDone();

    if (success) {
        setStatusIcon("#27ae60", "OK");
        m_installButton->setVisible(false);
        m_openPageButton->setVisible(false);
        m_checkButton->setText(tr("Check Again"));
        m_statusText->setText(message);

        // Unmissable completion feedback: a modal box with one-click restart.
        // The small status line alone was easy to overlook ("nothing
        // happened" after the update).
        QMessageBox box(this);
        box.setIcon(QMessageBox::Information);
        box.setWindowTitle(tr("Update installed"));
        box.setText(tr("<b>Update installed successfully.</b><br><br>"
                       "Restart %1 now to use the new version?")
                        .arg(QLatin1String(AppMeta::NAME)));
        QPushButton *restartBtn = box.addButton(tr("Restart now"), QMessageBox::AcceptRole);
        QPushButton *laterBtn = box.addButton(tr("Later"), QMessageBox::RejectRole);
        box.setDefaultButton(restartBtn);
        box.exec();
        if (box.clickedButton() == restartBtn) {
            // Same pattern as the auto-restart after Set Defaults
            QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
            QApplication::quit();
        }
    } else {
        setStatusIcon("#e74c3c", "!");
        m_installButton->setEnabled(true);
        m_checkButton->setText(tr("Retry"));
        m_statusText->setText(message);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void UpdateDialog::setStatusIcon(const QString &color, const QString &text)
{
    m_statusIcon->setStyleSheet(
        QStringLiteral("font-size: 16pt; font-weight: bold; color: %1; "
                        "background-color: %2; border-radius: 10px; "
                        "min-width: 20px; max-width: 20px; "
                        "min-height: 20px; max-height: 20px; "
                        "padding: 2px;")
            .arg(color, color));
    m_statusIcon->setText(text);
}

void UpdateDialog::setStateChecking()
{
    setStatusIcon("#3498db", "...");
    m_statusText->setText(tr("Checking for updates..."));
    m_checkButton->setEnabled(false);
    m_installButton->setVisible(false);
    m_openPageButton->setVisible(false);
    m_changelogView->setVisible(false);
    m_gitHint->setVisible(false);
    m_progressBar->setVisible(false);
}

void UpdateDialog::setStateDone()
{
    m_checkButton->setEnabled(true);
}

void UpdateDialog::setStateInstalling()
{
    setStatusIcon("#3498db", "...");
    m_installButton->setEnabled(false);
    m_checkButton->setEnabled(false);
    m_openPageButton->setVisible(false);
}
