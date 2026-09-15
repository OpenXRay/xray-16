#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <arm64|x86_64> <install-directory>"
    exit 1
fi

ARCH="$1"
case "${ARCH}" in
    arm64|x86_64) ;;
    *) echo "Unsupported macOS architecture: ${ARCH}"; exit 1 ;;
esac

mkdir -p "$2"
INSTALL_DIR="$(cd "$2" && pwd)"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/openxray-sdl2.XXXXXX")"
trap 'rm -rf "${BUILD_DIR}"' EXIT

# Keep this version aligned with the Windows SDL2 NuGet packages.
SDL_VERSION="2.32.4"
SDL_SHA256="f15b478253e1ff6dac62257ded225ff4e7d0c5230204ac3450f1144ee806f934"
ARCHIVE="${BUILD_DIR}/SDL2-${SDL_VERSION}.tar.gz"

# Homebrew's sdl2 alias provides SDL2-compat, which depends on SDL3.
curl --fail --location --retry 3 \
    "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.tar.gz" \
    --output "${ARCHIVE}"
printf '%s  %s\n' "${SDL_SHA256}" "${ARCHIVE}" | shasum -a 256 --check
tar -xzf "${ARCHIVE}" -C "${BUILD_DIR}"

cmake -G Ninja -S "${BUILD_DIR}/SDL2-${SDL_VERSION}" -B "${BUILD_DIR}/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DCMAKE_INSTALL_NAME_DIR="${INSTALL_DIR}/lib" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF \
    -DSDL2_DISABLE_SDL2MAIN=ON -DSDL2_DISABLE_UNINSTALL=ON
cmake --build "${BUILD_DIR}/build" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-4}"
cmake --install "${BUILD_DIR}/build"

echo "Native SDL2 ${SDL_VERSION} installed. Configure OpenXRay with:"
echo "  -DSDL2_DIR=${INSTALL_DIR}/lib/cmake/SDL2"
