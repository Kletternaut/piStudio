# Scripts Directory

This directory contains build/install helpers and diagnostics scripts for piStudio.

## Files

### Build & Install
- **`install.sh`** - Build and install piStudio (`./resources/scripts/install.sh`):
  - default: build + `make install` to `/usr/local` (override with `--prefix <dir>`)
  - `--deb`: build + create the `.deb` package via CPack
  - `--uninstall`: remove installed binaries, icons, desktop entry and translations

### Code Quality
- **`lint.sh`** - Static analysis: clang-tidy (required), clang-format check/fix, cppcheck (optional if installed)

### Diagnostics
- **`detect-af.sh`** - Map camera ↔ I2C bus ↔ autofocus V4L2 subdevice (`cam -l`, `v4l2-ctl`)
- **`system-info.sh`** - Collect system info (display server, desktop environment, xRDP/VNC, sessions) into a log file

### Templates
- **`piStudio.desktop.template`** - Desktop entry template; CMake installs it to
  `/usr/share/applications/piStudio.desktop` (Name/Exec/Icon already point to piStudio)
- **`piStudio.conf.template`** - Minimal config template (`[General]` with `rpicam-focus`,
  empty `[Paths]`). The app creates its real config on first start.

## Notes

- The former desktop-integration scripts (`setup-desktop-integration.sh`,
  `remove-desktop-integration.sh`, `icon_installer.sh`, `icon_uninstaller.sh`) were removed
  with the 0.6.0 rebrand: icons and the desktop entry are installed by CMake/CPack, and the
  DEB postinst refreshes icon cache and desktop database automatically.
- Icons live in `resources/icons/` (PNG, 16–512 px) and `resources/images/piStudio.svg`
  (scalable).
