// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
//
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// AppMeta.h - Single source of truth for the product name and all
// technical identifiers derived from it.
//
// Every code path that needs the product name, the binary/package name,
// config paths, icon names, the translation base name or the GitHub
// repository MUST read it from here - never hardcode it.
//
// Keep in sync with:
//   - CMakeLists.txt                   (APP_NAME / APP_BINARY / APP_PACKAGE / APP_REPO_*)
//   - resources/scripts/install.sh     (APP_NAME / APP_BINARY at the top)
//   - BACKUP/backup.sh                 (APP_BINARY at the top)
//   - resources/resources.qrc          (aliases must match the real file names)
//
// NOTE for translators: tr() calls must use literal strings with a %1
// placeholder for the product name, e.g. tr("%1 Help").arg(AppMeta::NAME).
// lupdate only extracts literal strings - it never sees constants, so a
// future rename requires changing this header only, not the .ts files.

#ifndef APPMETA_H
#define APPMETA_H

#include <QString>

namespace AppMeta {

    // Display name (window titles, About dialog, notices).
    inline constexpr const char *NAME = "piStudio";

    // Binary / package name. Used for the executable name, the .deb
    // package name, the desktop entry and the update-check asset filter.
    inline constexpr const char *BINARY = "piStudio";

    // Config: directory and file name below $HOME (e.g. ~/.config/piStudio/).
    inline constexpr const char *CONFIG_DIR  = ".config/piStudio";
    inline constexpr const char *CONFIG_FILE = "piStudio.conf";

    // Default video output directory below $HOME (system installs only).
    inline constexpr const char *OUTPUT_DIR = "Videos/piStudio";

    // Icon theme name and embedded resource aliases.
    inline constexpr const char *ICON_THEME    = "piStudio";
    inline constexpr const char *ICON_RESOURCE = ":/piStudio.svg";
    inline constexpr const char *LOGO_RESOURCE = ":/piStudio_text.svg";

    // Pre-rendered raster logo for the splash screen (native display size).
    // White text variant - readable on dark desktop backgrounds. The splash
    // uses a 1-bit window mask (QPixmap::mask()), which works best with a
    // raster image whose alpha channel is preserved 1:1 - the same
    // architecture as the previous embedded-PNG logo. The vector SVG is
    // anti-aliased by Qt and would produce frayed mask edges.
    inline constexpr const char *SPLASH_RESOURCE = ":/piStudio_splash_wtext.png";

    // Translation base name ("<base>_de", "<base>_en") and the installed
    // share directory that contains the .qm files.
    inline constexpr const char *I18N_BASENAME = "piStudio";
    inline constexpr const char *SHARE_DIR     = "/usr/share/piStudio";

    // GitHub repository and update-check metadata.
    inline constexpr const char *REPO_OWNER = "Kletternaut";
    inline constexpr const char *REPO_NAME  = "piStudio";
    inline constexpr const char *UPDATE_UA  = "piStudio-update-checker/1.0";

    // Full GitHub repository URL (https://github.com/<owner>/<repo>).
    inline QString repoUrl() {
        return QStringLiteral("https://github.com/%1/%2")
            .arg(QLatin1String(REPO_OWNER), QLatin1String(REPO_NAME));
    }

    // GitHub releases API URL; pass true for the /releases/latest endpoint.
    inline QString releasesApiUrl(bool latest) {
        return QStringLiteral("https://api.github.com/repos/%1/%2/releases%3")
            .arg(QLatin1String(REPO_OWNER), QLatin1String(REPO_NAME),
                 latest ? QLatin1String("/latest") : QLatin1String(""));
    }

} // namespace AppMeta

#endif // APPMETA_H
