#!/bin/bash

echo "Cleaning up test packages and preparing for real build..."

# Remove any existing packages
rm -f ../xibo-player*.deb

# Check if we have the necessary dependencies to build
echo "Checking if we can build the real package..."

# Detect architecture
HOST_ARCH=$(dpkg --print-architecture)
echo "Architecture: $HOST_ARCH"

# Check basic build tools
MISSING_DEPS=""

if ! command -v cmake &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS cmake"
fi

if ! command -v g++ &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS g++"
fi

if ! command -v pkg-config &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS pkg-config"
fi

if [ ! -z "$MISSING_DEPS" ]; then
    echo "❌ Missing build dependencies:$MISSING_DEPS"
    echo "Install with: sudo apt update && sudo apt install$MISSING_DEPS"
    exit 1
else
    echo "✅ Basic build tools available"
fi

# Remove the test package if installed
if dpkg -l | grep -q "xibo-player"; then
    echo "Removing test package..."
    sudo apt remove -y xibo-player
fi

echo ""
echo "Ready to build the real package!"
echo "Run: ./build-deb.sh"
echo ""
echo "Or if you want to install dependencies first:"
echo "Run: ./install-dependencies.sh"
