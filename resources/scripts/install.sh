#!/bin/bash
# resources/scripts/install.sh — Build and install the application
#
# Usage:
#   ./resources/scripts/install.sh             (build + install to /usr/local)
#   ./resources/scripts/install.sh --prefix /usr   (install to /usr instead)
#   ./resources/scripts/install.sh --uninstall     (remove installed files)
#   ./resources/scripts/install.sh --deb           (build + create .deb package)
#
# Requires: sudo (for --uninstall and system-wide install)

set -e

# Product name — keep in sync with src/app/AppMeta.h and CMakeLists.txt
APP_NAME="piStudio"
APP_BINARY="piStudio"

REPO_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$REPO_DIR/build"
PREFIX="/usr/local"

# ── Argument parsing ─────────────────────────────────────────────
UNINSTALL=0
DEB_ONLY=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix)    PREFIX="$2"; shift 2 ;;
        --uninstall) UNINSTALL=1; shift ;;
        --deb)       DEB_ONLY=1; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ── Uninstall ────────────────────────────────────────────────────
if [[ "$UNINSTALL" -eq 1 ]]; then
    echo "[install] Removing ${APP_NAME} from $PREFIX ..."
    sudo rm -f "$PREFIX/bin/${APP_BINARY}"
    sudo rm -f "$PREFIX/bin/udp_object_detection"
    sudo rm -f "$PREFIX/share/applications/${APP_BINARY}.desktop"
    sudo rm -f "$PREFIX/share/icons/hicolor/scalable/apps/${APP_BINARY}.svg"
    for size in 16 22 24 32 48 64 128 256 512; do
        sudo rm -f "$PREFIX/share/icons/hicolor/${size}x${size}/apps/${APP_BINARY}.png"
    done
    sudo rm -rf "$PREFIX/share/${APP_BINARY}"
    sudo update-icon-caches "$PREFIX/share/icons/hicolor" 2>/dev/null || true
    sudo update-desktop-database -q "$PREFIX/share/applications" 2>/dev/null || true
    echo "[install] Uninstall complete."
    exit 0
fi

# ── Dependencies check ───────────────────────────────────────────
echo "[install] Checking dependencies..."
MISSING=()
for pkg in cmake make g++ qtbase5-dev libqt5x11extras5-dev; do
    if ! dpkg -s "$pkg" &>/dev/null; then
        MISSING+=("$pkg")
    fi
done
if [[ ${#MISSING[@]} -gt 0 ]]; then
    echo "[install] Installing missing packages: ${MISSING[*]}"
    sudo apt-get install -y "${MISSING[@]}"
fi

# ── Build ────────────────────────────────────────────────────────
echo "[install] Configuring build in $BUILD_DIR ..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "[install] Building ($(nproc) jobs)..."
make -j"$(nproc)"
# ── DEB package ───────────────────────────────────────
if [[ "$DEB_ONLY" -eq 1 ]]; then
    echo "[install] Creating .deb package..."
    cpack
    DEB_FILE=$(ls "$BUILD_DIR"/*.deb 2>/dev/null | head -1)
    echo ""
    echo "[install] Package created: $DEB_FILE"
    echo "[install] Install with: sudo dpkg -i $DEB_FILE"
    exit 0
fi
# ── Install ──────────────────────────────────────────────────────
echo "[install] Installing to $PREFIX ..."
sudo make install

echo "[install] Refreshing icon cache and desktop database..."
sudo update-icon-caches "$PREFIX/share/icons/hicolor" 2>/dev/null || true
sudo update-desktop-database -q "$PREFIX/share/applications" 2>/dev/null || true

echo ""
echo "[install] Done. Run '${APP_BINARY}' from terminal or find it in your application menu."
