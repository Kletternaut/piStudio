#!/bin/bash
# scripts/lint.sh — Static analysis for piStudio
# Runs clang-tidy (required) and cppcheck (optional, if installed).
# Usage: ./scripts/lint.sh [--fix]
#   --fix  Pass to clang-tidy to apply auto-fixes

set -e

FIX=""
if [[ "$1" == "--fix" ]]; then
    FIX="--fix"
fi

BUILD_DIR="$(dirname "$0")/../../build"
SRC_DIR="$(dirname "$0")/../../src"

# ---------------------------------------------------------------------------
# clang-tidy
# ---------------------------------------------------------------------------
if command -v clang-tidy &>/dev/null; then
    echo "[lint] Running clang-tidy..."
    if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
        echo "[lint] ERROR: compile_commands.json not found in $BUILD_DIR"
        echo "[lint] Run: cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .."
        exit 1
    fi
    find "$SRC_DIR" -name "*.cpp" | sort | while read -r f; do
        clang-tidy $FIX -p "$BUILD_DIR" "$f" 2>/dev/null
    done
    echo "[lint] clang-tidy done."
else
    echo "[lint] WARNING: clang-tidy not found. Install with: sudo apt install clang-tidy"
fi

# ---------------------------------------------------------------------------
# clang-format (check only, no auto-fix unless --fix passed)
# ---------------------------------------------------------------------------
if command -v clang-format &>/dev/null; then
    echo "[lint] Checking clang-format..."
    FAILED=0
    find "$SRC_DIR" \( -name "*.cpp" -o -name "*.h" \) | sort | while read -r f; do
        diff <(clang-format "$f") "$f" > /dev/null || { echo "[lint] Format mismatch: $f"; FAILED=1; }
    done
    if [[ "$FIX" == "--fix" ]]; then
        find "$SRC_DIR" \( -name "*.cpp" -o -name "*.h" \) -exec clang-format -i {} \;
        echo "[lint] clang-format: auto-fixed all files."
    elif [ "$FAILED" -eq 0 ]; then
        echo "[lint] clang-format: all files OK."
    else
        echo "[lint] Run './resources/scripts/lint.sh --fix' to auto-format."
        exit 1
    fi
else
    echo "[lint] WARNING: clang-format not found. Install with: sudo apt install clang-format"
fi

# ---------------------------------------------------------------------------
# cppcheck (optional)
# ---------------------------------------------------------------------------
if command -v cppcheck &>/dev/null; then
    echo "[lint] Running cppcheck..."
    cppcheck \
        --std=c++17 \
        --enable=warning,style,performance,portability \
        --suppress=missingIncludeSystem \
        --quiet \
        "$SRC_DIR"
    echo "[lint] cppcheck done."
else
    echo "[lint] INFO: cppcheck not installed (optional). Install with: sudo apt install cppcheck"
fi

echo "[lint] All checks complete."
