# piStudio — Developer Architecture Guide

**Version**: 0.6.0-beta
**Branch**: main
**Last Updated**: August 2026

---

## Overview

piStudio is a Qt5/C++ desktop control center that wraps the
Raspberry Pi camera stack (`rpicam-vid`, `rpicam-still`, `rpicam-jpeg`, `rpicam-raw`) with a tabbed GUI, real-time preview, GStreamer streaming, and Hailo AI object detection. It is 100 % parameter-compatible with rpicam-apps and also drives the rpicam-apps runtime control interface (`rpicam-rt` capability, Kletternaut/rpicam-apps fork, branch `feature/rt-roi`) for live parameter updates.

---

## Source Tree

```
src/
├── main.cpp                    Entry point, splash screen, MainWindow creation
├── Version.h                   Single source of truth for VERSION_STRING
│
├── gui/                        MainWindow and all its split files
│   ├── MainWindow.h/cpp        Central hub (~8600 lines), tab orchestration
│   ├── MainWindowHelpers.cpp   Parameter building, command construction
│   ├── MainWindowCmdHelpers.cpp  GStreamer pipeline building, shell commands
│   ├── MainWindowConfigHelpers.cpp  Save/load config files
│   ├── MainWindowDropdowns.cpp   Reset-button colors, dropdown logic
│   ├── MainWindowAudio.cpp     Audio tab setup
│   ├── MainWindowROI.cpp       Region of interest overlay logic
│   ├── CollapsibleHelper.*     Manages collapsible UI groups (63 groups)
│   ├── GuiSetupDialog.*        Tab visibility + collapsible group config
│   ├── CameraInstance.*        Per-camera state (process, settings group)
│   ├── CameraInstanceManager.* Manages dual-camera instances
│   ├── ResourceBroker.*        Shared resource arbitration between cameras
│   └── ...                     Various widget subclasses (RefreshableComboBox etc.)
│
├── tabs/                       Tab adapter layer — thin wrappers (~50 lines each)
│   ├── ITabPlugin.h            Interface all tabs implement
│   ├── ActionsTab.h/cpp
│   ├── GStreamerTab.h/cpp
│   ├── GstLaunchTab.h/cpp
│   ├── InferenceTab.h/cpp
│   └── ToolsTab.h/cpp
│
├── modules/                    Implementation layer — the actual logic
│   ├── addons/
│   │   ├── actions/            ActionsModule — detection-triggered actions
│   │   ├── odr/                OdrModule — Hailo AI object detection UI
│   │   └── tools/              ToolsModule — video conversion, framerate tools
│   ├── camera/                 V4L2Controller — autofocus via ioctl polling
│   └── streaming/              GStreamerModule, GstLaunchModule
│
├── app/                        Application-level services
│   ├── TabRegistryService.*    Registers tabs and manages tab strip order
│   └── TabVisibilityService.*  Reads QSettings to show/hide tabs
│
├── inference/                  UDP receiver for Hailo detection JSON
├── utils/                      Logging utilities
└── i18n/                       Translation files (EN, DE)
```

---

## Two-Layer Tab Architecture

Feature tabs are split across two layers: **tabs/** and **modules/**.

### Layer 1: `src/tabs/` — Adapters

Each `*Tab` class implements the `ITabPlugin` interface and is owned by `MainWindow`
as a member (e.g. `m_actionsTab`).

```cpp
// ITabPlugin interface (src/tabs/ITabPlugin.h)
class ITabPlugin {
    virtual QString  tabName()     const = 0;   // displayed tab label
    virtual int      tabPriority() const = 0;   // position in tab strip
    virtual QString  settingKey()  const = 0;   // QSettings key for visibility
    virtual void     initialize(tabGroup, registry, helpers, parent) = 0;
    virtual QWidget* tab()         const = 0;   // the actual QWidget
    virtual void     saveSettings(QSettings&)   = 0;
    virtual void     loadSettings(QSettings&)   = 0;
    virtual void     onCameraStarted() {}        // optional hook
    virtual void     onCameraStopped() {}        // optional hook
};
```

A Tab class is intentionally small. It creates the Module, calls `module->setup()`,
and exposes a `module()` accessor for MainWindow to connect signals:

```cpp
// ActionsTab.h — full public API
class ActionsTab : public QObject, public ITabPlugin {
    void setCameraInterface(const CameraInterface &);
    ActionsModule *module() const;   // direct access for signal wiring
};
```

### Layer 2: `src/modules/` — Implementation

Modules contain all the real logic: widget construction, slots, QSettings I/O,
command-line parameter building. They are **not** aware of `ITabPlugin`.

Modules are large (500–800 lines) and intentionally kept separate so they can be
unit-tested or reused independently of the tab infrastructure.

### Data Flow

```
MainWindow
 └─ m_actionsTab->initialize(tabGroup, registry, helpers, parent)
         └─ ActionsTab creates ActionsModule
                └─ actionsModule->setup(tabGroup, helpers, parent)
                         └─ builds all QWidgets, connects internal signals

MainWindow
 └─ m_actionsTab->module()->executeDetection(object, confidence, fullDetection)
         └─ ActionsModule performs the action (plays sound, saves image, etc.)
```

### Mapping: Tab ↔ Module ↔ Feature

| Tab class        | Module class       | Feature                         |
|------------------|--------------------|---------------------------------|
| `ActionsTab`     | `ActionsModule`    | Detection-triggered actions     |
| `GStreamerTab`   | `GStreamerModule`  | GStreamer live streaming        |
| `GstLaunchTab`   | `GstLaunchModule`  | Custom gst-launch pipelines     |
| `InferenceTab`   | `OdrModule`        | Hailo AI object detection UI    |
| `ToolsTab`       | `ToolsModule`      | Video conversion, tools         |

---

## MainWindow Structure

`MainWindow` is the central hub. It is split across several files to keep individual
files manageable:

| File                          | Responsibility                              |
|-------------------------------|---------------------------------------------|
| `MainWindow.cpp`              | setup*, tab creation, V4L2 polling, process |
| `MainWindowHelpers.cpp`       | `buildCommand()`, parameter assembly        |
| `MainWindowCmdHelpers.cpp`    | `buildGStreamerPipeline()`, shell helpers   |
| `MainWindowConfigHelpers.cpp` | `saveConfigurationToFile()`, load/save      |
| `MainWindowDropdowns.cpp`     | Reset-button colors, dropdown change slots  |
| `MainWindowAudio.cpp`         | Audio tab setup                             |
| `MainWindowROI.cpp`           | ROI overlay interaction                     |

### Tab Member Naming

All tab adapter instances follow the `m_*Tab` naming convention:

```cpp
ActionsTab   *m_actionsTab   = nullptr;
GStreamerTab *m_gstreamerTab = nullptr;
GstLaunchTab *m_gstLaunchTab = nullptr;
InferenceTab *m_inferenceTab = nullptr;
ToolsTab     *m_toolsTab     = nullptr;
```

---

## Tab Registration and Visibility

Tabs are registered via `TabRegistryService` during `initialize()`. The service
sorts tabs by `tabPriority()` and adds them to the `QTabWidget`.

`TabVisibilityService` reads `QSettings` to decide whether a tab is shown at all.
The setting key is returned by `ITabPlugin::settingKey()`. An empty key means the
tab is always visible.

Special rule: the **Log tab** has priority `999` — it always appears as the
rightmost tab regardless of which other tabs are enabled.

---

## Config File Architecture (MCIM)

```
piStudio.conf           Global settings + Camera 0 tab state
piStudio_cam0.conf      Camera 0 hardware/capture settings
piStudio_cam1.conf      Camera 1 hardware/capture settings
```

Tab-specific settings are stored under `[Camera0-Tab]` and `[Camera1-Tab]` sections
inside `piStudio.conf`. Always use `AppPaths::globalConf()` rather than hardcoding
file names:

```cpp
QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
settings.beginGroup(m_tabGroup);   // "Camera0-Tab" or "Camera1-Tab"
settings.setValue("MyKey", value);
```

---

## Collapsible UI Groups

The GUI has 63 collapsible groups managed by `CollapsibleHelper`. State is persisted
in QSettings under keys like `UI/Tools/VideoConverterGroup`.

To add a new collapsible group:

```cpp
// In a Module::setup() or MainWindow::setup*():
auto *helper = new CollapsibleHelper(groupBox, this);
helper->makeCollapsible(groupBox, "UI/MySection/MyGroup");
allCollapsibleHelpers.append(helper);
```

---

## Build

```bash
cd build
cmake ..
make -j4
./piStudio
```

Check only for errors and warnings:

```bash
make -j4 2>&1 | grep -E "error:|warning:"
```

---

## Version Bumping

**Rule: always bump the version BEFORE implementing a fix or feature.**

1. Edit `src/Version.h`: increment `VERSION_STRING` (e.g. `0.4.03` → `0.4.04`)
2. Add a section to `CHANGELOG.md`
3. Commit and push

---

## Adding a New Feature Tab

1. Create `src/modules/addons/mytab/MyModule.h/cpp` with the widget logic.
2. Create `src/tabs/MyTab.h/cpp` implementing `ITabPlugin`, instantiating `MyModule`.
3. Add both files to `SOURCES` and `HEADERS` in `CMakeLists.txt`.
4. Add `src/tabs` is already in `target_include_directories` — no change needed.
5. In `MainWindow.h`: add `MyTab *m_myTab = nullptr;` and `#include "../tabs/MyTab.h"`.
6. In `MainWindow.cpp`: add `setupMyTab()` and call it from `setupTabs()`.
