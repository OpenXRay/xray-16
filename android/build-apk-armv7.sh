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

ndk_dir=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}
sdk_dir=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}
deps_prefix=${ANDROID_DEPS_PREFIX:-}
sdl_dir=${SDL2_ANDROID_HOME:-}
gradle_bin=${GRADLE_BIN:-}
android_lto=${XRAY_ANDROID_ENABLE_LTO:-OFF}
build_dir=${XRAY_ANDROID_APK_BUILD_DIR:-"$repo_dir/build/android-apk-armv7"}

remove_path()
{
    if [ -e "$1" ] || [ -L "$1" ]; then
        find "$1" -depth -delete
    fi
}

if [ -z "$ndk_dir" ] || [ ! -f "$ndk_dir/build/cmake/android.toolchain.cmake" ]; then
    echo "ANDROID_NDK_HOME must point to an installed Android NDK" >&2
    exit 2
fi
if [ -z "$sdk_dir" ] || [ ! -x "$sdk_dir/platform-tools/adb" ]; then
    echo "ANDROID_SDK_ROOT must point to an installed Android SDK" >&2
    exit 2
fi
if [ -z "$deps_prefix" ] || [ ! -f "$deps_prefix/lib/cmake/SDL2/SDL2Config.cmake" ]; then
    echo "ANDROID_DEPS_PREFIX must contain the Android/armeabi-v7a dependency prefix" >&2
    exit 2
fi
if [ -z "$sdl_dir" ] || [ ! -f "$sdl_dir/android-project/gradlew" ]; then
    echo "SDL2_ANDROID_HOME must point to an SDL2 source tree with android-project" >&2
    exit 2
fi

native_build_dir="$build_dir/native"
ANDROID_NDK_HOME="$ndk_dir" \
ANDROID_DEPS_PREFIX="$deps_prefix" \
XRAY_ANDROID_BUILD_DIR="$native_build_dir" \
XRAY_ANDROID_SHARED=ON \
"$script_dir/build-armv7.sh" "-DXRAY_ENABLE_LTO=$android_lto" "$@"

native_lib="$repo_dir/bin/armv7-a/ReleaseMasterGold/libmain.so"
if [ ! -f "$native_lib" ]; then
    echo "native APK library was not produced: $native_lib" >&2
    exit 1
fi

project_dir="$build_dir/gradle-project"
remove_path "$project_dir"
mkdir -p "$project_dir"
cp -R "$sdl_dir/android-project/." "$project_dir/"

# A reusable SDL tree may contain Gradle task history and outputs from an
# earlier OpenXRay package. Never let those copied files make assembleDebug
# consider a stale APK up to date after the launcher manifest or native engine
# changed.
for stale_path in \
    "$project_dir/.gradle" \
    "$project_dir/.cxx" \
    "$project_dir/build" \
    "$project_dir/app/.cxx" \
    "$project_dir/app/build" \
    "$project_dir/app/src/main/java/org/openxray" \
    "$project_dir/app/src/main/jniLibs" \
    "$project_dir/app/src/main/assets"; do
    remove_path "$stale_path"
done

cp "$repo_dir/android/apk/app/build.gradle" "$project_dir/app/build.gradle"
python3 "$repo_dir/android/apk/generate-options.py" --check
cp "$repo_dir/android/apk/app/src/main/AndroidManifest.xml" "$project_dir/app/src/main/AndroidManifest.xml"
mkdir -p "$project_dir/app/src/main/java/org/openxray/app"
cp "$repo_dir/android/apk/app/src/main/java/org/openxray/app/"*.java \
    "$project_dir/app/src/main/java/org/openxray/app/"
mkdir -p "$project_dir/app/src/main/res/values"
cp "$repo_dir/android/apk/app/src/main/res/values/strings.xml" "$project_dir/app/src/main/res/values/strings.xml"
cp "$repo_dir/android/apk/app/src/main/res/values/styles.xml" "$project_dir/app/src/main/res/values/styles.xml"
asset_root="$project_dir/app/src/main/assets"
remove_path "$asset_root"
mkdir -p "$asset_root/gamedata"
cp "$repo_dir/res/fsgame.ltx" "$asset_root/fsgame.ltx"
cp -R "$repo_dir/res/gamedata/." "$asset_root/gamedata/"
cp "$repo_dir/android/apk/android_mobile_minimum.ltx" \
    "$asset_root/gamedata/configs/android_mobile_minimum.ltx"
if [ -d "$asset_root/gamedata/gamedata" ]; then
    echo "APK asset staging unexpectedly nested gamedata inside itself" >&2
    exit 1
fi

native_lib_dir="$project_dir/app/src/main/jniLibs/armeabi-v7a"
mkdir -p "$native_lib_dir"
cp "$native_lib" "$native_lib_dir/libmain.so"
cp "$deps_prefix/lib/libopenal.so" "$native_lib_dir/libopenal.so"
cp "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/libc++_shared.so" \
    "$native_lib_dir/libc++_shared.so"

strip_bin="$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
if [ -x "$strip_bin" ]; then
    "$strip_bin" --strip-unneeded \
        "$native_lib_dir/libmain.so" \
        "$native_lib_dir/libopenal.so" \
        "$native_lib_dir/libc++_shared.so"
fi

chmod +x "$project_dir/gradlew"
(
    cd "$project_dir"
    set --
    if [ "${XRAY_ANDROID_GRADLE_OFFLINE:-OFF}" = "ON" ]; then
        set -- --offline
    fi
    if [ -n "$gradle_bin" ]; then
        "$gradle_bin" "$@" --no-daemon --no-build-cache clean assembleDebug
    else
        ./gradlew "$@" --no-daemon --no-build-cache clean assembleDebug
    fi
)

apk="$project_dir/app/build/outputs/apk/debug/app-debug.apk"
mkdir -p "$repo_dir/build"
port_version=$(sed -n '1p' "$script_dir/PORT_VERSION")
output_apk="$repo_dir/build/openxray-armv7-launcher-v$port_version-debug.apk"

# AGP 8.1 aligns uncompressed native-library ZIP entries to 4 KiB. Re-align
# those package entries to 16 KiB before signing. This is package-level
# alignment; it does not rewrite the ELF segments of the pinned ARMv7
# dependencies. The Gradle debug build above creates the standard debug
# keystore when it is not present yet.
build_tools_dir=$(find "$sdk_dir/build-tools" -mindepth 1 -maxdepth 1 -type d -print | sort -V | tail -1)
zipalign_bin="$build_tools_dir/zipalign"
apksigner_bin="$build_tools_dir/apksigner"
aapt_bin="$build_tools_dir/aapt"
if [ ! -x "$zipalign_bin" ] || [ ! -x "$apksigner_bin" ] || [ ! -x "$aapt_bin" ]; then
    echo "Android SDK build-tools with aapt, zipalign and apksigner are required" >&2
    exit 2
fi

if ! "$aapt_bin" dump badging "$apk" | grep -Fq "versionName='$port_version'"; then
    echo "Gradle produced an APK with a stale launcher version (expected $port_version)" >&2
    exit 1
fi
if ! unzip -p "$apk" lib/armeabi-v7a/libmain.so | cmp - "$native_lib_dir/libmain.so"; then
    echo "Gradle produced an APK with a stale native engine" >&2
    exit 1
fi
packaged_abis=$(zipinfo -1 "$apk" | sed -n 's#^lib/\([^/]*\)/.*#\1#p' | sort -u)
if [ "$packaged_abis" != "armeabi-v7a" ]; then
    echo "Gradle produced unexpected APK ABIs: $packaged_abis" >&2
    exit 1
fi
if zipinfo -1 "$apk" | grep -q '^assets/gamedata/gamedata/'; then
    echo "Gradle packaged a duplicate nested gamedata tree" >&2
    exit 1
fi
duplicate_entries=$(zipinfo -1 "$apk" | sort | uniq -d)
if [ -n "$duplicate_entries" ]; then
    echo "Gradle produced duplicate APK entries:" >&2
    echo "$duplicate_entries" >&2
    exit 1
fi

if [ -n "${ANDROID_DEBUG_KEYSTORE:-}" ]; then
    debug_keystore=$ANDROID_DEBUG_KEYSTORE
elif [ -n "${ANDROID_USER_HOME:-}" ]; then
    debug_keystore="$ANDROID_USER_HOME/debug.keystore"
elif [ -n "${HOME:-}" ]; then
    debug_keystore="$HOME/.android/debug.keystore"
else
    echo "Set ANDROID_DEBUG_KEYSTORE when HOME and ANDROID_USER_HOME are unavailable" >&2
    exit 2
fi
if [ ! -f "$debug_keystore" ]; then
    echo "Gradle did not create the debug keystore: $debug_keystore" >&2
    exit 2
fi

aligned_apk="$build_dir/app-debug-16k-aligned.apk"
"$zipalign_bin" -f -P 16 4 "$apk" "$aligned_apk"
"$apksigner_bin" sign \
    --ks "$debug_keystore" \
    --ks-pass "pass:${ANDROID_DEBUG_KEYSTORE_PASS:-android}" \
    --key-pass "pass:${ANDROID_DEBUG_KEY_PASS:-android}" \
    --ks-key-alias "${ANDROID_DEBUG_KEY_ALIAS:-androiddebugkey}" \
    --v4-signing-enabled false \
    --out "$output_apk" \
    "$aligned_apk"
unlink "$aligned_apk"
"$zipalign_bin" -c -P 16 4 "$output_apk"
"$apksigner_bin" verify "$output_apk"
printf '%s\n' "$output_apk"
