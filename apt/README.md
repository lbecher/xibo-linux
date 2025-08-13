# Xibo Player - Debian Package (.deb)

This directory contains the scripts and files required to create a Debian package (.deb) for the Xibo Player.

## Structure

```
apt/
├── DEBIAN/
│   ├── control             # Package metadata
│   ├── postinst            # Post-installation script
│   └── postrm              # Post-removal script
├── usr/
│   ├── bin/                # Binaries (copied during the build)
│   └── share/
│       ├── applications/   # .desktop files
│       ├── icons/          # Icons (copied during the build)
│       └── xibo-player/    # Resources (copied during the build)
├── build-deb.sh            # Main build script
├── install-dependencies.sh # Script to install dependencies
└── README.md               # This file
```

## How to Use

### 1. Install dependencies (optional)

```bash
cd apt/
./install-dependencies.sh
```

### 2. Build the .deb package

```bash
cd apt/
./build-deb.sh
```

This script will:
- Automatically detect the host architecture
- Install build dependencies
- Build third-party libraries (date-tz, sqlite-orm)
- Compile Xibo Player for the current architecture
- Update the control file with the correct architecture
- Create the .deb package

### 3. Install the package

The package name will include the detected architecture:

```bash
sudo dpkg -i ../xibo-player_1.8-R7_<architecture>.deb
sudo apt-get install -f  # If there are unsatisfied dependencies
```

Where `<architecture>` can be: amd64, arm64, armhf, i386, riscv64, etc.

### 4. Run

After installation, you can run:

- **Xibo Player**: `xibo-player` or via the application menu
- **Xibo Options**: `xibo-options` or via the application menu
- **Xibo Watchdog**: `xibo-watchdog` (usually used internally)

## Dependencies

### Runtime Dependencies (installed automatically)
- libcrypto++6
- libboost-date-time1.71.0 (or 1.74.0)
- libboost-filesystem1.71.0 (or 1.74.0)
- libboost-program-options1.71.0 (or 1.74.0)
- libboost-thread1.71.0 (or 1.74.0)
- libglu1-mesa
- freeglut3
- libzmq5
- libgtkmm-3.0-1v5
- libcanberra-gtk3-module
- libwebkit2gtk-4.0-37
- libgpm2
- libslang2
- gstreamer1.0-plugins-good
- gstreamer1.0-plugins-base
- gstreamer1.0-gl
- gstreamer1.0-libav
- gstreamer1.0-gtk3
- libspdlog1
- libsqlite3-0

### Build Dependencies (installed during the build)
- cmake
- g++
- pkg-config
- libcrypto++-dev
- libboost-all-dev
- libgtkmm-3.0-dev
- libwebkit2gtk-4.0-dev
- libglibmm-2.4-dev
- libzmq3-dev
- libspdlog-dev
- libssl-dev
- libsqlite3-dev
- libcurl4-gnutls-dev

## Uninstall

```bash
sudo apt remove xibo-player
```

## Notes

- The package is built for the current host architecture (automatically detected)
- Supported architectures: amd64, arm64, armhf, i386, riscv64, and others
- The build process downloads and compiles some third-party libraries locally
- The package installs binaries in `/usr/bin/` and resources in `/usr/share/xibo-player/`
- Configuration files are stored in the user's home directory
- GStreamer paths are automatically configured based on the target architecture
