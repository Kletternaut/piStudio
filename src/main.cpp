// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
//
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// main.cpp - Application entry point for piStudio.

#include <QApplication>
#include <QLabel>
#include "../utils/AppPaths.h"
#include <QMovie>
#include <QTimer>
#include <QScreen>
#include <QSettings>
#include <QBitmap>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#include <QMenuBar>
#include <QFileDialog>
#include <QShortcut>
#include <QProcess>
#include <QRegularExpression>
#include <QCheckBox>
#include <QGroupBox>
#include <QFont>
#include <QStyle>
#include <QMessageBox>
#include <QMenu>
#include <QWidgetAction>
#include <QFrame>
#include <QHBoxLayout>
#include <functional>
#include "../utils/DebugLogger.h"
#include "../app/AppMeta.h"
#include "MainWindow.h"
#include "gui/CollapsibleHelper.h"
#include "../Version.h" // Include for the version number
#include "gui/LogWindow.h"
#include "gui/UpdateDialog.h"
#include "gui/ProfileDialog.h"
#include "modules/update/UpdateChecker.h"

// Draw a small circular camera marker (digit 0/1) with QPainter instead of a
// stylesheet circle — perfectly round and crisp on every platform/theme.
static QIcon makeCamMarkerIcon(const QString &digit, const QColor &color)
{
    const int size = 20;
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(2.0, 2.0, size - 4.0, size - 4.0));
    QFont f = p.font();
    f.setPixelSize(11);
    p.setFont(f);
    p.setPen(QPen(color));
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, digit);
    p.end();
    return QIcon(pm);
}

// Click filter for the profile menu rows: fires the callback on mouse
// release and closes the menu. Plain QFrame/QLabel rows cannot use
// QAction::triggered, so events are intercepted here.
class ProfileRowClickFilter : public QObject {
public:
    std::function<void()> callback;
    explicit ProfileRowClickFilter(QObject *parent, std::function<void()> cb)
        : QObject(parent), callback(std::move(cb)) {}
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            if (callback) callback();
            return true;
        }
        return QObject::eventFilter(watched, event);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Load language setting and install translator
    QSettings langSettings(AppPaths::globalConf(), QSettings::IniFormat);
    QString language = langSettings.value("Language/Selected", "de").toString();

    QTranslator appTranslator;
    QString translationFile = QString("%1_%2")
        .arg(QLatin1String(AppMeta::I18N_BASENAME), language);
    // Search order: next to binary (dev/build), installed path, source tree
    if (appTranslator.load(translationFile, QCoreApplication::applicationDirPath()) ||
        appTranslator.load(translationFile, AppMeta::SHARE_DIR) ||
        appTranslator.load(translationFile, AppPaths::i18n())) {
        app.installTranslator(&appTranslator);
    }

    // Install Qt base translator for standard dialogs
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + language, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
    }

    // Set application icon early so the splash screen can use it
    QIcon appIcon = QIcon::fromTheme(QLatin1String(AppMeta::ICON_THEME),
                                     QIcon(QLatin1String(AppMeta::ICON_RESOURCE)));
    app.setWindowIcon(appIcon);
    QCoreApplication::setApplicationName(QLatin1String(AppMeta::NAME));

    // Show splash screen only when enabled
    // Global setting → [%General] section in the app config file.
    // CRITICAL: read inside beginGroup("General") — GuiSetupDialog stores
    // it under that group, which the INI format maps to [%General]. A flat
    // "General/..." key would read the different [General] section.
    QSettings mainSettings(AppPaths::globalConf(), QSettings::IniFormat);
    mainSettings.beginGroup("General");
    bool splashScreenEnabled = mainSettings.value("splashScreenEnabled", true).toBool();
    mainSettings.endGroup();
    // Splash/Duration maps to section [Splash], key Duration in the INI file.
    // Seed the default on first run so the config documents the setting —
    // otherwise the file only contains explicitly changed values.
    if (!mainSettings.contains("Splash/Duration")) {
        mainSettings.setValue("Splash/Duration", 1000);
    }
    int splashDuration = mainSettings.value("Splash/Duration", 1000).toInt(); // Duration in milliseconds, default 1000ms

    QLabel splash;
    QPixmap splashPixmap(QLatin1String(AppMeta::SPLASH_RESOURCE));
    splash.setFixedSize(300, 300); // Splash window size
    splash.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    splash.setAttribute(Qt::WA_TranslucentBackground); // Enable transparency
    splash.setStyleSheet("QLabel { background: transparent; }"); // Transparent background

    // IMPORTANT: the fade effect must live on a CHILD widget, not on the
    // top-level window. A QGraphicsEffect on a top-level window with a
    // window mask renders shifted (the known Qt offset bug). All fadable
    // content therefore sits in a child container; the window's mask
    // handling stays completely untouched.
    QWidget *splashContent = new QWidget(&splash);
    splashContent->setGeometry(0, 0, 300, 300);
    splashContent->setStyleSheet("background: transparent;");

    QLabel *logoLabel = new QLabel(splashContent);
    logoLabel->setGeometry(0, 0, 300, 300);
    logoLabel->setStyleSheet("background: transparent;");
    logoLabel->setAlignment(Qt::AlignCenter); // Center the logo

    // Mask and display pixmap must use the SAME offset, pixel-aligned, so
    // the logo content and the window mask match exactly.
    if (!splashPixmap.isNull()) {
        QBitmap mask = splashPixmap.mask();
        const int offsetX = (300 - mask.width()) / 2;
        const int offsetY = (300 - mask.height()) / 2;

        // Display pixmap: 300x300 transparent canvas, logo at the offset.
        QPixmap displayPixmap(300, 300);
        displayPixmap.fill(Qt::transparent);
        QPainter displayPainter(&displayPixmap);
        displayPainter.drawPixmap(offsetX, offsetY, splashPixmap);
        displayPainter.end();
        logoLabel->setPixmap(displayPixmap);

        // Centered mask: 300x300 with the logo mask at the same offset.
        QBitmap centeredMask(300, 300);
        centeredMask.fill(Qt::color0); // Transparent
        QPainter maskPainter(&centeredMask);
        maskPainter.drawPixmap(offsetX, offsetY, mask);
        maskPainter.end();
        splash.setMask(centeredMask);
    } else {
        logoLabel->setPixmap(splashPixmap);
    }

    // Fade effect on the child container: starts fully transparent so the
    // splash can fade in on show; fades logo and version label together.
    QGraphicsOpacityEffect *splashFadeEffect = nullptr;
    // Separate effect for the version label: it fades in delayed after the
    // logo. Declared here so the fade block below can access it.
    QGraphicsOpacityEffect *versionFadeEffect = nullptr;
    if (splashScreenEnabled) {
        splashFadeEffect = new QGraphicsOpacityEffect(splashContent);
        splashFadeEffect->setOpacity(0.0);
        splashContent->setGraphicsEffect(splashFadeEffect);
    }

    if (splashScreenEnabled) {
        // Try to load the saved MainWindow geometry (from the app config file!)
        QByteArray savedGeometry = mainSettings.value("Geometry/MainWindow").toByteArray();
        QRect mainWindowRect;

        if (!savedGeometry.isEmpty()) {
            // Create a temporary widget to extract the geometry
            QWidget tempWidget;
            tempWidget.restoreGeometry(savedGeometry);
            mainWindowRect = tempWidget.geometry();
        } else {
            // Fallback: center of the screen
            QRect screenGeometry = QApplication::primaryScreen()->geometry();
            mainWindowRect = QRect(
                (screenGeometry.width() - 800) / 2,  // Assumed MainWindow width: 800
                (screenGeometry.height() - 600) / 2, // Assumed MainWindow height: 600
                800, 600
            );
        }

        // Position the splash screen in the center of the MainWindow area
        int x = mainWindowRect.x() + (mainWindowRect.width() - splash.width()) / 2;
        int y = mainWindowRect.y() + (mainWindowRect.height() - splash.height()) / 2;
        splash.move(x, y);

        // Version info on the splash (re-enabled, without background).
        // Sibling of splashContent so it can fade independently: delayed
        // fade-in after the logo, synchronous fade-out with it.
        QLabel *versionLabel = new QLabel(&splash);
        versionLabel->setText(QStringLiteral("V %1").arg(VERSION_STRING));
        versionLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold; background: transparent;");
        versionLabel->adjustSize();
        // Position: centered on the intersection of the right vertical and
        // the top horizontal divider line of a 3x3 grid laid over the logo
        // (2/3 of the width, 1/3 of the height), then shifted +35px right
        // and +20px down.
        if (!splashPixmap.isNull()) {
            const int logoW = splashPixmap.width();
            const int logoH = splashPixmap.height();
            const int logoX = (splash.width() - logoW) / 2;
            const int logoY = (splash.height() - logoH) / 2;
            const int gridX = logoX + (2 * logoW) / 3;
            const int gridY = logoY + logoH / 3;
            versionLabel->move(gridX - versionLabel->width() / 2 + 35,
                               gridY - versionLabel->height() / 2 + 20);
        }

        // Delayed fade-in: the version label starts fully transparent and
        // is faded in by the fade block after the logo started fading.
        versionFadeEffect = new QGraphicsOpacityEffect(versionLabel);
        versionFadeEffect->setOpacity(0.0);
        versionLabel->setGraphicsEffect(versionFadeEffect);

        // Ensure splash uses the application icon
        splash.setWindowIcon(appIcon);
        splash.show();
    }

    // Detect how many cameras are connected before building the UI.
    // Run rpicam-vid --list-cameras synchronously; count "N : ..." lines.
    int detectedCameraCount = 0;
    {
        QProcess detectProc;
        detectProc.start("rpicam-vid", QStringList() << "--list-cameras");
        detectProc.waitForFinished(3000);
        if (detectProc.error() == QProcess::FailedToStart) {
            // rpicam-apps binaries are missing entirely (e.g. built from
            // source but not installed, or the package was never added).
            // The app starts anyway, but recording will not work.
            qWarning() << "[" << AppMeta::NAME << "] rpicam-vid not found in PATH — "
                          "install rpicam-apps via apt or build it from source";
        }
        QString detectOutput = QString::fromLocal8Bit(detectProc.readAllStandardOutput())
                             + QString::fromLocal8Bit(detectProc.readAllStandardError());
        static const QRegularExpression re(R"(^\s*(\d+)\s*:)",
                                           QRegularExpression::MultilineOption);
        QRegularExpressionMatchIterator it = re.globalMatch(detectOutput);
        QList<int> found;
        while (it.hasNext()) {
            int idx = it.next().captured(1).toInt();
            if (!found.contains(idx)) found.append(idx);
        }
        detectedCameraCount = found.isEmpty() ? 1 : found.size(); // fallback: at least 1
    }

    auto *cam0 = new MainWindow(0);
    cam0->fixCameraIndex(0);

    MainWindow *cam1 = nullptr;
    if (detectedCameraCount >= 2) {
        cam1 = new MainWindow(1);
        cam1->fixCameraIndex(1);
    }

    // Link instances so each can check whether the other's preview is running
    if (cam1) {
        cam0->setSiblingWindow(cam1);
        cam1->setSiblingWindow(cam0);
    }

    // The outer window holds the tab widget
    auto *outerWindow = new QMainWindow();
    outerWindow->setWindowTitle(QLatin1String(AppMeta::NAME));
    // Set window icon for the outer window (app icon already set above)
    outerWindow->setWindowIcon(appIcon);

    auto *tabs = new QTabWidget(outerWindow);
    tabs->setTabPosition(QTabWidget::North);

    // Each MainWindow embedded as tab widget: set flags first, then add as tab.
    cam0->setWindowFlags(Qt::Widget);
    tabs->addTab(cam0, QCoreApplication::translate("CameraInstanceManager", "Camera 0"));

    if (cam1) {
        cam1->setWindowFlags(Qt::Widget);
        tabs->addTab(cam1, QCoreApplication::translate("CameraInstanceManager", "Camera 1"));
    }

    // --- Shared Log window (standalone, toggleable via View menu) ---
    auto *logWindow = new LogWindow(outerWindow);
    // Route all log output from both instances to the shared widget
    cam0->setSharedLogWidget(logWindow->logWidget());
    DebugLogger::setLogWidget(logWindow->logWidget());
    if (cam1) {
        cam1->setSharedLogWidget(logWindow->logWidget());
    }

    outerWindow->setCentralWidget(tabs);

    // --- Shared MenuBar als Corner-Widget rechts neben den Tabs ---
    auto *sharedMenuBar = new QMenuBar();
    sharedMenuBar->setNativeMenuBar(false);

    // Helper lambda: returns the active MainWindow instance
    auto activeCamera = [tabs]() -> MainWindow* {
        return qobject_cast<MainWindow*>(tabs->currentWidget());
    };

    // --- File menu (first menu of the shared menu bar) ---
    // Save/Load Config moved here from the former bottom button row of the
    // camera window to save one full row of vertical space.
    QMenu *fileMenu = sharedMenuBar->addMenu(QCoreApplication::translate("MainWindow", "&File"));
    QAction *saveConfigAction = fileMenu->addAction(QCoreApplication::translate("MainWindow", "Save Config"));
    QAction *loadConfigAction = fileMenu->addAction(QCoreApplication::translate("MainWindow", "Load Config"));
    QObject::connect(saveConfigAction, &QAction::triggered, [activeCamera]() {
        MainWindow *cam = activeCamera();
        if (!cam) return;
        QString filePath = QFileDialog::getSaveFileName(
            cam,
            QCoreApplication::translate("MainWindow", "Save Configuration"),
            cam->rpicamConfigFilePath(),
            QCoreApplication::translate("MainWindow", "Config Files (*.txt);;All Files (*.*)")
        );
        if (!filePath.isEmpty()) {
            if (!filePath.endsWith(".txt", Qt::CaseInsensitive)) {
                filePath += ".txt";
            }
            cam->saveConfigurationToFile(filePath);
        }
    });
    QObject::connect(loadConfigAction, &QAction::triggered, [activeCamera]() {
        MainWindow *cam = activeCamera();
        if (!cam) return;
        QString filePath = QFileDialog::getOpenFileName(
            cam,
            QCoreApplication::translate("MainWindow", "Load Configuration"),
            cam->rpicamConfigFilePath(),
            QCoreApplication::translate("MainWindow", "Config Files (*.txt);;All Files (*.*)")
        );
        if (!filePath.isEmpty()) {
            cam->loadConfigurationFromFile(filePath);
        }
    });

    // --- Profiles menu: manage and quick-activate camera settings profiles ---
    QMenu *profilesMenu = sharedMenuBar->addMenu(QCoreApplication::translate("MainWindow", "&Profiles"));

    // Helper: list of existing camera instances
    auto camWindows = [cam0, cam1]() -> QList<MainWindow*> {
        QList<MainWindow*> windows;
        if (cam0) windows << cam0;
        if (cam1) windows << cam1;
        return windows;
    };

    QAction *manageProfilesAction = profilesMenu->addAction(QCoreApplication::translate("MainWindow", "Manage Profiles…"));
    QObject::connect(manageProfilesAction, &QAction::triggered, [outerWindow, camWindows]() {
        ProfileDialog dlg(outerWindow, camWindows());
        dlg.exec();
    });

    // Shortcut: open the manager directly in "new profile" state
    QAction *newProfileAction = profilesMenu->addAction(QCoreApplication::translate("MainWindow", "New Profile…"));
    QObject::connect(newProfileAction, &QAction::triggered, [outerWindow, camWindows]() {
        ProfileDialog dlg(outerWindow, camWindows());
        dlg.startNewProfile();
        dlg.exec();
    });

    // Update the currently active profile with the current widget values.
    // Only cameras that are part of the profile scope are overwritten;
    // name and comment stay untouched.
    QAction *updateProfileAction = profilesMenu->addAction(QCoreApplication::translate("MainWindow", "Update Profile"));
    QObject::connect(updateProfileAction, &QAction::triggered, [camWindows, activeCamera]() {
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        const QString active = s.value("Profiles/Active").toString();
        if (active.isEmpty()) return;

        for (MainWindow *w : camWindows()) {
            if (!w) continue;
            if (ProfileDialog::groupHasKeys(QString("Profiles/%1/Cam%2").arg(active).arg(w->cameraIndex()))) {
                w->saveProfileSnapshot(active);
            }
        }
        if (auto *w = activeCamera()) {
            w->appendLog(QCoreApplication::translate("MainWindow", "Profile '%1' updated.").arg(active));
        }
    });

    profilesMenu->addSeparator();

    // Dynamic quick-select entries, rebuilt every time the menu opens
    QList<QAction*> dynamicProfileActions;
    QObject::connect(profilesMenu, &QMenu::aboutToShow, [profilesMenu, updateProfileAction, &dynamicProfileActions, camWindows]() {
        for (QAction *a : dynamicProfileActions) {
            profilesMenu->removeAction(a);
            a->deleteLater();
        }
        dynamicProfileActions.clear();

        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        const QString active = s.value("Profiles/Active").toString();
        const QStringList names = ProfileDialog::profileNames();

        updateProfileAction->setEnabled(!active.isEmpty());

        // Menu shows only the real profiles. Clicking an inactive profile
        // activates it; clicking the active profile deactivates it (no active
        // profile means None -> Set Defaults apply immediately).
        // The active profile gets a blue frame badge, same visual language
        // as the rpicam-apps version label in System Info, sized to the text.
        // Marker rows: only detected cameras get a marker. A single-camera
        // session shows just the "0" circle; a dual-camera session shows
        // "0" and "1". Dark = camera in scope, light grey = not in scope.
        const bool hasCam1Window = [&camWindows]() {
            for (MainWindow *w : camWindows()) {
                if (w && w->cameraIndex() == 1) return true;
            }
            return false;
        }();

        for (const QString &name : names) {
            const bool isActive = (name == active);
            const bool hasCam0 = ProfileDialog::groupHasKeys(
                QString("Profiles/%1/Cam0").arg(name));
            const bool hasCam1 = ProfileDialog::groupHasKeys(
                QString("Profiles/%1/Cam1").arg(name));

            // Row: reserved marker column + profile name (bold when active)
            // on the left, camera markers right-aligned. Fixed row height +
            // vertical centering keeps all rows aligned.
            // Active style: filled triangle marker in project blue, whole
            // row tinted, NO border (the 1px-border alignment problem is
            // gone entirely). Inactive rows: empty marker column, neutral.
            auto *row = new QFrame;
            row->setCursor(Qt::PointingHandCursor);
            row->setFixedHeight(28);
            auto *lay = new QHBoxLayout(row);
            lay->setContentsMargins(6, 0, 10, 0);
            lay->setSpacing(4);

            // Reserved marker column: fixed width on EVERY row so names and
            // circles stay column-aligned between active/inactive entries.
            auto *markerLbl = new QLabel;
            markerLbl->setFixedWidth(18);
            markerLbl->setAlignment(Qt::AlignCenter);
            if (isActive) {
                // Filled triangle, project blue (#3498db, same as group boxes)
                markerLbl->setText(QString::fromUtf8("\u25B6"));
                QFont mf = markerLbl->font();
                mf.setPixelSize(11);
                markerLbl->setFont(mf);
                markerLbl->setStyleSheet(QStringLiteral("color: #3498db;"));
            }
            markerLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

            auto *nameLbl = new QLabel(name);
            if (isActive) {
                QFont f = nameLbl->font();
                f.setBold(true);
                nameLbl->setFont(f);
            }
            nameLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

            auto makeCamMarker = [](const QString &digit, bool inScope) {
                const QColor col = inScope ? QColor("#333333")
                                           : QColor("#c8c8c8");
                auto *lbl = new QLabel;
                lbl->setPixmap(makeCamMarkerIcon(digit, col).pixmap(20, 20));
                lbl->setFixedSize(20, 20);
                lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
                return lbl;
            };
            auto *cam0Lbl = makeCamMarker(QStringLiteral("0"), hasCam0);

            lay->addWidget(markerLbl, 0, Qt::AlignVCenter);
            lay->addWidget(nameLbl, 0, Qt::AlignVCenter);
            // Stretch BEFORE the markers pushes them to the right edge
            lay->addStretch();
            lay->addWidget(cam0Lbl, 0, Qt::AlignVCenter);
            if (hasCam1Window) {
                auto *cam1Lbl = makeCamMarker(QStringLiteral("1"), hasCam1);
                lay->addWidget(cam1Lbl, 0, Qt::AlignVCenter);
            }

            row->setObjectName(QStringLiteral("profileRow"));
            // NOTE: selector must use the objectName — a plain "QFrame"
            // selector would also match the child QLabels (QLabel inherits
            // from QFrame), giving every element a frame.
            // The :hover rules give the rows the usual menu hover reaction —
            // a stylesheet with :hover also enables hover events for the
            // widget automatically.
            row->setStyleSheet(isActive
                ? "QFrame#profileRow { background-color: #e7f3fe; }"
                  "QFrame#profileRow:hover { background-color: #d6e9fc; }"
                : "QFrame#profileRow { background: transparent; }"
                  "QFrame#profileRow:hover { background-color: #e8f1fc; }");

            auto *click = new ProfileRowClickFilter(row,
                [name, isActive, camWindows, profilesMenu]() {
                    if (isActive) {
                        // Toggle off: no active profile -> None (Set Defaults)
                        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
                        s.remove("Profiles/Active");
                        for (MainWindow *w : camWindows()) {
                            if (w) w->loadStartupDefaults();
                        }
                    } else {
                        ProfileDialog::activateProfile(name, camWindows());
                    }
                    profilesMenu->hide();
                });
            row->installEventFilter(click);

            auto *widgetAction = new QWidgetAction(profilesMenu);
            widgetAction->setDefaultWidget(row);
            profilesMenu->addAction(widgetAction);
            dynamicProfileActions << widgetAction;
        }
    });

    QMenu *viewMenu = sharedMenuBar->addMenu(QCoreApplication::translate("MainWindow", "&View"));
    // No setShortcut() here — Qt's shortcut column creates a big gap.
    // Shortcut hint is embedded in the translated text instead.
    // A QShortcut on outerWindow handles the actual key binding.
    QAction *toggleAllAction = viewMenu->addAction(QCoreApplication::translate("MainWindow", "Toggle Groups  Ctrl+0"));
    QObject::connect(toggleAllAction, &QAction::triggered, [tabs, activeCamera]() {
        MainWindow *active = activeCamera();
        if (!active) return;
        int activeIndex = tabs->currentIndex();
        bool collapse = !active->firstGroupCollapsed();
        // Activate each tab individually and call toggleAllGroups(),
        // exactly as with the manual workaround:
        // switch tab -> Ctrl+0 -> next tab -> Ctrl+0
        for (int i = 0; i < tabs->count(); ++i) {
            if (i != activeIndex) {
                tabs->setCurrentIndex(i);
                MainWindow *w = qobject_cast<MainWindow*>(tabs->widget(i));
                if (w) w->toggleAllGroups();
            }
        }
        // Return to originally active tab and toggle there
        tabs->setCurrentIndex(activeIndex);
        active->toggleAllGroups();
    });

    viewMenu->addSeparator();
    QAction *logAction = viewMenu->addAction(QCoreApplication::translate("MainWindow", "&Log"));
    logAction->setCheckable(true);
    logAction->setChecked(false);
    QObject::connect(logAction, &QAction::toggled, logWindow, [logWindow](bool checked) {
        if (checked) {
            logWindow->show();
        } else {
            logWindow->hide();
        }
    });
    // Sync menu action when window is closed via X button
    QObject::connect(logWindow, &LogWindow::visibilityChanged, logAction, &QAction::setChecked);

    QMenu *setupMenu = sharedMenuBar->addMenu(QCoreApplication::translate("MainWindow", "&Setup"));
    QAction *globalSetupAction = setupMenu->addAction(QCoreApplication::translate("MainWindow", "Global Settings…"));
    QObject::connect(globalSetupAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->openGlobalSetupDialog();
    });
    QAction *cameraSetupAction = setupMenu->addAction(QCoreApplication::translate("MainWindow", "Camera Setup…"));
    QObject::connect(cameraSetupAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->openGuiSetupDialog();
    });
    QAction *setDefaultsAction = setupMenu->addAction(QCoreApplication::translate("MainWindow", "Set Defaults"));
    QObject::connect(setDefaultsAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->saveStartupDefaults();
    });
    QAction *resetDefaultsAction = setupMenu->addAction(QCoreApplication::translate("MainWindow", "Reset Defaults"));
    QObject::connect(resetDefaultsAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->resetStartupDefaults();
    });

    QMenu *helpMenu = sharedMenuBar->addMenu(QCoreApplication::translate("MainWindow", "&Help"));
    QAction *helpAction = helpMenu->addAction(QCoreApplication::translate("MainWindow", "&Help"));
    QObject::connect(helpAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->showHelp();
    });
    QAction *updateAction = helpMenu->addAction(QCoreApplication::translate("MainWindow", "Check for &Updates..."));
    QObject::connect(updateAction, &QAction::triggered, [activeCamera]() {
        UpdateDialog dlg(activeCamera());
        dlg.exec();
    });
    helpMenu->addSeparator();
    QAction *supportAction = helpMenu->addAction(QCoreApplication::translate("MainWindow", "&Support"));
    QObject::connect(supportAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->showSupportDialog();
    });
    QAction *sysInfoAction = helpMenu->addAction(QCoreApplication::translate("MainWindow", "System &Information"));
    QObject::connect(sysInfoAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->showSystemInfo();
    });
    QAction *aboutAction = helpMenu->addAction(QCoreApplication::translate("MainWindow", "&About"));
    QObject::connect(aboutAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->showAboutDialog();
    });
    helpMenu->addSeparator();
    QAction *donateAction = helpMenu->addAction(QCoreApplication::translate("MainWindow", "&Donate"));
    QObject::connect(donateAction, &QAction::triggered, [activeCamera]() {
        if (auto *w = activeCamera()) w->showAboutDialog(4);
    });

    // --- Update bell (right of the Help menu) ---
    // Signals available updates in the default view: the bell appears only
    // when an update was found; the tooltip names the version and a click
    // opens the update dialog. Painter-drawn icon (no emoji), orange like
    // the app's other warning accents.
    auto makeBellIcon = [](const QColor &color) {
        QPixmap pm(24, 24);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(color, 1.6));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(5.0, 3.0, 14.0, 14.0), 0, 180 * 16); // dome
        p.drawLine(QPointF(4.0, 10.0), QPointF(20.0, 10.0));  // base
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(12.0, 15.0), 2.2, 2.2);         // clapper
        p.end();
        return QIcon(pm);
    };

    auto *updateBellAction = new QAction(
        makeBellIcon(QColor(QStringLiteral("#e67700"))), QString(), sharedMenuBar);
    updateBellAction->setVisible(false);
    // QMenuBar hides the QWidget addAction overloads — go through QWidget
    // explicitly to add a plain icon action right of the Help menu.
    static_cast<QWidget*>(sharedMenuBar)->addAction(updateBellAction);
    QObject::connect(updateBellAction, &QAction::triggered, [activeCamera]() {
        UpdateDialog dlg(activeCamera());
        dlg.exec();
    });


    // Prevent the corner widget menu bar from forcing the window wider.
    // minimumSizeHint() of QMenuBar drives the outer QTabWidget's minimum width;
    // setMinimumWidth(0) removes that constraint while setSizePolicy lets it shrink.
    sharedMenuBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    sharedMenuBar->setMinimumWidth(0);

    tabs->setCornerWidget(sharedMenuBar, Qt::TopRightCorner);
    tabs->tabBar()->setExpanding(false);
    tabs->tabBar()->setUsesScrollButtons(false);

    // Ctrl+0 shortcut: toggle all collapsible groups in the active camera tab
    auto *toggleShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_0), outerWindow);
    QObject::connect(toggleShortcut, &QShortcut::activated, toggleAllAction, &QAction::trigger);

    // Eigene MenuBars der MainWindow-Instanzen komplett ersetzen
    // hide() / setFixedHeight(0) reichen nicht – QMainWindow reserviert
    // intern trotzdem Layout-Platz. setMenuBar() mit leerer Bar löst das.
    // Replace each camera window's own menu bar with an empty one
    // to prevent layout space being reserved for it.
    for (auto *cam : QList<MainWindow*>{cam0, cam1}) {
        if (!cam) continue;
        auto *emptyBar = new QMenuBar();
        emptyBar->setFixedHeight(0);
        emptyBar->setNativeMenuBar(false);
        cam->setMenuBar(emptyBar);
    }

    // Restore the saved window size
    QSettings geomSettings(AppPaths::globalConf(), QSettings::IniFormat);
    QByteArray savedGeometry = geomSettings.value("Geometry/MainWindow").toByteArray();
    if (!savedGeometry.isEmpty()) {
        outerWindow->restoreGeometry(savedGeometry);
    }

    // Save the outer window geometry on exit
    QObject::connect(&app, &QApplication::aboutToQuit, outerWindow, [outerWindow]() {
        QSettings s(AppPaths::globalConf(), QSettings::IniFormat);
        s.setValue("Geometry/MainWindow", outerWindow->saveGeometry());
    });

    // Auto-adjust height when switching between camera tabs
    QObject::connect(tabs, &QTabWidget::currentChanged, [activeCamera]() {
        if (auto *cam = activeCamera()) {
            cam->adjustWindowToOptimalSize();
        }
    });

    // Show splash screen for the configured duration with fade in/out.
    // The fade animates the graphics effect's opacity: content-level fading
    // that works under X11 with the hard pixmap mask and without any
    // window-level transparency support.
    auto showAndResize = [outerWindow, activeCamera]() {
        outerWindow->show();
        if (auto *cam = activeCamera()) {
            cam->adjustWindowToOptimalSize();
        }
    };
    if (splashScreenEnabled) {
        // Split the configured duration into fade-in, hold and fade-out.
        // Fades are capped at 300 ms each (25% at the default 1000 ms) so
        // the effect stays quick; short durations scale down proportionally.
        const int fadeCap = 300;
        const int fadeInMs = qMin(fadeCap, splashDuration / 4);
        const int fadeOutMs = qMin(fadeCap, splashDuration / 4);
        const int holdMs = qMax(0, splashDuration - fadeInMs - fadeOutMs);

        auto *fadeIn = new QPropertyAnimation(splashFadeEffect, "opacity", &splash);
        fadeIn->setDuration(fadeInMs);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);

        auto *fadeOut = new QPropertyAnimation(splashFadeEffect, "opacity", &splash);
        fadeOut->setDuration(fadeOutMs);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);

        // Delayed fade-in of the version label: starts after the logo's
        // fade-in duration, uses the same duration.
        auto *versionFadeIn = new QPropertyAnimation(versionFadeEffect, "opacity", &splash);
        versionFadeIn->setDuration(qMax(150, fadeInMs));
        versionFadeIn->setStartValue(0.0);
        versionFadeIn->setEndValue(1.0);

        // Synchronous fade-out of the version label. No fixed start value:
        // the animation continues from the current opacity so there is no
        // visual jump even if the fade-in is still running.
        auto *versionFadeOut = new QPropertyAnimation(versionFadeEffect, "opacity", &splash);
        versionFadeOut->setDuration(fadeOutMs);
        versionFadeOut->setEndValue(0.0);

        QObject::connect(fadeIn, &QPropertyAnimation::finished, &splash,
                         [holdMs, fadeOut, versionFadeIn, versionFadeOut]() {
            QTimer::singleShot(holdMs, fadeOut, [fadeOut, versionFadeIn, versionFadeOut]() {
                if (versionFadeIn) versionFadeIn->stop();
                fadeOut->start();
                if (versionFadeOut) versionFadeOut->start();
            });
        });
        QObject::connect(fadeOut, &QPropertyAnimation::finished, &splash,
                         [&splash, showAndResize]() {
            splash.close();
            showAndResize();
        });

        fadeIn->start();
        if (versionFadeIn) {
            QTimer::singleShot(fadeInMs, &splash, [versionFadeIn]() { versionFadeIn->start(); });
        }
    } else {
        showAndResize();
    }

    // One-time notice: native Wayland (labwc) sessions have limited
    // functionality (no global mouse grab for ROI selection, limited window
    // placement). Remote X11 sessions (XRDP) are unaffected and skipped.
    // Shown once – the acknowledgement is stored in the global config.
    {
        QTimer::singleShot(1200, outerWindow, [outerWindow]() {
            auto waylandActive = []() {
                if (qgetenv("XDG_SESSION_TYPE") == "wayland") return true;
                if (!qgetenv("WAYLAND_DISPLAY").isEmpty()) return true;
                return false;
            };
            auto remoteActive = []() {
                if (!qgetenv("XRDP_SESSION").isEmpty()) return true;
                QString sessionId = qgetenv("XDG_SESSION_ID");
                if (!sessionId.isEmpty()) {
                    QProcess proc;
                    proc.start("loginctl", {"show-session", sessionId, "-p", "Remote"});
                    if (proc.waitForFinished(2000) && proc.exitCode() == 0 &&
                        proc.readAllStandardOutput().contains("Remote=yes")) {
                        return true;
                    }
                }
                return false;
            };

            if (!waylandActive() || remoteActive()) return;

            QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
            settings.beginGroup("General");
            bool acknowledged = settings.value("WaylandNoticeAcknowledged", false).toBool();
            settings.endGroup();
            if (acknowledged) return;

            QMessageBox notice(outerWindow);
            notice.setIcon(QMessageBox::Information);
            notice.setWindowTitle(QCoreApplication::translate("MainWindow", "Wayland Notice"));
            notice.setText(QCoreApplication::translate("MainWindow",
                "<b>Wayland session detected</b><br><br>"
                "%1 is running on a native Wayland session (e.g. labwc). "
                "Some features have limited functionality on Wayland:<br>"
                "• ROI selection (no global mouse capture)<br>"
                "• precise window placement<br><br>"
                "For full functionality, use an X11 session. "
                "This notice will not be shown again.").arg(QLatin1String(AppMeta::NAME)));
            notice.setStandardButtons(QMessageBox::Ok);
            notice.exec();

            QSettings save(AppPaths::globalConf(), QSettings::IniFormat);
            // Same group as the read above — a flat "General/..." key would
            // land in the different [General] INI section.
            save.beginGroup("General");
            save.setValue("WaylandNoticeAcknowledged", true);
            save.endGroup();
        });
    }

    // Background update check on startup — if a new version is found,
    // mark the "Check for Updates…" menu entry visually.
    // Controlled by Global Settings → "Check for updates on startup".
    {
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup("General");
        bool autoCheck = settings.value("Updates/AutoCheck", true).toBool();
        settings.endGroup();
        if (autoCheck) {
            QTimer::singleShot(5000, updateAction, [updateAction, updateBellAction]() {
                auto *checker = new UpdateChecker(updateAction); // parented to action
                QObject::connect(checker, &UpdateChecker::updateAvailable,
                                 updateAction, [updateAction, updateBellAction](const QString &latestVersion,
                                                              const QString &, const QString &,
                                                              const QString &, const QString &) {
                    updateAction->setText(
                        QCoreApplication::translate("MainWindow",
                            "Check for &Updates...  [v%1 available!]").arg(latestVersion));
                    updateAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
                    // Bell in the menu bar: visible only when an update exists
                    updateBellAction->setVisible(true);
                    updateBellAction->setToolTip(
                        QCoreApplication::translate("MainWindow",
                            "Update available: v%1 — click to open the update dialog")
                                .arg(latestVersion));
                });
                QObject::connect(checker, &UpdateChecker::errorOccurred,
                                 checker, &QObject::deleteLater);
                QObject::connect(checker, &UpdateChecker::upToDate,
                                 checker, &QObject::deleteLater);
                checker->checkForUpdates();
            });
        } else {
            qDebug() << "[UpdateChecker] Auto-check disabled in settings, skipping startup check";
        }
    }

    return app.exec();
}
