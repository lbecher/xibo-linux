#!/bin/bash

# Wrapper script for Xibo Player
# Sets up environment and runs the player

# Set GStreamer environment variables
export GST_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0
export GST_PLUGIN_SYSTEM_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0

# Run the watchdog that manages the player
exec xibo-watchdog "$@"
