#pragma once

#include <QCoreApplication>
#include <QString>
#include <QDir>
#include <QFile>
#include "../app/AppMeta.h"

// AppPaths: Central helper for runtime-relative file paths.
// All paths are resolved relative to the application binary directory.
// Binary is in build/, project root is one level up (build/../).
// Use these functions instead of any hardcoded absolute paths.

namespace AppPaths {

    // Project root directory (e.g. /home/user/rpicam-ctrl/) — the local
    // checkout folder name is independent of the product name in AppMeta.
    inline QString base() {
        return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../") + "/";
    }

    // Detect system-wide installation (binary in /usr/... or /opt/...).
    // When installed system-wide, the binary directory is not user-writable,
    // so writable paths (config, output, contentOutput) must redirect to
    // user-owned XDG directories.
    inline bool isSystemInstall() {
        QString appDir = QCoreApplication::applicationDirPath();
        return appDir.startsWith("/usr") || appDir.startsWith("/opt");
    }

    // Global config (Language, Window layout, Tab-visibility, all tab settings)
    // Structure: [General] for global keys, [Camera0-Tab] / [Camera1-Tab] for per-tab keys
    //
    // Single source of truth: always the user XDG config dir, independent of
    // dev build or system install, so every component (including the
    // collapsible-group state) reads and writes the same file.
    // The directory and file name come from AppMeta (product-name central).
    inline QString globalConf() {
        QDir().mkpath(QDir::homePath() + "/" + AppMeta::CONFIG_DIR);
        return QDir::homePath() + "/" + AppMeta::CONFIG_DIR + "/" + AppMeta::CONFIG_FILE;
    }

    // QSettings group name for a camera tab instance (e.g. "Camera0-Tab")
    // Use with settings.beginGroup(tabGroup(idx)) / settings.endGroup()
    inline QString tabGroup(int cameraIndex) {
        return QString("Camera%1-Tab").arg(cameraIndex);
    }

    // Resource subdirectories.
    // Writable paths (config, output, contentOutput) redirect to XDG user
    // directories on system installs so the user has write permission.
    // Read-only paths (resources, i18n) always resolve relative to base().
    inline QString resources()     { return base() + "resources/"; }

    inline QString config() {
        if (isSystemInstall()) {
            QString path = QDir::homePath() + "/" + AppMeta::CONFIG_DIR + "/config/";
            QDir().mkpath(path);
            return path;
        }
        return base() + "config/";
    }

    inline QString output() {
        if (isSystemInstall()) {
            QString path = QDir::homePath() + "/" + AppMeta::OUTPUT_DIR + "/";
            QDir().mkpath(path);
            return path;
        }
        return base() + "output/";
    }

    inline QString contentOutput() {
        if (isSystemInstall()) {
            QString path = QDir::homePath() + "/" + AppMeta::OUTPUT_DIR + "/";
            QDir().mkpath(path);
            return path;
        }
        return base() + "content_output/";
    }

    inline QString i18n()          { return base() + "src/i18n/"; }

    // Sanitize a path loaded from QSettings: if it points to a system directory
    // (/usr/ or /opt/) that normal users cannot write to, return the fallback.
    // This handles upgrades from older versions that stored non-writable defaults.
    inline QString sanitizeIfSystemPath(const QString &path, const QString &fallback) {
        if (path.startsWith("/usr/") || path.startsWith("/opt/")) {
            return fallback;
        }
        return path;
    }

    // Default tuning file directory based on hardware platform.
    // Raspberry Pi 5 (BCM2712) uses PiSP ISP → /usr/share/libcamera/ipa/rpi/pisp
    // Earlier Pis (BCM2711, BCM2710, BCM2708) use vc4 ISP → /usr/share/libcamera/ipa/rpi/vc4
    inline QString tuningFileBasePath() {
        QFile modelFile("/proc/device-tree/model");
        if (modelFile.open(QIODevice::ReadOnly)) {
            QString model = QString::fromUtf8(modelFile.readAll());
            modelFile.close();
            if (model.contains("Raspberry Pi 5"))
                return QStringLiteral("/usr/share/libcamera/ipa/rpi/pisp");
        }
        return QStringLiteral("/usr/share/libcamera/ipa/rpi/vc4");
    }

} // namespace AppPaths
