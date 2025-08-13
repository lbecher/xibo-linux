#!/bin/bash

set -e

echo "=== Installing Xibo Player Dependencies ==="

# Update package cache
sudo apt update

# Install runtime dependencies
echo "Installing runtime dependencies..."
sudo apt install -y \
    libcrypto++6 \
    libboost-date-time1.71.0 \
    libboost-filesystem1.71.0 \
    libboost-program-options1.71.0 \
    libboost-thread1.71.0 \
    libglu1-mesa \
    freeglut3 \
    libzmq5 \
    libgtkmm-3.0-1v5 \
    libcanberra-gtk3-module \
    libwebkit2gtk-4.0-37 \
    libgpm2 \
    libslang2 \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-base \
    gstreamer1.0-gl \
    gstreamer1.0-libav \
    gstreamer1.0-gtk3 \
    libspdlog1 \
    libsqlite3-0 || {
    
    # Fallback for different Boost versions
    echo "Trying alternative Boost versions..."
    sudo apt install -y \
        libboost-date-time1.74.0 \
        libboost-filesystem1.74.0 \
        libboost-program-options1.74.0 \
        libboost-thread1.74.0 || {
        
        # Final fallback
        echo "Installing generic Boost version..."
        sudo apt install -y \
            libboost-date-time-dev \
            libboost-filesystem-dev \
            libboost-program-options-dev \
            libboost-thread-dev
    }
}

echo "Dependencies installed successfully!"