#!/bin/bash

set -e

# Detect host architecture
HOST_ARCH=$(dpkg --print-architecture)
echo "Detected architecture: $HOST_ARCH"

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Current control file content:"
echo "================================"
cat "$SCRIPT_DIR/DEBIAN/control" | grep "Architecture:"
echo "================================"

# Update architecture
echo "Updating architecture..."
sed -i "s/ARCH_PLACEHOLDER/$HOST_ARCH/g" "$SCRIPT_DIR/DEBIAN/control"

echo "Updated control file content:"
echo "================================"
cat "$SCRIPT_DIR/DEBIAN/control" | grep "Architecture:"
echo "================================"

# Restore placeholder
echo "Restoring placeholder..."
sed -i "s/Architecture: $HOST_ARCH/Architecture: ARCH_PLACEHOLDER/g" "$SCRIPT_DIR/DEBIAN/control"

echo "Final control file content:"
echo "================================"
cat "$SCRIPT_DIR/DEBIAN/control" | grep "Architecture:"
echo "================================"
