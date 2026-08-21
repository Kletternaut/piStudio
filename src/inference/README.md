# UDP Object Detection Receiver

## Overview

This module implements a UDP-based object detection receiver for piStudio. It consists of a standalone C program that receives UDP packets and displays them via a new "Inference" tab in the GUI.

## Components

### 1. C Program: `udp_object_detection.c`

A lightweight C program that:
- Opens a UDP socket on port 12347 (configurable)
- Receives object detection data packets from `object_detect_udp_stage.cpp`
- Outputs detections in structured format
- Optimized for GUI integration

**Packet Format:**
```
Delimiter (4 bytes): 0xDDCCBBAA (little-endian)
X-Position (4 bytes): int32_t
Y-Position (4 bytes): int32_t
Width (4 bytes): int32_t
Height (4 bytes): int32_t
Name Length (1 byte): uint8_t
Object Name (variable): UTF-8 String
Confidence (4 bytes): float
```

**Output Format:**
```
[2025-11-03 14:32:45] DETECTION: person (87.50%) at [120,340] size [200x450]
```

### 2. GUI Integration: Inference Tab

The new tab provides:
- **Control**: Start/Stop buttons for the receiver
- **Port Configuration**: Adjustable UDP port (default: 12347)
- **Detection List**: Shows the last 100 detected objects
- **Filtering**: "Report only changes" option to show only new object types

## Usage

### Terminal (Standalone)

```bash
# Default port 12347
./udp_object_detection

# Custom port
./udp_object_detection 8888
```

### In the GUI

1. Open the **Inference** tab
2. Optional: Adjust the UDP port (default: 12347)
3. Optional: Enable "Report only changes" to filter repeated detections
4. Click **Start Receiver**
5. Detections appear automatically in the list
6. Click **Stop Receiver** to stop

### Test with Python Script

```bash
# Send test detections
./resources/utils/test_udp_detection.py --count 20 --interval 0.5
```

### Test with netcat

```bash
# Send test message (debugging)
echo "Test" | nc -u localhost 12347
```

## Technical Details

### Architecture

- **Process Isolation**: C receiver runs as a separate process
- **IPC**: Communication via stdout/stderr with QProcess
- **Threading**: Non-blocking through QProcess event loop
- **Resources**: Minimal memory footprint (~100 KB)

### Error Handling

- Packet delimiter validation
- Buffer overflow size checks
- Socket operation timeout handling
- Automatic cleanup on GUI closure

### Performance

- **Latency**: < 1ms per packet
- **Throughput**: > 1000 packets/second
- **CPU Load**: < 1% during typical usage

### Filtering Options

- **Report only changes**: Only shows detections when the detected object class changes
- Useful for reducing noise when the same object is continuously detected
- Last detected object is tracked and compared

## Build Integration

The program is automatically built via CMake:

```cmake
add_executable(udp_object_detection src/inference/udp_object_detection.c)
install(TARGETS udp_object_detection DESTINATION bin)
```

## Future Enhancements

Possible future features:
- Bounding box visualization in video preview
- Object class filtering
- Detection export as CSV/JSON
- Statistics (objects per second, average confidence)
- Multi-port support for multiple sources

## License

PolyForm Noncommercial License 1.0.0 (see LICENSE.md)
