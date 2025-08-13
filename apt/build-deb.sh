#!/bin/bash

set -e

# Define variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-apt"
APT_DIR="$SCRIPT_DIR"
PLAYER_DIR="$PROJECT_ROOT/player"

# Detect host architecture
HOST_ARCH=$(dpkg --print-architecture)
echo "Detected architecture: $HOST_ARCH"

echo "=== Building Xibo Player for APT ==="
echo "Project directory: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo "Target architecture: $HOST_ARCH"

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "Removing previous build..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Install build dependencies if needed
echo "Checking build dependencies..."
MISSING_DEPS=""

# Check if cmake is installed
if ! command -v cmake &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS cmake"
fi

# Check if g++ is installed
if ! command -v g++ &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS g++"
fi

# Check if pkg-config is installed
if ! command -v pkg-config &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS pkg-config"
fi

if [ ! -z "$MISSING_DEPS" ]; then
    echo "Missing build dependencies:$MISSING_DEPS"
    echo "Run: sudo apt update && sudo apt install$MISSING_DEPS"
    exit 1
fi

# Install development dependencies if needed
echo "Installing development dependencies..."
sudo apt update
sudo apt install -y \
    libcrypto++-dev \
    libboost-all-dev \
    libgtkmm-3.0-dev \
    libwebkit2gtk-4.0-dev \
    libglibmm-2.4-dev \
    libzmq3-dev \
    libspdlog-dev \
    libgtest-dev \
    libgmock-dev \
    libgstreamer-plugins-good1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-dev \
    libssl-dev \
    libsqlite3-dev \
    libcurl4-gnutls-dev

echo "Building third-party libraries..."

# Build date-tz
echo "Building date-tz..."
cd "$BUILD_DIR"
if [ ! -d "date-3.0.0" ]; then
    wget -q https://github.com/HowardHinnant/date/archive/v3.0.0.tar.gz -O date-3.0.0.tar.gz
    tar -xzf date-3.0.0.tar.gz
fi
cd date-3.0.0
mkdir -p build && cd build
cmake .. -DBUILD_TZ_LIB=ON -DBUILD_SHARED_LIBS=ON -DUSE_SYSTEM_TZ_DB=ON -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install

# Build sqlite-orm
echo "Building sqlite-orm..."
cd "$BUILD_DIR"
if [ ! -d "sqlite_orm-1.6" ]; then
    wget -q https://github.com/fnc12/sqlite_orm/archive/refs/tags/1.6.tar.gz -O sqlite_orm-1.6.tar.gz
    tar -xzf sqlite_orm-1.6.tar.gz
fi
cd sqlite_orm-1.6
mkdir -p build && cd build
cmake .. -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install

# Update library cache
sudo ldconfig

echo "Building Xibo Player..."
cd "$BUILD_DIR"
mkdir -p xibo-build && cd xibo-build

# Configure CMake
cmake "$PLAYER_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAPP_ENV=APT \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_PREFIX_PATH=/usr/local

# Build
make -j$(nproc)

echo "Installing files to package..."

# Create package directories
mkdir -p "$APT_DIR/usr/bin"
mkdir -p "$APT_DIR/usr/share/xibo-player"
mkdir -p "$APT_DIR/usr/share/icons/hicolor/256x256/apps"

# Copy binaries
cp bin/xibo-player "$APT_DIR/usr/bin/"
cp bin/xibo-options "$APT_DIR/usr/bin/"
cp bin/xibo-watchdog "$APT_DIR/usr/bin/"

# Copy and rename the wrapper
cp "$APT_DIR/xibo-wrapper.sh" "$APT_DIR/usr/bin/xibo-player-wrapper"

# Copy resources
cp "$PLAYER_DIR/resources/ui.glade" "$APT_DIR/usr/share/xibo-player/"
cp "$PLAYER_DIR/resources/splash.jpg" "$APT_DIR/usr/share/xibo-player/"
cp "$PLAYER_DIR/resources/xibo-player.png" "$APT_DIR/usr/share/icons/hicolor/256x256/apps/"

# Set permissions
chmod +x "$APT_DIR/usr/bin/xibo-"*

echo "Building .deb package..."
cd "$PROJECT_ROOT"

# Update control file with correct architecture
echo "Updating control file architecture from ARCH_PLACEHOLDER to $HOST_ARCH"
sed -i "s/ARCH_PLACEHOLDER/$HOST_ARCH/g" "$APT_DIR/DEBIAN/control"

# Verify the substitution worked
if grep -q "Architecture: $HOST_ARCH" "$APT_DIR/DEBIAN/control"; then
    echo "✅ Architecture updated successfully in control file"
else
    echo "❌ Failed to update architecture in control file"
    echo "Current content:"
    grep "Architecture:" "$APT_DIR/DEBIAN/control"
    exit 1
fi

# Build the package
PACKAGE_NAME="xibo-player_1.8-R7_${HOST_ARCH}.deb"
echo "Creating package: $PACKAGE_NAME"
dpkg-deb --build apt "$PACKAGE_NAME"

# Restore placeholder in control file for next build
sed -i "s/Architecture: $HOST_ARCH/Architecture: ARCH_PLACEHOLDER/g" "$APT_DIR/DEBIAN/control"
echo "Architecture placeholder restored in control file"

echo "=== Build completed ==="
echo "Package created: $PACKAGE_NAME"
echo ""
echo "To install:"
echo "  sudo dpkg -i $PACKAGE_NAME"
echo "  sudo apt-get install -f  # if there are unsatisfied dependencies"
echo ""
echo "To remove:"
echo "  sudo apt remove xibo-player"
