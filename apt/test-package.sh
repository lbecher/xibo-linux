#!/bin/bash

set -e

echo "=== Testing APT package structure ==="

APT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check directory structure
echo "Checking directory structure..."
REQUIRED_DIRS=(
    "DEBIAN"
    "usr/bin"
    "usr/share/applications"
    "usr/share/icons/hicolor/256x256/apps"
    "usr/share/xibo-player"
)

for dir in "${REQUIRED_DIRS[@]}"; do
    if [ ! -d "$APT_DIR/$dir" ]; then
        echo "❌ Directory missing: $dir"
        exit 1
    else
        echo "✅ Directory found: $dir"
    fi
done

# Check required files
echo "Checking required files..."
REQUIRED_FILES=(
    "DEBIAN/control"
    "DEBIAN/postinst"
    "DEBIAN/postrm"
    "usr/share/applications/xibo-player.desktop"
    "usr/share/applications/xibo-options.desktop"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$APT_DIR/$file" ]; then
        echo "❌ File missing: $file"
        exit 1
    else
        echo "✅ File found: $file"
    fi
done

# Check script permissions
echo "Checking script permissions..."
EXECUTABLE_FILES=(
    "DEBIAN/postinst"
    "DEBIAN/postrm"
    "build-deb.sh"
    "install-dependencies.sh"
    "xibo-wrapper.sh"
)

for file in "${EXECUTABLE_FILES[@]}"; do
    if [ ! -x "$APT_DIR/$file" ]; then
        echo "❌ File not executable: $file"
        exit 1
    else
        echo "✅ File executable: $file"
    fi
done

# Check script syntax
echo "Checking script syntax..."
bash -n "$APT_DIR/build-deb.sh" && echo "✅ build-deb.sh: syntax OK" || (echo "❌ build-deb.sh: syntax error"; exit 1)
bash -n "$APT_DIR/install-dependencies.sh" && echo "✅ install-dependencies.sh: syntax OK" || (echo "❌ install-dependencies.sh: syntax error"; exit 1)
bash -n "$APT_DIR/xibo-wrapper.sh" && echo "✅ xibo-wrapper.sh: syntax OK" || (echo "❌ xibo-wrapper.sh: syntax error"; exit 1)

# Check control file
echo "Checking control file..."
if grep -q "Package: xibo-player" "$APT_DIR/DEBIAN/control"; then
    echo "✅ control: package name OK"
else
    echo "❌ control: package name not found"
    exit 1
fi

if grep -q "Version: 1.8-R7" "$APT_DIR/DEBIAN/control"; then
    echo "✅ control: version OK"
else
    echo "❌ control: version not found"
    exit 1
fi

echo ""
echo "=== All tests passed! ==="
echo ""
echo "The APT package is ready to be built."
echo "Run: ./build-deb.sh"
