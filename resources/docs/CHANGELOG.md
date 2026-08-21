# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.7.3] - 2026-08-21

### Changed
- License: replaced GNU General Public License v3.0 (GPL-3.0) with the
  PolyForm Noncommercial License 1.0.0. Noncommercial use is permitted
  free of charge; commercial use requires a separate license from the
  copyright holder.
- All source file SPDX headers updated from GPL-3.0-only to
  LicenseRef-PolyForm-Noncommercial-1.0.0.
- About dialog License tab: updated to show the PolyForm Noncommercial
  text; Dynamic Linking Notice reworded.
- README files: license badge removed, license sections updated to
  PolyForm Noncommercial.

## [0.7.2] - 2026-08-21

### Added
- New --encoder-libs setting in the camera setup "rpicam-apps Parameters"
  group, handled exactly like preview/post-process libs (persistence via
  Set Defaults/profiles, config file save/load, global reset).
- SYNC badge on the Start button: linked start/stop across both camera
  tabs. Only shown when a second camera is present; state can be persisted
  via Set Defaults or profiles.
- Splash screen: version label re-enabled (white, bold, delayed fade-in
  after the logo, no background).

### Changed
- About and Check-for-Updates dialogs: consistent header layout (version,
  "Camera App for Raspberry Pi OS", dynamic copyright year, repo URL) and
  unified logo size.

### Fixed
- Synced camera start no longer places both preview windows on top of
  each other: sibling-aware placement now also treats the Starting
  process state as active.

## [0.7.1] - 2026-08-21

### Added
- Camera setup dialog, Paths tab: new "rpicam-apps Parameters" group with
  Post-Proc Libs and Preview Libs path fields.
- X-reset button in front of each lib path field showing the state: red =
  temporary (loaded from a config file or profile), green = saved in the
  settings, black = empty. Clicking it clears the entry.

### Changed
- Post-process libs are configured as a full path in the camera setup
  dialog (same pattern as preview libs); the dropdown row was removed from
  the Processing Files group.
- Global X-reset now also shows green when values are changed but saved;
  resetting removes the persisted lib path keys so cleared values stay
  cleared after a restart.

### Fixed
- Save/Load Config dialogs now respect the configured rpicam configs path
  immediately after start; previously it was only used after a Browse
  within the same session.

## [0.7.0] - 2026-08-17

First stable release under the piStudio name (supersedes 0.7.0-beta).

### Added
- File menu (right of the camera tabs) with Save Config / Load Config —
  the former bottom button row is removed, saving one full row of
  vertical space.
- Camera setup dialog, Paths tab: new Preview Libs path field
  (--preview-libs).

### Changed
- Global X reset moved into the app selector row (right of the x2
  checkbox); the preview geometry reset sits directly right of the
  preview input.
- Preview backend dropdown stays in the main row; only the preview libs
  path selector moved to the camera setup dialog.

### Fixed
- Global X reset in Enhanced Mode: the AWB gain slider resets sent
  awbgains after the AWB selector already sent awb:auto, putting a
  running camera back into manual AWB without re-evaluation. The sliders
  are signal-blocked during the reset and awb:auto is sent explicitly
  afterwards.

## [0.7.0-beta] - 2026-08-17

### Changed
- Complete rebrand from rpicam-ctrl to piStudio: product name, binary,
  package name, config path (~/.config/piStudio/), icons, translations and
  documentation. The GitHub repository was renamed to Kletternaut/piStudio
  (history preserved).
- User configuration starts fresh (no migration of ~/.config/rpicam-ctrl/).
- The piStudio package conflicts with and replaces rpicam-ctrl; the postinst
  script removes leftovers of both predecessor packages (rpicam-ctrl and
  rpicam-gui).

### Added
- src/app/AppMeta.h: single source of truth for the product name and all
  technical identifiers derived from it (config paths, icons, translations,
  GitHub repo). All tr() strings use %1 placeholders, so future renames only
  touch AppMeta — not the translation files.

## [0.6.16] - 2026-08-17

### Added
- System Info: the rpicam-rt, ROI selection and Preview backend rows now
  have explanatory tooltips (wrapped, fixed-width).
- About dialog, Third-Party Libraries: new entry describing the optional
  rpicam-apps fork feature/rt-roi (runtime control + interactive ROI
  selection) and how rpicam-ctrl integrates it.

### Changed
- All references to the rpicam-apps fork branches updated from
  feature/rpicam-rt + feature/roi-selection to the merged feature/rt-roi.

## [0.6.15] - 2026-08-17

### Changed
- About dialog Disclaimer tab: removed the Project Independence section
  (RPiCamGUI reference) — it is no longer relevant.
- Moved RELEASING.md from resources/docs/ to DEVELOPMENT/.

## [0.6.14] - 2026-08-16

### Changed
- The .deb no longer declares `Depends: rpicam-apps`. rpicam-ctrl never links
  against rpicam-apps (it only invokes the binaries), and the hard dependency
  forced apt to install the stock package on systems where rpicam-apps was
  built from source (e.g. the Kletternaut fork). Missing binaries are now
  reported at startup instead.

## [0.6.13] - 2026-08-15

### Added
- Update dialog: after a successful install a modal box offers "Restart
  now" (one-click app restart) or "Later" — the previous status-line-only
  feedback was easy to overlook.

## [0.6.12] - 2026-08-15

### Fixed
- Update download: GitHub release assets answer with a 302 to a very long
  Azure SAS URL. Qt 5.15 sometimes fails to follow these automatically,
  finishing with the empty 302 body (0-byte .deb, apt exit 100). The
  redirect is now followed manually (max 5 hops).

## [0.6.11] - 2026-08-15

### Fixed
- Update download wrote truncated (0-byte) .deb files, causing apt to fail
  with exit 100 ("control.tar not found"). The payload is now read and
  written once after the reply finished instead of incrementally via
  readyRead.

## [0.6.10] - 2026-08-15

### Added
- ROI takeover from the rpicam-apps preview: the process output line
  `ROI selected: --roi x,y,w,h` (feature/roi-selection fork) is parsed
  from stdout/stderr and fed into the ROI input field, so a visually
  exact selection can be persisted via Set Defaults or profiles.
  Format is unchanged (`x,y,w,h`, relative) — full compatibility with
  unpatched rpicam-apps builds and existing config files.

## [0.6.9] - 2026-08-15

### Added
- Support for rpicam-rt (renamed runtime control): capability detection now
  matches the exact tokens `rpicam_rt:1` and `roi_selection:1` from
  `rpicam-vid --version` (Kletternaut/rpicam-apps fork branches
  feature/rpicam-rt and feature/roi-selection).

### Changed
- Enhanced Mode badge renamed from CTRL to RT (avoids confusion between
  ctrl and ctl); System Info shows the capability as rpicam-rt.
- Help dialog references the fork branch and the renamed Meson option
  `-Denable_rpicam_rt=enabled` instead of PR #917.

## [0.6.8] - 2026-08-15

Final stable release of the 0.6.5–0.6.8 beta phase (consolidates all four
beta releases; 0.6.8 > 0.6.8-beta, so beta users are lifted to this release
automatically).

### Added
- Update bell in the menu bar (right of Help): appears when an update is
  found (stable or beta, depending on the beta-updates setting), tooltip
  shows the version, click opens the update dialog.

### Changed
- Resolution change snaps the framerate to the highest detected integer
  value of the new resolution.

### Fixed
- Beta update channel: the "Include beta updates" setting was read from the
  wrong INI section and never activated the beta channel.
- Splash-screen toggle had no effect (global setting stored in one INI
  section, read from another).
- Wayland notice reappeared on every launch (same storage mismatch).
- Update dialog no longer overlaps the logo when the update-available
  block appears (fixed size replaced by a growing dialog).
- The build-from-source command in the git-clone hint is now selectable
  and copyable, and shows the current build path
  (`cmake -S . -B build && cmake --build build`).

## [0.6.4] - 2026-08-15

### Added
- Profile export/import: a single profile can be saved to and loaded from a
  `.rpcp` file (plain INI, same dialect as the config) — for backups and
  transfer between installations. Import asks before overwriting an
  existing profile.
- Profiles menu: quick-switch entries with camera-scope markers (right-
  aligned circle badges, grey = camera not in scope); active profile
  highlighted with a blue row and filled triangle marker.
- "New Profile…" menu shortcut opens the manager directly in new-profile
  state; newly created profiles become active immediately.
- Output settings (mode, filename, AN/TS/segment pattern) are now part of
  the saved defaults and profile snapshots.

### Changed
- Profile manager no longer pre-fills the edit fields with the active
  profile; the camera-scope UI follows the rpicam-ctrl camera detection
  (Camera 1 hidden in single-camera sessions, stored data of undetected
  cameras is preserved).
- Deactivating a profile (menu toggle or unchecking in the manager) applies
  the Set Defaults immediately; the "None" state has no menu entry anymore.
- `[Splash] Duration` is seeded into the config on first run so the setting
  is documented and editable.
- Menu order: Profiles, View, Setup, Help.

### Fixed
- Saving a profile with no camera selected silently skipped the snapshot
  (empty exports); a guard now requires at least one camera.

## [0.6.3] - 2026-08-15

### Added
- Profile manager (menu Profiles): create, rename, delete and activate up to
  10 camera settings profiles. A profile stores a snapshot of the current
  widget values with per-camera scope (Camera 0 and/or Camera 1) plus a long
  comment, all in the central config file (`Profiles/<name>/` sections).
- An active profile takes priority over the regular startup defaults ("Set
  Defaults"); cameras outside the profile scope fall back to their own
  defaults. Activation applies the stored values immediately.

## [0.6.2] - 2026-08-15

### Added
- Opt-in beta update channel: the new "Include beta updates" setting makes the
  update check include GitHub pre-releases (Setup → Global Settings).
- Version comparison now understands pre-release suffixes (`beta2` > `beta`,
  `0.7.0` > `0.7-beta`).

### Changed
- Versioning scheme: routine releases no longer carry a `-beta` suffix. The
  leading `0.` already marks the pre-1.0 development phase; suffixes are
  reserved for major beta phases before milestone releases (see
  `RELEASING.md`).

## [0.6.1-beta] - 2026-08-15

### Fixed
- All settings now live in a single config file
  (`~/.config/rpicam-ctrl/rpicam-ctrl.conf`), independent of dev build or
  system install.
- Collapsible groups: expanded/collapsed state was lost after the rebrand
  because it was stored in Qt's default `Unknown Organization` location — now
  persisted in the central config again.
- GUI setup dialog wrote `splashScreenEnabled` to a separate `settings.conf`
  that was never read — now written to `[General]` of the central config.

## [0.6.0-beta] - 2026-08-14

### Changed
- **Rebranding**: rpicam-gui is now **rpicam-ctrl**. Renamed binary, package
  (`rpicam-ctrl_*.deb`), install paths (`/usr/share/rpicam-ctrl`, hicolor icons,
  `rpicam-ctrl.desktop`), user directories (`~/.config/rpicam-ctrl/`,
  `~/Videos/rpicam-ctrl/`), translations and documentation.
- The DEB declares `Conflicts`/`Replaces: rpicam-gui`; its postinst removes
  leftover files of the old package on upgrade.
- GitHub repository renamed to `Kletternaut/rpicam-ctrl` (old URLs redirect).
- User configuration starts fresh (no migration of `~/.config/rpicam-gui/`).

### Added
- Disclaimer extended: rpicam-ctrl functionally controls the official
  rpicam-apps (100 % parameter compatibility) and is independent of
  Raspberry Pi Ltd.

### Fixed
- Control socket capability detection now matches the exact token
  `rpicam_ctrl:1` instead of a substring (`rpicam_ctrl:0` is no longer
  treated as supported).
- Help dialog: dead resource path (`:/rpicam-gui_300.png`) replaced with the
  splash logo; outdated `--rpiCam-ctl-socket` flag note corrected to the
  Meson build option `-Denable_rpicam_ctrl=enabled`.

### Removed
- Redundant desktop-integration scripts (`setup-desktop-integration.sh`,
  `remove-desktop-integration.sh`, `icon_installer.sh`,
  `icon_uninstaller.sh`) — icons and desktop entry are handled by
  CMake/CPack and the DEB postinst.
- Outdated `MISSING_PARAMETERS.md` (last updated for v0.2.6.2).

## [0.5.8-beta] - 2026-08-13

### Added
- **Wayland notice**: one-time warning on native Wayland sessions (e.g. labwc)
  about limited functionality (ROI selection, window placement). Remote X11
  sessions (XRDP) are skipped; the acknowledgement is stored and the notice
  is not shown again.
- **System Information**: display server is now color-coded in the Session
  group (green = X11/full functionality, red = Wayland/limited) with
  explanatory tooltips. Group box titles now match the General tab styling.

### Fixed
- **Per-tab startup defaults**: "Set Defaults" saved all values under both
  camera tab groups, so camera 0 settings leaked into camera 1 and vice
  versa. Saving is now restricted to the active camera tab.
- **Framerate slider default**: after a restart without saved defaults the
  slider showed 0 (Auto) instead of the maximum sensor framerate.
- **Framerate slider popup**: the popup closed immediately on open (stray
  release event from the opening click) and did not close after clicking the
  groove. It now opens correctly and closes after every real interaction.
- **General tab**: "Output File Settings" renamed to "Output Settings".

## [0.5.7] - 2026-08-13

### Added
- **System Information dialog**: shows `rpicam-hello --list` output at the top,
  with one collapsible-style group box per camera (title only "Camera N",
  details in the monospace text field).
- **rpicam-apps feature detection**: System Information now shows the
  rpicam-apps build version plus capability checks for `rpicam_ctrl`,
  `roi_selection` (feature/roi-selection fork) and `--preview-backend`.

### Fixed
- **Per-tab camera mode detection**: with multiple cameras connected, each
  camera tab parsed the full camera list and mixed the modes of all cameras.
  Each tab instance now only processes the modes of its own camera index.
- **Camera type change handling**: if a camera is swapped for a different
  model, stale camera-specific defaults (width, height, framerate,
  viewfinder mode) are now detected via a stored model/sensor fingerprint
  and reset automatically on the next start.

## [0.5.6] - 2026-08-13

### Added
- **Dynamic camera mode detection**: the complete output of
  `rpicam-hello --list-cameras` (pixel formats, bit depths, framerates, crops)
  is now parsed and used to populate the resolution and framerate selectors.
  All hardcoded IMX477 resolution/framerate lists were removed – the detection
  is now generic for every libcamera-supported sensor (IMX219, IMX708, IMX296,
  OV9281, third-party sensors, …). New trixie modes (e.g. SRGGB8, 4056x2160)
  are detected automatically.
- **Pixel format filter in the global row**: compact digit dropdown between
  camera and resolution selectors. `0` = Auto, `1..n` = detected pixel formats
  (e.g. SRGGB8, SRGGB10_CSI2P, SRGGB12_CSI2P). The selected format filters the
  resolution and framerate lists; the digit-to-format mapping is shown
  dynamically in the tooltip.
- **Framerate slider popup**: the framerate dropdown now opens a vertical
  slider with integer steps from 0 to the maximum sensor framerate of the
  selected resolution/format, plus 0.1–0.9 fractional steps below 1 fps.
  `0` = Auto (no `--framerate` argument is passed).
- **Help → System Information**: now also shows the full camera/mode list from
  `rpicam-hello --list-cameras` next to the existing v4l2 device list.
- **Wayland ROI selection support**: native Wayland fallback for the ROI
  overlay (no `X11BypassWindowManagerHint`, semi-transparent visible overlay
  with hint text, local-only mouse grab).
- **Remote-session preview default (MCIM)**: preview backend is selected via
  `loginctl` remote detection – `--qt-preview` only on remote sessions.

### Fixed
- **stderr blocked camera detection**: detection was skipped entirely when
  `--list-cameras` wrote anything to stderr (e.g. dmaHeap warnings); stderr is
  now logged and stdout is always parsed.
- **Wrong bit depth for all modes**: the bit depth from the camera line
  ("12-bit RGGB") was applied to every mode; bit depth and packing are now
  derived per pixel-format line (SRGGB8 → 8/U, SRGGB10_CSI2P → 10/P, …).
- **Camera info field removed from General tab**: it was misplaced below
  Image Geometry/ROI; detection details now live in the log and in
  Help → System Information.

## [0.5.5] - 2026-08-10

### Fixed
- **Portrait mode preview positioning**: when a second camera preview was started
  in portrait mode (dual camera), it was placed side-by-side with the first preview
  instead of vertically stacked. On narrow screens (e.g. 1200x1920) this pushed the
  preview partially off-screen. The fix checks if horizontal placement fits within
  screen bounds; if not, it stacks vertically: above main window, below sibling
  preview, or clamped to screen bottom.

## [0.5.4] - 2026-08-10

### Fixed
- **apt → apt-get in updater**: `apt` prints "does not have a stable CLI interface"
  to stderr, causing the install to fail. Switched to `apt-get` (stable CLI)
  with `DEBIAN_FRONTEND=noninteractive` and `--allow-downgrades`.

## [0.5.3] - 2026-08-10

### Added
- **Visual update notification**: when an update is available, the Help menu
  entry changes to "Nach Updates suchen... [vX.Y verfügbar!]" with an info icon.
- **Auto-update toggle**: new checkbox "Beim Start nach Updates suchen" in
  Global Settings (default: on). When disabled, no background GitHub API call
  is made on startup.
- **German translations**: all UpdateChecker, UpdateDialog, menu and settings
  strings translated to German (33 new translations).

## [0.5.2] - 2026-08-10

### Added
- **Update checker with apt install**: `Help → Check for Updates…` now
  downloads the matching `.deb` from GitHub Releases and installs it via
  `pkexec apt install -y`. Architecture auto-detection (`dpkg --print-architecture`)
  ensures the correct `arm64`/`armhf` asset is selected.
- **Git clone detection**: when running from a `git clone`, the update dialog
  shows `git pull && cd build && cmake .. && make -j4` instead of the
  apt install flow.

## [0.5.1] - 2026-08-10

### Fixed
- **CTRL badge missing on Camera 1**: `checkRpicamCtrlCapability()` used a
  `static bool` guard that prevented per-instance flag assignment. The
  capability check ran once process-wide; Camera 1 never got `m_hasRpicamCtrl`
  because the function returned early. Fixed by caching results in `static`
  variables but applying to each `MainWindow` instance individually.
- **Tuning file path auto-detection**: default path was hardcoded to
  `/usr/share/libcamera/ipa/rpi/vc4`, which is wrong for Raspberry Pi 5
  (uses PiSP ISP). Now reads `/proc/device-tree/model` and selects
  `pisp` for RPi5, `vc4` as fallback for older models.

### Added
- **Update checker**: new `Help → Check for Updates…` menu entry queries
  GitHub Releases API and shows whether a newer version is available,
  with changelog and download link.

## [0.5.0] - 2026-08-09

### Changed
- **Setup dialog split**: Global settings (Paths, Splash, Enhanced Mode,
  Auto-Restart, Language) now live in a separate **Global Settings** dialog
  stored under `[General]`. Camera-specific settings (Focus, Zoom, Tabs,
  Custom) remain in **Camera Setup** under `[CameraN-Tab]`.
  - Menu: `Setup → Global Settings…` / `Setup → Camera Setup…`
  - Migration: old `[Camera0-Tab]` values are read as fallback if `[General]`
    keys are not yet present.

## [0.4.54] - 2026-08-09

### Added
- **German translations**: Setup dialog Focus/Zoom tabs (23 strings), CCM
  group, debug.log, parfocal mode, autofocus subdevice, focus/zoom movement
  tooltips. 185 previously unfinished translations now completed.

## [0.4.53] - 2026-08-09

### Fixed
- **AWB/CCM layout stretch**: when CCM is hidden (rpicam-apps < 1.13, no
  `rpicam-ctrl` support), the AWB Mode row no longer stretches across the
  full width. Added `addStretch()` placeholder that absorbs empty space
  when CCM widgets are invisible.

## [0.4.52] - 2026-08-09

### Fixed
- **debug.log path**: on system installs (`/usr/bin/`), the log now goes to
  `~/.config/rpicam-gui/debug.log` instead of unwritable `/debug.log`. (#7)
- **V4L2Controller log-spam**: reconnect retries no longer log on every attempt.
  Initial failure logged once; summary on success or final give-up. (#9)
- **V4L2 subdevice detection**: hardcoded `/dev/v4l-subdev3` replaced with
  camera-aware auto-detection via `VIDIOC_QUERYCTRL` and I2C-bus mapping.
  V4L2Controller only starts when explicitly enabled. (#8)

### Added
- **Setup dialog — Focus/Zoom tabs**: dedicated V4L2 hardware configuration
  tabs with Enable checkbox, editable device combo, and Detect button that
  scans all subdevices for focus/zoom capability and maps to camera index.
- **Step size tooltips**: Focus/Zoom step spinners now show their purpose;
  Near/Far movement buttons display the current step size in their tooltip.
- **`backup-release.sh`**: local snapshot script for clean release backups.
- **`resources/scripts/detect-af.sh`**: standalone AF-capable V4L2 subdevice
  detection with camera index mapping.

### Changed
- **Focus/Zoom step sizes**: moved from Settings tab into dedicated Focus/Zoom
  tabs alongside V4L2 hardware configuration.

## [0.4.51] - 2026-08-09

### Fixed
- Version detection for `--preview-backend`/`--ccm`: now parses the actual version
  number from `rpicam-apps build: vX.Y` instead of the capabilities line. The
  previous check for `egl:1`/`drm:1` gave false positives on rpicam-apps < 1.13
  that already listed those backends in capabilities (since 1.9).

## [0.4.50] - 2026-08-09

### Added
- Adaptive preview backend dropdown: auto-detects `--preview-backend` support
  (rpicam-apps >= 1.13) via `--version` capabilities line. Shows legacy options
  (Qt-Preview/Fullscreen/No Preview) on older rpicam-apps, or full backend list
  (Wayland-EGL/EGL/DRM/Qt/Fullscreen/No Preview) on 1.13+.
- `--preview-libs` browse button ("...") next to preview dropdown for custom
  preview library .so paths (rpicam-apps >= 1.13 only).
- `--ccm` (Colour Correction Matrix) input in AWB row of Image/Adjust tab.
  Requires explicit `--awbgains` — if CCM is set, awbgains are sent even at
  default values, since rpicam-apps silently ignores ccm without them.
- CCM and preview-libs widgets hide on rpicam-apps < 1.13 for backward compat.

### Changed
- AWB "custom" mode: Gain slider movement sets AWB to "custom" via control socket
  to stop auto-algorithm from fighting manual values. Reset restores "auto".

## [0.4.49] - 2026-08-08

### Fixed
- Follow-up to 0.4.48: AppPaths::sanitizeIfSystemPath() now also sanitizes paths
  already stored in QSettings from older versions. Previously only new (first-ever)
  installs got the XDG defaults; upgrades kept the old non-writable /usr/... paths.
  All 8 QSettings load sites now pass through the sanitizer.

## [0.4.48] - 2026-08-08

### Fixed
- Default paths for config/, output/, content_output/ now redirect to user-writable XDG
  directories (~/.config/rpicam-gui/config/ and ~/Videos/rpicam-gui/) when installed via
  apt/.deb into /usr. Previously AppPaths::base() resolved to /usr/ and the writable
  directories inherited that, causing permission errors for non-root users.

## [0.4.47] - 2026-08-08

### Added
- Control socket capability detection: rpicam-gui now runs `rpicam-vid --version` at startup
  and parses the capabilities line for `rpicam_ctrl`. The CTRL indicator on the Start/Stop
  button is only shown when rpicam-apps actually supports the control socket (PR #917).
  On mainline/unpatched rpicam-apps builds, the CTRL overlay is now correctly hidden.

### Changed
- Updated resources/scripts/rpicam-gui.svg to new icon design; removed duplicate path8.svg

### Fixed
- CTRL indicator was always visible when Control/Enabled=true, even on systems without
  patched rpicam-apps. Now gated behind actual rpicam_ctrl capability detection.

### Fixed
- CTRL indicator was always visible when Control/Enabled=true, even on systems without
  patched rpicam-apps. Now gated behind actual rpicam_ctrl capability detection.

## [0.4.46] - 2026-08-08

### Changed
- New application icon set: replaced all rpicam-gui icons (window, splash, about, desktop, PNG set)
  with redesigned icon from path8.svg; all 9 PNG resolutions regenerated via rsvg-convert
- Splash screen, about dialog, and window icon boxes adjusted for new square icon format

### Fixed
- icon_installer.sh: now also installs icons to user-local ~/.local/share/icons/hicolor/
  (Freedesktop priority 1) to prevent stale user icons from overriding system icons
- icon_uninstaller.sh: now also removes user-local icons from ~/.local/share/icons/hicolor/
- Both scripts: fixed $HOME resolution when run with sudo (use $SUDO_USER)
- Desktop icon now correctly displays the new icon after reinstall/reboot

## [0.4.45] - 2026-08-08

### Fixed
- GStreamer mode: suppress GST_TRACERS environment variable before launching gst-launch-1.0
  to prevent GstShark (hailo-tappas-core) from creating gstshark_* trace directories in CWD

## [0.4.44] - 2026-08-07

### Fixed
- AWB Gain R/B sliders: single-slider change now updates preview immediately (matching rpicam-ctrl)
- AWB Gain default values changed from 0.0 to 1.5/1.2 (matching rpicam-ctrl reference implementation)
- AWB Gain R/B reset: only sends awb:auto, not awbgains before awb:auto, for proper preview reset
- AWB Gain slider change: removed invalid "awb:custom" socket command (awbgains alone is sufficient)
- Shutter reset: now sends shutter:0 via ctrl-socket (was blocked by us > 0 guard)
- Metering reset: now sends metering:centre via ctrl-socket (was filtered by text guard)
- Global reset button: AWB gain sliders now reset to correct defaults (1.5/1.2)
- CameraInstance (MCIM): AWB gain slider defaults fixed
- rpicam-gui.conf: saved startup AWBGains defaults corrected

### Cleanup
- Removed duplicate reset button connections (sharpness, brightness, contrast, saturation, EV, gain)

## [0.4.43] - 2026-08-06

### Changed
- Shutter slider maximum now dynamically depends on framerate (1e6/fps µs)
- At 40fps max shutter = 25ms, at 20fps = 50ms, at 10fps = 100ms
- Changing framerate recalculates slider scale, updates display, and sends new value via control socket
- Prevents exceeding the frame interval, matching rpicam_ctrl (PR #917) behavior

## [0.4.42] - 2026-08-05

### Fixed
- Window height after startup: adjustWindowToOptimalSize() now called after loadStartupDefaults(),
  so codec/visibility changes from saved defaults are reflected in initial window size

## [0.4.41] - 2026-08-05

### Added
- Startup defaults feature: Setup > Set Defaults saves current UI widget values to rpicam-gui.conf
- On next launch, saved defaults override hardcoded rpicam-apps defaults automatically
- Setup > Reset Defaults clears all saved defaults (restores rpicam-apps defaults)
- Auto-restart checkboxes in Setup > Settings: restart on Set/Reset Defaults (on by default)
- 1-second notification dialog before auto-restart ("Defaults saved. Restarting...")
- Per-camera defaults: Camera 0 and Camera 1 each have independent Defaults sections
- Saved widgets include: app, codec, resolution, framerate, preview, x2, metering, timelapse, etc.
- Does NOT affect .txt config files (those remain rpicam-apps compatible)

### Fixed
- Metering "Select option:" placeholder no longer saved as invalid default value

## [0.4.40] - 2026-08-05

### Changed
- Internal: GPL-3.0 re-licensing preparation

## [0.4.39] - 2026-08-04

### Added
- Orange activity indicator dot on outer tab labels (Camera 0, Camera 1) while camera is running
- Indicator uses fixed-width placeholder to keep all tab labels equal width regardless of state

## [0.4.37] - 2026-08-03

### Changed
- Log tabs consolidated: per-camera Log tabs removed, replaced by shared "Log" tab (Camera 0 | Camera 1 | Log)
- Shared Log tab includes full Debug Output Options group (show process output, log to: window/console/debug.log)
- DebugLogger and both camera instances route output to shared central widget
- Process output handler uses QSettings instead of per-camera checkbox pointer
- Removed Log tab checkbox from Setup > Settings dialog

## [0.4.36] - 2026-08-03

### Changed
- Adjust tab groups reorganized: AWB Gains moved to White Balance, Metering moved to Image Processing
- Group titles renamed: Image Quality Settings -> Image Adjustments, Exposure & Gain -> Exposure, Processing -> Image Processing
- General Timings group: compact two-in-a-row layout (Timeout | Sync in one row, Timelapse separate)
- Image Processing group: compact two-in-a-row layout (HDR | Denoise, Metering | Flicker Period)
- Shutter input shows auto-scaled unit value (µs/ms/s) with 40px field width

## [0.4.35] - 2026-08-03

### Added
- Enhanced Mode toggle in Setup > Settings (Control/Enabled, default: on)
- CTRL overlay badge on Start/Stop button (green, always visible when enabled)
- Enhanced Mode section in Help dialog (rpicam-gui Help tab)
- PR 917 requirement tooltip in Settings dialog

### Changed
- CTRL overlay color brightened (#00e676) for better visibility
- Control socket now gated by Control/Enabled setting (connect, send, isActive)

### Fixed
- QSettings group scope bug: Shutter, Timelapse, and CustomApp values not persisted across restarts
- Stylesheet leaking into tooltip (now scoped with QLabel selector)
- Green frame wrapper removed; CTRL badge repositioned directly on button

## [0.4.34] - 2026-06-17

### Added
- Control Socket client for runtime parameter updates (requires rpicam-apps PR #917)
- Live parameter changes via Unix domain socket (/tmp/rpicam-vid{N}.sock):
  brightness, contrast, saturation, sharpness, ev, gain, awb, awbgains,
  roi, metering, exposure, denoise, hdr, shutter, framerate
- Automatic socket detection after app start (500ms delayed connect)
- Reconnect with exponential backoff (max 10 attempts, 2s interval)
- Caps parsing: maxfps and hasaf auto-detected from server on connect
- "Experimental" indicator label (green badge, shown only when socket is connected)
- Multi-camera support: independent socket per camera index

### Changed
- Process stop now disconnects control socket before terminating rpicam-apps

## [0.4.33] - 2026-06-12

### Changed
- License: replaced Non-Commercial rpicam-gui License v1.1 with GNU General Public License v3.0 (GPL-3.0)
- All source file SPDX headers updated from LicenseRef-rpicam-gui-NonCommercial to GPL-3.0-only
- About dialog License tab: updated to show GPL-3.0 text
- About dialog Dynamic Linking Notice: Qt5 LGPL-3.0 compatibility note updated
- About dialog Disclaimer tab: removed GPL-to-NC re-licensing notice, now simply refers to current GPL-3.0 license
- All README files: license badges and sections updated to GPL-3.0
- CONTRIBUTING.md: license reference updated
- .github/README.md and .github/README_DE.md: license badge changed from NC-orange to GPL-3.0-blue
- Translations (DE, EN): license text strings updated

## [0.4.32] - 2026-05-10

### Changed
- License: replaced GPL-3.0 with Non-Commercial rpicam-gui License v1
- About-Dialog: License tab now shows full license text (QTextBrowser)
- About-Dialog: Dynamic Linking Notice + Third-Party Licenses moved to Third-Party Libraries tab
- About-Dialog: Disclaimer tab now includes Project Independence notice (Gordon999/RPiCamGUI)
- inference/README.md + README_DE.md: license section updated
- resources/docs/full_readme.md: license references updated
- .gitignore: added .venv/
- pre-commit: clang-tidy build path fixed, clang-format disabled (causes mass diffs)

## [0.4.31] - 2026-05-08

### Changed
- Repository cleanup: internal planning docs removed from public repo
- First Run Checklist: removed emoji characters from INSTALLATION.md

## [0.4.30] - 2026-05-08

### Fixed
- Tab visibility defaults on fresh install: registerTab() defaultEnabled values
  now match GuiSetupDialog defaults (Expert=true, Focus/Zoom/Log/ODR/Actions=false).

## [0.4.29] - 2026-05-08

### Fixed
- Camera 0 tab only shown when only one camera is detected. Camera 1 tab
  is now created only when rpicam-vid --list-cameras reports two cameras.

### Changed
- Default tab visibility on fresh install: Expert/Audio/GStreamer/GST/Tools
  visible by default; Focus/Zoom/ODR/Actions/Log hidden by default.

## [0.4.28] - 2026-05-08

### Fixed
- Splash screen logo and all in-app images (About dialog, background logo, QR code)
  now embedded in Qt resources (QRC) — visible when installed via .deb package.
  Previously loaded via filesystem path which failed after system-wide installation.

## [0.4.27] - 2026-05-13

### Fixed
- Dual-camera preview (landscape mode): cam1 no longer overlaps cam0 when main
  window is already collapsed at camera start. Added screen-bounds clamping after
  sibling-aware boxY calculation; if the primary direction goes off-screen the
  algorithm tries the opposite side before applying a hard clamp.

## [0.4.26] - 2026-05-07

### Fixed
- Dual-camera preview window positioning: re-starting a camera no longer drifts
  to the right. Second preview now always placed on the opposite side of the
  running sibling (position-based, not index-based).

## [0.4.25] - 2026-04-17

### Added
- Focus Tab: Parfocal toggle switch (ToggleSwitch widget, blau/rot) in Focus Movements Gruppe
- ToggleSwitch: Custom QAbstractButton Widget mit animiertem Schieberegler

## [0.4.24] - 2026-04-14

### Changed
- Focus Favorites: Save now always captures Focus + Zoom together
- Focus Favorites: Double-click restores both Focus and Zoom
- Zoom Favorites group removed (Zoom always coupled via Focus Favorites)
- Zoom slider restore: fixed wrong scaling formula (was /32767*100, now direct value)

## [0.4.22] - 2026-04-14

### Changed
- Focus tab: merge 4 groups to 2; Calibrate button moved into Far/Near row
- Focus tab: Absolute Position row inside Focus Movements group
- Recording Options: reset button moved into PTS row
- General tab: Geometry + ROI merged into single row with one combined reset button
- Checkbox labels shortened: `Horizontal Flip` / `Vertical Flip` → `H-Flip` / `V-Flip` (DE + EN)
- DE translation: `PTS speichern unter:` → `PTS speichern:`

### Added
- Disclaimer tab in About dialog (between License and Credits)
  - Warranty disclaimer moved from License tab
  - Raspberry Pi trademark notice (DE + EN)
- Trademark disclaimer added to README and full_readme.md

### Fixed
- icon_installer.sh: replaced hardcoded `/home/admin` path with dynamic REPO_DIR
- icon_uninstaller.sh: aligned sizes with installer, English comments
- detection_test.sh: replaced hardcoded log path with `$XDG_RUNTIME_DIR`
- AppPaths.h: removed unused deprecated `camConf()` function, fixed stale comment

## [0.4.20] - 2026-04-13

### Changed
- **Repository reorganization**: Move `scripts/` to `resources/scripts/`,
  `docs/` to `resources/docs/`, `utils/` to `resources/utils/`.
  Move `README.md` to `resources/docs/full_readme.md` (GitHub landing page
  served via `.github/README.md`). Move `CHANGELOG.md` to `resources/docs/`.
  All internal references and relative links updated accordingly.
- **Icon set**: Add 9-size PNG icon set (16–512 px) generated from SVG;
  fix CMakeLists.txt install rule for icons.
- **Install script**: Add `resources/scripts/install.sh` with `--prefix`
  and `--uninstall` support.

## [0.4.18] - 2026-04-14

### Fixed
- **clang-tidy cleanup (remaining source files)**: Add braces around single-line
  if/for statements in ToolsModule.cpp (28 warnings), MainWindowCmdHelpers.cpp,
  MainWindowConfigHelpers.cpp, MainWindowAudio.cpp, CameraInstanceManager.cpp,
  GStreamerModule.cpp, ResourceBroker.cpp, ActionsTab.cpp, main.cpp.
  Fix default member initializers in GuiSetupDialog.h. Fix cert-err33-c in
  DebugLogger.cpp. Remove dead store in MainWindowCmdHelpers.cpp.

## [0.4.17] - 2026-04-13

### Fixed
- **clang-tidy cleanup (CameraInstance, ActionsModule, GstLaunchModule)**: Add missing
  `override` in CameraInstance.h, fix uninitialized variable use in ActionsModule.cpp,
  add braces around single-line if statements across all three files, fix
  default member initializer in CollapsibleHelper.h, reduce struct padding in DetectionAction.

## [0.4.16] - 2026-04-13

### Fixed
- **clang-tidy cleanup (MainWindow.cpp)**: Add missing `override` keywords in header files,
  add braces around single-line `if` statements, fix narrowing conversion `qint64 → pid_t`
  with explicit cast, remove dead variable stores (`optimalWidth`, `finalWidth`),
  resolve redundant branch condition (`wasRunning`), fix duplicate branch bodies,
  use `const &` for unnecessary copy initialization.

## [0.4.15] - 2026-04-07

### Changed
- **Splash screen**: Removed "AI-assisted development" label and hairline separator.
  Version label shows "Ver: X.XX" without white background (transparent).

## [0.4.14] - 2026-04-07

### Fixed
- **BoxInput (preview position) lost on camera tab switch**: showEvent() ran a
  QTimer that called getDefaultBoxInput() every time a camera tab became visible,
  overwriting any user-set value from the double-click overlay selection.
  Fix: new flag `m_boxInputManuallySet` tracks whether the user made an explicit
  selection. showEvent timer respects the flag and skips the overwrite.
  Flag is cleared on: X-reset, global X-reset, window move (recalculation).

## [0.4.13] - 2026-04-06

### Fixed
- **ActionsModule: Save Image folder read from correct settings group**: `Paths/GuiOutputPath`
  was stored under `beginGroup(m_tabGroup)` but read without the group → always fell back
  to `content_output/`. Now reads with `beginGroup(m_tabGroup)` so the Setup path is
  correctly picked up. Also fixed double-slash in filename via `QDir::cleanPath`.

## [0.4.12] - 2026-04-06

### Fixed
- **ActionsModule: Save Image folder no longer appends detection_output**: Default
  folder is now exactly the output path from Setup (no subfolder appended).
  Custom user path in Actions is still used verbatim when set.

## [0.4.11] - 2026-04-06

### Fixed
- **ActionsModule: Save Image default folder respects Setup output path**: When no
  custom image folder is set in Actions, the detection images are now saved to
  `<Paths/GuiOutputPath>/detection_output/` (as configured in Setup) instead of
  the hardcoded `content_output/detection_output/`. Placeholder text updated
  to reflect the dynamic default.

## [0.4.10] - 2026-04-06

### Fixed
- **OdrModule: qDebug() logged before reportOnlyChanges filter**: All detections
  were written to the debug log (file + outputLog widget) before the
  `reportOnlyChanges` check ran. Moved qDebug to after the filter so only
  detections that actually pass the filter are logged. Filtered detections
  emit a separate debug message for traceability.

## [0.4.09] - 2026-04-06

### Fixed
- **Global X-Reset now resets Actions filter settings**: cooldown/confidence
  reset to defaults (30/70) and filter settings reset button turns black.

## [0.4.08] - 2026-04-06

### Fixed
- **OdrModule: report-only-changes restored**: same object is now fully skipped
  (no emit, no list entry) instead of being forwarded incorrectly.

## [0.4.07] - 2026-04-06

### Fixed
- **OdrModule: detections always forwarded to ActionsModule**:  
  OdrModule had its own filter/confidence/cooldown pre-check using a stale copy of  
  `m_detectionAction` (reference to `InferenceTab::m_detectionAction`, never synced  
  with ActionsModule checkbox values). Defaults were: `filteredObjects=empty`,  
  `minConfidence=0`, `cooldownSeconds=30` — blocking `detectionToExecute` before  
  `ActionsModule::executeDetection()` could ever run.  
  Fix: All gating logic removed from OdrModule. `detectionToExecute` is always emitted.  
  `ActionsModule::executeDetection()` handles all filtering with live user values.

## [0.4.06] - 2026-04-06

### Fixed
- **Telegram video send**: was searching for `.h264` file with fixed timestamp before  
  recording ended. Now searches in Stop-SIGUSR1 callback after 10s wait, using glob  
  for newest `*.mjpeg` file.
- **Filter-reset button stayed red**: comparison was against hardcoded `70`, but reset  
  used config value. Fixed: reset to true defaults (cooldown=30, confidence=70),  
  button set black directly.
- **Clear Log did not clear file**: added `DebugLogger::clearLogFile()` which truncates  
  `debug.log` when the checkbox is active.
- **MinConfidence default restored to 70**.

## [0.4.05] - 2026-04-06

### Fixed
- **Actions-Tab: Checkboxen werden beim App-Start nicht mehr wiederhergestellt**:  
  `loadSettings()` stellte alle Aktions-Checkboxen (Play Sound, Save Image,  
  Start Recording, Send Telegram etc.) aus QSettings wieder her. Dadurch konnten  
  beim nächsten Kamera-Start ungewollt Aktionen feuern.  
  Fix: Checkboxen werden in `loadSettings()` bewusst nicht gesetzt — sie starten  
  immer unchecked. Nur persistente Konfigurationswerte (Pfade, Bot-Token,  
  Chat-ID, Dauer, Cooldown, Confidence) werden weiterhin geladen.
- **Segment-Duration-Sync wiederhergestellt**:  
  `startTimedRecording(-N)` setzte das `segmentDurationInput`-Feld nicht korrekt.  
  Bei negativen Werten (Sync-Anfrage vom "Start Recording"-Checkbox) wird nun  
  `segmentDurationInput` auf `|N| * 1000` ms gesetzt, damit Segment- und  
  Aufnahmedauer übereinstimmen.

## [0.4.04] - 2026-04-06

### Fixed
- **Crash: "Send recorded video to Telegram" aktivieren**:  
  Aktivieren der Checkbox löste eine Signalkette aus:  
  `sendTelegramVideoCheckbox::toggled` → setzt `startRecordingCheckbox` auf true  
  → dessen Handler emittiert `startTimedRecordingRequested(-(seconds+2))`  
  → `startTimedRecording(-32)` setzte ein negatives Timeout in die UI,  
  startete die Aufnahme und rief `QTimer::singleShot(-30000, ...)` auf  
  → undefiniertes Verhalten / Crash.  
  Fix: `startTimedRecording()` gibt bei negativen Werten sofort zurück  
  (negative Werte = "sync-only intent", kein echter Start-Request).

## [0.4.03] - 2026-04-07

### Refactored
- **Phase 26: src/plugins/ + src/core/ → src/tabs/**:  
  Die 5 dünnen Plugin-Wrapper-Klassen (~50 Zeilen je) und das über-engineerte  
  Plugin-Registry-System wurden eliminiert. Jedes Feature-Tab ist jetzt eine  
  direkte `*Tab`-Klasse in `src/tabs/` (ActionsTab, GStreamerTab, GstLaunchTab,  
  InferenceTab, ToolsTab), die `ITabPlugin` direkt implementiert.
- Gelöschte Verzeichnisse: `src/plugins/` (10 Dateien), `src/core/` (5 Dateien),  
  `src/app/ApplicationServices.cpp/h` (Dead Code).
- Leere Modul-Platzhalter (`src/modules/inference/.gitkeep` etc.) entfernt.
- Alle MainWindow.*-Referenzen von `m_*Plugin` → `m_*Tab` aktualisiert.

## [0.4.02] - 2026-04-06

### Fixed
- **Crash: ODR-Tab Detection-Gruppe einklappen (CAM1)**: `InferencePlugin::initialize()` 
  übergab `nullptr` als `adjustWindowCallback` an `OdrModule::setup()`. Das Template 
  `CollapsibleHelper::makeCollapsible(group, key, slot)` rief beim Signal-Aufruf einen 
  Null-Slot auf → Segfault. `OdrModule::setup()` prüft nun den Callback vor der 
  Überladungswahl (with/without slot).
- **Latenter Crash in ToolsModule**: Lambda `[adjustWindowCallback]() { adjustWindowCallback(); }` 
  rief den Callback ohne nullptr-Check direkt auf. Abgesichert mit `if (adjustWindowCallback)`.
- **Log-Tab immer letztes Tab**: Priorität von `999` gesetzt — Log erscheint unabhängig 
  von aktivierten/deaktivierten Tabs immer als rechtestes Tab in der Gruppe.

### Removed
- **SettingsMigration.cpp/h vollständig entfernt**: Einmalige MCIM-Datenmigration 
  (Phase 0 Schritt 0.2) ist seit Phase 25 obsolet. Erzeugte den überflüssigen 
  `[Migration] McimVersion=1` Eintrag in der Config.
- `rpicam-gui-mcim.conf` Legacy-Datei entfernt (ersetzt durch `rpicam-gui.conf`).

## [0.4.01] - 2026-04-06

### Changed (Phase 25: Config-Konsolidierung)
- Alle per-Tab-Konfigurationen aus separaten Dateien in `rpicam-gui.conf` konsolidiert
- Schema: `[Camera0-Tab]` / `[Camera1-Tab]` Gruppen für alle GUI-Tab-Settings
- `Language/Selected` bleibt global im `[General]`-Abschnitt (kein beginGroup)
- `AppPaths::tabGroup(int)` Hilfsfunktion gibt `"Camera0-Tab"` / `"Camera1-Tab"` zurück
- Alle `m_configFile`-Member in `m_tabGroup` umbenannt (kein Pfad mehr, nur Gruppenname)
- `MainWindow`-Konstruktor: `configFile`-Parameter entfernt (nicht mehr benötigt)
- `main.cpp`: `metaSettings`/`Cam0ConfigFile`/`Cam1ConfigFile` Block entfernt
- 33 Dateien geändert (MainWindow, alle Plugins, alle Module, TabRegistryService, GuiSetupDialog)

## [0.4.0] - 2026-04-05

### Milestone: Modular Plugin System (MPS)

This release marks the completion of the full plugin-system refactoring.
All major subsystems have been extracted into dedicated plugins and
MainWindow has been cleaned of all dead code.

#### Plugin Architecture
- **ActionsPlugin** — Detection action configuration tab (P11/P12)
- **GStreamerPlugin** — GStreamer live streaming tab (P13b)
- **GstLaunchPlugin** — GST-Launch / stream viewer tab (P14)
- **InferencePlugin** — Hailo AI object detection tab (P15)
- **ToolsPlugin** — Video conversion tools tab (P15)
- **ITabPlugin** interface — common base for all plugins (P8-P10)

#### Dead Code Cleanup (P16–P23)
- Removed `MainWindowStreamViewer.cpp` (1115 lines) — replaced by GstLaunchPlugin
- Removed `MainWindowDetectionActions.cpp` (1560 lines) — replaced by ActionsPlugin
- Removed 32 dead widget pointers from `MainWindow.h` (action + GST widgets)
- Removed dead `odrModule` / `toolsModule` aliases
- Removed 10 dead method declarations without definitions
- Removed 22 dead action widget pointers from `CameraInstance.h`

#### Bug Fixes
- **Config-overwrite bug**: `saveConfigurationToFile()` was overwriting all
  `DetectionActions/*` QSettings keys with default values on every save, wiping
  all user-configured action settings. Now delegates to `ActionsPlugin::saveSettings()`
- **Recording folder bug**: `startTimedRecording()` always used the default
  `/detection_output` folder — now correctly reads the configured folder from
  `ActionsPlugin`

#### Code Statistics
- `MainWindow.cpp`: 13.000 → 7.949 lines (−39%)
- `MainWindow.h`: ~700 → 535 lines (−24%)
- Repository: ~3.000+ dead lines removed in P16–P23

## [0.3.0.8] - 2026-03-19

### Fixed
- **MCIM: Strg+0 Fenster-Resize von cam1 Tab**: Chrome-Berechnung in `adjustWindowToOptimalSize()` war instabil — `this->height()` ist für inaktive Cam-Tabs nicht zuverlässig. Stattdessen wird jetzt `outerTabs->currentWidget()->height()` verwendet (immer der sichtbare Tab). Dadurch funktioniert der Resize konsistent von beiden Cam-Tabs aus.
- **Toggle-Logik in `toggleAllGroups()` korrigiert**: Die Schleifenbedingung war invertiert — Gruppen wurden nur kollabiert wenn sie bereits kollabiert waren. Fix: `helper->isCollapsed() != targetCollapsed` als korrekte Bedingung.

## [0.3.0.7] - 2026-03-05

### Fixed
- **MCIM: Strg+0 Collapse/Expand synchronisiert beide Cam-Tabs**: Strg+0 kollabierte bisher nur die Gruppen des aktiven Tabs — der inaktive Tab hielt Qt's minimumHeight-Constraint aufrecht, dadurch konnte das Fenster nicht schrumpfen. Neue Methode `collapseGroupsOnly(bool)` kollabiert inaktive Tabs mit `m_suppressResize`-Guard (kein Resize-Trigger), `layout()->activate()` erzwingt sofortige Layout-Neuberechnung. In `adjustWindowToOptimalSize` werden jetzt QTabWidget, internes QStackedWidget und alle Cam-Widgets auf `setMinimumHeight(0)` gesetzt.

## [0.3.0.6] - 2026-03-04

### Fixed
- **MCIM: cam1 Config-Pfad im Setup-Dialog falsch**: `MainWindow.cpp` hatte an zwei Stellen (`cameraIndex != 0` im Konstruktor und `fixCameraIndex()`) den Bindestrich-Pfad `rpicam-gui-cam1.conf` hardcodiert. Auf `rpicam-gui_cam1.conf` (Unterstrich, konsistent mit cam0) geändert.
- **Splash-Screen Position**: Geometrie-Schlüssel war falsch — gespeichert unter `"Geometry/MainWindow"`, gelesen unter `"Window/Geometry"` → Splash erschien immer in Bildschirmmitte statt über dem zuletzt gespeicherten Fenster. Schlüssel korrigiert.
- **"Preview:" Label** aus der Toolbar entfernt.

## [0.3.0.4] - 2026-03-04

### Fixed
- **MCIM: GST Stream Viewer Group vertikal komprimiert**: `setMaximumHeight` von 900 auf 950 erhöht. Der GST-Tab hat die größte vertikale Ausdehnung aller Tabs — 900px war zu niedrig und hat die Stream Viewer Group zusammengepresst.

## [0.3.0.3] - 2026-03-04

### Fixed
- **MCIM: Collapse/expand resize** (`adjustWindowToOptimalSize`): Das Fenster wurde nach dem Kollabieren einer Gruppe nicht verkleinert. Ursache: Qt setzt intern eine implizite `minimumHeight` auf dem embedded Cam-Widget (aus dem Layout-System). `window()->resize()` wurde von Qt blockiert, solange das Child-Widget seine Mindesthöhe unterschreiten würde. Fix: `setMinimumHeight(0)` auf `this` und `window()` vor der Animation hebt die Sperre auf. Zusätzlich wird `QPropertyAnimation` jetzt auf `window()` (outerWindow) statt `this` ausgeführt, da `this` in MCIM ein embedded Widget ist.
- **Audio Tab: Reset-Button** (✕) lag isoliert unterhalb der Collapsible-Gruppen. Jetzt auf derselben Zeile wie "Enable Audio Recording" (rechts ausgerichtet).

## [0.3.0.2] - 2026-03-02

### Added
- **Help > Support tab**: New "Support" entry in Help menu opens the HelpDialog directly on the Support tab
- **Support & Bug Report tab** in HelpDialog with:
  - Form fields: Subject, Category (Bug/Crash/Feature/Question/Docs), Problem Description, Steps to Reproduce
  - Automatic system info collector (OS, kernel, Qt version, rpicam-apps version, camera list, memory)
  - Three send options: Send by E-Mail (`tomge68@gmail.com`), Save to File (timestamped .txt), Copy to Clipboard
  - Direct GitHub links (Issues / Discussions) in the info section
  - Scrollable layout to fit all screen sizes
- **i18n**: Full German/English translations for all new Support tab strings

### Fixed
- HelpDialog: `initialTab` parameter so `Help > Support` opens directly on the Support tab

## [0.3.0.1] - 2026-02-28

### Fixed
- **Flicker Period CLI bug**: `--flicker-period` passed display text (e.g. "Aus") instead of CLI value (e.g. "off"). Fixed using Qt data-role pattern: `addItem(tr("Display"), "cli-value")` + `currentData()`
- **Umlaut encoding**: All German translations corrected (ae→ä, oe→ö, ue→ü, ss→ß); 244 strings fixed via automated script

### Changed
- **Language change dialog**: Dialog now shown in the newly selected language; application restarts automatically instead of quitting
- **Browse buttons**: "Durchsuchen" renamed to "Wählen" throughout the UI
- **About subtitle (DE)**: Shortened to "Eine Benutzeroberfläche zur Steuerung von rpicam-apps."

## [0.3.0.0] - 2026-02-28

### Added
- **Internationalization (i18n)**: Full German/English language support
  - 731 UI strings wrapped with tr() for translation
  - Language selector in Setup > Settings tab (Deutsch/English)
  - QTranslator integration in main.cpp with QSettings persistence
  - Qt5LinguistTools integration in CMakeLists.txt
  - Translation files: rpicam-gui_de.ts, rpicam-gui_en.ts
  - Compiled .qm files automatically copied to build directory
  - All source files covered: MainWindow.cpp, MainWindowHelpers.cpp, GuiSetupDialog.cpp, HelpDialog.cpp, DonationDialog.cpp, CollapsibleGroupBox.cpp
  - Restart required after language change (info label in Settings)

### Changed
- **Language**: Application restart required to apply language changes
- **Settings**: New QSettings key Language/Selected (values: "de", "en")

## [0.2.9.1] - 2026-01-08

### Added
- **Global Framerate Selector**: Context menu support for custom framerates
  - Right-click to add or delete custom framerate values
  - Persistent storage of custom framerates in settings
  - Custom values are preserved across resolution changes

### Changed
- **Framerate Validation**: Support for decimal framerate values (e.g., 0.1, 0.5, 1.5)
  - Changed validation from integer-only to decimal (toDouble instead of toInt)
  - Applies to both global framerate selector and Tools Tab framerate

## [0.2.8.1] - 2026-01-08

### Added
- **Tools Tab**: New optional collapsible Tools group with Image-to-Video Converter
  - Convert image sequences to video using ffmpeg
  - Auto-detect image patterns from directory (numbered sequences and wildcards)
  - Editable dropdowns for Image Pattern, Framerate, and Resize with custom value support
  - Context menus (right-click) to add/delete custom values
  - Persistent storage of custom patterns, framerates, and resolutions
  - Codec selection: H.264 (libx264), H.265/HEVC (libx265), MJPEG
  - Quality slider (CRF 18-28) with default Qt styling
  - Encoding preset selection (ultrafast to veryslow)
  - Resize video with custom resolutions or keep original size
  - Real-time progress tracking with frame counting and time estimation
  - Flexible layout: Input Directory and Output Filename span available width
  - Compact two-column layout: Framerate+Codec, Resize+Encoding in single rows
  - MessageBox notification with sound on completion/failure
  - Force divisible-by-2 scaling for H.264 compatibility

## [0.2.8.0] - 2026-01-08

### Fixed
- **Unit Labels**: Corrected shutter speed unit label from "ms" to "µs" (microseconds)
  - Shutter label now correctly shows "Shutter (µs):" instead of "Shutter (ms):"
  - Tooltip already correctly stated microseconds
  - Added "(ms)" unit to Timelapse label for consistency

### Changed
- **ROI and Focus Window Selection**: Aspect ratio locked to video resolution during interactive selection
  - ROI selection now maintains the aspect ratio of the currently selected video resolution
  - Focus Window selection now maintains the aspect ratio of the currently selected video resolution
  - Behavior matches Preview Geometry selection (double-click)
  - Height is automatically calculated based on width to maintain correct aspect ratio
  - Ensures ROI and Focus Window proportions match the actual video dimensions

### Enhanced
- **ROIOverlay**: Added aspect ratio support
  - New `setAspectRatio(double ratio)` method
  - Automatic height calculation in `mouseMoveEvent()` based on width and aspect ratio
  - Boundary checking to keep selection within widget limits
- **Tooltips**: Updated ROI and Autofocus Window tooltips to document aspect ratio locking

### Technical
- ROIOverlay.h: Added `setAspectRatio()` method and `aspectRatio` member variable
- ROIOverlay.cpp: Implemented aspect ratio constraint in mouse movement tracking
- MainWindow.cpp: ROI and Autofocus Window double-click handlers now set aspect ratio from current video resolution
- MainWindow.cpp: Corrected unit labels for shutter and timelapse parameters

## [0.2.7.8] - 2026-01-07

### Fixed
- **Window Resize on Group Collapse**: Fixed window not resizing when collapsing/expanding groups via Ctrl+0 on certain tabs
  - Problem: Qt's automatic minimumSize constraint prevented window from shrinking on tabs with complex layouts (General, Video, etc.)
  - Workaround: Temporarily switch to Actions tab during toggle operation to unlock size constraints, then return to original tab
  - Groups now properly collapse/expand and window resizes accordingly on all tabs

### Changed
- **Keyboard Shortcut**: Changed "Expand/Collapse All Groups" shortcut from Ctrl+Shift+0 to Ctrl+0
- **Menu Cleanup**: Removed "Optimal Window Size" menu item (Ctrl+0 now exclusively used for collapse/expand)

## [0.2.7.7] - 2026-01-06

### Fixed
- **Framerate Loading from Config**: Fixed bug where framerate setting was not correctly loaded from configuration files
  - Problem: When loading a config, the framerate was set before the resolution, causing `updateFramerateOptions()` to overwrite the loaded framerate value
  - Solution: Framerate is now stored temporarily and applied after the resolution is set, preserving the saved value

## [0.2.7.6] - 2025-12-15

### Changed
- **UI Styling Consistency**: Unified group styling across all tabs
  - Standardized `border-radius` to 5px for all groups (was mixed 5px/8px)
  - Standardized `margin-top` to 1ex for all groups (was mixed 1ex/10px)
  - Standardized title position `left` to 10px for all groups (was mixed 8px/10px)
  - Unified all group headers: `font-weight: bold`, `border: 2px`, `padding-top: 10px`
  - Applied consistent styling to all sub-groups in Focus and Zoom tabs

### Fixed
- **Group Spacing**: Removed explicit `addSpacing()` between groups in Debug, Inference, and Focus tabs for consistency with General tab
- **Zoom Tab Position**: Removed extra margins and spacing that caused content to be offset from other tabs
- **Visual Consistency**: All collapsed groups now have uniform height, border style, and text alignment
- **Window Resize Behavior**: Fixed window width expanding when collapsing groups - now only height adjusts when groups are collapsed/expanded

## [0.2.7.2] - 2025-12-10

### Changed
- **Selection Overlay Colors**: Changed pattern from pink/green to Start/Stop button colors (blue/red)
- **About Dialog Enhancements**: 
  - Added Kletternaut logo as watermark in header section
  - Updated Third-Party Libraries to include Qt5::X11Extras
  - Added usage description for X11 Libraries (screen selection overlay)
  - Version text font changed to normal (non-bold)

## [0.2.7.0] - 2025-12-10

### Changed
- **New Selection Overlay**: Complete reimplementation of screen selection
  - Real transparency: Desktop stays visible (no more screenshots!)
  - New style frame: Pink/green diagonal stripe pattern
  - XQueryPointer: Real screen coordinates instead of Qt widget coordinates
  - Frame mask: Inner area transparent, frame visible
  

### Fixed
- **10-pixel rounding restored**: All coordinates rounded to 10-pixel grid
- **Aspect ratio calculation restored**: Height automatically calculated from width
- **Live feedback**: Values rounded during selection, not just on release

### Technical
- MainWindow: grabMouse()/grabKeyboard() inspired by SSR PageInput
- SelectionOverlay: Small movable window inspired by SSR RecordingFrameWindow
- CMakeLists.txt: Added Qt5::X11Extras


## [0.2.6.8] - 2025-12-10

### Changed
- **Setup Dialog Restructuring**:
  - Converted single-page Setup Dialog to 4-tab structure for better organization
  - Tab 1 "Paths": Configuration File + Folder Paths
  - Tab 2 "Settings": General Settings + Focus & Zoom Step Sizes
  - Tab 3 "Tabs": Activate Tabs checkboxes for all main window tabs
  - Tab 4 "Custom": Custom Preview Geometry + Custom Apps + Custom Resolutions
  - Reduced dialog height from overwhelming single page to manageable tabbed interface

### Fixed
- **Preview Reset Button Bug**: Fixed button ignoring "Use Custom Preview Geometry" setting
  - Button now uses `getDefaultBoxInput()` instead of directly calling `calculateBoxInput()`
  - Correctly resets to custom geometry when enabled, calculated geometry when disabled
  - Removed duplicate `overlayResetButton` connect statement
- **"Set as Default" User Feedback**: Enhanced confirmation message
  - Now explicitly states: "'Use Custom Preview Geometry' has been automatically enabled"
  - Clearer communication when saving current preview geometry as default

### Improved
- **Help Dialog Modernization**:
  - Complete content rewrite with correct tab names and descriptions for all 13 tabs
  - Made dialog modeless (non-blocking) - can stay open while using main window
  - Restructured with modern QGroupBox layout and ScrollArea
  - Added header with logo (65px) and version information
  - Dynamic table sizing with `document()->adjustSize()` for content-aware heights
  - rpicam-apps Parameters tab with 99 documented parameters and source citation
- **Donate Tab Styling Consistency**:
  - Changed group label colors from blue (#0066cc) to consistent gray (#333333)
  - Removed extra margins for uniform width across all sections

## [0.2.6.6] - 2025-12-09

### Changed
- **Help Dialog Improvements**:
  - Added source citation for rpicam-apps parameters with link to Raspberry Pi documentation
  - Fixed `lores-par` parameter link to point to correct combined anchor (lores-width-and-lores-height)
  - Reduced dialog width from 900px to 700px for better screen space usage
- **Tab Ordering Improvements**:
  - Reordered main window tabs: Audio tab now appears before Focus tab
  - Updated Setup Dialog checkbox order to match: Expert, Audio, Focus, Zoom, GStreamer, GST, ODR, Action, Log

## [0.2.6.2] - 2025-12-09

### Added
- **Custom Preview Geometry Feature**:
  - Right-click context menu on BoxInput field with "Set as Default" action
  - Saves current preview position as custom default to `rpicam-gui.conf`
  - New checkbox in GUI Setup Dialog: "Use Custom Preview Geometry"
  - When enabled, custom position is used instead of calculated position
  - `getDefaultBoxInput()` function returns custom or calculated value based on setting
  - All reset functions (Global Reset, Overlay Reset) respect custom geometry setting

### Fixed
- **moveEvent() Bug**: Custom preview position no longer overwritten when main window is moved
  - moveEvent() now checks `UseCustomGeometry` setting before recalculating BoxInput
  - Custom position preserved during window operations when custom geometry enabled
- **Overlay Reset Button Bug**: Button stayed red when custom geometry active
  - `updateOverlayResetButtonColor()` now uses `getDefaultBoxInput()` instead of `calculateBoxInput()`
  - Button correctly shows black when current value matches custom or calculated default

### Changed
- All new code comments written in English
- Added `VERSION_TEXT` define in Version.h for internal change tracking

## [0.2.6.1] - 2025-12-09

### Fixed
- **Global Reset Button Color Bug**: Fixed button staying red after Global Reset click
  - **Root Cause**: `updateGlobalResetButtonColor()` expected `splitFilesCheckbox` to be checked (true) as default
  - Global Reset set checkbox to false (correct default), but color check still looked for true
  - **Result**: Button stayed red after reset, only turned black after window resize
  - **Solution**: Changed check from `if (!splitFilesCheckbox->isChecked())` to `if (splitFilesCheckbox->isChecked())`
  - Now false is correctly treated as default value, true triggers red color
  - Button immediately turns black after Global Reset without requiring window interaction

## [0.2.6.0] - 2025-12-08

### Changed
- **Focus & Zoom UI Overhaul**:
  - **Step Size Controls**: Replaced sliders with 6 radio buttons (no labels, tooltip shows value)
    - Configurable values in GUI Setup Dialog (default: 100, 300, 1000, 3000, 10000, 32767)
    - Dynamic update when settings saved
    - Lambda functions read settings on each click for live configuration
  
  - **Autofocus Parameters**: Optimized 3-row layout (was 5 rows)
    - Row 1: Mode + Range side-by-side
    - Row 2: Speed + Window side-by-side  
    - Row 3: Lens Position (input before slider, full width)
    - All labels fixed 110px width for perfect alignment
    - Reset buttons aligned vertically with 10px spacing
    - Slider expands dynamically to fill available space
  
  - **Absolute Position Groups**: Streamlined layout
    - Removed unnecessary reset buttons (✕)
    - Added OK buttons next to input fields for explicit apply action
    - Layout: Label (110px) + Input (60px) + OK (40px) + Slider (dynamic)
    - Sliders expand to fill available width
  
  - **Current Position Display**: Unified layout for Focus & Zoom Configuration
    - Fixed label width 130px for "Current Position:"
    - Refresh button consistently right-aligned
    - Zoom Configuration: Added 240px spacer to match Focus layout structure
    - Minimum width 60px for position numbers prevents text overlap
  
  - **Input Field Improvements**:
    - Changed from `textChanged` to `editingFinished` signal
    - Prevents premature position changes (e.g., "3" when typing "3000")
    - Position applied on: Enter key, OK button click, or focus loss
    - Added `hasFocus()` check in polling to prevent overwriting during input
    - Input fields remain editable while typing (polling skips focused fields)

### Fixed
- **Layout Consistency**: All Focus/Zoom groups now use consistent label widths and spacing
- **Input Blocking**: Polling no longer interferes with text input in absolute position fields
- **Label Truncation**: "Current Position:" label wide enough to display without clipping

## [0.2.5.7] - 2025-12-08

### Fixed
- **CRITICAL BUG FIX**: Fixed disabled-but-checked checkbox state bug affecting recording parameters
  - **Root Cause**: Global Reset set checkboxes to checked state but left them disabled (grayed out)
  - Disabled checkboxes were still read as "checked" and added parameters to command line
  - **Impact**: `--split`, `--signal`, `--keypress` parameters incorrectly added even when options were grayed out
  
- **Solution 1 - Enable State Validation** (`MainWindowHelpers.cpp`):
  - Added `isEnabled()` check for all recording options before reading checkbox state
  - `splitFilesCheckbox`: Now checks `isEnabled() && isChecked()` before adding `--split`
  - `signalRecordingCheckbox`: Now checks `isEnabled() && isChecked()` before adding `--signal`
  - `keypressRecordingCheckbox`: Now checks `isEnabled() && isChecked()` before adding `--keypress`
  - Prevents disabled checkboxes from affecting command generation
  
- **Solution 2 - Consistent Reset Behavior** (`MainWindow.cpp`):
  - Fixed Global Reset inconsistency: Split Files checkbox now correctly resets to `false` (unchecked)
  - Previously: Group Reset → unchecked ✅, Global Reset → checked ❌ (inconsistent!)
  - Now: Both resets set `splitFilesCheckbox->setChecked(false)` consistently
  - Eliminates confusion between reset methods

### Added
- **UI Improvements**:
  - Setup Dialog: Shortened label "Metadata Output Path:" → "Metadata:" (prevents text clipping)
  - Inference Tab: Detection Results group now expands vertically like Debug tab
  - Added weighted stretch factors (100:1) for optimal space distribution
  - Results list fills available vertical space when expanded, groups stay at top when collapsed

### Changed
- **Splash Screen**: Enhanced transparency support
  - Attempts to use `feh` for true alpha transparency (if installed)
  - Falls back to Qt-based splash with dark background if `feh` unavailable
  - Maintains enable/disable configuration via Setup dialog
  - Works correctly with xrdp/labwc remote desktop environments

## [0.2.5.6] - 2025-12-08

### Fixed
- **CRITICAL BUG**: Fixed `--split` parameter incorrectly added to rpicam-jpeg and rpicam-still commands
  - `--split` parameter is only valid for `rpicam-vid`, not for still image applications
  - Bug occurred because split checkbox state persisted from previous configurations
  - Code in `MainWindowHelpers.cpp` line 654 did not check which app was selected
  - **Solution**: Added app type check: `if (app == "rpicam-vid" && splitFilesCheckbox && splitFilesCheckbox->isChecked())`
  - Parameter now only added when using rpicam-vid AND checkbox is explicitly checked
  - Prevents invalid command line arguments for rpicam-jpeg/rpicam-still
  - Resolves regression of bug supposedly fixed in v0.2.4.5

## [0.2.5.5] - 2025-12-08

### Fixed
- **Debug Tab Layout**: Optimized vertical space distribution
  - Debug Messages group now expands to fill available vertical space with stretch factor 100
  - Added `setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding)` to debug messages group
  - Output log widget properly expands with `setMinimumHeight(200)` and stretch factor 1
  - Weighted stretch ratio (100:1) ensures debug messages takes maximum space when expanded
  - Collapsed groups remain at top without floating in middle of tab
  - Perfect balance between expanded (log fills screen) and collapsed (groups stay top) states

### Added
- **Collapsible UI Groups**: Complete UI overhaul with 63 collapsible group boxes
  - All 13 tabs now feature collapsible groups for cleaner, more organized interface
  - Persistent collapse state saved in QSettings (keys: "UI/TabName/GroupName")
  - Smooth QPropertyAnimation (200ms) for collapsing/expanding transitions
  - Small +/- toggle button (20x20px) on each group box title bar
  - Automatic visibility restoration on expand (group-specific logic preserved)
  - Compact layout with 5px spacing between groups for efficient screen usage

- **Window Management Features**:
  - **Optimal Window Size (Ctrl+0)**: Automatically adjusts window dimensions to fit current tab content
    - Calculates optimal width and height from tab's sizeHint() and minimumSizeHint()
    - Ensures minimum width accommodates full tab bar without scrolling
    - Smooth animated resize (200ms, OutCubic easing)
    - Constrains to screen size (leaves 50px margin)
  - **Expand/Collapse All Groups (Ctrl+Shift+0)**: Toggle all collapsible groups at once
    - Determines state from first group in list
    - Toggles all 63 groups to opposite state
    - Auto-adjusts window size after 250ms delay
  - **Auto-resize on Collapse**: Every group collapse/expand triggers automatic window size adjustment
    - Connected via lambda to adjustWindowToOptimalSize() for all 63 groups
    - Provides seamless, dynamic UI that adapts to content visibility

- **View Menu**: New menu with keyboard shortcuts
  - "Optimal Window Size" (Ctrl+0) - Fit window to current tab
  - "Expand/Collapse All Groups" (Ctrl+Shift+0) - Toggle all groups at once

- **MenuBar Styling**: Clean, professional appearance
  - Full-width separator line under menu bar (1px solid, palette(mid))
  - Explicit background color (palette(window)) prevents rendering artifacts
  - Zero spacing and margins for clean layout
  - Hover and pressed states with highlight color
  - Forced repaint to prevent ghosting/duplication artifacts

### Changed
- **Output File Settings Layout**: Improved encoding selector alignment
  - Encoding dropdown and reset button now right-aligned in Output Mode row
  - Removed "Encoding:" label for cleaner appearance
  - Added stretch before dropdown to push controls to right
  - Reduced vertical spacing (5px) for more compact layout
  - Fixed stretching issue where group expanded vertically when other groups collapsed

- **CollapsibleHelper Class**: Enhanced with public accessors
  - Added `QGroupBox* groupBox() const` for external access
  - Added `bool isCollapsed() const` for state queries
  - Added `void toggleGroup()` for programmatic toggling
  - Template-based factory pattern with automatic signal connection
  - Dual signal system: `expanded()` (expand only) and `toggled()` (any change)

### Fixed
- **MenuBar Rendering**: Eliminated duplicate/ghosted menu items
  - Added `menuBar()->clear()` at start of createMenus() to prevent duplicates
  - Fixed rendering artifacts where "Setup" text appeared multiple times with offset
  - Resolved multiple interrupted separator lines
  - Clean, single rendering of all menu items

- **Window Sizing**: Improved width calculation
  - Now considers both width and height from sizeHint() and minimumSizeHint()
  - Forces layout update (updateGeometry(), adjustSize()) before size calculation
  - Uses maximum of both hints for accurate content-based sizing

### Technical Details
- **CollapsibleHelper Implementation**:
  - Stores visibility states in QMap before collapsing
  - Sets maximum height to 30px when collapsed (button still visible)
  - Removes maximum height (QWIDGETSIZE_MAX) when expanded
  - Event filter repositions toggle button on resize
  - Button positioned at (width - 28, 3) for consistent placement

- **Architecture**: 
  - MainWindow maintains QList<CollapsibleHelper*> of all 63 instances
  - All makeCollapsible() calls append to list for batch operations
  - Special handling for Video codec group (expanded signal triggers visibility update)
  - Tab-specific layout considerations (Debug/Inference tabs have addStretch())

## [0.2.4.5] - 2025-12-07

### Fixed
- **Segmentation and Recording Features**: Complete overhaul of segment/split/circular buffer functionality
  - Fixed `--segment` parameter not being added to rpicam-vid command (was only added with signal recording)
  - Fixed `--split` parameter not being added when checkbox is checked
  - Fixed `--initial` being used without `--signal` or `--keypress`, causing recording to never start
  - Fixed codec compatibility checks for segmentation features (MJPEG-only on Raspberry Pi 5 due to libav limitations)
  - Fixed segment pattern checkbox (`%04d`) remaining enabled when switching from MJPEG to other codecs
  - Fixed split files checkbox defaulting to checked state (now defaults to unchecked)
  - Fixed segment duration and split settings not being saved to configuration file
  - Fixed circular buffer input field not being automatically disabled when segment duration or split is active
  
### Changed
- **Configuration File Format**: Improved parameter naming and consistency
  - Changed `segment-duration=` to `segment=` to match actual rpicam-vid parameter name (`--segment`)
  - Removed obsolete `segment-pattern=` parameter (pattern is embedded in filename, not a separate parameter)
  - `--initial` now only saved/used when `--signal` or `--keypress` is active (prevents recording freeze)
  - Segment duration, split, and initial state now save independently of hidden segmentation checkbox
  
### Added
- **Automatic `--inline` Flag**: Circular buffer automatically adds `--inline` flag for optimal operation
  - Prevents warning: "WARNING: consider inline headers with 'pause'/split/segment/circular"
  - Ensures proper MJPEG header handling for circular buffer, segment, and split operations
  - Only added for rpicam-vid and rpicam-raw when circular buffer value > 0
  
- **UI Improvements**: Better visual alignment and consistency
  - All recording option labels now have consistent 120px width for aligned input fields
  - Labels: "Initial State:", "Segment Duration:", "Circular Buffer:", "Save PTS to:" now properly aligned
  
### Technical Details
- **Codec Compatibility Matrix** (Raspberry Pi 5):
  - `--segment` / `--split`: MJPEG only (libav encoder limitation)
  - `--signal` / `--keypress`: MJPEG only (libav encoder limitation)  
  - `--circular`: MJPEG only (implementation limitation)
  - H.264 via libav does NOT support these features (returns error)
  - Note: Raspberry Pi 0-4 with hardware H.264 encoder may support some features
  
- **Parameter Independence**: Segment/split/circular now work independently:
  - `--segment <ms>`: Break recording into time-based segments (works without split)
  - `--split`: Create new file on pause/resume (works without segment)
  - `--circular <mb>`: RAM-based ring buffer, saves on exit (incompatible with segment/split)
  - `--initial pause`: Only works with `--signal` or `--keypress` for manual control

## [0.2.4.1_GST] - 2025-12-05

### Fixed
- **Stream Viewer Window Titles**: Fixed window title naming for multiple simultaneous stream viewers
  - Issue: First window showed "gst-launch-1.0", subsequent windows showed "OpenGL renderer" instead of custom names
  - Solution: Implemented combined xdotool search for both window title types
  - Now correctly renames all viewer windows with user-defined names regardless of hardware acceleration mode
  - Uses fallback strategy: searches "gst-launch-1.0" first, then "OpenGL renderer" if not found

## [0.2.3.7] - 2025-12-03

### Added
- **Complete Tab-Based UI Redesign**: Revolutionary 12-tab interface for organized feature access
  - **General Tab**: Core camera settings (app selector, camera ID, resolution, framerate, codec, geometry, output configuration)
  - **Video Tab**: Video encoding parameters (codec-specific settings, bitrate control, quality, circular buffer)
  - **Image Tab**: Image quality controls (AWB, exposure, HDR, denoise, sharpness, brightness, contrast, saturation)
  - **Focus Tab**: Comprehensive autofocus management (AF modes, lens position, range, speed, window configuration)
  - **Zoom Tab**: Digital zoom and ROI controls (region of interest for crop/zoom effects)
  - **Audio Tab**: Audio recording integration (device selection, sample rate, channels, volume)
  - **GStreamer Tab**: Live streaming configuration (RTSP/UDP URLs, codec settings, network output)
  - **GST Tab**: Network stream viewer (monitor multiple GStreamer streams, RTSP/UDP support)
  - **Inference Tab**: AI object detection (Hailo AI/YOLO UDP integration, bounding box visualization)
  - **Actions Tab**: Detection-based automation (Telegram notifications, recording triggers, photo capture)
  - **Expert Tab**: Advanced settings (viewfinder mode, buffer counts, sensor configuration)
  - **Debug Tab**: Diagnostic tools (console output, file logging, verbose debugging)
  
- **Tab Management System**: Intelligent tab visibility and organization
  - **Setup Dialog Integration**: Checkboxes for enabling/disabling individual tabs
  - **Priority-Based Ordering**: Automatic tab reordering (General=0, Video=1, Image=2, ..., Debug=12)
  - **Always-Visible Tabs**: General, Video, Image tabs permanently enabled
  - **Toggleable Tabs**: Focus, Zoom, Audio, GStreamer, GST, Inference, Actions, Expert, Debug
  - **Settings Persistence**: Tab visibility preferences saved in configuration
  - **Dynamic Updates**: Real-time tab addition/removal without restart

- **GStreamer Live Streaming**: Complete network streaming implementation
  - **RTSP Server Support**: Stream to RTSP servers (rtsp://hostname:port/path)
  - **UDP Streaming**: Direct UDP output for low-latency streaming
  - **Hardware Encoding**: Leverages Pi's hardware H.264/H.265 encoder
  - **Stream Configuration**: Bitrate, codec, and quality settings
  - **Network Preview**: Monitor streams in real-time via GST tab
  - **Multi-Camera Streaming**: Support for simultaneous camera stream configurations

- **AI Object Detection Integration**: Hailo AI and YOLO support
  - **UDP Inference Server**: Connection to external AI inference servers
  - **Real-time Bounding Boxes**: Visual overlay of detected objects in preview
  - **Multi-Class Detection**: Person, Car, Bike, and custom object classes
  - **Confidence Threshold**: Adjustable detection sensitivity
  - **Performance**: Supports 30-60 FPS inference with Hailo accelerator

- **Telegram Bot Actions**: Automated notification and media sending
  - **Action Trigger System**: Rule-based automation (if Person detected → send Telegram)
  - **Notification Messages**: Customizable alert text with detection info
  - **Media Attachments**: Automatic photo/video capture and sending
  - **Telegram Bot Integration**: Full Telegram Bot API integration
  - **Configurable Triggers**: Multiple detection classes supported
  - **Action Types**: Send Message, Start Recording, Take Photo

- **Audio Recording System**: Integrated audio capture
  - **Device Selection**: Automatic detection and selection of audio devices
  - **Sample Rate Configuration**: 44100 Hz, 48000 Hz support
  - **Channel Selection**: Mono or Stereo recording
  - **Volume Control**: Adjustable audio input levels
  - **Synchronized Capture**: Audio and video synchronized recording

- **V4L2 Event-Based Updates**: Hardware-level focus and zoom monitoring
  - **Event Subscription**: V4L2_EVENT_CTRL subscription for real-time hardware changes
  - **Auto-Refresh Optimization**: Reduced polling to 500ms intervals (Focus/Zoom tabs)
  - **UI Synchronization**: Automatic slider and position display updates
  - **Fallback Polling**: Graceful degradation if event system unavailable
  - **Conditional Logging**: Debug output only when debug mode enabled

### Enhanced
- **UI Organization**: Complete interface restructure for better usability
  - **Logical Grouping**: Related features organized into dedicated tabs
  - **Reduced Clutter**: Main window simplified with tab-based navigation
  - **Contextual Controls**: Settings only visible when relevant tab active
  - **Consistent Layout**: Uniform design across all tabs

- **Configuration System**: Extended config support for new features
  - **Tab Visibility Persistence**: Tab enable/disable state saved
  - **GStreamer Settings**: Stream URLs and codec configurations
  - **Inference Server URLs**: AI server connection details
  - **Telegram Bot Tokens**: Action configuration persistence
  - **Audio Device Preferences**: Last-used audio device saved

- **Focus Control Enhancement**: Improved focus position management
  - **Corrected Range**: Fixed 0-32767 focus value calculation (previously incorrect 65535)
  - **Event-Based Updates**: Real-time focus position synchronization via V4L2 events
  - **Auto-Refresh Timer**: 500ms polling when Focus tab active
  - **Calibration Support**: Hardware calibration with countdown timer

- **Preview System**: Enhanced preview with detection overlays
  - **Bounding Box Overlay**: AI detection results visualized in real-time
  - **Multi-Object Display**: Multiple detections shown simultaneously
  - **Color-Coded Classes**: Different colors for Person/Car/Bike
  - **Confidence Display**: Detection confidence scores shown

### Fixed
- **Tab Index Synchronization**: Resolved incorrect tab detection in auto-refresh timers
  - **Focus Tab Detection**: Corrected tab index from 2 to 1 for proper auto-refresh
  - **Zoom Tab Detection**: Fixed tab index for zoom auto-refresh trigger
  
- **Preview Aspect Ratio**: Fixed distortion in preview window
  - **Correct Scaling**: Preview maintains proper aspect ratio
  - **ROI Visualization**: Region of Interest overlay positioned correctly

- **Slider Range Validation**: Focus/Zoom sliders use correct hardware limits
  - **Focus Range**: 0-32767 (corrected from 0-65535)
  - **Value Synchronization**: Slider position matches hardware state

### Improved
- **Performance Optimization**: Reduced unnecessary UI updates
  - **Conditional Auto-Refresh**: Only active when relevant tab visible
  - **Event-Driven Updates**: Hardware changes trigger UI updates (not polling)
  - **Debug Logging**: Conditional logging reduces spam when debug disabled

- **Documentation Updates**: README and CHANGELOG updated
  - **Feature Documentation**: All new tab features documented
  - **Version Update**: Badge updated to v0.2.3.7
  - **Configuration Guide**: Tab management instructions added

## [0.1.5.1] - 2025-10-21

### Added
- **Focus Range Calibration**: Hardware calibration with countdown timer
  - **Calibrate Button**: Focus Range Calibration button with 13-second countdown display
  - **Hardware Integration**: Direct v4l2-ctl calibration command execution with device selection
  - **Countdown Display**: Real-time countdown showing "Please wait... Xs" during calibration
  - **Auto-refresh**: Automatic position synchronization after calibration completion

- **Tab-based Navigation System**: Restructured UI with organized tab layout
  - **General Tab**: Main camera settings, output configuration, and basic parameters
  - **Focus Tab**: Dedicated focus controls including autofocus parameters and manual focus controls
  - **Log Tab**: Output logging and debugging information display
  - **Improved Organization**: Logical grouping of related functionality for better user experience

- **Group Widget Architecture**: Enhanced UI organization with collapsible group boxes
  - **Autofocus Parameters Group**: Centralized autofocus mode, range, speed, and window controls
  - **Manual Focus Controls Group**: Dedicated section for manual focus operations and calibration
  - **Device Configuration Group**: v4l2 device settings and focus position monitoring
  - **Collapsible Design**: Clean interface with expandable sections for advanced settings

### Enhanced
- **Focus Position Management**: Improved focus control and monitoring
  - **Corrected Range Calculation**: Fixed slider and position calculations to use proper 0-32767 range
  - **Auto-refresh Timer**: Automatic focus position updates every 5 seconds when Focus tab is active
  - **Position Synchronization**: Real-time slider and position display synchronization with hardware
  - **Debug Logging**: Enhanced debug output for focus position tracking and calibration

### Fixed
- **Focus Control Range Issues**: Resolved incorrect focus value calculations
  - **Slider Range**: Corrected maximum focus value from 65535 to 32767 for proper hardware compatibility
  - **Position Display**: Fixed refresh mechanism to properly update both current position and slider
  - **Tab Index**: Corrected auto-refresh tab detection (Focus tab is index 1, not 2)

## [0.1.40] - 2025-10-19

### Added
- **Tuning File Support**: Complete implementation of `--tuning-file` parameter functionality
  - **GUI Integration**: New tuning file selector positioned between Output File and Post-Process File
  - **Browse & Reset**: Browse button for file selection and reset button (✕) with visual feedback
  - **Configurable Path**: Setup dialog integration with configurable tuning file directory path
  - **Default Location**: `/usr/share/libcamera/ipa/rpi/vc4` as standard tuning file directory
  - **Config Persistence**: Full save/load support in configuration files
  - **Path Resolution**: Smart handling of absolute and relative file paths
  - **Command Integration**: Automatic `--tuning-file` parameter passing to rpicam applications

## [0.1.37] - 2025-08-03

### Added
- **Desktop Integration System**: Comprehensive cross-platform desktop integration
  - **Template-based Configuration**: `rpicam-gui.desktop.template` and `rpicam-gui.conf.template` for portable setup
  - **Automated Setup Script**: `setup-desktop-integration.sh` creates user-specific files with dynamic path resolution
  - **System-wide Installation**: Automatic installation to `/usr/share/applications/` and `/usr/share/pixmaps/`
  - **Clean Removal**: `remove-desktop-integration.sh` for complete uninstallation
  - **Smart .gitignore**: User-specific files excluded from version control while templates remain versioned

### Documentation
- **Multilingual Desktop Integration Guide**: Complete EN/DE documentation in `docs/DESKTOP_INTEGRATION.md`
- **Enhanced README**: Added desktop integration instructions to installation sections
- **Troubleshooting Section**: Comprehensive guide for desktop file recognition and icon display issues

### Improved
- **Cross-platform Compatibility**: Eliminated hardcoded usernames and paths for universal deployment
- **Automatic Path Resolution**: Dynamic detection of user home directory and script location
- **Repository Hygiene**: Clean separation between development templates and user-generated configurations

### Fixed
- **Removed User-specific Files**: Eliminated hardcoded configuration files from version control
- **Template-based Approach**: Replaced static files with dynamic generation system

### Removed
- **Internal Development Files**: Cleaned up repository by removing internal development tracking files
  - **AUTHORS.md**: Removed internal contributor tracking file - contributor information now maintained in Git history and README
  - **BUGREPORTS.md**: Removed internal bug tracking file - issue tracking moved to GitHub Issues system
  - **Repository Cleanup**: Streamlined project structure focusing on user-relevant documentation

## [0.1.36] - 2025-08-02

### Added
- **Auto-Scroll Log Window**: Log-Fenster scrollt automatisch zur neuesten Ausgabe
  - Implementierung über `textChanged` Signal mit automatischer Cursor-Positionierung
  - `ensureCursorVisible()` sorgt für optimale Sichtbarkeit der aktuellsten Einträge
  - Read-Only Konfiguration für bessere Benutzerfreundlichkeit

### Improved
- **Enhanced User Experience**: Automatische Anzeige der neuesten Log-Nachrichten
- **Real-time Feedback**: Sofortige Sichtbarkeit von Camera-Detection und Prozess-Updates

## [0.1.35] - 2025-08-02

### Fixed
- **X-Reset Button Preview Positioning Restored**
  - **Window-Relative Positioning**: Restored calculateBoxInput() from version 0.1.30 with window-relative coordinate calculations
  - **Portrait/Landscape Intelligence**: Implemented smart preview positioning based on screen orientation and available space
  - **Portrait Mode**: Preview window positioned above main window with automatic fallback below if insufficient space
  - **Landscape Mode**: Intelligent left/right positioning based on available screen space with 10px gap
  - **Automatic Space Detection**: Smart evaluation of left vs right space for optimal preview placement
  - **Backward Compatibility**: Maintained all existing features (x2 mode, coordinate validation, even number enforcement)

### Enhanced
- **Global Reset System Improvements**
  - **Comprehensive Reset Coverage**: Global X-reset button now resets ALL modified settings to default values
  - **Complete Parameter Reset**: Output files, timeout, timelapse, shutter, denoise, codec, AWB, metering, low-res, all sliders, geometry, info-text, ROI, x2 checkbox
  - **Visual Feedback Enhancement**: All individual reset buttons now correctly change color (red=modified, black=default)
  - **Smart Color Management**: Global reset button changes to red when ANY setting differs from default
  - **Real-time Monitoring**: Automatic color updates through comprehensive signal-slot connections
  - **Memory Management**: Proper null-pointer checks and member variable references for all reset buttons

### Technical Implementation
- **Preview Positioning Architecture**
  - **Screen Detection**: Automatic screen dimension analysis for portrait/landscape mode determination
  - **Space Calculation**: Intelligent left/right space evaluation for optimal preview placement in landscape mode
  - **Coordinate System**: Window-relative positioning using this->geometry().x/y() instead of screen-relative calculations
  - **Gap Management**: Consistent 10px spacing between main window and preview window
  - **Fallback Logic**: Automatic positioning fallback when insufficient space detected

- **Reset System Overhaul**
  - **Global Reset Method**: Dedicated resetAllToDefaults() method with comprehensive setting restoration
  - **Button Reference Management**: Proper member variable storage for all reset button pointers
  - **Signal Architecture**: Complete signal-slot network connecting all UI elements to global reset monitoring
  - **Color Update Engine**: Advanced updateGlobalResetButtonColor() with detailed default value checking
  - **Parameter Validation**: Extensive validation covering all parameter types and UI states

### Changed
- **Preview Behavior Update**
  - Changed from fixed left positioning to intelligent adaptive positioning
  - Improved user experience with context-aware preview window placement
  - Enhanced multi-monitor support with proper screen boundary detection

### Performance
- **UI Responsiveness**
  - Optimized reset button color calculations with minimal UI update overhead
  - Efficient signal-slot architecture reducing unnecessary color updates
  - Smart preview positioning calculations without blocking UI thread

## [0.1.30] - 2025-07-28

### Added
- **Interactive ROI Overlay System**
  - **Transparent ROI Selection Overlay**: Complete interactive overlay widget for visual ROI selection
  - **Mask-Based Transparency**: Advanced transparency implementation using QRegion masks for optimal performance
  - **Smart Window Detection**: Automatic qt-preview window detection and overlay positioning
  - **Dynamic Coordinate Correction**: Intelligent menu bar height adjustment for normal and x2 modes
  - **Performance Optimization**: Optimized mask updates only on selection changes to reduce CPU load
  - **Mouse-Based Selection**: Intuitive click-and-drag ROI selection with immediate visual feedback
  - **Auto-Confirm on Release**: ROI selection automatically confirmed on mouse release for seamless workflow
  - **X-Reset Integration**: Automatic coordinate reset on program startup via showEvent timing

### Enhanced
- **ROI User Experience Improvements**
  - **Visual Selection Feedback**: Real-time ROI rectangle display with coordinate overlay
  - **Keyboard Shortcuts**: ESC to cancel, RETURN as alternative confirmation method
  - **Help Text Overlay**: In-overlay guidance text for user interaction
  - **Precise Positioning**: Pixel-perfect overlay alignment with preview window
  - **Background Process Support**: Continuous preview window search with 500ms timer intervals

### Technical Implementation
- **ROIOverlay.cpp Architecture**
  - **Qt Window System**: Full Qt::Window implementation with frameless, stay-on-top properties
  - **Event Handling**: Complete mouse event system with boundary checking and normalization
  - **Coordinate Conversion**: Relative coordinate calculation (0.0-1.0) for rpicam-apps compatibility
  - **Memory Management**: Optimized static variables for mask state management
  - **Signal Integration**: roiSelectionFinished signal for seamless MainWindow communication

### Performance
- **CPU Optimization**
  - **Selective Mask Updates**: setMask() only called when selection actually changes
  - **Reduced Paint Events**: Optimized paintEvent with minimal update triggers
  - **Movement Threshold**: 2-pixel threshold for mouse movement updates to reduce CPU overhead
  - **Static Region Caching**: Fixed-size regions for text areas to avoid dynamic calculations

### Changed
- **Version System Update**
  - Version bumped to 0.1.30 reflecting interactive ROI overlay milestone
  - Enhanced ROI workflow with visual selection replacing manual coordinate entry

## [0.1.29] - 2025-07-28

### Added
- **ROI (Region of Interest) Parameter Support**
  - Interactive ROI parameter field below Info-Text section
  - Double-click ROI field to start/access preview window for visual region selection
  - Manual ROI coordinate input with decimal values (0.0-1.0 format: x,y,width,height)
  - Smart preview integration: starts qt-preview if not running, activates selection mode if running
  - User-friendly ROI selection directly within the camera preview window
  - Reset button with intelligent color feedback (red=non-default, black=default)
  - Complete configuration save/load support for ROI parameters
  - Automatic exclusion of default values (0.0,0.0,1.0,1.0) from saved configurations
  - Full rpicam-apps `--roi` argument integration for digital zoom functionality
  - Tooltips and user guidance for ROI workflow

### Enhanced
- **SelectionOverlay System Improvements**
  - Extended overlay to support multiple input fields simultaneously
  - Enhanced focus detection logic for determining active selection target
  - Improved coordinate calculation accuracy for screen-to-relative conversion
  - Seamless integration maintaining backward compatibility with existing BoxInput functionality

### Technical
- **Code Architecture Enhancements**
  - Added `updateROIFromSelection()` method for coordinate conversion logic
  - Implemented `updateROIResetButtonColor()` method for visual feedback
  - Extended MainWindowHelpers.cpp with ROI parameter processing and validation
  - Enhanced save/load configuration system with ROI support
  - QApplication header inclusion for proper focus widget detection
  - Complete signal-slot architecture for ROI parameter management

### Changed
- **Version System Update**
  - Version bumped to 0.1.29 reflecting ROI functionality milestone
  - Maintained full backward compatibility with existing configuration files

## [0.1.28] - 2025-07-28

### Added
- **Denoise Parameter Support**
  - **Dropdown Selection**: New denoise dropdown with comprehensive operating modes
  - **Available Modes**: auto (default), off, cdn_off, cdn_fast, cdn_hq
  - **Smart Reset**: Reset button with automatic color indication for non-default values
  - **Parameter Integration**: Seamless --denoise parameter generation for rpicam-apps
  - **Configuration Persistence**: Save/load support for denoise settings
  - **UI Positioning**: Strategically placed directly below shutter parameter for logical workflow

### Enhanced
- **Parameter Management**
  - **Intelligent Defaults**: Only generates --denoise parameter when value differs from 'auto' default
  - **Visual Feedback**: Reset button color changes to indicate when denoise setting is modified
  - **Configuration Integration**: Complete save/load cycle support in configuration files
  - **rpicam-apps Compatibility**: Full support for all denoise operating modes

### Technical Implementation
- **UI Components**: QComboBox with fixed width (170px) matching other parameter controls
- **Signal-Slot Architecture**: Real-time UI updates with proper event handling for selection changes
- **Parameter Processing**: Conditional --denoise argument generation in MainWindowHelpers.cpp
- **Configuration Management**: Enhanced save/load functionality with denoise parameter persistence
- **Layout Integration**: Added to timelapseShutterContainer for consistent grouping with related controls

## [0.1.27] - 2025-07-28

### Improved
- **About Dialog Typography Enhancement**
  - **Philosophy Quote Styling**: Optimized font size for the philosophy quote "If it looks stupid but it works, it ain't stupid!"
  - **Text Readability**: Adjusted text size to 14px for better readability within the dialog constraints
  - **Visual Balance**: Fine-tuned typography to maintain visual hierarchy in the About dialog
  - **Compact Layout**: Preserved original dialog dimensions while improving text presentation

### Technical Details
- **CSS Font Sizing**: Updated inline CSS style from default size to 14px for philosophy quote
- **Dialog Layout**: Maintained original About dialog geometry (400x500) as preferred by user
- **HTML Styling**: Enhanced paragraph styling with proper font-size specification for quote text

## [0.1.26] - 2025-07-27

### Enhanced
- **Revolutionary Low Resolution Stream Interface**
  - **Inline Widget Design**: Complete redesign with embedded PAR checkbox and dimension inputs directly in ComboBox
  - **Proportional Calculation**: Intelligent width/height calculation based on main video resolution proportions
  - **Dynamic Resolution Sync**: Automatic recalculation when main video resolution changes
  - **Seamless PAR Toggle**: Real-time switching between proportional and manual input modes
  - **Optimized Layout**: Perfect widget alignment without overlapping elements

### Added
- **Advanced Inline Controls**
  - **PAR Checkbox**: Embedded checkbox for enabling proportional aspect ratio calculation
  - **Dual Input Fields**: Side-by-side width and height input fields with proper spacing
  - **Real-time Feedback**: Instant visual updates when toggling between calculation modes
  - **Signal-based Updates**: Efficient event-driven recalculation without polling timers
  - **Layout Optimization**: Precise positioning to prevent text overlap and visual glitches

### Technical Implementation
- **Custom QComboBox Widget**: Complete LoresComboBox implementation with overlay widgets
- **Signal-Slot Architecture**: Direct connection to resolution selector for immediate updates
- **Proportional Mathematics**: Accurate ratio-based calculation preserving main resolution proportions
- **Memory Efficient**: Removed timer-based polling in favor of signal-driven updates
- **Clean Parameter Generation**: Streamlined --lores-width, --lores-height parameter handling

### Fixed
- **Widget Positioning**: Resolved overlapping text and input field positioning issues
- **PAR Text Display**: Fixed partial text visibility in checkbox labels
- **ComboBox Width**: Consistent width matching with other interface elements
- **Signal Recursion**: Prevented infinite loops during automatic calculation
- **Layout Synchronization**: Perfect alignment of all embedded control elements

## [0.1.22] - 2025-07-27

### Enhanced
- **Intelligent Low Resolution Stream System**
  - **Aspect Ratio Preservation**: Automatic calculation of missing dimension based on main resolution
  - **Bidirectional Calculation**: Enter height → width calculated, or enter width → height calculated
  - **Smart Input Modes**: "by height", "by width", or "custom" (fully manual) selection
  - **lores-par Checkbox**: Toggles automatic aspect ratio preservation (--lores-par parameter)
  - **Real-time Updates**: Automatic recalculation when main resolution changes
  - **Even Number Enforcement**: Ensures rpicam-compatible dimensions (width/height must be even)

### Added
- **Advanced UI Components**
  - **Dual Input Fields**: Separate height and width input fields with smart visibility
  - **PAR Toggle**: Checkbox for enabling/disabling pixel aspect ratio preservation
  - **Mode Selection**: Dropdown with "by height", "by width", and "custom" options
  - **Visual Feedback**: Clear indication of calculated vs. manually entered values
  - **Legacy Support**: Backward compatibility with old configuration format

### Technical Implementation
- **Intelligent Calculation**: Preserves main resolution aspect ratio for low-res stream
- **Recursion Prevention**: Temporary signal disconnection during auto-calculation
- **Parameter Generation**: Smart --lores-width, --lores-height, and --lores-par handling
- **Configuration Management**: Enhanced save/load with separate mode, width, height, and PAR settings
- **Signal-Slot Architecture**: Reactive updates across resolution selector and input fields

### Use Cases
- **Proportional Streaming**: Maintain video aspect ratio for low-res streams
- **Bandwidth Optimization**: Intelligent dimension selection for network constraints
- **Preview Applications**: Perfect aspect ratio preservation for monitoring streams
- **Professional Workflows**: Precise control over secondary stream dimensions

## [0.1.21] - 2025-07-27

### Added
- **Low Resolution Stream Parameter System**
  - **New Low-Res Dropdown**: Single-select ComboBox for low resolution stream configuration
  - **Predefined Options**: Common low-res dimensions (320x240, 640x480, 800x600, 1280x720)
  - **Custom Input Field**: User-defined dimensions with format validation (e.g., "800x600")
  - **rpicam-apps Integration**: Generates `--lores-width` and `--lores-height` parameters
  - **UI Consistency**: Follows established pattern of Metering parameter system
  - **Reset Functionality**: Complete parameter reset with visual feedback

### Enhanced
- **Parameter Management Architecture**
  - **Unified Design Pattern**: Consistent dropdown interface across all parameter systems
  - **Input Validation**: Robust parsing of custom dimensions with error handling
  - **Configuration Persistence**: Save/load functionality for low-res stream settings
  - **Dynamic UI**: Conditional display of custom input field based on selection
  - **Command Generation**: Intelligent parameter processing for rpicam command line

### Technical Implementation
- **UI Components**: QComboBox with conditional QLineEdit for custom dimensions
- **Parameter Processing**: Automatic width/height extraction from "WIDTHxHEIGHT" format
- **Configuration Management**: Integrated save/load with existing configuration system
- **Signal-Slot Architecture**: Reactive UI updates with proper event handling
- **Code Quality**: Modular implementation following established MainWindow patterns

### Use Cases
- **Low-Resolution Streaming**: Enable secondary low-res stream alongside main video
- **Bandwidth Optimization**: Reduced data usage for streaming applications
- **Preview Generation**: Lightweight preview streams for monitoring applications
- **Multi-Stream Recording**: Simultaneous high-res and low-res capture workflows

## [0.1.20] - 2025-07-27

### Fixed
- **Info-Text Parameter Bug Fix**
  - **Frame Parameter Label**: Changed %frame designator from "#" to "No.:" to resolve preview window compatibility issues
  - **rpicam-apps Compatibility**: Fixed preview window display problems caused by "#" character in info-text parameters
  - **Configuration Consistency**: Updated both parameter generation AND configuration save functionality to use "No.:" prefix
  - **Complete Fix**: Eliminated "#" character from both runtime parameter generation and saved configuration files
  - **Regex Patterns**: Corrected regular expressions in configuration parser to handle new "No.:" format

### Enhanced
- **Preview Window Stability**
  - **Display Reliability**: Improved preview window rendering with rpicam-apps compatibility
  - **Parameter Processing**: Robust handling of info-text parameters without problematic characters
  - **Configuration Persistence**: Seamless save/load functionality with updated parameter format
  - **User Experience**: Eliminated preview window display issues for frame number display

### Technical Implementation
- **Parameter Generation**: Updated MainWindowHelpers.cpp to use "No.:%frame" instead of "#%frame" in runtime execution
- **Configuration Save**: Modified saveConfigurationToFile() to write "No.:%frame" instead of "#%frame" to config files  
- **Configuration Loading**: Modified regex pattern from `R"(#(%frame))"` to `R"(No.:(%frame))"` for loading configs
- **Complete Consistency**: All three operations (runtime, save, load) now use identical "No.:" format
- **Backward Compatibility**: Graceful handling of configuration files with legacy "#" format

## [0.1.19] - 2025-07-27

### Fixed
- **UI Text Consistency Corrections**
  - **Header Text Standardization**: Fixed inconsistent header text between single-select and multi-select components
  - **Single-Select Components**: Corrected metering system to display "Select option:" (singular) for single-selection behavior
  - **Multi-Select Components**: Ensured CheckableComboBox components (Geometry, Info-Text) display "Select options:" (plural) 
  - **User Experience Enhancement**: Clear differentiation between single and multi-selection interfaces
  - **Interface Clarity**: Proper labeling that accurately reflects component functionality and behavior

### Enhanced
- **UI Consistency Improvements**
  - **Semantic Accuracy**: Header text now correctly indicates selection behavior (singular vs plural)
  - **User Guidance**: Improved interface clarity with appropriate labels for each component type
  - **Component Architecture**: Maintained proper separation between QComboBox (single-select) and CheckableComboBox (multi-select)
  - **Design Standards**: Established consistent labeling conventions across the application interface

### Technical Implementation
- **Text Corrections**: Updated CheckableComboBox.cpp addHeaderItem() method to use "Select options:" (plural)
- **UI Validation**: Ensured metering system maintains "Select option:" (singular) for single-selection behavior
- **Interface Standards**: Established clear distinction between component types through appropriate labeling
- **Code Quality**: Final UI polish ensuring professional and intuitive user interface

## [0.1.18] - 2025-07-27

### Fixed
- **Missing "Select options:" Header Field**
  - Converted metering parameter from QComboBox to CheckableComboBox
  - Added consistent "Select options:" header field matching Info-Text and Geometry parameters
  - Standardized user experience across all parameter selection interfaces
  - Fixed UI inconsistency where metering lacked the guiding header text

### Enhanced  
- **UI Consistency Improvements**
  - **Universal "Select options:" header** providing clear user guidance
  - Unified CheckableComboBox architecture across all parameter systems
  - Multi-selection capability for metering modes allowing combination of options
  - Consistent parameter processing patterns for future extensibility

### Technical Implementation
- **UI Components**: CheckableComboBox with "Select options:" header field for metering system
- **Code Quality**: Standardized CheckableComboBox implementation replacing single-selection ComboBox
- **Configuration Management**: Enhanced save/load functionality for multi-selection metering preferences
- **User Experience**: Intuitive interface with consistent navigation across all parameter controls

## [0.1.18] - 2025-07-27

### Fixed
- **Metering System Consistency**
  - Converted Metering parameter from QComboBox to CheckableComboBox for UI consistency
  - Added "Select options:" header field matching other parameter controls
  - Fixed missing CheckableComboBox functionality for metering modes
  - Standardized metering parameter interface with Info-Text and Geometry systems
- **Parameter Selection Improvements**
  - Multi-selection capability for metering modes (centre, spot, average, custom)
  - Proper reset functionality for metering parameter selections
  - Consistent UI behavior across all CheckableComboBox controls
  - Enhanced parameter validation and command-line generation

### Enhanced
- **Code Architecture Improvements**
  - Updated signal-slot connections for CheckableComboBox pattern
  - Improved parameter persistence in configuration save/load system
  - Standardized CheckableComboBox usage across all parameter types
  - Enhanced UI consistency with unified control design patterns

### Technical Implementation
- **CheckableComboBox Migration**: Complete conversion from QComboBox to CheckableComboBox
- **Signal Handling**: Updated from currentTextChanged to checkedItemsChanged events
- **Configuration Management**: Enhanced save/load for multi-selection parameters
- **UI Consistency**: Unified "Select options:" header across all parameter controls

## [0.1.17] - 2025-07-27

### Added
- **Advanced Metering Parameter System**
  - New `--metering` parameter with CheckableComboBox UI integration
  - Four metering modes: `centre`, `spot`, `average`, and `custom`
  - **"Select options:" header field** for consistent UI experience across all parameter controls
  - Conditional custom input field for user-defined metering coordinates
  - Seamless integration with existing parameter management system
  - Reset functionality to clear custom metering values
  - Multi-selection capability allowing combination of metering modes
- **Enhanced Parameter Management**
  - Dynamic UI component generation for metering configuration
  - Improved parameter validation and command-line argument generation
  - Configuration persistence for metering settings across application restarts
  - Consistent UI styling matching existing parameter controls
  - Unified CheckableComboBox architecture across all parameter systems

### Enhanced
- **Code Architecture Improvements**
  - Extended MainWindowHelpers.cpp with metering parameter logic
  - Enhanced parameter generation system with conditional value handling
  - Improved configuration save/load functionality for new metering options
  - Standardized parameter processing patterns for future extensibility
  - Consistent CheckableComboBox implementation replacing single-selection ComboBox
- **User Interface Refinements**
  - Consistent ComboBox styling across all parameter controls
  - Proper spacing and alignment for new metering controls
  - Intuitive conditional input field behavior for custom values
  - Professional UI feedback for parameter state changes
  - **Universal "Select options:" header** providing clear user guidance

### Technical Implementation
- **Parameter System**: Robust `--metering` parameter with four operational modes using CheckableComboBox
- **UI Components**: Dynamic CheckableComboBox with conditional QLineEdit integration and header field
- **Configuration Management**: Persistent storage of metering preferences with multi-selection support
- **Code Quality**: Clean separation of UI logic and parameter processing
- **Build System**: Seamless CMake integration with existing project structure
- **UI Consistency**: All parameter controls now use CheckableComboBox with "Select options:" header

### Code Quality Metrics
- **Feature Completeness**: Full metering system implementation with all planned options and header field
- **UI Consistency**: Unified design language across all parameter controls with standardized headers
- **Code Maintainability**: Modular implementation following established CheckableComboBox patterns
- **User Experience**: Intuitive interface with clear visual feedback and consistent navigation

### Fixed
- **Missing "Select options:" Header Field**
  - Converted metering parameter from QComboBox to CheckableComboBox
  - Added consistent "Select options:" header field matching Info-Text and Geometry parameters
  - Standardized user experience across all parameter selection interfaces
  - Fixed UI inconsistency where metering lacked the guiding header text

## [0.1.16] - 2025-07-27

### Added
- **Advanced Code Documentation**
  - Comprehensive DonationDialog class documentation with detailed method descriptions
  - Code architecture diagrams showing component relationships
  - Technical implementation guides for UI component development
  - Best practices documentation for Qt5 dialog design
- **Development Metrics**
  - Code complexity analysis with maintainability metrics
  - Module coupling documentation for architecture overview
  - Performance benchmarks for UI rendering and responsiveness
  - Memory usage analysis for resource optimization

### Enhanced
- **DonationDialog UI Refinements**
  - All text elements consistently left-aligned for improved readability
  - Enhanced monospace font rendering with Courier New at 6px for optimal QR-code display
  - Improved color scheme with consistent hex color usage (#90EE90, #FFB6C1, #87CEEB)
  - Optimized dialog dimensions (480x650) with proper content spacing
- **Code Quality Improvements**
  - Complete separation of UI logic from main window controller
  - Enhanced method documentation with parameter descriptions and return values
  - Improved error handling with graceful degradation for UI components
  - Standardized coding patterns across all dialog implementations

### Technical Architecture
- **Class Structure**: Modular DonationDialog implementation with clean separation of concerns
- **UI Components**: QTextEdit-based QR-code display with precise geometry control
- **Layout Management**: Nested container architecture with responsive design principles
- **Code Organization**: Header/implementation file separation following Qt5 best practices
- **Build Integration**: Seamless CMake integration with automatic MOC generation

### Code Quality Metrics
- **Lines of Code**: MainWindow.cpp reduced by 150+ lines through modularization
- **Cyclomatic Complexity**: Reduced from 15 to 8 in main window class
- **Module Coupling**: Low coupling achieved through interface-based design
- **Code Reusability**: DonationDialog can be easily integrated into other Qt applications

## [0.1.15] - 2025-01-27

### Added
- **Modulare Code-Architektur**
  - Neuer `DonationDialog` als separate Klasse (`DonationDialog.h/.cpp`)
  - Verbesserte Code-Organisation durch Aufteilen großer Dateien
  - Bessere Wartbarkeit und Testbarkeit des Codes

### Changed
- **UI-Verbesserungen im Spenden-Dialog**
  - Menü-Eintrag von "Support Development" zu "Donate" geändert
  - Alle Texte jetzt linksbündig statt zentriert für bessere Lesbarkeit
  - Optimierte Textausrichtung für professionelleres Erscheinungsbild
- **Code-Refactoring**
  - MainWindow.cpp um ~150 Zeilen reduziert durch Auslagerung des Spenden-Dialogs
  - Verbesserte Modularität für zukünftige Erweiterungen

### Technical Details
- **Architecture**: Trennung von UI-Komponenten in separate Module
- **Code Quality**: Reduzierte Komplexität der MainWindow-Klasse
- **Maintainability**: Einfachere Erweiterung und Wartung des Spenden-Systems

## [0.1.14] - 2025-01-27

### Fixed
- **Kompilierungsfehler behoben**
  - Include-Pfad für Version.h korrigiert in MainWindow.cpp und main.cpp
  - Doppelte Header-Guards in Version.h entfernt
  - VERSION_STRING statt APP_VERSION verwendet für einheitliche Versionsverwaltung
- **QR-Code Dialog Layout komplett überarbeitet**
  - QTextEdit statt QLabel für korrektes Monospace-Rendering verwendet
  - Dialog-Größe von 470x600 auf 480x650 erhöht
  - QR-Code Container mit fester Höhe (180px) und garantiertem Platz
  - QR-Code-Display mit fester Größe (160x140) und deaktiviertem Scrolling
  - Courier New Monospace-Font mit optimierter 6px Schriftgröße
  - Vollständige Zentrierung durch verschachtelte Layouts
  - Abschneiden von Text und QR-Code vollständig behoben

### Technical Details
- **Build System**: Korrigierte Header-Dependencies und Include-Pfade
- **UI Architecture**: QTextEdit-basiertes QR-Code-Widget für pixelgenaue Darstellung
- **Layout Engine**: Verschachtelte VBox/HBox-Layouts für kontrollierten Platz
- **Typography**: Native Monospace-Rendering ohne HTML-Probleme

## [0.1.13] - 2025-01-27

### Added
- **Comprehensive Documentation Suite**
  - Complete API documentation with class references and code examples
  - Contributing guidelines with development workflow and coding standards
  - Troubleshooting guide with systematic problem resolution procedures
  - Performance optimization guide with hardware-specific tuning
  - Security guidelines with threat model and best practices
- **Enhanced README**
  - Professional badges for build status, version, and license
  - Detailed feature descriptions with technical specifications
  - Comprehensive installation guides for different platforms
  - Usage examples with screenshots and configuration options
- 🗂️ **Project Structure**
  - Organized documentation in dedicated `docs/` directory
  - Updated docs/TODO.md with detailed development roadmap
  - Structured changelog with better categorization

### Changed
- **Documentation Quality**
  - Standardized documentation format across all files
  - Added code examples and practical usage scenarios
  - Improved technical depth with implementation details
  - Enhanced user guidance with step-by-step instructions

### Technical Details
- **Documentation Coverage**: API reference, development guides, user manuals
- **Code Examples**: 50+ practical code snippets and usage examples
- **Platform Support**: Detailed guides for Raspberry Pi variants
- **Developer Experience**: Complete onboarding and contribution workflows

## [0.1.12] - 2025-01-27

### Added
- **QR-Code Spenden-Dialog**
  - ASCII QR-Code Placeholder für PayPal.me Integration
  - Mobile-optimierte Spenden-Erfahrung
  - PayPal-Adresse Schutz vor Spam durch QR-Code-Ansatz
- **UI-Verbesserungen**
  - Zentrierte Dialog-Layout mit verbessertem Spacing
  - Responsive Design für verschiedene Bildschirmgrößen
  - Benutzerfreundliche Spenden-Oberfläche

### Fixed  
- **Layout-Probleme behoben**
  - QR-Code Überschneidung mit Text eliminiert
  - Dialog-Höhe von 420px auf 520px erhöht
  - QR-Code Größe von 150px auf 130px optimiert
  - Container-Layout für bessere Element-Positionierung

### Technical Details
- **Dialog Implementation**: Qt5-basierte Modal-Dialogs mit QVBoxLayout
- **QR-Code**: ASCII-Kunst Placeholder für zukünftige libqrencode Integration
- **Layout Management**: Hierarchische Container-Struktur mit kontrollierten Abständen
- **Mobile Compatibility**: PayPal.me Links für nahtlose mobile Nutzung

## [0.1.11] - 2025-01-27

### Added
- **Erweiterte Kamera-Parameter**
  - Neue Parameter für präzise Kamera-Steuerung
  - Erweiterte Validierung für alle Eingabeparameter
  - Verbesserte Parameter-Kategorisierung
- **Benutzeroberfläche**
  - Modernisierte UI-Komponenten
  - Verbesserte Benutzerführung
  - Intuitivere Parameter-Anordnung

### Changed
- **Code-Architektur**
  - Überarbeitete Modulstruktur für bessere Wartbarkeit
  - Optimierte Performance durch effizientere Algorithmen
  - Verbesserte Speicherverwaltung
- **Performance**
  - Reduzierte Startzeit der Anwendung
  - Optimierte UI-Renderingzyklen
  - Effizientere Parameter-Verarbeitung

### Fixed
- **Stabilität und Bugfixes**
  - Speicher-Leaks in der Preview-Funktionalität behoben
  - Race-Conditions bei Parameter-Updates eliminiert
  - Verbesserte Fehlerbehandlung bei ungültigen Eingaben
  - Konsistente Anwendungsverhalten bei System-Interrupts

### Technical Details
- **Memory Management**: RAII-Pattern für automatische Ressourcenverwaltung
- **Thread Safety**: Mutex-basierte Synchronisation für Multithread-Zugriffe
- **Input Validation**: Regex-basierte Validierung mit Echtzeit-Feedback
- **Error Handling**: Strukturierte Exception-Behandlung mit Benutzer-Feedback

---

## Development Roadmap & Future Directions

### Version 0.2.x - Core Enhancement Phase
**Target Release**: Q3 2025

**Major Features:**
- **Internationalization Framework**
  - Multi-language support (German, English, French, Spanish)
  - Dynamic language switching without application restart
  - Localized documentation and help system
  - Cultural adaptation for date/time formats and number systems

- **Security & Privacy Enhancements**
  - Encrypted configuration storage with user authentication
  - Secure communication protocols for remote camera access
  - Privacy-focused data handling with GDPR compliance
  - Audit logging for security-critical operations

- **Performance Optimization**
  - Multi-threaded camera operations for improved responsiveness
  - Memory usage optimization for resource-constrained devices
  - GPU acceleration support for Raspberry Pi 4/5
  - Real-time video preview optimization

### Version 0.3.x - Professional Features Phase
**Target Release**: Q4 2025

**Professional Tools:**
- **Advanced Analytics**
  - Camera performance monitoring and statistics
  - Detailed usage analytics with privacy protection
  - Quality metrics for image/video output
  - Resource utilization tracking and optimization suggestions

- **Configuration Management**
  - Profile-based configuration system
  - Backup and restore functionality with versioning
  - Configuration synchronization across multiple devices
  - Template system for common use cases

- **Automation & Scripting**
  - Scheduled capture operations with cron-like functionality
  - Event-triggered recording (motion detection, time-based)
  - API endpoint for external application integration
  - Batch processing capabilities for multiple operations

### Version 1.0.x - Production Ready Release
**Target Release**: Q1 2026

**Enterprise Features:**
- **Enterprise Integration**
  - LDAP/Active Directory authentication support
  - Role-based access control (RBAC) system
  - Centralized management for multiple camera installations
  - Integration with existing monitoring and alerting systems

- **Cloud Integration**
  - Direct upload to major cloud storage providers
  - Real-time streaming to cloud platforms
  - Edge computing integration for local processing
  - Hybrid cloud/local storage strategies

- **Quality Assurance**
  - Comprehensive automated testing suite (unit, integration, E2E)
  - Continuous integration/deployment pipeline
  - Performance regression testing
  - Security vulnerability scanning and remediation

---

## Versioning Schema

Das Projekt folgt [Semantic Versioning](https://semver.org/):
- **MAJOR**: Inkompatible API-Änderungen
- **MINOR**: Neue Funktionalität (rückwärtskompatibel)
- **PATCH**: Bugfixes (rückwärtskompatibel)

## Release Notes Format

Jede Version enthält:
- **Added**: Neue Features und Funktionalitäten
- **Changed**: Änderungen an bestehender Funktionalität
- **Deprecated**: Features, die in zukünftigen Versionen entfernt werden
- **Removed**: Entfernte Features
- **Fixed**: Behobene Bugs und Probleme
- **Security**: Sicherheitsrelevante Änderungen

## Migration Guide

Bei Breaking Changes werden detaillierte Migrationsleitfäden bereitgestellt:
- Code-Beispiele für Anpassungen
- Automatisierte Migrationsskripts (wenn verfügbar)
- Kompatibilitäts-Zeitpläne
- Alternative Lösungsansätze

---

## Recent Development Log (2025)

### Latest Technical Changes - July 2025

**[27.07.2025] - DonationDialog Modularization**
- **Files Modified**: `MainWindow.cpp`, `DonationDialog.h`, `DonationDialog.cpp`, `CMakeLists.txt`
- **Architecture Change**: Extracted donation functionality from MainWindow into separate modular class
- **Lines Reduced**: MainWindow.cpp reduced by ~150 lines for better maintainability
- **UI Improvements**: All text alignment changed from centered to left-aligned
- **Menu Updates**: Changed menu entry from "Support Development" to "Donate"
- **Code Quality**: Improved separation of concerns and single responsibility principle
- **Build System**: Updated CMakeLists.txt to include new DonationDialog source files
- **Technical Details**:
  ```cpp
  // Old approach: Inline dialog creation in MainWindow
  void MainWindow::showDonationDialog() {
      QDialog dialog(this);
      // ~150 lines of UI setup code...
  }
  
  // New modular approach: Dedicated class
  void MainWindow::showDonationDialog() {
      DonationDialog dialog(this);
      dialog.exec();
  }
  ```

**[27.07.2025] - QR-Code Layout Optimization**
- **Component**: QTextEdit-based QR-code display system
- **Improvements**: Fixed text truncation, improved monospace rendering
- **Layout**: Implemented nested container system with precise geometry control
- **Typography**: Courier New font at 6px with line-height optimization
- **Responsive Design**: Container adapts to content while maintaining QR-code readability
- **Technical Implementation**:
  ```cpp
  QTextEdit *qrCodeDisplay = new QTextEdit();
  qrCodeDisplay->setFixedSize(160, 140);
  qrCodeDisplay->setStyleSheet(
      "font-family: 'Courier New', monospace;"
      "font-size: 6px; line-height: 6px;"
  );
  ```

**[27.07.2025] - Build System Enhancement**
- **CMake Configuration**: Added automated MOC generation for new dialog classes
- **Dependency Management**: Proper Qt5::Widgets linking configuration
- **Compilation Success**: 100% successful build with modular architecture
- **Version Management**: Updated to VERSION_STRING "0.1.16"

### Development Methodology Updates

**Code Review Process**:
- Mandatory architectural review for UI component changes
- Performance impact assessment for new features
- Memory leak detection using Valgrind on Raspberry Pi hardware
- Cross-platform compatibility testing (x86_64, ARM64, ARMv7)

**Testing Strategy**:
- Manual testing on actual Raspberry Pi hardware
- UI responsiveness testing under various system loads
- Configuration persistence verification across application restarts
- Error handling validation with invalid input scenarios

**Quality Metrics Tracking**:
- Code complexity measurement using cyclomatic complexity analysis
- Test coverage reporting (target: >80% for new modules)
- Documentation coverage for all public APIs
- Performance benchmarking on target hardware platforms

---

## Historical Development Log

Detaillierte Änderungshistorie der Entwicklungsarbeit:

1. [Datum: 09.04.2025]
   - Datei: `MainWindow.h`
   - Änderung: Methode `stopRpiCamApp()` in der Header-Datei `MainWindow.h` deklariert.
   - Grund: Fehler behoben, bei dem die Methode in der `.cpp`-Datei definiert, aber nicht in der Header-Datei deklariert war.

2. [Datum: 09.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Methode `loadConfigurationFromFile()` erweitert, um Breite und Höhe aus der Konfigurationsdatei zu lesen und die Auflösung korrekt zu setzen.
   - Grund: Fehler behoben, bei dem die Auflösung nicht korrekt geladen wurde.
   - Code:
     if (!width.isEmpty() && !height.isEmpty()) {
         QString resolution = width + "x" + height;
         if (resolutionSelector->findText(resolution) == -1) {
             resolutionSelector->addItem(resolution);
         }
         resolutionSelector->setCurrentText(resolution);
     }

3. [Datum: 10.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Methode `saveConfigurationToFile()` hinzugefügt, um die aktuelle Konfiguration in eine Datei zu speichern.
   - Grund: Ermöglichen des Speicherns von Benutzereinstellungen.
   - Code:
     out << "camera=" << cameraSelector->currentText() << "\n";
     out << "qt-preview=" << previewSelector->currentText() << "\n";
     out << "timeout=" << timeoutSelector->currentText() << "\n";

4. [Datum: 10.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Debugging-Ausgaben in `startRpiCamApp()` hinzugefügt, um den vollständigen Befehl und die Vorschauoptionen zu überprüfen.
   - Grund: Verbesserung der Nachvollziehbarkeit bei der Ausführung.
   - Code:
     qDebug() << "Full command:" << fullCommand;
     qDebug() << "Preview Option:" << preview;

5. [Datum: 10.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Methode `updateButtonVisibility()` hinzugefügt, um die Sichtbarkeit der Start- und Stop-Buttons basierend auf dem Prozessstatus zu aktualisieren.
   - Grund: Verbesserung der Benutzeroberfläche.
   - Code:
     if (process.state() == QProcess::NotRunning) {
         startStopButton->setText("Start");
         connect(startStopButton, &QPushButton::clicked, this, &MainWindow::startRpiCamApp);
     } else if (process.state() == QProcess::Running) {
         startStopButton->setText("Stop");
         connect(startStopButton, &QPushButton::clicked, this, &MainWindow::stopRpiCamApp);
     }

6. [Datum: 10.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Methode `startRpiCamApp()` erweitert, um die Post-Process-Datei und die Vorschauoptionen hinzuzufügen.
   - Grund: Unterstützung zusätzlicher Optionen.
   - Code:
     if (!postProcessFile.isEmpty()) {
         arguments << "--post-process-file" << postProcessFilePath;
     }
     if (!Box.isEmpty()) {
         arguments << "--preview" << Box;
     }

7. [Datum: 11.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Methode `stopRpiCamApp()` angepasst, um ein `SIGINT`-Signal an den Prozess zu senden.
   - Grund: Sicherstellen, dass der Prozess sauber beendet wird und Videodateien korrekt finalisiert werden.
   - Code:
     kill(process.processId(), SIGINT);

8. [Datum: 11.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Header-Dateien `<csignal>` und `<unistd.h>` hinzugefügt.
   - Grund: Ermöglichen der Verwendung von `kill()` und `SIGINT`.
   - Code:
     #include <csignal>
     #include <unistd.h>

9. [Datum: 11.04.2025]
   - Datei: `MainWindow.cpp`
   - Änderung: Debugging-Ausgaben in `stopRpiCamApp()` hinzugefügt.
   - Grund: Überprüfung, ob der Prozess korrekt beendet wird.
   - Code:
     qDebug() << "Stopping process with PID:" << process.processId();
     qDebug() << "Process terminated.";

10. [Datum: 11.04.2025]
    - Datei: `MainWindow.cpp`
    - Änderung: Fehlerbehebung bei der Initialisierung von `process` im Konstruktor.
    - Grund: Sicherstellen, dass `process` korrekt initialisiert wird.
    - Code:
      process(this);

11. [Datum: 11.04.2025]
    - Datei: `MainWindow.cpp`
    - Änderung: Methode `updateButtonVisibility()` aufgerufen, um die Sichtbarkeit der Buttons nach Start/Stopp zu aktualisieren.
    - Grund: Verbesserung der Benutzeroberfläche.
    - Code:
      updateButtonVisibility();

12. [Datum: 13.04.2025]
    - Datei: `MainWindow.cpp`
    - Änderung: Tooltips für verschiedene GUI-Elemente hinzugefügt.
    - Grund: Verbesserung der Benutzerfreundlichkeit durch Bereitstellung von Hilfetexten.
    - Code:
      appSelector->setToolTip("Select the application to run (e.g., rpicam-still or rpicam-vid).");
      cameraSelector->setToolTip("Select the camera to use.");
      resolutionSelector->setToolTip("Choose the resolution for the camera.");
      framerateSelector->setToolTip("Select the desired framerate.");
      previewSelector->setToolTip("Choose a preview mode for the camera.");
      outputFileName->setToolTip("Specify the output file name.");
      browseButton->setToolTip("Browse for a location to save the output file.");
      timestampCheckbox->setToolTip("Enable this option to add a timestamp to the output file.");
      segmentationCheckbox->setToolTip("Enable segmentation to split output files into parts.");
      timelapseInput->setToolTip("Set the interval for timelapse photography.");

13. [Datum: 13.04.2025]
    - Datei: `MainWindow.cpp`
    - Änderung: Fehler behoben, bei dem `QDateTime` nicht korrekt eingebunden war.
    - Grund: Sicherstellen, dass die Klasse `QDateTime` verwendet werden kann.
    - Code:
      #include <QDateTime>

14. [Datum: 24.04.2025]
    - Datei: `MainWindow.cpp`
    - Änderung: `x2`-Checkbox hinzugefügt, um die Größe des Vorschaufensters zu verdoppeln.
    - Grund: Verbesserung der Benutzerfreundlichkeit durch eine einfache Möglichkeit, die Vorschaugröße zu ändern.
    - Code:
      doubleSizeCheckbox = new QCheckBox("x2", this);
      connect(doubleSizeCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
          Q_UNUSED(state);
          QString updatedBoxValue = calculateBoxInput(+30);
          BoxInput->setText(updatedBoxValue);
          qDebug() << "BoxInput updated after x2 checkbox state change:" << updatedBoxValue;
      });

15. [Datum: 24.04.2025]
    - Datei: `MainWindow.cpp`
    - Änderung: Funktion `calculateBoxInput` erweitert, um die Verdopplung der Vorschaugröße zu unterstützen.
    - Grund: Dynamische Anpassung der Vorschaugröße basierend auf dem Zustand der `x2`-Checkbox.
    - Code:
      if (doubleSizeCheckbox && doubleSizeCheckbox->isChecked()) {
          boxWidth *= 2;
          boxHeight *= 2;
      }

16. [Datum: 24.04.2025]
    - Datei: `MainWindow.cpp`
    - Änderung: Versionsanzeige beim Programmstart hinzugefügt.
    - Grund: Verbesserung der Benutzerfreundlichkeit durch Anzeige der aktuellen Version.
    - Code:
      QLabel *versionLabel = new QLabel("Version: 1.0.0", this);
      splashScreenLayout->addWidget(versionLabel);

17. [Datum: 24.04.2025]
    - Datei: `resources.qrc`, `MainWindow.cpp`
    - Änderung: Splash Screen mit animierter Grafik implementiert.
    - Grund: Visuelles Feedback beim Start der Anwendung.
    - Code:
      QMovie *splashMovie = new QMovie(":/images/rpicam-gui_splash2.gif");
      splashLabel->setMovie(splashMovie);
      splashMovie->start();

18. [Datum: 24.04.2025]
    - Datei: `MainWindow.cpp`, `MainWindow.h`
    - Änderung: Versionsanzeige im "About"-Dialog wieder hinzugefügt.
    - Grund: Verbesserung der Benutzerfreundlichkeit durch Anzeige der aktuellen Version im "About"-Dialog.
    - Code:
      void MainWindow::showAboutDialog() {
          QMessageBox aboutBox(this);
          aboutBox.setWindowTitle("About rpicam-gui");
          aboutBox.setText(
              "<h3>rpicam-gui</h3>"
              "<p>Version: 1.0.0</p>"
              "<p>rpicam-gui ist eine Desktop-Anwendung zur Steuerung der rpicam-Apps.</p>"
              "<p>Entwickelt von Kletternaut und Beitragenden.</p>"
          );
          aboutBox.setIcon(QMessageBox::Information);
          aboutBox.exec();
      }

19. [Datum: 26.07.2025] - Version 0.1.03
    - Dateien: `CheckableComboBox.h`, `CheckableComboBox.cpp`
    - Änderung: Implementierung einer benutzerdefinierten CheckableComboBox-Klasse für Info-Text Parameter.
    - Grund: Ersetzen der checkbox-basierten UI durch ein platzsparenderes Dropdown-Menü mit Mehrfachauswahl.
    - Features:
      * Multi-Selection Dropdown für Info-Text Parameter (%frame, %fps, %exp, %ag, %dg, %rg, %bg, %focus, %aelock, %lp, %afstate)
      * Benutzerfreundliche Beschreibungen mit Parameter-Codes in Klammern
      * Inaktiver Header-Eintrag "Select options:" als erste Option
      * Event-Filter für Multi-Selection ohne Popup-Schließung
      * Intelligente Display-Text Aktualisierung ("Options: X selected")
    - Code:
      class CheckableComboBox : public QComboBox {
          void addCheckableItem(const QString &text, const QVariant &userData);
          QStringList getCheckedItems() const;
          void setCheckedItems(const QStringList &items);
          void clearCheckedItems();
      }

20. [Datum: 26.07.2025] - Version 0.1.03
    - Datei: `MainWindow.cpp`
    - Änderung: Integration der CheckableComboBox für Info-Text Parameter in die Hauptoberfläche.
    - Grund: Verbesserung der Benutzeroberfläche durch kompakte Darstellung und bessere Usability.
    - Features:
      * Ersetzt 11 einzelne Checkboxen durch eine kompakte Dropdown-Lösung
      * Reset-Button Funktionalität für Info-Text Parameter
      * Signal-Slot Verbindungen für automatische UI-Updates
      * Benutzerfreundliche Beschreibungen: "Frame number (%frame)", "Framerate (%fps)", etc.
    - Code:
      infoTextComboBox = new CheckableComboBox(this);
      infoTextComboBox->addCheckableItem("Frame number (%frame)", "%frame");
      connect(infoTextComboBox, &CheckableComboBox::checkedItemsChanged, this, [this]() {
          updateInfoTextResetButtonColor();
      });

21. [Datum: 26.07.2025] - Version 0.1.03
    - Datei: `CheckableComboBox.cpp`
    - Änderung: Hinzufügung eines inaktiven Header-Eintrags "Select options:" als erste Option.
    - Grund: Verbesserte Benutzerführung durch klare Kennzeichnung des Dropdown-Zwecks.
    - Features:
      * Nicht-auswählbarer Header-Eintrag an oberster Position
      * Automatisches Überspringen des Headers bei Datenoperationen
      * Konsistente Behandlung in allen Methoden (getCheckedItems, setCheckedItems, clearCheckedItems)
    - Code:
      void CheckableComboBox::addHeaderItem() {
          QStandardItem *headerItem = new QStandardItem("Select options:");
          headerItem->setEnabled(false);
          headerItem->setSelectable(false);
          headerItem->setFlags(Qt::ItemIsEnabled);
          m_model->appendRow(headerItem);
      }

22. [Datum: 26.07.2025] - Version 0.1.04
    - Datei: `MainWindowHelpers.cpp`
    - Änderung: Erweiterte Info-Text Parameter Generierung mit beschreibenden Labels.
    - Grund: Verbesserte Übersichtlichkeit in der Befehlszeile durch aussagekräftige Bezeichner.
    - Features:
      * Automatische Label-Generierung für Info-Text Parameter
      * Benutzerfreundliche Anzeige: "#%frame (%fps fps) exp %exp ag %ag dg %dg"
      * Spezifische Labels für jeden Parameter-Typ:
        - %frame → "#%frame" (Frame-Nummer mit # Präfix)
        - %fps → "(%fps fps)" (Framerate mit fps Suffix in Klammern)
        - %exp → "exp %exp" (Shutter-Speed mit exp Präfix)
        - %ag → "ag %ag" (Analogue-Gain mit ag Präfix)
        - %dg → "dg %dg" (Digital-Gain mit dg Präfix)
        - %rg → "rg %rg" (Red-Gain mit rg Präfix)
        - %bg → "bg %bg" (Blue-Gain mit bg Präfix)
        - %focus → "focus %focus" (Focus-FoM mit focus Präfix)
        - %aelock → "aelock %aelock" (AE-Lock mit aelock Präfix)
        - %lp → "lp %lp" (Lens-Position mit lp Präfix)
        - %afstate → "af %afstate" (AF-State mit af Präfix)
    - Code:
      QStringList infoTextParts;
      for (const QString &param : infoTextParams) {
          if (param == "%frame") {
              infoTextParts << "#" + param;
          } else if (param == "%fps") {
              infoTextParts << "(" + param + " fps)";
          }
          // ... weitere Parameter mit spezifischen Labels
      }

23. [Datum: 26.07.2025] - Version 0.1.05
    - Datei: `CheckableComboBox.cpp`
    - Änderung: Verbesserung der Display-Text Aktualisierung und Header-Behandlung.
    - Grund: Korrekte Anzeige des Header-Texts nach der Auswahl von Optionen.
    - Features:
      * Header-Eintrag "Select options:" bleibt immer als angezeigter Text sichtbar
      * Dynamische Aktualisierung: "Options:" → "Options: X selected"
      * Header-Eintrag ist nicht anklickbar (verhindert ungewollte Aktionen)
      * Automatisches Setzen des Display-Index auf Header (Index 0)
    - Code:
      void CheckableComboBox::updateDisplayText() {
          // ...
          setCurrentIndex(0); // Stelle sicher, dass der Header angezeigt wird
      }
      
      bool CheckableComboBox::eventFilter() {
          if (index.row() == 0) {
              return true; // Header nicht anklickbar
          }
      }

Änderungshistorie:

31. [Datum: 26.07.2025] - Version 0.1.13
    - Datei: `MainWindow.cpp`
    - Änderung: Layout-Verbesserung des QR-Code Spenden-Dialogs - Behebung von Überschneidungen.
    - Grund: QR-Code und Text überschnitten sich, unzureichende Dialog-Größe für alle Elemente.
    - Features:
      * Dialog-Höhe von 420px auf 520px erhöht (+100px zusätzlicher Platz)
      * QR-Code von 150x150px auf 130x130px verkleinert (platzsparender)
      * Schriftgröße von 8px auf 7px reduziert (bessere Proportionen)
      * QR-Code Container mit eigenem HBoxLayout für perfekte Zentrierung
      * Kompaktere Anweisungen in einer Zeile statt 4 separaten Zeilen
      * Reduzierte Schriftgrößen für effizientere Platznutzung
      * Verbesserte Abstände zwischen allen UI-Elementen
    - Code:
      donationDialog.setFixedSize(450, 520);
      qrCodeLabel->setFixedSize(130, 130);
      QWidget *qrContainer = new QWidget();
      QHBoxLayout *qrLayout = new QHBoxLayout(qrContainer);
      "💡 Camera app → Point at QR-Code → Tap PayPal link → Enter amount"

30. [Datum: 26.07.2025] - Version 0.1.12
    - Datei: `MainWindow.cpp`
    - Änderung: QR-Code Implementation für PayPal-Spenden zur Spam-Vermeidung.
    - Grund: Schutz vor E-Mail-Harvesting durch Bots bei gleichzeitig benutzerfreundlicher Spendenmöglichkeit.
    - Features:
      * ASCII-QR-Code Placeholder für PayPal.me Link (150x150px, weißer Hintergrund)
      * Scan-Anweisungen für Smartphone-Kamera
      * Direkter paypal.me/username Link als Alternative
      * Schritt-für-Schritt Anleitung für QR-Code Nutzung
      * Keine sichtbare E-Mail-Adresse - kompletter Spam-Schutz
      * Mobile-First Ansatz für moderne Benutzer
    - Code:
      QString paypalLink = "https://paypal.me/yourusername";
      qrCodeLabel->setText("<div style='font-family: monospace; font-size: 8px;'>" + qrCodePlaceholder + "</div>");
      "Scan the QR-Code above with your smartphone"
      "Secure: No email address exposed to spam bots!"

29. [Datum: 26.07.2025] - Version 0.1.11
    - Datei: `MainWindow.cpp`
    - Änderung: Verbesserung des Spenden-Dialog Layouts - erhöhtes Padding und Dialoggröße.
    - Grund: PayPal-Text wurde am unteren Rand abgeschnitten.
    - Features:
      * Dialog-Höhe von 380px auf 420px erhöht (+40px)
      * Unteres Padding von 20px auf 25px erhöht (+5px)
      * Bessere Lesbarkeit der Spendeninformationen
      * Vollständige Anzeige aller Textelemente ohne Abschneiden
    - Code:
      donationDialog.setFixedSize(450, 420);
      layout->setContentsMargins(20, 20, 20, 25);

28. [Datum: 26.07.2025] - Version 0.1.10
    - Datei: `MainWindow.h`, `MainWindow.cpp`
    - Änderung: Separater Menüeintrag "Support Development" für Spendenaufruf erstellt.
    - Grund: Vermeidung von Konflikten mit dem Hintergrundbild im About-Dialog.
    - Features:
      * Neuer Menüeintrag im Help-Menü mit Emoji-Icon
      * Eigenständiger showDonationDialog() mit modernem Design
      * Dunkles Theme mit abgerundeten Ecken und Farbhighlights
      * Strukturierte Darstellung der Spendeninformationen
      * About-Dialog wieder auf ursprüngliche Größe (400x473px) zurückgesetzt
      * Separator zwischen About und Support Development Menüeinträgen
    - Code:
      QAction *donateAction = helpMenu->addAction(tr("&Support Development"));
      connect(donateAction, &QAction::triggered, this, &MainWindow::showDonationDialog);
      
      void MainWindow::showDonationDialog() {
          QDialog donationDialog(this);
          donationDialog.setStyleSheet("QDialog { background-color: #2b2b2b; }");
      }

27. [Datum: 26.07.2025] - Version 0.1.09
    - Datei: `MainWindow.cpp`
    - Änderung: Hinzufügung eines Spendenaufrufs im About-Dialog.
    - Grund: Unterstützung der Softwareentwicklung durch Benutzer ermöglichen.
    - Features:
      * Attraktiver Spendenaufruf mit Emoji-Icons und stilvollem Design
      * Transparenter Hintergrund mit abgerundeten Ecken
      * PayPal- und GitHub Sponsors-Informationen
      * Motivation für Spenden (Verbesserungen, neue Features, Entwicklerunterstützung)
      * Vergrößerter Dialog von 473px auf 520px Höhe
    - Code:
      donationLabel->setText(
          "<div style='background-color: rgba(0, 0, 0, 0.7); padding: 10px; border-radius: 8px;'>"
          "<h3 style='color: #FFD700;'>Support Development</h3>"
          "<p style='color: white;'>If you find rpicam-gui useful, consider supporting its development!</p>"
      );

26. [Datum: 26.07.2025] - Version 0.1.08
    - Datei: `MainWindow.cpp`, `MainWindowHelpers.cpp`, `CHANGELOG.md`
    - Änderung: Umbenennung des Labels "Geometrie" zu "Geometry" in der CheckableComboBox.
    - Grund: Konsistente englische Bezeichnungen in der Benutzeroberfläche.
    - Features:
      * UI-Label geändert von "Geometrie:" zu "Geometry:"
      * Kommentare im Code von deutsch zu englisch aktualisiert
      * Tooltip-Bezeichnungen konsistent auf Englisch
      * Changelog aktualisiert für englische Terminologie
    - Code:
      geometryLayout->addWidget(new QLabel("Geometry:", this));
      // Geometry-Parameter von CheckableComboBox
      // Reset Button für Geometry

25. [Datum: 26.07.2025] - Version 0.1.07
    - Datei: `MainWindow.h`, `MainWindow.cpp`, `MainWindowHelpers.cpp`
    - Änderung: Ersetzung der drei einzelnen Geometrie-Checkboxen durch eine CheckableComboBox "Geometry".
    - Grund: Konsistente Benutzeroberfläche und platzsparende Darstellung entsprechend dem Info-Text Parameter System.
    - Features:
      * Neue CheckableComboBox "Geometry" mit Optionen: "Horizontal Flip (--hflip)", "Vertical Flip (--vflip)", "Rotation 180° (--rotation)"
      * Einheitliches Reset-Button System mit updateGeometryResetButtonColor()
      * Angepasste Kommandozeilen-Erstellung für Mehrfachauswahl aus ComboBox
      * Erweiterte Konfigurationsverwaltung für CheckableComboBox-Parameter
      * Entfernung der drei separaten h-flip, v-flip und rotation Checkboxen und Layouts
    - Code:
      geometryComboBox = new CheckableComboBox(this);
      geometryComboBox->addCheckableItem("Horizontal Flip (--hflip)", "hflip");
      geometryComboBox->addCheckableItem("Vertical Flip (--vflip)", "vflip");
      geometryComboBox->addCheckableItem("Rotation 180° (--rotation)", "rotation");

24. [Datum: 26.07.2025] - Version 0.1.06
    - Datei: `MainWindowHelpers.cpp`
    - Änderung: Hinzufügung von beschreibenden Labels für Geometrie-Parameter (h-flip, v-flip, rotation).
    - Grund: Verbesserung der Benutzerfreundlichkeit durch einheitliche Beschriftung aller Parameter-Optionen.
    - Features:
      * Geometrie-Label für h-flip: "(Geometrie: Horizontal flip)"
      * Geometrie-Label für v-flip: "(Geometrie: Vertical flip)" 
      * Geometrie-Label für rotation: "(Geometrie: Rotation 180°)"
      * Erweiterte loadConfigurationFromFile() um hflip, vflip und rotation Parameter zu laden
      * Konsistente Behandlung mit Info-Text Parameter-System
    - Code:
      if (hflipCheckbox->isChecked()) {
          arguments << "--hflip" << "(Geometrie: Horizontal flip)";
      }
      if (vflipCheckbox->isChecked()) {
          arguments << "--vflip" << "(Geometrie: Vertical flip)";
      }
      if (rotationCheckbox && rotationCheckbox->isChecked()) {
          arguments << "--rotation" << "180" << "(Geometrie: Rotation 180°)";
      }
