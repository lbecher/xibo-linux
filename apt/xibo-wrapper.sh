#!/bin/bash

# Wrapper script for Xibo Player
# Sets up environment and runs the player

# Detect architecture and set GStreamer paths
ARCH=$(dpkg --print-architecture)
case $ARCH in
    amd64)
        GST_LIB_PATH="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"
        ;;
    arm64)
        GST_LIB_PATH="/usr/lib/aarch64-linux-gnu/gstreamer-1.0"
        ;;
    armhf)
        GST_LIB_PATH="/usr/lib/arm-linux-gnueabihf/gstreamer-1.0"
        ;;
    i386)
        GST_LIB_PATH="/usr/lib/i386-linux-gnu/gstreamer-1.0"
        ;;
    riscv64)
        GST_LIB_PATH="/usr/lib/riscv64-linux-gnu/gstreamer-1.0"
        ;;
    *)
        # Fallback to multiarch path detection
        GST_LIB_PATH="/usr/lib/$(gcc -dumpmachine)/gstreamer-1.0"
        ;;
esac

# Set GStreamer environment variables
export GST_PLUGIN_PATH="$GST_LIB_PATH"
export GST_PLUGIN_SYSTEM_PATH="$GST_LIB_PATH"

# Run the watchdog that manages the player
exec xibo-watchdog "$@"
