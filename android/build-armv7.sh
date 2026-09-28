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

"$script_dir/apply-patches.sh" "$repo_dir"
"$script_dir/prepare-source.sh" "$repo_dir"

ndk_dir=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}
build_dir=${XRAY_ANDROID_BUILD_DIR:-"$repo_dir/build/android-armv7"}
deps_prefix=${ANDROID_DEPS_PREFIX:-}
android_arm_mode=${XRAY_ANDROID_ARM_MODE:-arm}

case "$android_arm_mode" in
    arm|thumb) ;;
    *)
        echo "XRAY_ANDROID_ARM_MODE must be either arm or thumb" >&2
        exit 2
        ;;
esac

if [ -z "$ndk_dir" ] || [ ! -f "$ndk_dir/build/cmake/android.toolchain.cmake" ]; then
    echo "ANDROID_NDK_HOME must point to an installed Android NDK" >&2
    exit 2
fi

set -- "$@"
if [ -n "$deps_prefix" ]; then
    set -- "$@" "-DCMAKE_PREFIX_PATH=$deps_prefix"
    if [ -f "$deps_prefix/lib/cmake/SDL2/SDL2Config.cmake" ]; then
        set -- "$@" "-DSDL2_DIR=$deps_prefix/lib/cmake/SDL2"
    fi
    if [ -f "$deps_prefix/lib/cmake/OpenAL/OpenALConfig.cmake" ]; then
        set -- "$@" "-DOpenAL_DIR=$deps_prefix/lib/cmake/OpenAL"
    fi
    set -- "$@" \
        "-DJPEG_INCLUDE_DIR=$deps_prefix/include" \
        "-DJPEG_LIBRARY=$deps_prefix/lib/libjpeg.a" \
        "-DOGG_INCLUDE_DIR=$deps_prefix/include" \
        "-DOGG_LIBRARY=$deps_prefix/lib/libogg.a" \
        "-DVORBIS_INCLUDE_DIR=$deps_prefix/include" \
        "-DVORBIS_LIBRARY=$deps_prefix/lib/libvorbis.a" \
        "-DVORBISENC_LIBRARY=$deps_prefix/lib/libvorbisenc.a" \
        "-DVORBISFILE_LIBRARY=$deps_prefix/lib/libvorbisfile.a" \
        "-DTHEORA_INCLUDE_DIR=$deps_prefix/include" \
        "-DTHEORA_LIBRARY=$deps_prefix/lib/libtheora.a" \
        "-DTHEORADEC_LIBRARY=$deps_prefix/lib/libtheoradec.a" \
        "-DTHEORAENC_LIBRARY=$deps_prefix/lib/libtheoraenc.a" \
        "-DLZO_ROOT_DIR=$deps_prefix" \
        "-DLZO_INCLUDE_DIR=$deps_prefix/include" \
        "-DLZO_LIBRARY=$deps_prefix/lib/liblzo2.a"
fi
if [ -n "${SDL2_DIR:-}" ]; then
    set -- "$@" "-DSDL2_DIR=$SDL2_DIR"
fi
if [ "${XRAY_ANDROID_SHARED:-ON}" = "ON" ]; then
    set -- "$@" "-DXRAY_ANDROID_SHARED=ON"
    set -- "$@" "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
fi

# LuaJIT builds an i386 host helper even when the target is Android ARMv7.
# Make the known cross-host compiler and the bundled static emulator automatic,
# so a clean build never depends on manually repeated CMake cache flags.
luajit_host_compiler=${LUAJIT_HOST_C_COMPILER:-}
if [ -z "$luajit_host_compiler" ] && [ -x "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/i686-linux-android26-clang" ]; then
    luajit_host_compiler="$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/i686-linux-android26-clang"
fi
luajit_host_prefix=${LUAJIT_HOST_EXECUTABLE_PREFIX:-}
if [ -z "$luajit_host_prefix" ]; then
    for qemu_candidate in "${XRAY_QEMU_I386_STATIC:-}" \
        "$repo_dir/../qemu-user-static-root/usr/bin/qemu-i386-static" \
        "$repo_dir/../qemu-i386-static"; do
        if [ -x "$qemu_candidate" ]; then
            luajit_host_prefix="$qemu_candidate"
            break
        fi
    done
fi
luajit_host_extra_ldflags=${LUAJIT_HOST_EXTRA_LDFLAGS:-}
case "$luajit_host_compiler" in
    *android*)
        case " $luajit_host_extra_ldflags " in
            *" -lm "*) ;;
            *) luajit_host_extra_ldflags="$luajit_host_extra_ldflags -lm" ;;
        esac
        ;;
esac
if [ -n "$luajit_host_compiler" ]; then
    set -- "$@" "-DLUAJIT_HOST_C_COMPILER=$luajit_host_compiler"
fi
if [ -n "$luajit_host_extra_ldflags" ]; then
    set -- "$@" "-DLUAJIT_HOST_EXTRA_LDFLAGS=$luajit_host_extra_ldflags"
fi
if [ -n "$luajit_host_prefix" ]; then
    set -- "$@" "-DLUAJIT_HOST_EXECUTABLE_PREFIX=$luajit_host_prefix"
else
    echo "warning: no qemu-i386-static found; LuaJIT host bootstrap may require a 32-bit runtime" >&2
fi

cmake -S "$repo_dir" -B "$build_dir" -G "${CMAKE_GENERATOR:-Ninja}" \
    -DCMAKE_TOOLCHAIN_FILE="$ndk_dir/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=armeabi-v7a \
    -DANDROID_PLATFORM=android-36 \
    -DANDROID_STL=c++_shared \
    -DANDROID_ARM_MODE="$android_arm_mode" \
    -DBUILD_SHARED_LIBS=OFF \
    -DXRAY_USE_LUAJIT=ON \
    -DXRAY_ENABLE_TRACY=OFF \
    -DMEMORY_ALLOCATOR=standard \
    "$@"

cmake --build "$build_dir" --target xr_3da
