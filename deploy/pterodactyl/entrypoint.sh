#!/bin/bash
# =============================================================================
# AyuGram Pterodactyl Entrypoint
# =============================================================================
# Pterodactyl runs this script as the container entrypoint.
# It handles the full lifecycle: xvfb → VNC → noVNC → AyuGram
# =============================================================================

set -e

export DISPLAY=:99
export XDG_RUNTIME_DIR=/tmp/runtime-$(id -u)

mkdir -p "$XDG_RUNTIME_DIR"
rm -f /tmp/.X99-lock

# --- 1. X Virtual Framebuffer ---
echo "[ayugram] Starting Xvfb on :99..."
Xvfb :99 -screen 0 1280x800x24 -ac +extension GLX +render -noreset &
XVFB_PID=$!
sleep 1

# --- 2. x11vnc ---
VNC_ARG="-nopw"
if [ -n "$VNC_PASSWORD" ]; then
    mkdir -p /home/user/.vnc
    x11vnc -storepasswd "$VNC_PASSWORD" /home/user/.vnc/passwd
    VNC_ARG="-rfbauth /home/user/.vnc/passwd"
fi

echo "[ayugram] Starting x11vnc..."
x11vnc -display :99 -forever -shared -rfbport 5900 $VNC_ARG &
VNC_PID=$!

# --- 3. noVNC (web client) ---
echo "[ayugram] Starting noVNC on port 8080..."
/opt/noVNC/utils/novnc_proxy --vnc localhost:5900 --listen 8080 &
NOVNC_PID=$!

# --- 4. AyuGram ---
echo "[ayugram] Starting AyuGram Desktop..."
/home/user/SandyGram -- &
AYUGRAM_PID=$!

echo ""
echo "============================================="
echo "  AyuGram is running"
echo "  noVNC:  http://localhost:8080/vnc.html"
echo "  VNC:    localhost:5900"
echo "  Pass:   ${VNC_PASSWORD:-(none)}"
echo "============================================="
echo ""

# Cleanup on exit
cleanup() {
    echo "[ayugram] Shutting down..."
    kill $AYUGRAM_PID 2>/dev/null
    kill $NOVNC_PID 2>/dev/null
    kill $VNC_PID 2>/dev/null
    kill $XVFB_PID 2>/dev/null
    exit 0
}
trap cleanup SIGTERM SIGINT

# Wait for any child to exit
wait -n $XVFB_PID $VNC_PID $NOVNC_PID $AYUGRAM_PID 2>/dev/null
cleanup
