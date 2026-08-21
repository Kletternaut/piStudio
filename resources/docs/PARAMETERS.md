# rpicam-apps Parameter Reference

All parameters supported by `rpicam-vid`, `rpicam-still`, `rpicam-raw`, `rpicam-jpeg`, and `rpicam-hello`.  
Click a parameter name to open the official Raspberry Pi documentation.

---

## Common

| Parameter | Description |
|---|---|
| [--timeout](https://www.raspberrypi.com/documentation/computers/camera_software.html#timeout) | Time for which the program runs (milliseconds) |
| [--preview](https://www.raspberrypi.com/documentation/computers/camera_software.html#preview) | Preview window dimensions (x,y,w,h) |
| [--fullscreen](https://www.raspberrypi.com/documentation/computers/camera_software.html#fullscreen) | Force fullscreen preview mode |
| [--qt-preview](https://www.raspberrypi.com/documentation/computers/camera_software.html#qt-preview) | Use Qt-based preview window |
| [--nopreview](https://www.raspberrypi.com/documentation/computers/camera_software.html#nopreview) | Disable preview window |
| [--info-text](https://www.raspberrypi.com/documentation/computers/camera_software.html#info-text) | Show info text overlay on preview |
| [--width / --height](https://www.raspberrypi.com/documentation/computers/camera_software.html#width-and-height) | Capture image width/height in pixels |
| [--viewfinder-width / --viewfinder-height](https://www.raspberrypi.com/documentation/computers/camera_software.html#viewfinder-width-and-viewfinder-height) | Viewfinder width/height in pixels |
| [--tuning-file](https://www.raspberrypi.com/documentation/computers/camera_software.html#tuning-file) | Camera tuning file (JSON format) |
| [--lores-width / --lores-height](https://www.raspberrypi.com/documentation/computers/camera_software.html#lores-width-and-lores-height) | Low resolution stream width/height |
| [--lores-par](https://www.raspberrypi.com/documentation/computers/camera_software.html#lores-width-and-lores-height) | Preserve pixel aspect ratio of low-res image |
| [--mode](https://www.raspberrypi.com/documentation/computers/camera_software.html#mode) | Camera sensor mode (W:H:bit-depth:packing) |
| [--viewfinder-mode](https://www.raspberrypi.com/documentation/computers/camera_software.html#viewfinder-mode) | Viewfinder sensor mode |
| [--buffer-count](https://www.raspberrypi.com/documentation/computers/camera_software.html#buffer-count) | Number of capture buffers |
| [--viewfinder-buffer-count](https://www.raspberrypi.com/documentation/computers/camera_software.html#viewfinder-buffer-count) | Number of viewfinder buffers |
| [--no-raw](https://www.raspberrypi.com/documentation/computers/camera_software.html#no-raw) | Disable RAW stream request |
| [--post-process-file](https://www.raspberrypi.com/documentation/computers/camera_software.html#post-process-file) | JSON file for post-processing |
| [--post-process-libs](https://www.raspberrypi.com/documentation/computers/camera_software.html#post-process-libs) | Custom location for post-processing .so files |

---

## Transform

| Parameter | Description |
|---|---|
| [--rotation](https://www.raspberrypi.com/documentation/computers/camera_software.html#rotation) | Image rotation (0, 180) |
| [--hflip](https://www.raspberrypi.com/documentation/computers/camera_software.html#hflip) | Horizontal flip |
| [--vflip](https://www.raspberrypi.com/documentation/computers/camera_software.html#vflip) | Vertical flip |
| [--roi](https://www.raspberrypi.com/documentation/computers/camera_software.html#roi) | Region of interest — digital zoom (x,y,w,h normalized) |

---

## Camera Controls

| Parameter | Description |
|---|---|
| [--shutter](https://www.raspberrypi.com/documentation/computers/camera_software.html#shutter) | Fixed shutter speed (microseconds) |
| [--gain](https://www.raspberrypi.com/documentation/computers/camera_software.html#gain) | Fixed gain value (also: --analoggain) |
| [--metering](https://www.raspberrypi.com/documentation/computers/camera_software.html#metering) | Metering mode (centre, spot, average, custom) |
| [--exposure](https://www.raspberrypi.com/documentation/computers/camera_software.html#exposure) | Exposure mode (normal, sport) |
| [--ev](https://www.raspberrypi.com/documentation/computers/camera_software.html#ev) | Exposure compensation in stops |
| [--awb](https://www.raspberrypi.com/documentation/computers/camera_software.html#awb) | Auto white balance mode |
| [--awbgains](https://www.raspberrypi.com/documentation/computers/camera_software.html#awbgains) | Manual AWB gains (red,blue) |
| [--brightness](https://www.raspberrypi.com/documentation/computers/camera_software.html#brightness) | Brightness adjustment (-1.0 to 1.0) |
| [--contrast](https://www.raspberrypi.com/documentation/computers/camera_software.html#contrast) | Contrast adjustment (1.0 = normal) |
| [--saturation](https://www.raspberrypi.com/documentation/computers/camera_software.html#saturation) | Saturation adjustment (1.0 = normal, 0.0 = greyscale) |
| [--sharpness](https://www.raspberrypi.com/documentation/computers/camera_software.html#sharpness) | Sharpness adjustment (1.0 = normal) |
| [--denoise](https://www.raspberrypi.com/documentation/computers/camera_software.html#denoise) | Denoise mode (auto, off, cdn_off, cdn_fast, cdn_hq) |
| [--flicker-period](https://www.raspberrypi.com/documentation/computers/camera_software.html#flicker-period) | Manual flicker correction period |

---

## Autofocus

| Parameter | Description |
|---|---|
| [--autofocus-mode](https://www.raspberrypi.com/documentation/computers/camera_software.html#autofocus-mode) | Autofocus mode (default, manual, auto, continuous) |
| [--autofocus-range](https://www.raspberrypi.com/documentation/computers/camera_software.html#autofocus-range) | Focus distance range (normal, macro, full) |
| [--autofocus-speed](https://www.raspberrypi.com/documentation/computers/camera_software.html#autofocus-speed) | Focus movement speed (normal, fast) |
| [--autofocus-window](https://www.raspberrypi.com/documentation/computers/camera_software.html#autofocus-window) | AF metering window (x,y,w,h normalized) |
| [--lens-position](https://www.raspberrypi.com/documentation/computers/camera_software.html#lens-position) | Manual lens position (reciprocal distance) |
| [--autofocus-on-capture](https://www.raspberrypi.com/documentation/computers/camera_software.html#autofocus-on-capture) | Trigger autofocus before still capture |

---

## HDR

| Parameter | Description |
|---|---|
| [--hdr](https://www.raspberrypi.com/documentation/computers/camera_software.html#hdr) | High Dynamic Range mode (off, auto, single-exp) |

---

## Video Encoding

| Parameter | Description |
|---|---|
| [--codec](https://www.raspberrypi.com/documentation/computers/camera_software.html#codec) | Video codec (h264, mjpeg, yuv420, libav) |
| [--bitrate](https://www.raspberrypi.com/documentation/computers/camera_software.html#bitrate) | Video bitrate (bits per second) |
| [--intra](https://www.raspberrypi.com/documentation/computers/camera_software.html#intra) | Intra frame period for video encoding |
| [--profile](https://www.raspberrypi.com/documentation/computers/camera_software.html#profile) | H.264 profile (baseline, main, high) |
| [--level](https://www.raspberrypi.com/documentation/computers/camera_software.html#level) | H.264 level (4, 4.1, 4.2) |
| [--inline](https://www.raspberrypi.com/documentation/computers/camera_software.html#inline) | Insert PPS/SPS headers with every I-frame (h264) |
| [--framerate](https://www.raspberrypi.com/documentation/computers/camera_software.html#framerate) | Video framerate (fps) |
| [--frames](https://www.raspberrypi.com/documentation/computers/camera_software.html#frames) | Run for exact number of frames |
| [--segment](https://www.raspberrypi.com/documentation/computers/camera_software.html#segment) | Split video into segments (milliseconds) |
| [--circular](https://www.raspberrypi.com/documentation/computers/camera_software.html#circular) | Circular buffer size (MB) — saved on exit |
| [--sync](https://www.raspberrypi.com/documentation/computers/camera_software.html#sync) | Multi-camera sync (off, server, client) |
| [--split](https://www.raspberrypi.com/documentation/computers/camera_software.html#split) | Create new file when paused/resumed |
| [--save-pts](https://www.raspberrypi.com/documentation/computers/camera_software.html#save-pts) | Save presentation timestamps to file |
| [--listen](https://www.raspberrypi.com/documentation/computers/camera_software.html#listen) | Wait for socket connection before recording |
| [--initial](https://www.raspberrypi.com/documentation/computers/camera_software.html#initial) | Initial recording state (record, pause) |
| [--keypress](https://www.raspberrypi.com/documentation/computers/camera_software.html#keypress) | Enable keyboard shortcuts during recording |
| [--signal](https://www.raspberrypi.com/documentation/computers/camera_software.html#signal) | Enable signal control during recording |

---

## Still Capture

| Parameter | Description |
|---|---|
| [--encoding](https://www.raspberrypi.com/documentation/computers/camera_software.html#encoding) | Image encoding (jpg, png, bmp, rgb, yuv420) |
| [--quality](https://www.raspberrypi.com/documentation/computers/camera_software.html#quality) | JPEG quality (0-100) |
| [--exif](https://www.raspberrypi.com/documentation/computers/camera_software.html#exif) | EXIF metadata tags |
| [--timelapse](https://www.raspberrypi.com/documentation/computers/camera_software.html#timelapse) | Timelapse interval (milliseconds) |
| [--framestart](https://www.raspberrypi.com/documentation/computers/camera_software.html#framestart) | Frame number to start capture |
| [--datetime](https://www.raspberrypi.com/documentation/computers/camera_software.html#datetime) | Add datetime to filename |
| [--timestamp](https://www.raspberrypi.com/documentation/computers/camera_software.html#timestamp) | Add timestamp to filename |
| [--restart](https://www.raspberrypi.com/documentation/computers/camera_software.html#restart) | Restart time interval (seconds) |
| [--thumb](https://www.raspberrypi.com/documentation/computers/camera_software.html#thumb) | Thumbnail dimensions (w:h:quality) |
| [--raw](https://www.raspberrypi.com/documentation/computers/camera_software.html#raw) | Save RAW image alongside encoded image |
| [--latest](https://www.raspberrypi.com/documentation/computers/camera_software.html#latest) | Create symlink to latest file |
| [--immediate](https://www.raspberrypi.com/documentation/computers/camera_software.html#immediate) | Start capture immediately without preview |
| [--zsl](https://www.raspberrypi.com/documentation/computers/camera_software.html#zsl) | Zero Shutter Lag mode |

---

## LibAV / Audio

| Parameter | Description |
|---|---|
| [--libav-format](https://www.raspberrypi.com/documentation/computers/camera_software.html#libav-format) | LibAV output format (mp4, mkv, avi) |
| [--libav-audio](https://www.raspberrypi.com/documentation/computers/camera_software.html#libav-audio) | Enable audio recording |
| [--audio-codec](https://www.raspberrypi.com/documentation/computers/camera_software.html#audio-codec) | Audio codec (aac, mp3, opus) |
| [--audio-source](https://www.raspberrypi.com/documentation/computers/camera_software.html#audio-source) | Audio source (pulse, alsa) |
| [--audio-device](https://www.raspberrypi.com/documentation/computers/camera_software.html#audio-device) | Audio device name |
| [--audio-channels](https://www.raspberrypi.com/documentation/computers/camera_software.html#audio-channels) | Number of audio channels |
| [--audio-bitrate](https://www.raspberrypi.com/documentation/computers/camera_software.html#audio-bitrate) | Audio bitrate |
| [--audio-samplerate](https://www.raspberrypi.com/documentation/computers/camera_software.html#audio-samplerate) | Audio sample rate (Hz) |
| [--av-sync](https://www.raspberrypi.com/documentation/computers/camera_software.html#av-sync) | Audio/video time offset (microseconds) |
| [--libav-video-codec](https://www.raspberrypi.com/documentation/computers/camera_software.html#libav-video-codec) | LibAV video codec |
| [--libav-video-codec-opts](https://www.raspberrypi.com/documentation/computers/camera_software.html#libav-video-codec-opts) | LibAV video codec options |
| [--low-latency](https://www.raspberrypi.com/documentation/computers/camera_software.html#low-latency) | Enable low-latency encoding presets |

---

## Output

| Parameter | Description |
|---|---|
| [--output](https://www.raspberrypi.com/documentation/computers/camera_software.html#output) | Output filename (use `-` for stdout) |
| [--wrap](https://www.raspberrypi.com/documentation/computers/camera_software.html#wrap) | Wrap file counter at this number |
| [--flush](https://www.raspberrypi.com/documentation/computers/camera_software.html#flush) | Flush output data immediately |

---

## Camera Selection

| Parameter | Description |
|---|---|
| [--camera](https://www.raspberrypi.com/documentation/computers/camera_software.html#camera) | Camera index for multi-camera setups |
| [--list-cameras](https://www.raspberrypi.com/documentation/computers/camera_software.html#list-cameras) | List available cameras |

---

## Advanced

| Parameter | Description |
|---|---|
| [--verbose](https://www.raspberrypi.com/documentation/computers/camera_software.html#verbose) | Verbose output for debugging |
| [--config](https://www.raspberrypi.com/documentation/computers/camera_software.html#config) | Read options from config file |
| [--help](https://www.raspberrypi.com/documentation/computers/camera_software.html#help) | Display help information |
| [--version](https://www.raspberrypi.com/documentation/computers/camera_software.html#version) | Display version information |
| [--metadata](https://www.raspberrypi.com/documentation/computers/camera_software.html#metadata) | Save metadata to file or stdout |
| [--metadata-format](https://www.raspberrypi.com/documentation/computers/camera_software.html#metadata-format) | Metadata format (json, txt) |

---

## piStudio Enhanced Mode (Experimental)

Enhanced Mode enables live, runtime parameter updates via Unix domain socket to a modified rpicam-apps build (PR #917). When available, parameter changes in the GUI are sent immediately without restarting the camera.

| Parameter Key | GUI Widget | Value Format |
|---|---|---|
| `brightness` | Brightness slider | float (-1.0 to 1.0) |
| `contrast` | Contrast slider | float (0.0 to 5.0) |
| `saturation` | Saturation slider | float (0.0 to 1.0) |
| `sharpness` | Sharpness slider | float (0.0 to 5.0) |
| `ev` | EV slider | float (-9.9 to 9.9) |
| `gain` | Gain slider | float (0.0 to 20.0) |
| `awb` | AWB selector | string (auto, incandescent, tungsten, fluorescent, indoor, daylight, cloudy) |
| `awbgains` | AWB Red/Blue sliders | float,float (red,blue) |
| `roi` | ROI input | string (x,y,w,h normalized) |
| `metering` | Metering selector | string (centre, spot, average, custom) |
| `hdr` | HDR selector | string (off, auto, sensor, single-exp) |
| `denoise` | Denoise selector | string (auto, off, cdn_off, cdn_fast, cdn_hq) |
| `shutter` | Shutter input | integer (microseconds) |
| `framerate` | Framerate selector | integer (fps, clamped to caps:maxfps) |

**Socket Protocol:**
- Path: `/tmp/rpicam-vid{N}.sock` (N = camera index)
- Wire format: `key:value\n` (text, newline-terminated)
- Multiple commands can be batched in a single write
- Server sends `caps:maxfps=N,hasaf=0|1` on connect
- If socket is unavailable (mainline rpicam-apps), falls back to standard restart-on-change behavior

**Indicator:** A green "CTRL" badge on the Start/Stop button indicates Enhanced Mode is available. The feature can be disabled in Setup > Settings.

> **Requires:** rpicam-apps PR #917 — https://github.com/raspberrypi/rpicam-apps/pull/917

---

> Source: [Raspberry Pi Camera Software Documentation](https://www.raspberrypi.com/documentation/computers/camera_software.html)  
> © 2012–2025 Raspberry Pi Ltd — Licensed under CC BY-SA 4.0
