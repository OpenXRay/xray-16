#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
kit_root=${XRAY_ANDROID_KIT_ROOT:-}

if [ -z "$kit_root" ]; then
    for candidate in "$repo_dir"/../openxray-android-build-kit-v* \
        "$repo_dir/.openxray-android-build-kit"; do
        if [ -f "$candidate/build-kit-env.sh" ]; then
            kit_root="$candidate"
        fi
    done
fi

if [ -n "$kit_root" ]; then
    export XRAY_ANDROID_KIT_ROOT="$kit_root"
    . "$kit_root/build-kit-env.sh"
fi

target=apk
if [ "$#" -gt 0 ]; then
    case "$1" in
        --apk) target=apk; shift ;;
        --native) target=native; shift ;;
        --help|-h)
            echo "usage: $0 [--apk|--native] [build options]"
            exit 0
            ;;
    esac
fi

case "$target" in
    apk) "$script_dir/build-apk-armv7.sh" "$@" ;;
    native) "$script_dir/build-armv7.sh" "$@" ;;
esac

manifest="$repo_dir/build/android-apk-armv7/build-manifest.txt"
mkdir -p "$(dirname "$manifest")"
{
    echo "OpenXRay Android ARMv7 build manifest"
    echo "build_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "repo_commit=$(git -C "$repo_dir" rev-parse HEAD)"
    echo "repo_branch=$(git -C "$repo_dir" rev-parse --abbrev-ref HEAD)"
    echo "target=$target"
    echo "ndk=${ANDROID_NDK_HOME:-unset}"
    echo "sdk=${ANDROID_SDK_ROOT:-unset}"
    echo "deps=${ANDROID_DEPS_PREFIX:-unset}"
    echo "sdl=${SDL2_ANDROID_HOME:-unset}"
    echo "arm_mode=${XRAY_ANDROID_ARM_MODE:-arm}"
    echo "lto=${XRAY_ANDROID_ENABLE_LTO:-OFF}"
    cmake --version | head -1
    if [ -n "${GRADLE_BIN:-}" ] && [ -x "${GRADLE_BIN:-}" ]; then
        "$GRADLE_BIN" --version | sed -n '1,5p'
    fi
} > "$manifest"
echo "Build manifest: $manifest"
