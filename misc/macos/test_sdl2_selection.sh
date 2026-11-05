#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TEST_DIR="$(mktemp -d "${TMPDIR:-/tmp}/openxray-sdl2-test.XXXXXX")"
trap 'rm -rf "${TEST_DIR}"' EXIT

mkdir -p "${TEST_DIR}/project" "${TEST_DIR}/fallback"
cat > "${TEST_DIR}/project/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.23)
project(SDL2SelectionTests NONE)

# Only the fixture package may satisfy discovery, independent of the host.
set(CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH FALSE)
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
list(APPEND CMAKE_MODULE_PATH "${XRAY_CMAKE_DIR}")
include(XRay.SDL2)

get_target_property(sdl2_imported SDL2::SDL2 IMPORTED)
if (EXPECT_INSTALLED AND NOT sdl2_imported)
    message(FATAL_ERROR "Expected the installed SDL2 package, but selected the source fallback")
elseif (NOT EXPECT_INSTALLED AND sdl2_imported)
    message(FATAL_ERROR "Expected the source fallback, but retained an unsuitable imported target")
endif()
CMAKE

# Exercise selection without downloading or compiling SDL in every test case.
cat > "${TEST_DIR}/fallback/CMakeLists.txt" <<'CMAKE'
add_library(SDL2 INTERFACE)
add_library(SDL2::SDL2 ALIAS SDL2)
CMAKE

write_package() {
    local directory="$1"
    local version="$2"
    local marker="$3"
    mkdir -p "${directory}"
    printf '%s\n' "${marker}" > "${directory}/libSDL2.dylib"
    printf 'set(PACKAGE_VERSION "%s")\n' "${version}" > "${directory}/SDL2ConfigVersion.cmake"
    cat >> "${directory}/SDL2ConfigVersion.cmake" <<'CMAKE'
if (PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
CMAKE
    cat > "${directory}/SDL2Config.cmake" <<'CMAKE'
add_library(SDL2::SDL2 SHARED IMPORTED)
set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_CONFIGURATIONS RELEASE
    IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/libSDL2.dylib"
)
CMAKE
}

run_case() {
    local name="$1"
    local expected_installed="$2"
    shift 2
    if ! cmake -S "${TEST_DIR}/project" -B "${TEST_DIR}/build-${name}" \
        -DXRAY_CMAKE_DIR="${ROOT_DIR}/cmake" \
        -DFETCHCONTENT_SOURCE_DIR_SDL2="${TEST_DIR}/fallback" \
        -DEXPECT_INSTALLED="${expected_installed}" "$@" > "${TEST_DIR}/${name}.log" 2>&1; then
        cat "${TEST_DIR}/${name}.log"
        exit 1
    fi
    echo "PASS: ${name}"
}

write_package "${TEST_DIR}/native" 2.32.4 native-sdl2
write_package "${TEST_DIR}/minimum" 2.0.18 native-sdl2
write_package "${TEST_DIR}/old" 2.0.17 native-sdl2
write_package "${TEST_DIR}/major3" 3.0.0 native-sdl3
write_package "${TEST_DIR}/compat" 2.32.70 SDL2COMPAT_DEBUG_LOGGING

run_case native ON -DSDL2_DIR="${TEST_DIR}/native"
run_case minimum ON -DSDL2_DIR="${TEST_DIR}/minimum"
run_case old OFF -DSDL2_DIR="${TEST_DIR}/old"
run_case major3 OFF -DSDL2_DIR="${TEST_DIR}/major3"
run_case compat OFF -DSDL2_DIR="${TEST_DIR}/compat"
run_case missing OFF -DCMAKE_DISABLE_FIND_PACKAGE_SDL2=TRUE

# Reconfiguration must change targets when an installed package is replaced.
run_case native OFF -DSDL2_DIR="${TEST_DIR}/compat"
run_case native ON -DSDL2_DIR="${TEST_DIR}/native"
