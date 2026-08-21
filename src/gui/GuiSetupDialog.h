#ifndef GUISETUPDIALOG_H
#define GUISETUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

class GuiSetupDialog : public QDialog {
    Q_OBJECT

public:
    explicit GuiSetupDialog(const QString &tabGroup = QString(), int cameraIndex = 0, QWidget *parent = nullptr, bool globalMode = false);
    ~GuiSetupDialog() override;

    bool isSplashScreenEnabled() const;
    void loadGuiSettings();
    // Effective lib path values currently active in the running instance
    // (may come from a loaded config file or an active profile).
    void setEffectiveLibPaths(const QString &previewLibs, const QString &postProcessLibs, const QString &encoderLibs);
    QStringList getCustomAppEntries() const;
    QStringList getCustomResolutions() const;

signals:
    void settingsSaved();  // Signal wird ausgelöst wenn Settings gespeichert werden

private slots:
    void saveGuiSettings();
    void browserpicamConfigFilePath();

private:
    QLineEdit *outputPathEdit = nullptr;
    QLineEdit *postProcessPathEdit = nullptr;
    QLineEdit *postProcessLibsPathEdit = nullptr;
    QLineEdit *tuningFilePathEdit = nullptr;
    QLineEdit *metadataPathEdit;
    QLineEdit *rpicamConfigPathEdit;
    QLineEdit *toolsInputPathEdit;
    QLineEdit *toolsOutputPathEdit;
    QLineEdit *previewLibsPathEdit = nullptr;    // camera mode only (Paths tab)
    QLineEdit *encoderLibsPathEdit = nullptr;    // camera mode only (Paths tab)
    QPushButton *postProcessLibsStatusLabel = nullptr; // X-reset/state button: red=temp, green=saved (camera mode)
    QPushButton *previewLibsStatusLabel = nullptr;     // X-reset/state button: red=temp, green=saved (camera mode)
    QPushButton *encoderLibsStatusLabel = nullptr;     // X-reset/state button: red=temp, green=saved (camera mode)
    QString m_effectivePreviewLibs;      // active value (config file / profile)
    QString m_effectivePostProcessLibs;  // active value (config file / profile)
    QString m_effectiveEncoderLibs;      // active value (config file / profile)
    QCheckBox *splashScreenCheckbox;
    QCheckBox *useCustomPreviewGeometryCheckbox;
    QComboBox *languageSelector = nullptr;
    QCheckBox *expertTabCheckbox;
    QCheckBox *focusTabCheckbox;
    QCheckBox *zoomTabCheckbox;
    QCheckBox *audioTabCheckbox;
    QCheckBox *gstreamerTabCheckbox;
    QCheckBox *gstTabCheckbox;
    QCheckBox *odrTabCheckbox;
    QCheckBox *actionsTabCheckbox;
    QCheckBox *toolsTabCheckbox;
    QCheckBox *controlSocketCheckbox = nullptr;
    QCheckBox *autoRestartSetDefaultsCheckbox = nullptr;
    QCheckBox *autoRestartResetDefaultsCheckbox = nullptr;
    QCheckBox *autoUpdateCheckCheckbox = nullptr;
    QCheckBox *betaUpdatesCheckbox = nullptr;

    // V4L2 Hardware configuration — Focus
    QCheckBox *v4l2FocusCheckbox = nullptr;
    QComboBox *v4l2FocusCombo = nullptr;
    QLabel *v4l2FocusStatusLabel = nullptr;

    // V4L2 Hardware configuration — Zoom
    QCheckBox *v4l2ZoomCheckbox = nullptr;
    QComboBox *v4l2ZoomCombo = nullptr;
    QLabel *v4l2ZoomStatusLabel = nullptr;

    QStringList customAppEntries; // Liste für benutzerdefinierte Apps
    QList<QLineEdit *> customAppInputs; // Eingabefelder für benutzerdefinierte Apps
    QList<QLineEdit *> customResolutionInputs; // Eingabefelder für benutzerdefinierte Auflösungen
    QString m_tabGroup; // Tab-Gruppen-Name fuer QSettings (z.B. "Camera0-Tab")
    int m_cameraIndex = 0; // Kamera-Index für Meta-Config-Speicherung
    bool m_globalMode = false; // true = Global Settings (Paths + Settings), false = Camera Setup (Focus/Zoom/Tabs/Custom)

    // Focus & Zoom Step Size SpinBoxes
    QSpinBox *focusStep1SpinBox = nullptr;
    QSpinBox *focusStep2SpinBox = nullptr;
    QSpinBox *focusStep3SpinBox = nullptr;
    QSpinBox *focusStep4SpinBox = nullptr;
    QSpinBox *focusStep5SpinBox = nullptr;
    QSpinBox *focusStep6SpinBox = nullptr;
    QSpinBox *zoomStep1SpinBox = nullptr;
    QSpinBox *zoomStep2SpinBox = nullptr;
    QSpinBox *zoomStep3SpinBox = nullptr;
    QSpinBox *zoomStep4SpinBox = nullptr;
    QSpinBox *zoomStep5SpinBox = nullptr;
    QSpinBox *zoomStep6SpinBox = nullptr;

    void setupCustomAppInputs(QVBoxLayout *layout); // Methode zur Einrichtung der Eingabefelder
    void setupCustomResolutionInputs(QVBoxLayout *layout); // Methode zur Einrichtung der Resolution-Eingabefelder
};

#endif // GUISETUPDIALOG_H
