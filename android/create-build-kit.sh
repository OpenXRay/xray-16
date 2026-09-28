#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
port_version=$(sed -n '1p' "$script_dir/PORT_VERSION")
kit_version=${XRAY_ANDROID_KIT_VERSION:-$port_version}
output=${1:-"$repo_dir/build/openxray-android-build-kit-v$kit_version.tar.zst"}

ndk_dir=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}
sdk_dir=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}
deps_dir=${ANDROID_DEPS_PREFIX:-}
sdl_dir=${SDL2_ANDROID_HOME:-}
gradle_bin=${GRADLE_BIN:-}
gradle_cache_source=${XRAY_GRADLE_CACHE_SOURCE:-${GRADLE_USER_HOME:-}}
if [ -z "$gradle_cache_source" ] && [ -n "${HOME:-}" ]; then
    gradle_cache_source="$HOME/.gradle"
fi
qemu_i386=${XRAY_QEMU_I386_STATIC:-${LUAJIT_HOST_EXECUTABLE_PREFIX:-}}
if [ -z "$qemu_i386" ] && [ -x "$(CDPATH= cd -- "$repo_dir/.." && pwd)/qemu-user-static-root/usr/bin/qemu-i386-static" ]; then
    qemu_i386="$(CDPATH= cd -- "$repo_dir/.." && pwd)/qemu-user-static-root/usr/bin/qemu-i386-static"
fi

for required in "$ndk_dir" "$sdk_dir" "$deps_dir" "$sdl_dir" "$gradle_bin"; do
    if [ -z "$required" ]; then
        echo "Set ANDROID_NDK_HOME, ANDROID_SDK_ROOT, ANDROID_DEPS_PREFIX, SDL2_ANDROID_HOME and GRADLE_BIN before packaging the build kit" >&2
        exit 2
    fi
done

[ -f "$ndk_dir/build/cmake/android.toolchain.cmake" ] || { echo "NDK is invalid: $ndk_dir" >&2; exit 2; }
[ -x "$sdk_dir/platform-tools/adb" ] || { echo "SDK is invalid: $sdk_dir" >&2; exit 2; }
[ -f "$deps_dir/lib/cmake/SDL2/SDL2Config.cmake" ] || { echo "dependency prefix is invalid: $deps_dir" >&2; exit 2; }
[ -f "$sdl_dir/android-project/gradlew" ] || { echo "SDL2 source is invalid: $sdl_dir" >&2; exit 2; }
[ -x "$gradle_bin" ] || { echo "Gradle is invalid: $gradle_bin" >&2; exit 2; }
[ -d "$gradle_cache_source/caches/modules-2" ] || {
    echo "Gradle dependency cache is missing; set XRAY_GRADLE_CACHE_SOURCE after one successful APK build" >&2
    exit 2
}

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT HUP INT TERM
kit_dir="$stage/openxray-android-build-kit-v$kit_version"
mkdir -p "$kit_dir/toolchain" "$kit_dir/harness"

mkdir -p "$kit_dir/toolchain/android-ndk-r30"
cp -a "$ndk_dir/." "$kit_dir/toolchain/android-ndk-r30/"
mkdir -p "$kit_dir/toolchain/android-sdk"
cp -a "$sdk_dir/." "$kit_dir/toolchain/android-sdk/"
mkdir -p "$kit_dir/toolchain/android-deps-armv7"
cp -a "$deps_dir/." "$kit_dir/toolchain/android-deps-armv7/"
mkdir -p "$kit_dir/toolchain/SDL"
cp -a "$sdl_dir/." "$kit_dir/toolchain/SDL/"
gradle_dir=$(CDPATH= cd -- "$(dirname -- "$gradle_bin")/.." && pwd)
mkdir -p "$kit_dir/toolchain/gradle-8.1.1"
cp -a "$gradle_dir/." "$kit_dir/toolchain/gradle-8.1.1/"
mkdir -p "$kit_dir/toolchain/gradle-user-home"
cp -a "$gradle_cache_source/caches" "$kit_dir/toolchain/gradle-user-home/caches"
if [ -d "$gradle_cache_source/native" ]; then
    cp -a "$gradle_cache_source/native" "$kit_dir/toolchain/gradle-user-home/native"
fi
if [ -n "$qemu_i386" ] && [ -x "$qemu_i386" ]; then
    cp "$qemu_i386" "$kit_dir/toolchain/qemu-i386-static"
fi
cp -a "$repo_dir/android/." "$kit_dir/harness/"

cp "$repo_dir/android/build-kit-env.sh" "$kit_dir/build-kit-env.sh"
chmod +x "$kit_dir/build-kit-env.sh" "$kit_dir/harness/"*.sh

{
    echo "OpenXRay Android ARMv7 build kit"
    echo "kit_version=$kit_version"
    echo "source_commit=$(git -C "$repo_dir" rev-parse HEAD)"
    echo "source_branch=$(git -C "$repo_dir" rev-parse --abbrev-ref HEAD)"
    echo "source_date=$(git -C "$repo_dir" show -s --format=%cI HEAD)"
    echo "ndk=$(basename "$ndk_dir")"
    echo "sdk=$(basename "$sdk_dir")"
    echo "dependencies=$(basename "$deps_dir")"
    echo "sdl=$(basename "$sdl_dir")"
    echo "gradle=$(basename "$gradle_dir")"
    echo "gradle_dependency_cache=embedded (offline mode enabled)"
    apk_name="openxray-armv7-launcher-v$port_version-debug.apk"
    if [ -f "$repo_dir/build/$apk_name" ]; then
        (cd "$repo_dir/build" && sha256sum "$apk_name")
    fi
} > "$kit_dir/BUILD-MANIFEST.txt"

mkdir -p "$(dirname "$output")"
tar -C "$stage" -I 'zstd -T0 -10' -cf "$output" "openxray-android-build-kit-v$kit_version"
sha256sum "$output"
echo "$output"
