#!/bin/bash
# detect-af.sh — Find AF-capable V4L2 subdevices and map to camera indices
set -euo pipefail

echo "=== Mapping: Camera ↔ I2C bus ↔ AF subdevice ==="
echo ""

# Step 1: Build camera → I2C bus table from cam -l
echo "--- Cameras ---"
cam -l 2>/dev/null | while IFS= read -r line; do
    dt=$(echo "$line" | grep -oP 'i2c@[0-9a-f]+' || true)
    [ -z "$dt" ] && continue
    idx=$(echo "$line" | grep -oP '^[0-9]+' || true)
    [ -z "$idx" ] && continue
    cam_idx=$((idx - 1))
    for ol in /sys/bus/i2c/devices/i2c-*/of_node; do
        bnum=$(echo "$ol" | grep -oP 'i2c-[0-9]+')
        onode=$(readlink -f "$ol" 2>/dev/null | grep -oP 'i2c@[0-9a-f]+' || true)
        if [ "$onode" = "$dt" ]; then
            echo "Camera $cam_idx  →  $bnum  (DT: $dt)"
        fi
    done
done

echo ""
echo "--- AF Subdevices ---"
found=0
for s in /dev/v4l-subdev*; do
    if ! v4l2-ctl -d "$s" -l 2>/dev/null | grep -qE 'focus_absolute|focus_relative'; then
        continue
    fi
    found=1
    sn=$(basename "$s")
    drv=$(cat "/sys/class/video4linux/${sn}/device/name" 2>/dev/null || echo "unknown")
    link=$(readlink "/sys/class/video4linux/${sn}/device" 2>/dev/null || true)
    busnum=$(echo "$link" | grep -oP '[0-9]+(?=-)' | head -1 || echo "?")
    echo "AF: $s  →  i2c-${busnum}  driver=$drv"
done

if [ "$found" -eq 0 ]; then
    echo "No AF-capable V4L2 subdevice found."
fi
