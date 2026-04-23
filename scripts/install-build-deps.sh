#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Install build dependencies for xibo-linux.

Usage:
  ./scripts/install-build-deps.sh [--with-cross] [--arch <arm64|armhf>]...

Options:
  --with-cross         Also install cross toolchains and target-arch dev packages.
  --arch <arch>        Target architecture for cross deps. Repeatable.
                       Defaults to arm64 and armhf when --with-cross is enabled.
  -h, --help           Show this help.

Examples:
  ./scripts/install-build-deps.sh
  ./scripts/install-build-deps.sh --with-cross
  ./scripts/install-build-deps.sh --with-cross --arch arm64
EOF
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Missing required command: $1" >&2
        exit 1
    }
}

run_apt() {
    if [[ ${EUID} -eq 0 ]]; then
        apt-get "$@"
    else
        sudo apt-get "$@"
    fi
}

normalize_arch() {
    case "$1" in
        arm64|aarch64) echo "arm64" ;;
        armhf|arm) echo "armhf" ;;
        amd64|x86_64) echo "amd64" ;;
        *)
            echo "Unsupported architecture: $1" >&2
            exit 1
            ;;
    esac
}

with_cross="false"
declare -a cross_arches=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --with-cross)
            with_cross="true"
            shift
            ;;
        --arch)
            [[ $# -ge 2 ]] || { echo "--arch requires a value" >&2; exit 1; }
            cross_arches+=("$(normalize_arch "$2")")
            shift 2
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

if [[ "${with_cross}" == "true" && ${#cross_arches[@]} -eq 0 ]]; then
    cross_arches=(arm64 armhf)
fi

require_cmd dpkg
require_cmd apt-get

native_packages=(
    build-essential
    cmake
    git
    pkg-config
    debhelper-compat
    debhelper
    devscripts
    dpkg-dev
    fakeroot
    libgtest-dev
    libgmock-dev
    libspdlog-dev
    libssl-dev
    libzmq3-dev
    libsqlite3-dev
    libsqlitecpp-dev
    libhowardhinnant-date-dev
    libcrypto++-dev
    libdbus-1-dev
    libgstreamer1.0-dev
    libgstreamer-plugins-base1.0-dev
    libboost-dev
    libboost-system-dev
    libboost-thread-dev
    libboost-filesystem-dev
    libboost-date-time-dev
    libboost-program-options-dev
    libgtkmm-4.0-dev
    libwebkitgtk-6.0-dev
)

echo "==> Updating apt index"
run_apt update

echo "==> Installing native build dependencies"
run_apt install -y "${native_packages[@]}"

if [[ "${with_cross}" != "true" ]]; then
    exit 0
fi

declare -A cross_toolchain_pkg=(
    [arm64]=crossbuild-essential-arm64
    [armhf]=crossbuild-essential-armhf
)

target_dev_packages=(
    libglibmm-2.68-dev
    libgstreamer1.0-dev
    libgstreamer-plugins-base1.0-dev
    libwebkitgtk-6.0-dev
    libsqlite3-dev
    libssl-dev
    libzmq3-dev
    libboost-system-dev
    libboost-thread-dev
    libboost-filesystem-dev
    libboost-date-time-dev
    libboost-program-options-dev
    libcrypto++-dev
    libdbus-1-dev
    libhowardhinnant-date-dev
    libgtkmm-4.0-dev
)

foreign_arches="$(dpkg --print-foreign-architectures)"
for arch in "${cross_arches[@]}"; do
    if [[ "${arch}" == "amd64" ]]; then
        continue
    fi

    if ! grep -qx "${arch}" <<<"${foreign_arches}"; then
        echo "==> Adding foreign architecture ${arch}"
        if [[ ${EUID} -eq 0 ]]; then
            dpkg --add-architecture "${arch}"
        else
            sudo dpkg --add-architecture "${arch}"
        fi
    fi
done

echo "==> Updating apt index after adding foreign architectures"
run_apt update

for arch in "${cross_arches[@]}"; do
    if [[ "${arch}" == "amd64" ]]; then
        continue
    fi

    echo "==> Installing cross toolchain for ${arch}"
    run_apt install -y "${cross_toolchain_pkg[${arch}]}"

    arch_packages=()
    for pkg in "${target_dev_packages[@]}"; do
        arch_packages+=("${pkg}:${arch}")
    done

    echo "==> Installing target development packages for ${arch}"
    run_apt install -y "${arch_packages[@]}"
done
