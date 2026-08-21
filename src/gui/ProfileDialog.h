// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// ProfileDialog.h - Manage camera settings profiles (create, rename, delete,
// activate). A profile is a snapshot of the current widget values of one or
// both camera instances, stored in the global config under Profiles/<name>/.

#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include <QList>

class MainWindow;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QTextEdit;
class QCheckBox;
class QPushButton;

class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    // camWindows: the camera instances (cam0 and optionally cam1).
    // The dialog saves snapshots from and applies profiles to these instances.
    explicit ProfileDialog(QWidget *parent, const QList<MainWindow*> &camWindows);

    // Activate a profile: store Profiles/Active and apply the stored values
    // immediately to every camera instance that is part of the profile scope.
    static void activateProfile(const QString &name, const QList<MainWindow*> &camWindows);

    // Names of all saved profiles (sorted, case-insensitive).
    static QStringList profileNames();

    // True if the QSettings group exists and contains at least one key.
    static bool groupHasKeys(const QString &group);

    // Put the dialog into "new profile" state: clears the edit fields and
    // enables them (used by the Profiles menu shortcut).
    void startNewProfile();

private slots:
    void refreshList();
    void onNewClicked();
    void onSaveClicked();
    void onDeleteClicked();
    void onExportClicked();
    void onImportClicked();
    void onItemClicked(QListWidgetItem *item);
    void onItemChanged(QListWidgetItem *item);

private:
    // Load name/comment/scope of a profile into the edit fields.
    // Empty name clears the fields.
    void loadProfileIntoFields(const QString &name);

    // Enable/disable the edit fields and the Save/Delete buttons.
    // Locked when no profile is selected and none is active.
    void setFieldsEnabled(bool enabled);

    MainWindow *camWindowFor(int camIndex) const;

    QList<MainWindow*> m_camWindows;

    QListWidget *m_profileList = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QTextEdit *m_commentEdit = nullptr;
    QCheckBox *m_cam0Check = nullptr;
    QCheckBox *m_cam1Check = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QPushButton *m_importButton = nullptr;

    // Guard flag while refreshing check states programmatically
    bool m_refreshing = false;

    // Name of the profile currently loaded into the fields (empty = none/new).
    QString m_currentName;
};

#endif // PROFILEDIALOG_H
