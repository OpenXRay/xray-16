#!/bin/sh
set -eu

kit_root=${XRAY_ANDROID_KIT_ROOT:?XRAY_ANDROID_KIT_ROOT must point to the extracted OpenXRay Android build kit}

export ANDROID_NDK_HOME="$kit_root/toolchain/android-ndk-r30"
export ANDROID_SDK_ROOT="$kit_root/toolchain/android-sdk"
export ANDROID_DEPS_PREFIX="$kit_root/toolchain/android-deps-armv7"
export SDL2_ANDROID_HOME="$kit_root/toolchain/SDL"
export GRADLE_BIN="$kit_root/toolchain/gradle-8.1.1/bin/gradle"
if [ -d "$kit_root/toolchain/gradle-user-home/caches/modules-2" ]; then
    export GRADLE_USER_HOME="$kit_root/toolchain/gradle-user-home"
    export XRAY_ANDROID_GRADLE_OFFLINE=ON
fi
if [ -x "$kit_root/toolchain/qemu-i386-static" ]; then
    export XRAY_QEMU_I386_STATIC="$kit_root/toolchain/qemu-i386-static"
fi

if [ ! -f "$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" ]; then
    echo "OpenXRay Android kit is incomplete: NDK is missing" >&2
    exit 2
fi
if [ ! -x "$ANDROID_SDK_ROOT/platform-tools/adb" ]; then
    echo "OpenXRay Android kit is incomplete: Android SDK is missing" >&2
    exit 2
fi
if [ ! -f "$ANDROID_DEPS_PREFIX/lib/cmake/SDL2/SDL2Config.cmake" ]; then
    echo "OpenXRay Android kit is incomplete: native dependencies are missing" >&2
    exit 2
fi
if [ ! -f "$SDL2_ANDROID_HOME/android-project/gradlew" ]; then
    echo "OpenXRay Android kit is incomplete: SDL2 Android project is missing" >&2
    exit 2
fi
if [ ! -x "$GRADLE_BIN" ]; then
    echo "OpenXRay Android kit is incomplete: Gradle is missing" >&2
    exit 2
fi
if [ "${XRAY_ANDROID_GRADLE_OFFLINE:-OFF}" = "ON" ] \
    && [ ! -d "${GRADLE_USER_HOME:-}/caches/modules-2" ]; then
    echo "OpenXRay Android kit is incomplete: offline Gradle cache is missing" >&2
    exit 2
fi

gradle_bin_dir=$(CDPATH= cd -- "$(dirname -- "$GRADLE_BIN")" && pwd)
kit_path="$ANDROID_SDK_ROOT/platform-tools:$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$gradle_bin_dir"
for cmake_bin in "$ANDROID_SDK_ROOT"/cmake/*/bin; do
    if [ -x "$cmake_bin/cmake" ]; then
        kit_path="$cmake_bin:$kit_path"
    fi
done
export PATH="$kit_path:$PATH"
