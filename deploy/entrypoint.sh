#!/bin/bash
set -e

# Clean up any leftover lock files from previous runs
rm -f /home/user/.local/share/SandyGram/data/tdata/working

# Start dbus if available
if command -v dbus-daemon &>/dev/null; then
    dbus-daemon --system --fork 2>/dev/null || true
fi

# Generate a fresh X authority file
touch /home/user/.Xauthority

# Start AyuGram under xvfb (virtual framebuffer)
exec xvfb-run -a -s "-screen 0 1024x768x24" \
    /home/user/SandyGram \
    -- \
    "$@"
