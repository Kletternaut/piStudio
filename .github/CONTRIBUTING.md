# Contributing to piStudio

Thank you for your interest in contributing! Contributions of all kinds are welcome — bug reports, feature requests, documentation improvements, and code changes.

---

## Prerequisites

### Build dependencies

```bash
sudo apt install \
    qt5-qmake qtbase5-dev qtbase5-dev-tools \
    libqt5x11extras5-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    cmake make g++
```

### Recommended toolchain

- GCC 10+ or Clang 14+
- CMake 3.16+
- Qt 5.15+

---

## Building the project

```bash
git clone https://github.com/Kletternaut/piStudio.git
cd piStudio
mkdir build-mcim && cd build-mcim
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j4
./piStudio
```

> `CMAKE_EXPORT_COMPILE_COMMANDS=ON` is required for clang-tidy to work correctly.

---

## Code style

The project uses **C++17** with the following conventions:

- **Indentation:** 4 spaces (no tabs)
- **Braces:** Stroustrup — opening brace on a new line for functions and classes, same line for `if`/`for`/`while`
- **Pointer style:** `auto *ptr` (left-aligned `*`)
- **Line length:** 120 characters max

A `.clang-format` file in the repository root encodes these rules.

### Auto-format a file

```bash
clang-format -i src/gui/MainWindow.cpp
```

### Format all source files

```bash
pre-commit run clang-format --all-files
# or without pre-commit:
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### Install clang-format (if missing)

```bash
sudo apt install clang-format
```

---

## Static analysis

### clang-tidy (installed)

```bash
# Single file
clang-tidy -p build-mcim src/gui/MainWindow.cpp

# All files via lint script
./resources/scripts/lint.sh

# With auto-fix
./resources/scripts/lint.sh --fix
```

### cppcheck (optional)

```bash
sudo apt install cppcheck
cppcheck --std=c++17 --enable=warning,style,performance --quiet src/
```

---

## pre-commit hooks

pre-commit runs clang-format, clang-tidy, and basic file checks automatically before every commit.

```bash
pip install pre-commit
pre-commit install
```

Run manually on all files:

```bash
pre-commit run --all-files
```

---

## Pull request checklist

Before opening a pull request, make sure:

- [ ] Code compiles without errors or warnings (`make -j4`)
- [ ] All modified files are formatted (`clang-format` or `pre-commit`)
- [ ] No new clang-tidy warnings introduced
- [ ] Commit messages are in English and follow the format:
  ```
  [TYPE] Short description (50 chars max)
  ```
  Types: `FEATURE`, `FIX`, `REFACTOR`, `DOCS`, `VERSION`

---

## License

By contributing to this project, you agree that your contributions will be licensed under the **PolyForm Noncommercial License 1.0.0**, the same license as the project.
