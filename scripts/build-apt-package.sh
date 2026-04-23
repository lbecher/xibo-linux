#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Build a .deb package for xibo-linux.

Usage:
  ./scripts/build-apt-package.sh [options]

Options:
  --arch <amd64|arm64|armhf>   Target architecture (default: host arch).
  --build-dir <dir>             Build directory (default: build-<arch>).
  --output-dir <dir>            Output directory for .deb (default: dist).
  --version <ver>               Debian package version (default: git-derived).
  --clean                       Remove build directory before configuring.
  -h, --help                    Show this help.

Examples:
  ./scripts/build-apt-package.sh
  ./scripts/build-apt-package.sh --arch arm64 --clean
  ./scripts/build-apt-package.sh --arch armhf --version 1.8.0
EOF
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Missing required command: $1" >&2
        exit 1
    }
}

normalize_arch() {
    case "$1" in
        amd64|x86_64) echo "amd64" ;;
        arm64|aarch64) echo "arm64" ;;
        armhf|arm) echo "armhf" ;;
        *)
            echo "Unsupported architecture: $1" >&2
            exit 1
            ;;
    esac
}

debian_version_from_git() {
    local raw
    raw="$(git describe --tags --always --dirty 2>/dev/null || git rev-parse --short HEAD)"
    raw="${raw#v}"
    raw="$(echo "${raw}" | sed 's/[^0-9A-Za-z.+~-]/-/g')"
    if [[ ! "${raw}" =~ ^[0-9] ]]; then
        raw="0~${raw}"
    fi
    echo "${raw}"
}

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

host_arch="$(dpkg --print-architecture)"
target_arch="${host_arch}"
build_dir=""
output_dir="${repo_root}/dist"
pkg_version=""
clean_build="false"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch)
            [[ $# -ge 2 ]] || { echo "--arch requires a value" >&2; exit 1; }
            target_arch="$(normalize_arch "$2")"
            shift 2
            ;;
        --build-dir)
            [[ $# -ge 2 ]] || { echo "--build-dir requires a value" >&2; exit 1; }
            build_dir="$2"
            shift 2
            ;;
        --output-dir)
            [[ $# -ge 2 ]] || { echo "--output-dir requires a value" >&2; exit 1; }
            output_dir="$2"
            shift 2
            ;;
        --version)
            [[ $# -ge 2 ]] || { echo "--version requires a value" >&2; exit 1; }
            pkg_version="$2"
            shift 2
            ;;
        --clean)
            clean_build="true"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
done

require_cmd cmake
require_cmd dpkg-deb
require_cmd git

if [[ -z "${build_dir}" ]]; then
    build_dir="${repo_root}/build-${target_arch}"
fi

if [[ -z "${pkg_version}" ]]; then
    pkg_version="$(debian_version_from_git)"
fi

if [[ "${clean_build}" == "true" ]]; then
    rm -rf "${build_dir}"
fi

mkdir -p "${build_dir}" "${output_dir}"

cmake_args=(
    -S "${repo_root}/player"
    -B "${build_dir}"
    -DCMAKE_BUILD_TYPE=Release
)

if [[ "${target_arch}" != "${host_arch}" ]]; then
    case "${target_arch}" in
        arm64)
            target_triplet="aarch64-linux-gnu"
            target_cpu="aarch64"
            ;;
        armhf)
            target_triplet="arm-linux-gnueabihf"
            target_cpu="arm"
            ;;
        amd64)
            target_triplet="x86_64-linux-gnu"
            target_cpu="x86_64"
            ;;
    esac

    require_cmd "${target_triplet}-gcc"
    require_cmd "${target_triplet}-g++"

    export CC="${target_triplet}-gcc"
    export CXX="${target_triplet}-g++"
    export PKG_CONFIG_LIBDIR="/usr/lib/${target_triplet}/pkgconfig:/usr/share/pkgconfig"
    export PKG_CONFIG_PATH="/usr/lib/${target_triplet}/pkgconfig"

    cmake_args+=(
        -DCMAKE_SYSTEM_NAME=Linux
        -DCMAKE_SYSTEM_PROCESSOR="${target_cpu}"
        -DCMAKE_C_COMPILER="${CC}"
        -DCMAKE_CXX_COMPILER="${CXX}"
        -DCMAKE_LIBRARY_ARCHITECTURE="${target_triplet}"
        -DCMAKE_FIND_ROOT_PATH="/usr/${target_triplet}"
    )
fi

echo "==> Configuring (${target_arch})"
cmake "${cmake_args[@]}"

echo "==> Building (${target_arch})"
cmake --build "${build_dir}" -j"$(nproc)"

staging_dir="${output_dir}/pkgroot-xibo-player-${target_arch}"
rm -rf "${staging_dir}"
mkdir -p "${staging_dir}/DEBIAN"
mkdir -p "${staging_dir}/usr/bin"
mkdir -p "${staging_dir}/usr/share/xibo-player"
mkdir -p "${staging_dir}/usr/share/applications"
mkdir -p "${staging_dir}/usr/share/icons/hicolor/256x256/apps"

install -m 0755 "${build_dir}/bin/xibo-player" "${staging_dir}/usr/bin/xibo-player"
install -m 0755 "${build_dir}/bin/xibo-options" "${staging_dir}/usr/bin/xibo-options"
install -m 0755 "${build_dir}/bin/xibo-watchdog" "${staging_dir}/usr/bin/xibo-watchdog"

install -m 0644 "${repo_root}/player/resources/ui.glade" "${staging_dir}/usr/share/xibo-player/ui.glade"
install -m 0644 "${repo_root}/player/resources/splash.jpg" "${staging_dir}/usr/share/xibo-player/splash.jpg"
install -m 0644 "${repo_root}/player/resources/xibo-player.desktop" "${staging_dir}/usr/share/applications/xibo-player.desktop"
install -m 0644 "${repo_root}/player/resources/xibo-player.png" "${staging_dir}/usr/share/icons/hicolor/256x256/apps/xibo-player.png"

installed_size="$(du -sk "${staging_dir}" | awk '{print $1}')"

cat > "${staging_dir}/DEBIAN/control" <<EOF
Package: xibo-player
Version: ${pkg_version}
Section: video
Priority: optional
Architecture: ${target_arch}
Maintainer: Xibo Linux Maintainers <support@xibo.org.uk>
Depends: libc6, libstdc++6, libgtkmm-4.0-0, libglibmm-2.68-1t64, libwebkitgtk-6.0-4, libgstreamer1.0-0, libgstreamer-plugins-base1.0-0, libsqlite3-0, libssl3t64, libzmq5, libcrypto++8t64, libdbus-1-3, libdate-tz3, libboost-system1.83.0, libboost-thread1.83.0, libboost-filesystem1.83.0, libboost-date-time1.83.0, libboost-program-options1.83.0
Installed-Size: ${installed_size}
Description: Xibo Linux Player
 Digital signage player for Xibo CMS.
EOF

deb_path="${output_dir}/xibo-player_${pkg_version}_${target_arch}.deb"

echo "==> Building package ${deb_path}"
dpkg-deb --build --root-owner-group "${staging_dir}" "${deb_path}"

echo "Done: ${deb_path}"
