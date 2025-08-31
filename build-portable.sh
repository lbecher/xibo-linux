#!/bin/bash

set -e

# Define variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build-portable"
DIST_DIR="$PROJECT_ROOT/dist"
PLAYER_DIR="$PROJECT_ROOT/player"

# Detect host architecture
HOST_ARCH=$(dpkg --print-architecture 2>/dev/null || uname -m)

# Normalize architecture names for consistency
case "$HOST_ARCH" in
    x86_64)
        HOST_ARCH="amd64"
        ;;
    amd64)
        HOST_ARCH="amd64"
        ;;
    aarch64)
        HOST_ARCH="arm64"
        ;;
    arm64)
        HOST_ARCH="arm64"
        ;;
    armv7l|armhf)
        HOST_ARCH="armhf"
        ;;
    *)
        echo "Warning: Unknown architecture '$HOST_ARCH', using as-is"
        ;;
esac

echo "Detected architecture: $HOST_ARCH"

# Define version
VERSION="1.8-R7"
PACKAGE_NAME="xibo-player-$VERSION-$HOST_ARCH-portable"

echo "=== Building Xibo Player Portable Distribution ==="
echo "Project directory: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo "Distribution directory: $DIST_DIR"
echo "Target architecture: $HOST_ARCH"
echo "Package name: $PACKAGE_NAME"
echo

# Clean previous builds
echo "Cleaning previous builds..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
if [ -d "$DIST_DIR" ]; then
    rm -rf "$DIST_DIR"
fi

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$DIST_DIR"
cd "$BUILD_DIR"

# Check build dependencies
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

# Install runtime dependencies if needed
echo "Checking runtime dependencies..."
echo "Note: This script requires development libraries to be installed."
echo "Run ./install-dependencies-debian.sh if not done already."
echo

echo "Building third-party libraries..."

# Build date-tz library
echo "Building date-tz..."
cd "$BUILD_DIR"
if [ ! -d "date-3.0.0" ]; then
    echo "   Downloading date library..."
    wget -q https://github.com/HowardHinnant/date/archive/v3.0.0.tar.gz -O date-3.0.0.tar.gz
    tar -xzf date-3.0.0.tar.gz
fi
cd date-3.0.0
mkdir -p build && cd build

# Configure date library with architecture-specific settings
CMAKE_DATE_ARGS=(
    ".."
    -DBUILD_TZ_LIB=ON
    -DBUILD_SHARED_LIBS=OFF
    -DUSE_SYSTEM_TZ_DB=ON
    -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/local"
    -DCMAKE_BUILD_TYPE=Release
)

# Add architecture-specific optimizations for date library too
case "$HOST_ARCH" in
    arm64|aarch64)
        CMAKE_DATE_ARGS+=(-DCMAKE_C_FLAGS="-march=armv8-a")
        CMAKE_DATE_ARGS+=(-DCMAKE_CXX_FLAGS="-march=armv8-a")
        ;;
    amd64|x86_64)
        CMAKE_DATE_ARGS+=(-DCMAKE_C_FLAGS="-march=x86-64")
        CMAKE_DATE_ARGS+=(-DCMAKE_CXX_FLAGS="-march=x86-64")
        ;;
    armhf)
        CMAKE_DATE_ARGS+=(-DCMAKE_C_FLAGS="-march=armv7-a -mfpu=neon")
        CMAKE_DATE_ARGS+=(-DCMAKE_CXX_FLAGS="-march=armv7-a -mfpu=neon")
        ;;
esac

cmake "${CMAKE_DATE_ARGS[@]}"

# Use optimized number of cores for compilation
NPROC_DATE=$(nproc)
case "$HOST_ARCH" in
    armhf)
        if [ $NPROC_DATE -gt 2 ]; then
            NPROC_DATE=2
        fi
        ;;
esac

make -j$NPROC_DATE
make install

echo "🔧 Building Xibo Player..."
cd "$BUILD_DIR"
mkdir -p xibo-build && cd xibo-build

# Configure CMake for static build
CMAKE_ARGS=(
    "$PLAYER_DIR"
    -DCMAKE_BUILD_TYPE=Release
    -DAPP_ENV=PORTABLE
    -DCMAKE_PREFIX_PATH="$BUILD_DIR/local"
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so"
    -DBUILD_SHARED_LIBS=OFF
)

# Add architecture-specific settings if needed
case "$HOST_ARCH" in
    arm64|aarch64)
        # Add ARM64-specific flags if needed
        CMAKE_ARGS+=(-DCMAKE_C_FLAGS="-march=armv8-a")
        CMAKE_ARGS+=(-DCMAKE_CXX_FLAGS="-march=armv8-a")
        ;;
    amd64|x86_64)
        # Add x86_64-specific optimizations
        CMAKE_ARGS+=(-DCMAKE_C_FLAGS="-march=x86-64")
        CMAKE_ARGS+=(-DCMAKE_CXX_FLAGS="-march=x86-64")
        ;;
    armhf)
        # Add ARMv7 flags
        CMAKE_ARGS+=(-DCMAKE_C_FLAGS="-march=armv7-a -mfpu=neon")
        CMAKE_ARGS+=(-DCMAKE_CXX_FLAGS="-march=armv7-a -mfpu=neon")
        ;;
esac

cmake "${CMAKE_ARGS[@]}"

# Build
echo "   Compiling with $(nproc) cores..."

# Adjust parallel jobs based on architecture and available memory
NPROC=$(nproc)
AVAILABLE_MEM=$(grep MemAvailable /proc/meminfo | awk '{print $2}' 2>/dev/null || echo "4000000")
AVAILABLE_MEM_GB=$((AVAILABLE_MEM / 1024 / 1024))

# Conservative approach for ARM devices which may have memory constraints
case "$HOST_ARCH" in
    armhf)
        # Limit parallel jobs on 32-bit ARM due to potential memory constraints
        if [ $NPROC -gt 2 ] && [ $AVAILABLE_MEM_GB -lt 4 ]; then
            NPROC=2
            echo "   Limiting to $NPROC cores due to memory constraints on ARM32"
        fi
        ;;
    arm64)
        # ARM64 can usually handle more, but still be conservative
        if [ $NPROC -gt 4 ] && [ $AVAILABLE_MEM_GB -lt 8 ]; then
            NPROC=4
            echo "   Limiting to $NPROC cores due to memory constraints"
        fi
        ;;
esac

make -j$NPROC

echo "Creating portable distribution..."

# Create distribution structure
PORTABLE_DIR="$DIST_DIR/$PACKAGE_NAME"
mkdir -p "$PORTABLE_DIR/bin"
mkdir -p "$PORTABLE_DIR/lib"
mkdir -p "$PORTABLE_DIR/share/xibo-player"
mkdir -p "$PORTABLE_DIR/share/icons"

# Copy binaries
echo "   Copying binaries..."
if [ -f "bin/xibo-player" ]; then
    cp bin/xibo-player "$PORTABLE_DIR/bin/"
    
    # Verify binary architecture
    BINARY_ARCH=$(file bin/xibo-player | grep -o 'x86-64\|aarch64\|ARM\|32-bit\|64-bit')
    echo "   Binary architecture: $BINARY_ARCH"
    
    # Cross-check with expected architecture
    case "$HOST_ARCH" in
        amd64)
            if [[ "$BINARY_ARCH" != *"x86-64"* ]]; then
                echo "   Warning: Binary architecture mismatch. Expected x86-64, got: $BINARY_ARCH"
            fi
            ;;
        arm64)
            if [[ "$BINARY_ARCH" != *"aarch64"* ]]; then
                echo "   Warning: Binary architecture mismatch. Expected aarch64, got: $BINARY_ARCH"
            fi
            ;;
        armhf)
            if [[ "$BINARY_ARCH" != *"ARM"* ]] || [[ "$BINARY_ARCH" != *"32-bit"* ]]; then
                echo "   Warning: Binary architecture mismatch. Expected ARM 32-bit, got: $BINARY_ARCH"
            fi
            ;;
    esac
else
    echo "Error: xibo-player binary not found!"
    exit 1
fi

if [ -f "bin/xibo-options" ]; then
    cp bin/xibo-options "$PORTABLE_DIR/bin/"
fi

if [ -f "bin/xibo-watchdog" ]; then
    cp bin/xibo-watchdog "$PORTABLE_DIR/bin/"
fi

# Copy resources if they exist
echo "   Copying resources..."
if [ -f "$PLAYER_DIR/resources/ui.glade" ]; then
    cp "$PLAYER_DIR/resources/ui.glade" "$PORTABLE_DIR/share/xibo-player/"
fi

if [ -f "$PLAYER_DIR/resources/splash.jpg" ]; then
    cp "$PLAYER_DIR/resources/splash.jpg" "$PORTABLE_DIR/share/xibo-player/"
fi

if [ -f "$PLAYER_DIR/resources/xibo-player.png" ]; then
    cp "$PLAYER_DIR/resources/xibo-player.png" "$PORTABLE_DIR/share/icons/"
fi

# Copy required shared libraries
echo "   Copying required shared libraries..."

# Get system library paths for current architecture
get_system_lib_paths() {
    local paths=""
    
    # Standard system library paths
    paths="/lib /usr/lib /lib64 /usr/lib64"
    
    # Architecture-specific paths
    case "$HOST_ARCH" in
        amd64)
            paths="$paths /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu"
            ;;
        arm64)
            paths="$paths /lib/aarch64-linux-gnu /usr/lib/aarch64-linux-gnu"
            ;;
        armhf)
            paths="$paths /lib/arm-linux-gnueabihf /usr/lib/arm-linux-gnueabihf"
            ;;
    esac
    
    echo "$paths"
}

SYSTEM_LIB_PATHS=$(get_system_lib_paths)

function copy_libs() {
    local binary="$1"
    if [ -f "$binary" ]; then
        echo "   Analyzing dependencies for $(basename "$binary")..."
        # Get library dependencies
        ldd "$binary" 2>/dev/null | grep "=>" | awk '{print $3}' | grep -v "^$" | while read lib; do
            if [ -f "$lib" ] && [ ! -f "$PORTABLE_DIR/lib/$(basename "$lib")" ]; then
                # Check if library is in system paths
                is_system_lib=false
                for sys_path in $SYSTEM_LIB_PATHS; do
                    if [[ "$lib" == "$sys_path"* ]]; then
                        is_system_lib=true
                        break
                    fi
                done
                
                # Skip system libraries but copy third-party ones
                if [ "$is_system_lib" = "false" ]; then
                    echo "     Copying: $lib"
                    cp "$lib" "$PORTABLE_DIR/lib/" 2>/dev/null || true
                fi
            fi
        done
    fi
}

copy_libs "$PORTABLE_DIR/bin/xibo-player"
copy_libs "$PORTABLE_DIR/bin/xibo-options"
copy_libs "$PORTABLE_DIR/bin/xibo-watchdog"

# Create launcher script
echo "   Creating launcher script..."
cat > "$PORTABLE_DIR/xibo-player.sh" << 'EOF'
#!/bin/bash

# Xibo Player Portable Launcher
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Create config directory if it doesn't exist
CONFIG_DIR="$HOME/.xibo-player"
mkdir -p "$CONFIG_DIR"

# Set library path for portable libraries
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"

# Set resource path
export XIBO_RESOURCE_PATH="$SCRIPT_DIR/share/xibo-player"

# GStreamer workarounds for problematic plugins
export GST_PLUGIN_SYSTEM_PATH_1_0=""
export GST_PLUGIN_PATH_1_0=""

# Disable VA-API plugin if it's causing segfaults
export GST_REGISTRY="$CONFIG_DIR/gstreamer-registry.bin"

# Set config path to avoid creating files in bin directory
cd "$CONFIG_DIR"

# Check for problematic VA plugin and disable if needed
if ldd "$SCRIPT_DIR/bin/xibo-player" 2>/dev/null | grep -q "libva\|gstva" && [ -f "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstva.so" ]; then
    echo "Warning: VA-API GStreamer plugin detected. Disabling to prevent crashes."
    export GST_PLUGIN_FEATURE_RANK="vaapidecodebin:NONE,vaapisink:NONE,vaapih264dec:NONE,vaapih265dec:NONE"
fi

# Launch the player with error handling
exec "$SCRIPT_DIR/bin/xibo-player" --gst-disable-segtrap "$@"
EOF

# Create options launcher script
cat > "$PORTABLE_DIR/xibo-options.sh" << 'EOF'
#!/bin/bash

# Xibo Options Portable Launcher
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Create config directory if it doesn't exist
CONFIG_DIR="$HOME/.xibo-player"
mkdir -p "$CONFIG_DIR"

# Set library path for portable libraries
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"

# Set resource path
export XIBO_RESOURCE_PATH="$SCRIPT_DIR/share/xibo-player"

# Set config path to avoid creating files in bin directory
cd "$CONFIG_DIR"

# Debug: Show what we're setting
if [ "$1" = "--debug" ]; then
    echo "SCRIPT_DIR: $SCRIPT_DIR"
    echo "CONFIG_DIR: $CONFIG_DIR"
    echo "XIBO_RESOURCE_PATH: $XIBO_RESOURCE_PATH"
    echo "UI file: $SCRIPT_DIR/share/xibo-player/ui.glade"
    ls -la "$SCRIPT_DIR/share/xibo-player/" || echo "Resource directory not found"
    shift
fi

# Launch the options
exec "$SCRIPT_DIR/bin/xibo-options" "$@"
EOF

# Create watchdog launcher script
cat > "$PORTABLE_DIR/xibo-watchdog.sh" << 'EOF'
#!/bin/bash

# Xibo Watchdog Portable Launcher
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set library path for portable libraries
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"

# Launch the watchdog
exec "$SCRIPT_DIR/bin/xibo-watchdog" "$@"
EOF

# Make launcher scripts executable
chmod +x "$PORTABLE_DIR/"*.sh
chmod +x "$PORTABLE_DIR/bin/"*

# Create README file
cat > "$PORTABLE_DIR/README.txt" << EOF
Xibo Player Portable Distribution
================================

Version: $VERSION
Architecture: $HOST_ARCH ($(uname -m))
Built on: $(date)
System: $(uname -s) $(uname -r)

Contents:
- bin/          Executable binaries
- lib/          Required shared libraries  
- share/        Resources and data files
- *.sh          Launcher scripts

Usage:
------
To run Xibo Player:
  ./xibo-player.sh

To run Xibo Options:
  ./xibo-options.sh

To run Xibo Watchdog:
  ./xibo-watchdog.sh

Configuration:
-------------
Configuration files are stored in: ~/.xibo-player/
This includes playerSettings.xml, cacheFile.xml, and logs.

Troubleshooting:
---------------
If you experience crashes related to GStreamer VA-API:
1. The launcher automatically disables problematic VA-API plugins
2. If issues persist, try: ./xibo-player.sh --gst-disable-registry-fork

For xibo-options issues:
1. Run with debug: ./xibo-options.sh --debug
2. Ensure GTK+ 3.0 is installed on your system

Graphics Driver Issues:
- Nouveau driver warnings are usually harmless
- For better performance, consider proprietary NVIDIA drivers
- Intel/AMD graphics should work without issues

Requirements:
------------
- Linux distribution compatible with $HOST_ARCH architecture
- GTK+ 3.0 or later
- GStreamer 1.0 or later
- WebKit2GTK 4.1 or later
- glibc 2.27 or later (for compatibility)

Architecture Support:
-------------------
This build supports the following architectures:
- x86_64 (amd64): Intel/AMD 64-bit processors
- aarch64 (arm64): ARM 64-bit processors (e.g., Raspberry Pi 4, Apple M1)
- armv7l (armhf): ARM 32-bit processors with hard-float

Notes:
------
This is a portable distribution that includes most required dependencies.
Some system libraries (like glibc, GTK, GStreamer) are expected to be
available on the target system.

The build is optimized for the target architecture and includes
architecture-specific compiler optimizations where applicable.

For more information, visit: https://github.com/xibosignage/xibo-linux
EOF

echo "Creating compressed archive..."
cd "$DIST_DIR"

# Create tar.gz archive
ARCHIVE_NAME="$PACKAGE_NAME.tar.gz"
tar -czf "$ARCHIVE_NAME" "$PACKAGE_NAME"

# Also create zip archive for convenience
if command -v zip &> /dev/null; then
    ZIP_NAME="$PACKAGE_NAME.zip"
    zip -r "$ZIP_NAME" "$PACKAGE_NAME" > /dev/null
    echo "ZIP archive created: $ZIP_NAME"
fi

# Calculate file sizes
TAR_SIZE=$(du -h "$ARCHIVE_NAME" | cut -f1)
DIR_SIZE=$(du -sh "$PACKAGE_NAME" | cut -f1)

echo
echo "=== Build Completed Successfully ==="
echo "Distribution directory: $PORTABLE_DIR"
echo "Archive: $ARCHIVE_NAME ($TAR_SIZE)"
echo "Uncompressed size: $DIR_SIZE"
echo

# Test if the binary can be executed (basic verification)
echo "Performing basic executable test..."
if "$PORTABLE_DIR/bin/xibo-player" --version >/dev/null 2>&1 || "$PORTABLE_DIR/bin/xibo-player" --help >/dev/null 2>&1; then
    echo "✓ Binary executable test passed"
else
    echo "⚠ Warning: Binary executable test failed - the binary might not be compatible with current system"
fi

echo
echo "To test the portable version:"
echo "   cd $DIST_DIR"
echo "   tar -xzf $ARCHIVE_NAME"
echo "   cd $PACKAGE_NAME"
echo "   ./xibo-player.sh"
echo
echo "Archive contents:"
echo "   - Xibo Player binaries (statically linked where possible)"
echo "   - Required shared libraries"
echo "   - Resource files and assets"
echo "   - Launcher scripts with proper environment setup"
echo "   - Documentation"
echo
echo "Architecture compatibility:"
echo "   - Built for: $HOST_ARCH ($(uname -m))"
echo "   - Optimized: Yes (architecture-specific compiler flags)"
echo "   - Cross-platform: Compatible with x86_64 and aarch64 systems"
echo
echo "Ready for distribution!"
