#!/bin/bash
# resources/utils/detection_test.sh — Test script for object detection actions
# Executed when an object is detected.
#
# Parameters (optional):
# $1 = object name
# $2 = confidence (%)
# $3 = timestamp

LOG_FILE="${XDG_RUNTIME_DIR:-/tmp}/piStudio_detection_script.log"

echo "==================================" >> "$LOG_FILE"
echo "Detection Script executed!" >> "$LOG_FILE"
echo "Time: $(date '+%Y-%m-%d %H:%M:%S')" >> "$LOG_FILE"

# Print parameters if provided
if [ $# -gt 0 ]; then
    echo "Object: $1" >> "$LOG_FILE"
    echo "Confidence: $2%" >> "$LOG_FILE"
    echo "Timestamp: $3" >> "$LOG_FILE"
fi

echo "==================================" >> "$LOG_FILE"
echo "" >> "$LOG_FILE"

# Example: send desktop notification
# notify-send "Object Detected" "$1 detected with $2% confidence"

# Example: blink LED via GPIO
# echo 1 > /sys/class/gpio/gpio17/value
# sleep 0.5
# echo 0 > /sys/class/gpio/gpio17/value

# Example: call webhook
# curl -X POST https://example.com/webhook -d "object=$1&confidence=$2"

exit 0
