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
deps_prefix=${ANDROID_DEPS_PREFIX_ARMV7:-}
deps_prefix64=${ANDROID_DEPS_PREFIX_ARM64:-}
sdl_dir=${SDL2_ANDROID_HOME:-}
gradle_bin=${GRADLE_BIN:-}
android_lto=${XRAY_ANDROID_ENABLE_LTO:-OFF}
arm64_only=${XRAY_ANDROID_ARM64_ONLY:-OFF}
build_dir=${XRAY_ANDROID_APK_BUILD_DIR:-"$repo_dir/build/android-apk-dual"}

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
if [ -z "$deps_prefix" ] || [ ! -f "$deps_prefix/lib/cmake/SDL2/SDL2Config.cmake" ] ||
    [ -z "$deps_prefix64" ] || [ ! -f "$deps_prefix64/lib/cmake/SDL2/SDL2Config.cmake" ]; then
    echo "Both ANDROID_DEPS_PREFIX_ARMV7 and ANDROID_DEPS_PREFIX_ARM64 are required" >&2
    exit 2
fi
if [ -z "$sdl_dir" ] || [ ! -f "$sdl_dir/android-project/gradlew" ]; then
    echo "SDL2_ANDROID_HOME must point to an SDL2 source tree with android-project" >&2
    exit 2
fi

native_build_dir="$build_dir/native"
if [ "${XRAY_ANDROID_SKIP_NATIVE_BUILD:-OFF}" != ON ]; then
glslc_bin=${GLSLC_BIN:-glslc}
if ! command -v "$glslc_bin" >/dev/null 2>&1; then
    echo "glslc is required to compile the embedded Vulkan shaders" >&2
    exit 2
fi
for generator in \
    embed_vulkan_deferred_shaders.py \
    embed_vulkan_scene_shaders.py \
    embed_vulkan_weather_shader.py \
    embed_vulkan_water_shader.py \
    embed_vulkan_local_light_shader.py; do
    python3 "$repo_dir/tools/$generator" --glslc "$glslc_bin"
done
if [ "$arm64_only" != ON ]; then
ANDROID_NDK_HOME="$ndk_dir" \
ANDROID_DEPS_PREFIX="$deps_prefix" \
XRAY_ANDROID_BUILD_DIR="$native_build_dir" \
XRAY_ANDROID_SHARED=ON \
"$script_dir/build-armv7.sh" "-DXRAY_ENABLE_LTO=$android_lto" "$@"
fi
ANDROID_NDK_HOME="$ndk_dir" \
ANDROID_DEPS_PREFIX="$deps_prefix64" \
XRAY_ANDROID_BUILD_DIR="$build_dir/native-arm64" \
XRAY_ANDROID_SHARED=ON \
LUAJIT_HOST_EXECUTABLE_PREFIX= \
"$script_dir/build-arm64.sh" "-DXRAY_ENABLE_LTO=$android_lto" "$@"
fi

native_lib="$repo_dir/bin/armv7-a/ReleaseMasterGold/libmain.so"
native_lib64="$repo_dir/bin/aarch64/ReleaseMasterGold/libmain.so"
if [ ! -f "$native_lib64" ] || { [ "$arm64_only" != ON ] && [ ! -f "$native_lib" ]; }; then
    echo "native APK library was not produced" >&2
    exit 1
fi
abi_artifacts="$native_lib64:$build_dir/native-arm64/src/Layers/xrRenderVK/xrRenderVK.a
$native_lib64:$build_dir/native-arm64/src/xrScriptEngine/xrScriptEngine.a"
if [ "$arm64_only" != ON ]; then
    abi_artifacts="$abi_artifacts
$native_lib:$build_dir/native/src/Layers/xrRenderVK/xrRenderVK.a
$native_lib:$build_dir/native/src/xrScriptEngine/xrScriptEngine.a"
fi
printf '%s\n' "$abi_artifacts" | while IFS= read -r abi_artifact; do
    binary=${abi_artifact%%:*}
    dependency_archive=${abi_artifact#*:}
    if [ ! -f "$dependency_archive" ] || [ "$binary" -ot "$dependency_archive" ]; then
        echo "native APK library is older than its dependency $dependency_archive: $binary" >&2
        exit 1
    fi
done

project_dir=$(mktemp -d "$build_dir/gradle-project.XXXXXXXX")
trap 'remove_path "$project_dir"' EXIT
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

if [ "$arm64_only" = ON ]; then
    abi_filters="abiFilters 'arm64-v8a'"
else
    abi_filters="abiFilters 'armeabi-v7a', 'arm64-v8a'"
fi
sed "s/abiFilters .armeabi-v7a./$abi_filters/" \
    "$repo_dir/android/apk/app/build.gradle" > "$project_dir/app/build.gradle"
cp "$repo_dir/android/PORT_VERSION" "$project_dir/android-version.txt"
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
glslc_bin=${GLSLC_BIN:-glslc}
python3 "$repo_dir/tools/compile_vulkan_shader.py" --compiler glslc --dxc "$glslc_bin" \
    --manifest "$repo_dir/res/gamedata/shaders/vk/opaque-variants.json" \
    --output-dir "$repo_dir/res/gamedata/shaders"
cp -R "$repo_dir/res/gamedata/." "$asset_root/gamedata/"
python3 "$repo_dir/tools/check_vulkan_shader_assets.py" \
    --shader-root "$asset_root/gamedata/shaders" \
    --manifest "$repo_dir/res/gamedata/shaders/vk/opaque-variants.json"
cp "$repo_dir"/android/apk/quality_*.ltx "$asset_root/gamedata/configs/"
python3 "$repo_dir/tools/write_android_engine_bundle_marker.py" "$asset_root/gamedata"
if [ -d "$asset_root/gamedata/gamedata" ]; then
    echo "APK asset staging unexpectedly nested gamedata inside itself" >&2
    exit 1
fi

native_lib_dir="$project_dir/app/src/main/jniLibs/armeabi-v7a"
if [ "$arm64_only" != ON ]; then
mkdir -p "$native_lib_dir"
cp "$native_lib" "$native_lib_dir/libmain.so"
cp "$deps_prefix/lib/libopenal.so" "$native_lib_dir/libopenal.so"
cp "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/libc++_shared.so" \
    "$native_lib_dir/libc++_shared.so"
fi
native_lib_dir64="$project_dir/app/src/main/jniLibs/arm64-v8a"
mkdir -p "$native_lib_dir64"
cp "$native_lib64" "$native_lib_dir64/libmain.so"
cp "$deps_prefix64/lib/libopenal.so" "$native_lib_dir64/libopenal.so"
cp "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" \
    "$native_lib_dir64/libc++_shared.so"
# Optional official validation binary, supplied by the Android Vulkan SDK.
# Keep it out of release builds and ensure the ELF matches the packaged ABI.
if [ "$arm64_only" != ON ] && [ -n "${XRAY_ANDROID_VALIDATION_LAYER_ARMV7:-}" ]; then
    validation_layer=$XRAY_ANDROID_VALIDATION_LAYER_ARMV7
    if [ ! -f "$validation_layer" ]; then
        echo "Android validation layer does not exist: $validation_layer" >&2
        exit 2
    fi
    readelf_bin="$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf"
    if ! "$readelf_bin" -h "$validation_layer" | grep -Eq 'Machine:.*ARM($|[[:space:]])'; then
        echo "Android validation layer must be an ARMv7 ELF" >&2
        exit 2
    fi
    cp "$validation_layer" "$native_lib_dir/libVkLayer_khronos_validation.so"
fi
if [ -n "${XRAY_ANDROID_VALIDATION_LAYER_ARM64:-}" ]; then
    layer64=$XRAY_ANDROID_VALIDATION_LAYER_ARM64
    if [ ! -f "$layer64" ] || ! "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h "$layer64" | grep -Eq 'Machine:.*AArch64'; then
        echo "Android ARM64 validation layer must be an AArch64 ELF" >&2
        exit 2
    fi
    cp "$layer64" "$native_lib_dir64/libVkLayer_khronos_validation.so"
fi

strip_bin="$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
if [ ! -x "$strip_bin" ]; then
    echo "Android NDK llvm-strip is required for APK packaging" >&2
    exit 2
fi
if [ "$arm64_only" != ON ]; then
"$strip_bin" --strip-unneeded "$native_lib_dir/libmain.so" \
    "$native_lib_dir/libopenal.so" "$native_lib_dir/libc++_shared.so"
fi
"$strip_bin" --strip-unneeded \
    "$native_lib_dir64/libmain.so" \
    "$native_lib_dir64/libopenal.so" \
    "$native_lib_dir64/libc++_shared.so"
for packaged_main in "$native_lib_dir64/libmain.so"; do
    if "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -S "$packaged_main" | grep -q '[.]debug_'; then
        echo "Native debug sections must not be packaged in the APK: $packaged_main" >&2
        exit 1
    fi
done
if [ "$arm64_only" != ON ] && "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -S \
    "$native_lib_dir/libmain.so" | grep -q '[.]debug_'; then
    echo "Native ARMv7 debug sections must not be packaged" >&2
    exit 1
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
python3 "$repo_dir/tools/check_vulkan_shader_assets.py" \
    --shader-root "$asset_root/gamedata/shaders" \
    --manifest "$repo_dir/res/gamedata/shaders/vk/opaque-variants.json" \
    --apk "$apk"
if ! unzip -p "$apk" assets/gamedata/openxray-bundle.sha256 | \
    cmp - "$asset_root/gamedata/openxray-bundle.sha256"; then
    echo "Gradle packaged stale engine gamedata fingerprint" >&2
    exit 1
fi
if [ "$arm64_only" != ON ]; then
    python3 "$repo_dir/tools/check_android_vulkan_route.py" \
        --repo "$repo_dir" --sdl-root "$sdl_dir" --native-lib "$native_lib" \
        --readelf "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" --apk "$apk"
fi
python3 "$repo_dir/tools/check_android_vulkan_route.py" \
    --repo "$repo_dir" --sdl-root "$sdl_dir" --native-lib "$native_lib64" --abi arm64-v8a \
    --readelf "$ndk_dir/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" --apk "$apk"
mkdir -p "$repo_dir/build"
port_version=$(sed -n '1p' "$project_dir/android-version.txt")
port_version_code=$(sed -n '2p' "$project_dir/android-version.txt")
if ! cmp -s "$script_dir/PORT_VERSION" "$project_dir/android-version.txt"; then
    echo "Android version changed during the build; rebuild from a stable commit" >&2
    exit 1
fi
if [ "$arm64_only" = ON ]; then
    output_apk="$repo_dir/build/openxray-arm64-launcher-v$port_version-debug.apk"
else
    output_apk="$repo_dir/build/openxray-universal-launcher-v$port_version-debug.apk"
fi

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

apk_badging=$("$aapt_bin" dump badging "$apk")
if ! printf '%s\n' "$apk_badging" | grep -Fq "versionCode='$port_version_code' versionName='$port_version'"; then
    echo "Gradle produced an APK with a stale launcher version (expected $port_version / $port_version_code)" >&2
    exit 1
fi
if [ "$arm64_only" != ON ] && ! unzip -p "$apk" lib/armeabi-v7a/libmain.so | cmp - "$native_lib_dir/libmain.so"; then
    echo "Gradle produced an APK with a stale native engine" >&2
    exit 1
fi
if [ "$arm64_only" != ON ] && [ -n "${XRAY_ANDROID_VALIDATION_LAYER_ARMV7:-}" ] &&
    ! unzip -p "$apk" lib/armeabi-v7a/libVkLayer_khronos_validation.so |
        cmp - "$native_lib_dir/libVkLayer_khronos_validation.so"; then
    echo "Gradle did not package the requested Vulkan validation layer" >&2
    exit 1
fi
for abi in arm64-v8a $(if [ "$arm64_only" != ON ]; then printf 'armeabi-v7a'; fi); do
    libdir="$native_lib_dir"
    if [ "$abi" = arm64-v8a ]; then libdir="$native_lib_dir64"; fi
    for library in libmain.so libopenal.so libc++_shared.so; do
        if ! unzip -p "$apk" "lib/$abi/$library" | cmp - "$libdir/$library"; then
            echo "Gradle packaged a stale $abi/$library" >&2
            exit 1
        fi
    done
done
if [ -n "${XRAY_ANDROID_VALIDATION_LAYER_ARM64:-}" ] &&
    ! unzip -p "$apk" lib/arm64-v8a/libVkLayer_khronos_validation.so |
        cmp - "$native_lib_dir64/libVkLayer_khronos_validation.so"; then
    echo "Gradle did not package the ARM64 validation layer" >&2
    exit 1
fi
packaged_abis=$(zipinfo -1 "$apk" | sed -n 's#^lib/\([^/]*\)/.*#\1#p' | sort -u)
expected_abis=arm64-v8a
if [ "$arm64_only" != ON ]; then expected_abis="$(printf 'arm64-v8a\narmeabi-v7a')"; fi
if [ "$packaged_abis" != "$expected_abis" ]; then
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
python3 "$script_dir/create-update-manifest.py" \
    --apk "$output_apk" \
    --output "$build_dir/android-update.json"
printf '%s\n' "$output_apk"

manifest="$build_dir/build-manifest.txt"
{
    echo "repo_commit=${XRAY_SOURCE_COMMIT:-$(git -C "$repo_dir" rev-parse HEAD)}"
    echo "version=$port_version"
    echo "version_code=$port_version_code"
    echo "ndk=$ndk_dir"
    echo "sdk=$sdk_dir"
    echo "deps_armv7=$deps_prefix"
    echo "deps_arm64=$deps_prefix64"
    echo "sdl=$sdl_dir"
    echo "arm_mode=${XRAY_ANDROID_ARM_MODE:-arm}"
    echo "lto=$android_lto"
} > "$manifest"
echo "Build manifest: $manifest"
echo "Android release update manifest: $build_dir/android-update.json"
