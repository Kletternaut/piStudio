#!/bin/bash
# system-info.sh - Comprehensive system information collector
# Collects display server, desktop environment, and session information

LOG_FILE="system-info-$(date +%Y%m%d_%H%M%S).log"

echo "=================================" | tee "$LOG_FILE"
echo "SYSTEM INFORMATION REPORT" | tee -a "$LOG_FILE"
echo "Generated: $(date)" | tee -a "$LOG_FILE"
echo "User: $(whoami)" | tee -a "$LOG_FILE"
echo "Hostname: $(hostname)" | tee -a "$LOG_FILE"
echo "=================================" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# 1. DISPLAY SERVER TYPE
echo "=== DISPLAY SERVER TYPE ===" | tee -a "$LOG_FILE"
echo "XDG_SESSION_TYPE: $XDG_SESSION_TYPE" | tee -a "$LOG_FILE"
echo "DISPLAY: $DISPLAY" | tee -a "$LOG_FILE"
echo "WAYLAND_DISPLAY: $WAYLAND_DISPLAY" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# 2. DESKTOP ENVIRONMENT
echo "=== DESKTOP ENVIRONMENT ===" | tee -a "$LOG_FILE"
echo "XDG_CURRENT_DESKTOP: $XDG_CURRENT_DESKTOP" | tee -a "$LOG_FILE"
echo "DESKTOP_SESSION: $DESKTOP_SESSION" | tee -a "$LOG_FILE"
echo "XDG_SESSION_DESKTOP: $XDG_SESSION_DESKTOP" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# 3. SESSION INFORMATION
echo "=== SESSION INFORMATION ===" | tee -a "$LOG_FILE"
if command -v loginctl &> /dev/null; then
    echo "Current sessions:" | tee -a "$LOG_FILE"
    loginctl list-sessions 2>/dev/null | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    
    # Get current session details
    CURRENT_SESSION=$(loginctl | grep "$(whoami)" | awk '{print $1}' | head -1)
    if [ ! -z "$CURRENT_SESSION" ]; then
        echo "Current session details (Session: $CURRENT_SESSION):" | tee -a "$LOG_FILE"
        loginctl show-session "$CURRENT_SESSION" 2>/dev/null | tee -a "$LOG_FILE"
    fi
else
    echo "loginctl not available" | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"

# 4. RUNNING DISPLAY SERVERS
echo "=== RUNNING DISPLAY SERVERS ===" | tee -a "$LOG_FILE"
echo "X11 processes:" | tee -a "$LOG_FILE"
ps aux | grep -E "[Xx]org|[Xx]11" | grep -v grep | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

echo "Wayland compositors:" | tee -a "$LOG_FILE"
ps aux | grep -E "(wayland|weston|sway|labwc|river|hyprland|mutter|kwin|gnome-shell)" | grep -v grep | tee -a "$LOG_FILE"
if [ $? -ne 0 ]; then
    echo "No Wayland compositors found" | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"

# 5. REMOTE DESKTOP
echo "=== REMOTE DESKTOP SERVICES ===" | tee -a "$LOG_FILE"
echo "xRDP processes:" | tee -a "$LOG_FILE"
ps aux | grep xrdp | grep -v grep | tee -a "$LOG_FILE"
if [ $? -ne 0 ]; then
    echo "No xRDP processes found" | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"

echo "VNC processes:" | tee -a "$LOG_FILE"
ps aux | grep vnc | grep -v grep | tee -a "$LOG_FILE"
if [ $? -ne 0 ]; then
    echo "No VNC processes found" | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"

# 6. CURRENT USERS AND SESSIONS
echo "=== ACTIVE USERS ===" | tee -a "$LOG_FILE"
echo "who output:" | tee -a "$LOG_FILE"
who | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

echo "w output:" | tee -a "$LOG_FILE"
w | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# 7. WINDOW MANAGER DETECTION
echo "=== WINDOW MANAGER ===" | tee -a "$LOG_FILE"
if [ ! -z "$WAYLAND_DISPLAY" ]; then
    echo "Wayland session detected" | tee -a "$LOG_FILE"
    # Try to detect Wayland compositor
    for compositor in sway labwc river hyprland weston mutter kwin_wayland gnome-shell; do
        if pgrep "$compositor" > /dev/null; then
            echo "Detected Wayland compositor: $compositor" | tee -a "$LOG_FILE"
        fi
    done
elif [ ! -z "$DISPLAY" ]; then
    echo "X11 session detected" | tee -a "$LOG_FILE"
    # Try to detect window manager
    if command -v xprop &> /dev/null; then
        WM=$(xprop -root _NET_WM_NAME 2>/dev/null | cut -d'"' -f2)
        if [ ! -z "$WM" ]; then
            echo "Window manager: $WM" | tee -a "$LOG_FILE"
        fi
    fi
fi
echo "" | tee -a "$LOG_FILE"

# 8. SYSTEM INFORMATION
echo "=== SYSTEM INFORMATION ===" | tee -a "$LOG_FILE"
echo "OS Release:" | tee -a "$LOG_FILE"
if [ -f /etc/os-release ]; then
    cat /etc/os-release | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"

echo "Kernel:" | tee -a "$LOG_FILE"
uname -a | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# 9. QT INFORMATION
echo "=== QT INFORMATION ===" | tee -a "$LOG_FILE"
if command -v qmake &> /dev/null; then
    echo "Qt version:" | tee -a "$LOG_FILE"
    qmake --version | tee -a "$LOG_FILE"
else
    echo "qmake not found" | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"

# 10. SUMMARY
echo "=== SUMMARY ===" | tee -a "$LOG_FILE"
if [ ! -z "$WAYLAND_DISPLAY" ]; then
    echo "✓ Running on WAYLAND" | tee -a "$LOG_FILE"
    echo "  Display: $WAYLAND_DISPLAY" | tee -a "$LOG_FILE"
elif [ ! -z "$DISPLAY" ]; then
    echo "✓ Running on X11" | tee -a "$LOG_FILE"
    echo "  Display: $DISPLAY" | tee -a "$LOG_FILE"
    if ps aux | grep xrdp | grep -v grep > /dev/null; then
        echo "  ⚠ Connected via xRDP (Remote Desktop)" | tee -a "$LOG_FILE"
    fi
else
    echo "✗ No display server detected" | tee -a "$LOG_FILE"
fi

echo "Desktop Environment: $XDG_CURRENT_DESKTOP" | tee -a "$LOG_FILE"
echo "Session Type: $XDG_SESSION_TYPE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

echo "=================================" | tee -a "$LOG_FILE"
echo "Report saved to: $LOG_FILE" | tee -a "$LOG_FILE"
echo "=================================" | tee -a "$LOG_FILE"

# Make the script show the filename at the end
echo ""
echo "🔍 System information collected!"
echo "📄 Report saved to: $LOG_FILE"
echo ""
echo "💡 Quick summary:"
if [ ! -z "$WAYLAND_DISPLAY" ]; then
    echo "   Display Server: WAYLAND ($WAYLAND_DISPLAY)"
elif [ ! -z "$DISPLAY" ]; then
    echo "   Display Server: X11 ($DISPLAY)"
    if ps aux | grep xrdp | grep -v grep > /dev/null; then
        echo "   Connection: Remote Desktop (xRDP)"
    fi
fi
echo "   Desktop: $XDG_CURRENT_DESKTOP"
echo "   Session: $XDG_SESSION_TYPE"
