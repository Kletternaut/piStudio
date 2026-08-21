cmake_minimum_required(VERSION 3.16)

# Read version from Version.h — single source of truth
# VERSION_STRING may contain an apt/CPack suffix (e.g. "0.5.8-beta").
# The numeric part is extracted for project(); the full string is used for
# CPack and the .deb filename.
file(STRINGS "${CMAKE_SOURCE_DIR}/src/Version.h" _version_line
     REGEX "#define VERSION_STRING")
string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" VERSION_STRING "${_version_line}")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" VERSION_NUM "${VERSION_STRING}")

# Product name variables — keep in sync with src/app/AppMeta.h
set(APP_NAME "piStudio")      # display name
set(APP_BINARY "piStudio")    # executable / target name
set(APP_PACKAGE "piStudio")   # .deb package name
set(APP_REPO_OWNER "Kletternaut")
set(APP_REPO "piStudio")

project(${APP_PACKAGE} VERSION ${VERSION_NUM})

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type
set(CMAKE_BUILD_TYPE Debug)

# Include directories
include_directories(src/controllers src/models src/utils src/gui src/app src/core src/modules)

# Enable automatic MOC generation
set(CMAKE_AUTOMOC ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Network X11Extras LinguistTools)

# Find X11
find_package(X11 REQUIRED)

# Add source files
set(SOURCES
    src/app/TabRegistryService.cpp
    src/app/TabVisibilityService.cpp
    src/modules/control/ControlSocketClient.cpp
    src/modules/update/UpdateChecker.cpp
    src/modules/addons/tools/ToolsModule.cpp
    src/modules/addons/odr/OdrModule.cpp
    src/modules/camera/V4L2Controller.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindowHelpers.cpp
    src/gui/MainWindowCmdHelpers.cpp
    src/gui/MainWindowConfigHelpers.cpp
    src/gui/MainWindowAudio.cpp
    src/gui/MainWindowROI.cpp
    src/gui/MainWindowDropdowns.cpp
    src/gui/HelpDialog.cpp
    src/gui/SelectionOverlay.cpp
    src/gui/ROIOverlay.cpp
    src/gui/GuiSetupDialog.cpp
    src/gui/CollapsibleGroupBox.cpp
    src/gui/CollapsibleHelper.cpp
    src/gui/CheckableComboBox.cpp
    src/gui/LoresComboBox.cpp
    src/gui/RefreshableComboBox.cpp
    src/gui/DonationDialog.cpp
    src/utils/DebugLogger.cpp
    src/gui/V4L2EventWatcher.cpp
    src/gui/V4L2Helpers.cpp
    src/gui/ResourceBroker.cpp
    src/gui/CameraInstance.cpp
    src/gui/CameraInstanceManager.cpp
    src/gui/LogWindow.cpp
    src/gui/UpdateDialog.cpp
    src/gui/ProfileDialog.cpp
    src/modules/streaming/GStreamerModule.cpp
    src/modules/streaming/GstLaunchModule.cpp
    src/modules/addons/actions/ActionsModule.cpp
    src/tabs/ActionsTab.cpp
    src/tabs/GStreamerTab.cpp
    src/tabs/GstLaunchTab.cpp
    src/tabs/InferenceTab.cpp
    src/tabs/ToolsTab.cpp
    src/main.cpp
)

set(HEADERS
    src/modules/streaming/GStreamerModule.h
    src/modules/streaming/GstLaunchModule.h
    src/modules/control/ControlSocketClient.h
    src/modules/update/UpdateChecker.h
    src/modules/addons/actions/ActionsModule.h
    src/tabs/ITabPlugin.h
    src/tabs/ActionsTab.h
    src/tabs/GStreamerTab.h
    src/tabs/GstLaunchTab.h
    src/tabs/InferenceTab.h
    src/tabs/ToolsTab.h
    src/app/TabRegistryService.h
    src/app/TabVisibilityService.h
    src/gui/MainWindow.h
    src/gui/HelpDialog.h
    src/gui/ROIOverlay.h
    src/gui/CustomLineEdit.h
    src/gui/GuiSetupDialog.h
    src/gui/CollapsibleHelper.h
    src/gui/LoresComboBox.h
    src/gui/RefreshableComboBox.h
    src/gui/DonationDialog.h
    src/utils/DebugLogger.h
    src/Version.h
    src/gui/V4L2EventWatcher.h
    src/gui/V4L2Helpers.h
    src/gui/ResourceBroker.h
    src/gui/CameraInstance.h
    src/gui/CameraInstanceManager.h
    src/gui/LogWindow.h
    src/gui/UpdateDialog.h
    src/gui/ProfileDialog.h
)

qt5_add_resources(QT_RESOURCES resources/resources.qrc)

# Translation files
set(TS_FILES
    src/i18n/piStudio_de.ts
    src/i18n/piStudio_en.ts
)
qt5_add_translation(QM_FILES ${TS_FILES})

add_executable(${APP_BINARY} ${SOURCES} ${HEADERS} ${QT_RESOURCES} ${QM_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network Qt5::X11Extras ${X11_LIBRARIES})

# Copy translation .qm files next to the executable
add_custom_command(TARGET ${APP_BINARY} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/i18n
    COMMAND ${CMAKE_COMMAND} -E copy ${QM_FILES} ${CMAKE_CURRENT_BINARY_DIR}/
    COMMENT "Copying translation files to build directory"
)

target_include_directories(${PROJECT_NAME} PRIVATE src/gui src/utils src/app src/tabs src/modules src/modules/control src/modules/addons/tools src/modules/addons/odr src/modules/addons/actions src/modules/camera src/modules/streaming src/modules/update ${X11_INCLUDE_DIR})

# Build UDP Object Detection receiver (C program)
add_executable(udp_object_detection src/inference/udp_object_detection.c)

# Install both executables
install(TARGETS ${APP_BINARY} udp_object_detection DESTINATION bin)

# Install translation files
install(FILES ${QM_FILES} DESTINATION share/${APP_BINARY})

# Install icons — SVG (scalable) + pre-generated PNG set
install(FILES resources/images/${APP_BINARY}.svg
    DESTINATION share/icons/hicolor/scalable/apps)
foreach(_size 16 22 24 32 48 64 128 256 512)
    install(FILES resources/icons/${_size}x${_size}/${APP_BINARY}.png
        DESTINATION share/icons/hicolor/${_size}x${_size}/apps)
endforeach()

# Install desktop entry (generate from template, product names substituted)
configure_file(resources/scripts/${APP_BINARY}.desktop.template
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_BINARY}.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${APP_BINARY}.desktop
    DESTINATION share/applications)

# ─── CPack / DEB ────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "${APP_PACKAGE}")
# Full version incl. Debian suffix (e.g. "0.6.0-beta") – apt/CPack-compatible.
set(CPACK_PACKAGE_VERSION "${VERSION_STRING}")
set(CPACK_PACKAGE_CONTACT "https://github.com/${APP_REPO_OWNER}/${APP_REPO}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Graphical parameter configurator for rpicam-apps (Raspberry Pi Camera)")
set(CPACK_PACKAGE_DESCRIPTION
    "${APP_NAME} wraps rpicam-vid, rpicam-still, rpicam-jpeg and rpicam-raw\nwith a tabbed GUI, real-time preview, GStreamer streaming and AI object\ndetection on Raspberry Pi.")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
set(CPACK_DEBIAN_PACKAGE_SECTION "video")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt5widgets5 (>= 5.15), libqt5network5 (>= 5.15), libqt5x11extras5 (>= 5.15)")
# NOTE: rpicam-apps is deliberately NOT a hard dependency.
# The app never links against rpicam-apps — it only invokes the binaries
# (rpicam-vid, rpicam-still, ...) via QProcess. Users who build rpicam-apps
# from source (e.g. the Kletternaut fork) have no apt package, and a hard
# Depends would force apt to install the stock package alongside their
# custom build. Missing binaries are reported at startup instead.
# Replaces the predecessor packages rpicam-gui (renamed in 0.6.0) and
# rpicam-ctrl (renamed in 0.7.0 to piStudio).
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "rpicam-gui, rpicam-ctrl")
set(CPACK_DEBIAN_PACKAGE_REPLACES "rpicam-gui, rpicam-ctrl")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kletternaut <https://github.com/${APP_REPO_OWNER}/${APP_REPO}>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/${APP_REPO_OWNER}/${APP_REPO}")

# Post-install: remove leftovers of the predecessor packages rpicam-gui
# and rpicam-ctrl, then refresh icon cache and .desktop DB
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/postinst
"#!/bin/sh\nset -e\nrm -f /usr/bin/rpicam-gui\nrm -f /usr/share/applications/rpicam-gui.desktop\nrm -f /usr/share/icons/hicolor/scalable/apps/rpicam-gui.svg\nfor _size in 16 22 24 32 48 64 128 256 512; do\n  rm -f \"/usr/share/icons/hicolor/\${_size}x\${_size}/apps/rpicam-gui.png\"\ndone\nrm -rf /usr/share/rpicam-gui\nrm -f /usr/bin/rpicam-ctrl\nrm -f /usr/share/applications/rpicam-ctrl.desktop\nrm -f /usr/share/icons/hicolor/scalable/apps/rpicam-ctrl.svg\nfor _size in 16 22 24 32 48 64 128 256 512; do\n  rm -f \"/usr/share/icons/hicolor/\${_size}x\${_size}/apps/rpicam-ctrl.png\"\ndone\nrm -rf /usr/share/rpicam-ctrl\nif command -v update-icon-caches >/dev/null 2>&1; then\n  update-icon-caches /usr/share/icons/hicolor\nfi\nif command -v update-desktop-database >/dev/null 2>&1; then\n  update-desktop-database -q /usr/share/applications\nfi\n")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/postinst")

set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

include(CPack)
