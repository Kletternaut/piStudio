// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// ProfileDialog.cpp - Manage camera settings profiles.
//
// Config layout (global config file, INI format):
//   [Profiles]
//   Active=<name>                     (empty/none = use Set Defaults)
//   Profiles/<name>/Comment           (long description text)
//   Profiles/<name>/Cam0/<keys>       (widget snapshot of camera 0)
//   Profiles/<name>/Cam1/<keys>       (widget snapshot of camera 1)
//
// Scope of a profile = which Cam<N> groups exist. On startup (and on
// activation) only cameras that are part of the scope are overwritten;
// the other cameras fall back to their own Set Defaults.

#include "ProfileDialog.h"
#include "MainWindow.h"
#include "../utils/AppPaths.h"
#include "../app/AppMeta.h"

#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

namespace {
constexpr int MAX_PROFILES = 10;

QString profileCamGroup(const QString &name, int camIndex)
{
    return QString("Profiles/%1/Cam%2").arg(name).arg(camIndex);
}
} // namespace

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------
bool ProfileDialog::groupHasKeys(const QString &group)
{
    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    s.beginGroup(group);
    const bool has = !s.allKeys().isEmpty();
    s.endGroup();
    return has;
}

QStringList ProfileDialog::profileNames()
{
    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    s.beginGroup("Profiles");
    QStringList names = s.childGroups();
    s.endGroup();
    names.sort(Qt::CaseInsensitive);
    return names;
}

void ProfileDialog::activateProfile(const QString &name, const QList<MainWindow*> &camWindows)
{
    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    s.setValue("Profiles/Active", name);

    // Apply immediately to every camera instance that is part of the scope.
    for (MainWindow *w : camWindows) {
        if (!w) continue;
        if (groupHasKeys(profileCamGroup(name, w->cameraIndex()))) {
            w->loadProfileValues(name);
        }
    }
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
ProfileDialog::ProfileDialog(QWidget *parent, const QList<MainWindow*> &camWindows)
    : QDialog(parent)
    , m_camWindows(camWindows)
{
    setWindowTitle(tr("Profiles"));
    setMinimumSize(560, 480);

    // Group style definition (same look as the GUI setup dialog)
    QString groupStyle =
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 8px;"
        "    background-color: #ecf0f1;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 8px;"
        "    padding: 0 3px 0 3px;"
        "}";

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    auto *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(10);

    // --- Left: profile list ------------------------------------------------
    auto *listGroup = new QGroupBox(tr("Profiles"), this);
    listGroup->setStyleSheet(groupStyle);
    auto *listLayout = new QVBoxLayout(listGroup);

    m_profileList = new QListWidget(listGroup);
    m_profileList->setMinimumWidth(200);
    listLayout->addWidget(m_profileList);

    auto *listButtons = new QHBoxLayout;
    auto *newButton = new QPushButton(tr("New"), listGroup);
    m_deleteButton = new QPushButton(tr("Delete"), listGroup);
    m_importButton = new QPushButton(tr("Import…"), listGroup);
    m_importButton->setToolTip(tr("Import a profile from a .rpcp file."));
    listButtons->addWidget(newButton);
    listButtons->addWidget(m_deleteButton);
    listButtons->addWidget(m_importButton);
    listButtons->addStretch();
    listLayout->addLayout(listButtons);

    contentLayout->addWidget(listGroup, 2);

    // --- Right: profile details --------------------------------------------
    auto *detailGroup = new QGroupBox(tr("Profile Details"), this);
    detailGroup->setStyleSheet(groupStyle);
    auto *detailLayout = new QVBoxLayout(detailGroup);

    auto *nameLayout = new QHBoxLayout;
    auto *nameLabel = new QLabel(tr("Name:"), detailGroup);
    nameLabel->setFixedWidth(90);
    m_nameEdit = new QLineEdit(detailGroup);
    m_nameEdit->setMaxLength(40);
    m_nameEdit->setToolTip(tr("Profile name. To rename a profile, change the name and press Save."));
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);
    detailLayout->addLayout(nameLayout);

    auto *scopeLayout = new QHBoxLayout;
    auto *scopeLabel = new QLabel(tr("Cameras:"), detailGroup);
    scopeLabel->setFixedWidth(90);
    m_cam0Check = new QCheckBox(tr("Camera 0"), detailGroup);
    m_cam1Check = new QCheckBox(tr("Camera 1"), detailGroup);
    scopeLayout->addWidget(scopeLabel);
    scopeLayout->addWidget(m_cam0Check);
    scopeLayout->addWidget(m_cam1Check);
    scopeLayout->addStretch();
    detailLayout->addLayout(scopeLayout);

    auto *commentLabel = new QLabel(tr("Comment:"), detailGroup);
    detailLayout->addWidget(commentLabel);
    m_commentEdit = new QTextEdit(detailGroup);
    m_commentEdit->setToolTip(tr("Long description of the profile's purpose (stored in the config file)."));
    detailLayout->addWidget(m_commentEdit, 1);

    auto *saveButton = new QPushButton(tr("Save"), detailGroup);
    saveButton->setToolTip(tr("Create a new profile or update the selected one (including rename). The current camera settings are stored as a snapshot for the selected cameras."));
    detailLayout->addWidget(saveButton);
    m_saveButton = saveButton;

    auto *exportButton = new QPushButton(tr("Export…"), detailGroup);
    exportButton->setToolTip(tr("Export the selected profile to a .rpcp file (backup, transfer between installations)."));
    detailLayout->addWidget(exportButton);
    m_exportButton = exportButton;

    contentLayout->addWidget(detailGroup, 3);

    mainLayout->addLayout(contentLayout, 1);

    auto *closeButton = new QPushButton(tr("Close"), this);
    mainLayout->addWidget(closeButton, 0, Qt::AlignRight);

    // Only the cameras that actually exist (piStudio camera detection)
    // are offered. A single-camera session hides the Camera 1 checkbox.
    m_cam0Check->setEnabled(camWindowFor(0) != nullptr);
    const bool hasCam1 = (camWindowFor(1) != nullptr);
    m_cam1Check->setVisible(hasCam1);
    m_cam1Check->setEnabled(hasCam1);

    // --- Connections --------------------------------------------------------
    // Checkable items (rectangular checkboxes, drawn by the list view itself
    // so the text is never covered) replace widget-based radio buttons.
    connect(m_profileList, &QListWidget::itemClicked, this, &ProfileDialog::onItemClicked);
    connect(m_profileList, &QListWidget::itemChanged, this, &ProfileDialog::onItemChanged);
    connect(newButton, &QPushButton::clicked, this, &ProfileDialog::onNewClicked);
    connect(saveButton, &QPushButton::clicked, this, &ProfileDialog::onSaveClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &ProfileDialog::onDeleteClicked);
    connect(exportButton, &QPushButton::clicked, this, &ProfileDialog::onExportClicked);
    connect(m_importButton, &QPushButton::clicked, this, &ProfileDialog::onImportClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    refreshList();
}

// ---------------------------------------------------------------------------
// Internals
// ---------------------------------------------------------------------------
MainWindow *ProfileDialog::camWindowFor(int camIndex) const
{
    for (MainWindow *w : m_camWindows) {
        if (w && w->cameraIndex() == camIndex) return w;
    }
    return nullptr;
}

void ProfileDialog::loadProfileIntoFields(const QString &name)
{
    if (name.isEmpty()) {
        m_nameEdit->clear();
        m_commentEdit->clear();
        m_cam0Check->setChecked(false);
        m_cam1Check->setChecked(false);
        return;
    }

    m_nameEdit->setText(name);

    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    m_commentEdit->setPlainText(s.value("Profiles/" + name + "/Comment").toString());
    m_cam0Check->setChecked(groupHasKeys(profileCamGroup(name, 0)));
    m_cam1Check->setChecked(groupHasKeys(profileCamGroup(name, 1)));
}

void ProfileDialog::refreshList()
{
    // Prevent itemChanged from firing while we set the check states
    m_refreshing = true;
    m_profileList->clear();

    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    const QString active = s.value("Profiles/Active").toString();
    const QStringList names = profileNames();

    // Only real profiles are listed. A checked profile is active; no check
    // mark at all means "None" (Set Defaults apply). There is deliberately
    // no None entry — an absent active profile IS the None state.
    for (const QString &name : names) {
        auto *item = new QListWidgetItem(name, m_profileList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(name == active ? Qt::Checked : Qt::Unchecked);
    }

    m_refreshing = false;

    // Do NOT pre-fill the edit fields with the active profile. The dialog
    // starts clean and locked. Pre-filling used the STORED scope of the
    // active profile, while m_currentName stayed empty — a following Save
    // was treated as "new profile" and silently skipped the camera
    // snapshots because the inherited checkboxes were unchecked.
    // Fields are only populated by explicit user action: "New" or
    // clicking a profile in the list.
    if (!m_currentName.isEmpty()) {
        // Keep the currently edited profile in the fields (after Save etc.)
        loadProfileIntoFields(m_currentName);
        setFieldsEnabled(true);
    } else {
        loadProfileIntoFields(QString());
        setFieldsEnabled(false);
    }
}

void ProfileDialog::setFieldsEnabled(bool enabled)
{
    m_nameEdit->setEnabled(enabled);
    m_commentEdit->setEnabled(enabled);
    m_cam0Check->setEnabled(enabled && camWindowFor(0) != nullptr);
    m_cam1Check->setEnabled(enabled && camWindowFor(1) != nullptr);
    m_saveButton->setEnabled(enabled);
    m_deleteButton->setEnabled(enabled);
}

void ProfileDialog::onNewClicked()
{
    m_currentName.clear();
    m_nameEdit->clear();
    m_commentEdit->clear();
    m_cam0Check->setChecked(camWindowFor(0) != nullptr);
    m_cam1Check->setChecked(false);
    m_profileList->setCurrentRow(-1);
    setFieldsEnabled(true);
    m_nameEdit->setFocus();
}

void ProfileDialog::startNewProfile()
{
    onNewClicked();
}

void ProfileDialog::onSaveClicked()
{
    const QString newName = m_nameEdit->text().trimmed();
    if (newName.isEmpty()) {
        QMessageBox::warning(this, tr("Profiles"), tr("Profile name must not be empty."));
        return;
    }

    const QStringList existing = profileNames();
    const bool isNew = (m_currentName.isEmpty());
    const bool isRename = !isNew && (m_currentName != newName);

    if (isNew && existing.contains(newName)) {
        QMessageBox::warning(this, tr("Profiles"), tr("A profile with this name already exists."));
        return;
    }
    if (isNew && existing.size() >= MAX_PROFILES) {
        QMessageBox::warning(this, tr("Profiles"), tr("Maximum of 10 profiles reached."));
        return;
    }

    // Guard: a profile without any camera snapshot is useless and was the
    // source of "empty export" confusion. The hidden Cam1 checkbox still
    // reflects the stored scope, so dual-camera profiles edited in a
    // single-camera session pass this check correctly.
    if (!m_cam0Check->isChecked() && !m_cam1Check->isChecked()) {
        QMessageBox::warning(this, tr("Profiles"),
            tr("Select at least one camera for the profile."));
        return;
    }

    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);

    // Rename: move the active flag and drop the old group
    if (isRename) {
        if (s.value("Profiles/Active").toString() == m_currentName) {
            s.setValue("Profiles/Active", newName);
        }
        s.remove("Profiles/" + m_currentName);
    }

    // Scope: snapshot for checked cameras, remove groups for unchecked ones.
    // Cameras that are NOT currently detected (e.g. editing a dual-camera
    // profile during a single-camera session) keep their stored snapshot
    // untouched — a save must not silently strip the other camera's data.
    for (int camIdx : {0, 1}) {
        MainWindow *w = camWindowFor(camIdx);
        if (!w) continue; // camera not detected: leave stored data as is

        const QString group = profileCamGroup(newName, camIdx);
        const bool checked = camIdx == 0 ? m_cam0Check->isChecked()
                                         : m_cam1Check->isChecked();
        // Always remove first so stale keys from an older snapshot disappear
        s.remove(group);
        if (checked) {
            w->saveProfileSnapshot(newName);
        }
    }

    s.setValue("Profiles/" + newName + "/Comment", m_commentEdit->toPlainText());
    s.sync();

    // A newly created profile becomes active immediately — the user just
    // captured the current state, so it should take effect right away.
    // (Renames keep their previous active/inactive state.)
    if (isNew) {
        activateProfile(newName, m_camWindows);
    }

    m_currentName = newName;
    refreshList();
}

void ProfileDialog::onDeleteClicked()
{
    const QString name = m_currentName;
    if (name.isEmpty()) {
        QMessageBox::information(this, tr("Profiles"), tr("No profile selected."));
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Delete Profile"),
        tr("Delete profile '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
    const bool wasActive = (s.value("Profiles/Active").toString() == name);
    s.remove("Profiles/" + name);
    if (wasActive) {
        s.remove("Profiles/Active");
        // No active profile means "None": apply Set Defaults immediately
        for (MainWindow *w : m_camWindows) {
            if (w) w->loadStartupDefaults();
        }
    }
    s.sync();

    m_currentName.clear();
    loadProfileIntoFields(QString());
    refreshList();

    if (wasActive) {
        QMessageBox::information(this, tr("Profiles"),
            tr("Active profile deleted — falling back to Set Defaults."));
    }
}

// ---------------------------------------------------------------------------
// Export / Import — single profile per .rpcp file
// ---------------------------------------------------------------------------
// File format: a plain INI file (same dialect as the global config), with
// the profile name in [Profile] and the snapshot data (Comment + Cam<N>
// groups) under [Data]. All copies go through QSettings, so the INI escaping
// of slashes/newlines round-trips exactly.
void ProfileDialog::onExportClicked()
{
    if (m_currentName.isEmpty()) {
        QMessageBox::information(this, tr("Profiles"), tr("No profile selected."));
        return;
    }

    const QString defaultFile = QDir::homePath() + "/" + m_currentName + ".rpcp";
    QString path = QFileDialog::getSaveFileName(this, tr("Export Profile"),
        defaultFile, tr("%1 Profile (*.rpcp)").arg(QLatin1String(AppMeta::NAME)));
    if (path.isEmpty()) return;
    if (!path.endsWith(QLatin1String(".rpcp"), Qt::CaseInsensitive))
        path += QLatin1String(".rpcp");

    QSettings cfg(AppPaths::globalConf(), QSettings::IniFormat);
    QSettings out(path, QSettings::IniFormat);
    out.clear();
    out.setValue("Profile/Name", m_currentName);

    cfg.beginGroup("Profiles/" + m_currentName);
    const QStringList keys = cfg.allKeys();
    for (const QString &key : keys) {
        out.setValue("Data/" + key, cfg.value(key));
    }
    cfg.endGroup();
    out.sync();
}

void ProfileDialog::onImportClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Profile"),
        QDir::homePath(), tr("%1 Profile (*.rpcp)").arg(QLatin1String(AppMeta::NAME)));
    if (path.isEmpty()) return;

    QSettings in(path, QSettings::IniFormat);
    QString name = in.value("Profile/Name").toString().trimmed();
    if (name.isEmpty()) {
        // Fallback: use the file name without extension
        name = QFileInfo(path).completeBaseName();
    }

    const QStringList existing = profileNames();
    if (existing.contains(name)) {
        const auto answer = QMessageBox::question(this, tr("Import Profile"),
            tr("Profile '%1' already exists. Overwrite?").arg(name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    } else if (existing.size() >= MAX_PROFILES) {
        QMessageBox::warning(this, tr("Profiles"), tr("Maximum of 10 profiles reached."));
        return;
    }

    // Copy the stored snapshot into the global config. The existing group is
    // removed first so stale keys from an older version disappear.
    QSettings cfg(AppPaths::globalConf(), QSettings::IniFormat);
    cfg.remove("Profiles/" + name);

    in.beginGroup("Data");
    const QStringList keys = in.allKeys();
    for (const QString &key : keys) {
        cfg.setValue("Profiles/" + name + "/" + key, in.value(key));
    }
    in.endGroup();
    cfg.sync();

    m_currentName = name;
    loadProfileIntoFields(name);
    refreshList();
}

void ProfileDialog::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    m_currentName = item->text();
    loadProfileIntoFields(item->text());
    setFieldsEnabled(true);
}

void ProfileDialog::onItemChanged(QListWidgetItem *item)
{
    if (m_refreshing || !item) return;

    if (item->checkState() == Qt::Checked) {
        // Exclusive behaviour: one rectangular checkbox at a time
        m_refreshing = true;
        for (int i = 0; i < m_profileList->count(); ++i) {
            QListWidgetItem *other = m_profileList->item(i);
            if (other != item && other->checkState() == Qt::Checked) {
                other->setCheckState(Qt::Unchecked);
            }
        }
        m_refreshing = false;

        const QString name = item->text();
        m_currentName = name;
        activateProfile(name, m_camWindows);
        loadProfileIntoFields(name);
        setFieldsEnabled(true);
    } else {
        // Unchecking the active profile deactivates it: no active profile
        // means "None" — the per-camera Set Defaults apply immediately.
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        if (item->text() == s.value("Profiles/Active").toString()) {
            s.remove("Profiles/Active");
            for (MainWindow *w : m_camWindows) {
                if (w) w->loadStartupDefaults();
            }
        }
    }
}
