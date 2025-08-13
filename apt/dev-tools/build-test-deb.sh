#!/bin/bash

set -e

# Detect host architecture
HOST_ARCH=$(dpkg --print-architecture)
echo "Detected architecture: $HOST_ARCH"

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Creating test package without binaries..."

# Update control file with correct architecture
echo "Updating control file architecture to $HOST_ARCH"
sed -i "s/ARCH_PLACEHOLDER/$HOST_ARCH/g" "$SCRIPT_DIR/DEBIAN/control"

# Verify the substitution worked
if grep -q "Architecture: $HOST_ARCH" "$SCRIPT_DIR/DEBIAN/control"; then
    echo "✅ Architecture updated successfully in control file"
else
    echo "❌ Failed to update architecture in control file"
    exit 1
fi

# Create dummy binaries for testing
echo "Creating dummy binaries..."
touch "$SCRIPT_DIR/usr/bin/xibo-player"
touch "$SCRIPT_DIR/usr/bin/xibo-options"
touch "$SCRIPT_DIR/usr/bin/xibo-watchdog"
touch "$SCRIPT_DIR/usr/bin/xibo-player-wrapper"

# Set permissions
chmod +x "$SCRIPT_DIR/usr/bin/xibo-"*

# Build the package
cd "$PROJECT_ROOT"
PACKAGE_NAME="xibo-player-test_1.8-R7_${HOST_ARCH}.deb"
echo "Creating test package: $PACKAGE_NAME"
dpkg-deb --build apt "$PACKAGE_NAME"

# Restore placeholder in control file
sed -i "s/Architecture: $HOST_ARCH/Architecture: ARCH_PLACEHOLDER/g" "$SCRIPT_DIR/DEBIAN/control"
echo "Architecture placeholder restored"

# Clean up dummy files
rm -f "$SCRIPT_DIR/usr/bin/xibo-"*

echo "Test package created: $PACKAGE_NAME"
echo "You can test installation with: sudo dpkg -i $PACKAGE_NAME"
