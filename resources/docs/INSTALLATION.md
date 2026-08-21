# Installation Guide - piStudio

## Option 1: Install via .deb Package (Easiest)

```bash
# 1. Download the latest package
wget -P /tmp https://github.com/Kletternaut/piStudio/releases/download/v0.7.3/piStudio_0.7.3_arm64.deb

# 2. Install with automatic dependency resolution
sudo apt install /tmp/piStudio_0.7.3_arm64.deb
```

> `apt install ./...` automatically installs all required dependencies.
> Do NOT use `dpkg -i` — it does not resolve dependencies.

After installation, launch from the application menu or run:

```bash
piStudio
```

---

## Option 2: Build from Source (Quick)

```bash
# 1. Install build dependencies
sudo apt install -y qtbase5-dev qttools5-dev-tools libqt5widgets5 \
                    libqt5x11extras5-dev \
                    cmake build-essential git \
                    rpicam-apps xdotool

# 2. Clone and build
git clone https://github.com/Kletternaut/piStudio.git
cd piStudio
mkdir build && cd build
cmake ..
make -j$(nproc)

# 3. Run
./piStudio
```

---

## Option 3: Detailed Build from Source

### Prerequisites

**Required System:**
- Raspberry Pi OS (Bookworm or newer)
- Raspberry Pi 3B+ or newer (recommended: Pi 4B/5)
- Camera Module enabled
- At least 1GB free space

**Required Dependencies:**
- `qtbase5-dev` - Qt5 development framework
- `libqt5x11extras5-dev` - Qt5 X11 Extras (for screen selection overlay)
- `cmake`, `build-essential` - Build tools
- `rpicam-apps` - Raspberry Pi camera applications

#### 1. Install Dependencies

```bash
sudo apt update
sudo apt install -y qtbase5-dev qttools5-dev-tools libqt5widgets5 \
                    libqt5x11extras5-dev \
                    cmake build-essential git \
                    rpicam-apps
```

#### 2. Optional Runtime Dependencies

Some features need additional packages:

| Feature | Package(s) |
|---|---|
| GStreamer streaming | `gstreamer1.0-tools gstreamer1.0-plugins-ugly` |
| Video conversion (ffmpeg) | `ffmpeg` |
| V4L2 autofocus helpers | `v4l-utils` |
| Audio recording | `pulseaudio-utils alsa-utils` |
| ROI screen selection | `xdotool` |

Install all at once:
```bash
sudo apt install -y gstreamer1.0-tools gstreamer1.0-plugins-ugly \
                    v4l-utils ffmpeg pulseaudio-utils alsa-utils xdotool
```

#### 3. Clone Repository
```bash
git clone https://github.com/Kletternaut/piStudio.git
cd piStudio
```

#### 4. Build Application
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Compile (use all CPU cores)
make -j$(nproc)
```

#### 5. Verify Installation
```bash
# Test camera functionality
rpicam-hello --list-cameras

# Run piStudio
./piStudio
```

---

## Optional: System-wide Installation

Install piStudio system-wide with proper icon and desktop integration:

```bash
# From project root directory
resources/scripts/install.sh            # builds + make install to /usr/local
# or
resources/scripts/install.sh --deb      # builds + creates the .deb package
```

`make install` installs the binary to `/usr/local/bin`, translations to
`/usr/local/share/piStudio`, icons to the hicolor theme and the desktop entry to
`/usr/local/share/applications`. Icon cache and desktop database are refreshed
automatically.

---

## Troubleshooting

### Camera Not Detected
```bash
# Check camera connection
rpicam-hello --list-cameras
# If nothing is listed, check hardware connection and enable in raspi-config
```

### Qt Build Errors
```bash
# Install missing Qt5 development packages
sudo apt install -y qtbase5-dev-tools qt5-qmake

# Check Qt version
qmake --version
```

### Permission Issues
```bash
# Add user to video group
sudo usermod -a -G video $USER

# Logout and login again, or reboot
sudo reboot
```

### CMake Configuration Issues
```bash
# Ensure minimum CMake version (3.16+)
cmake --version

# Clear build directory and retry
cd piStudio
rm -rf build/
mkdir build && cd build
cmake ..
```

## Project Structure After Build

```
piStudio/
├── build/
│   ├── piStudio          # Main executable
│   ├── udp_object_detection # UDP detection receiver
│   ├── CMakeCache.txt
│   └── ...
├── src/                     # Source code
├── resources/
│   ├── docs/                # Documentation
│   ├── icons/               # Icon PNGs (16-512 px)
│   ├── images/              # SVG logo, splash, screenshots
│   └── scripts/             # install.sh, lint.sh, diagnostics
└── CMakeLists.txt
```

## Build Options

### Debug Build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Release Build (Optimized)
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Custom Install Prefix
```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/piStudio ..
make -j$(nproc)
sudo make install
```

## Update piStudio

- **.deb install:** use the built-in update check (Help → Check for Updates) or download
  and install the newest .deb as described in Option 1.
- **Source build:** pull the latest `main` branch and rebuild:
  ```bash
  cd piStudio
  git pull origin main
  cd build
  make -j$(nproc)
  ```

## Uninstall

### Local Installation (source build, run from build/)
```bash
# Simply remove the directory
rm -rf piStudio/
```

### System-wide Installation (/usr/local)
```bash
resources/scripts/install.sh --uninstall
```

### .deb Installation
```bash
sudo apt remove piStudio
```

## System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| **CPU** | ARM Cortex-A72 (Pi 4B) | Pi 5 |
| **RAM** | 1GB | 2GB+ |
| **Storage** | 500MB free | 1GB+ free |
| **OS** | Pi OS Bookworm / Trixie | Pi OS Trixie |
| **Qt** | 5.15+ | 5.15+ |
| **CMake** | 3.16+ | 3.20+ |

## First Run Checklist

1. [ ] Camera detected: `rpicam-hello --list-cameras`
2. [ ] piStudio starts without errors
3. [ ] Preview window shows camera feed
4. [ ] Can capture test photo/video
5. [ ] Settings can be saved/loaded

---

**Need help?** Open an issue on [GitHub](https://github.com/Kletternaut/piStudio/issues)
