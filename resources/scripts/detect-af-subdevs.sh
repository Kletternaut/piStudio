#!/bin/bash
# detect-af-subdevs.sh — Find AF-capable V4L2 subdevices and map to camera indices.
# Uses v4l2-ctl and media-ctl. /dev/media0 = Camera 0, /dev/media1 = Camera 1.

set -euo pipefail

echo "=== Scanning for AF-capable V4L2 subdevices ==="
echo ""

found_any=false

for subdev in /dev/v4l-subdev*; do
    subdev_name=$(basename "$subdev")

    # Check if this subdev supports focus_absolute or focus_relative
    if ! v4l2-ctl -d "$subdev" -l 2>/dev/null | grep -qE 'focus_absolute|focus_relative'; then
        continue
    fi

    found_any=true

    # Get driver name from sysfs
    driver=$(cat "/sys/class/video4linux/$subdev_name/device/name" 2>/dev/null || echo "unknown")

    # Find which media device this subdev belongs to
    media_idx="?"
    sensor_info=""
    for media in /dev/media*; do
        media_num=$(basename "$media" | sed 's/media//')
        output=$(media-ctl -d "$media" -p 2>/dev/null)
        if echo "$output" | grep -q "$subdev_name"; then
            media_idx="$media_num"
            # Try to extract sensor model from the same media device
            sensor_info=$(echo "$output" | grep -oP 'entity \d+: \S+' | head -1)
            break
        fi
    done

    echo "AF-Subdev:   $subdev"
    echo "  Driver:    $driver"
    echo "  Media:     /dev/media${media_idx}  →  Camera $media_idx"
    if [ -n "$sensor_info" ]; then
        echo "  Sensor:    $sensor_info"
    fi
    echo ""
done

if ! $found_any; then
    echo "No AF-capable V4L2 subdevice found."
    echo ""
    echo "Available subdevices:"
    for s in /dev/v4l-subdev*; do
        name=$(basename "$s")
        drv=$(cat "/sys/class/video4linux/$name/device/name" 2>/dev/null || echo "?")
        echo "  $s  driver=$drv"
    done
fi
