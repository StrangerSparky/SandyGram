#!/bin/bash
# =============================================================================
# AyuGram Pterodactyl Startup Script
# =============================================================================
# Starts xvfb, x11vnc, noVNC, and AyuGram in the correct order.
# Pterodactyl monitors port 8080 (noVNC web UI) for server status.
# =============================================================================

set -e

export DISPLAY=:99
export XDG_RUNTIME_DIR=/tmp/runtime-$(id -u)

mkdir -p "$XDG_RUNTIME_DIR"

echo "[ayugram] Starting X virtual framebuffer (xvfb)..."
Xvfb :99 -screen 0 1024x768x24 -ac +extension GLX &
XVFB_PID=$!

# Wait for X to be ready
sleep 1

echo "[ayugram] Starting x11vnc on port 5900..."
x11vnc -display :99 -forever -nopw -shared -rfbport 5900 &
VNC_PID=$!

echo "[ayugram] Starting noVNC web client on port 8080..."
/opt/noVNC/utils/novnc_proxy --vnc localhost:5900 --listen 8080 &
NOVNC_PID=$!

echo "[ayugram] Starting AyuGram..."
/home/user/SandyGram &
AYUGRAM_PID=$!

echo "[ayugram] All services started."
echo "[ayugram] noVNC web UI: http://localhost:8080/vnc.html"
echo "[ayugram] VNC direct: localhost:5900"

# Wait for any process to exit
wait -n $XVFB_PID $VNC_PID $NOVNC_PID $AYUGRAM_PID

# Exit with status of process that exited first
exit $?
