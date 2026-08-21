#ifndef VERSION_H
#define VERSION_H

// piStudio Version Information
// VERSION_STRING is apt/CPack compatible: [0-9]+\.[0-9]+\.[0-9]+ with an
// optional Debian suffix (e.g. "0.6.0-beta"). The suffix appears in the UI
// display and in the .deb package name; CMake uses only the numeric part.
#define VERSION_STRING "0.7.3"
#define VERSION_TEXT "PolyForm Noncommercial license"

// Build information
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Application metadata (product name, binary name, repository URL, ...)
// lives in src/app/AppMeta.h - single source of truth, never hardcode it.

#endif // VERSION_H
