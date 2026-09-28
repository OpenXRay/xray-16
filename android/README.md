# OpenXRay Android port

This directory contains the experimental Android build maintained by this
fork. It produces an SDL2 launcher APK and a native OpenXRay engine for the
`armeabi-v7a` ABI. Proprietary S.T.A.L.K.E.R. files are not included.

## Runtime scope and limitations

- Minimum Android version: Android 8.0 / API 26.
- Target and compile SDK: API 36.
- Packaged ABI: `armeabi-v7a` only. A 64-bit phone is compatible only if its
  Android build still supports 32-bit applications.
- Gameplay needs OpenGL ES 3.1, at least four draw buffers and four color
  attachments. Renderer decisions do not use GPU vendor/model allowlists.
- The Vulkan choice currently runs a VK0 device/surface/swapchain/present probe
  and then explicitly falls back to GLES gameplay. It is not a Vulkan gameplay
  renderer yet.
- Call of Pripyat is the intended profile. SoC and CS choices only pass the
  existing compatibility flags and do not change upstream game support.
- The 32-bit address space remains a limit for large levels and mods. Devices
  without usable BC/S3TC texture support decode those textures in memory.

The launcher is portrait and the engine activity is landscape. The engine uses
the selected installation as its filesystem root. In particular, the normal
desktop paths remain in use:

```text
<STALKER>/_appdata_/user.ltx
<STALKER>/_appdata_/savedgames/
<STALKER>/_appdata_/screenshots/
<STALKER>/_appdata_/logs/
```

The launcher creates the four `_appdata_` directories when needed and performs
an actual write/delete test before starting the engine. Game resources,
archives, shaders and `fsgame.ltx` are not rewritten.

## Supported build host

The maintained build path is Linux x86_64. On Debian or Ubuntu install the host
tools first:

```sh
sudo apt update
sudo apt install git openjdk-17-jdk python3 ninja-build cmake unzip zip zstd
```

JDK 17 is required by the pinned Android Gradle Plugin. `adb`, `aapt`,
`zipalign` and `apksigner` come from the Android SDK in the prepared kit. The
kit also supplies the static `qemu-i386` used by the LuaJIT cross-build helper.

Clone the fork with submodules and select its `dev` branch:

```sh
git clone --recursive https://github.com/r0shn1ch/xray-16.git
cd xray-16
git switch dev
git submodule update --init --recursive
```

## Recommended build with the prepared kit

Extract the OpenXRay Android build-kit archive. Its top-level directory must
contain `build-kit-env.sh`, `BUILD-MANIFEST.txt` and `toolchain/`. The currently
tested kit contains:

- Android NDK r30 (`30.0.16248370`);
- Android SDK platform 36, build-tools 36.0.0 and platform-tools;
- CMake 3.31.6 and Ninja;
- Gradle 8.1.1 with an offline dependency cache;
- SDL 2.30.2 source and prebuilt ARMv7 SDL2, OpenAL Soft, JPEG, Ogg, Vorbis,
  Theora and LZO dependencies;
- `qemu-i386-static` for the LuaJIT host bootstrap.

Build the complete APK from the repository root:

```sh
export XRAY_ANDROID_KIT_ROOT=/absolute/path/to/openxray-android-build-kit-v0.8.0
test -f "$XRAY_ANDROID_KIT_ROOT/build-kit-env.sh"
./android/build-harness.sh --apk
```

Build only the native engine with:

```sh
./android/build-harness.sh --native
```

The APK and reproducibility manifest are written to:

```text
build/openxray-armv7-launcher-v<android/PORT_VERSION>-debug.apk
build/android-apk-armv7/build-manifest.txt
```

Android ReleaseMasterGold builds use ARM instruction mode and `-O2`. LTO is
disabled by default while the ARMv7 startup regression is being isolated. Both
settings can be changed explicitly for comparison builds:

```sh
XRAY_ANDROID_ENABLE_LTO=ON ./android/build-harness.sh --apk
XRAY_ANDROID_ARM_MODE=thumb ./android/build-harness.sh --apk
```

Do not use those overrides for a baseline bug report. The manifest records the
commit, toolchain paths, ARM mode and LTO choice used for each build.

## Build with a separately installed toolchain

This path is for maintainers who already have matching ARMv7 dependencies. The
repository does not currently build all third-party Android libraries from
source, so `ANDROID_DEPS_PREFIX` is mandatory.

Install SDK components equivalent to the prepared kit, for example:

```sh
sdkmanager \
  "platform-tools" \
  "platforms;android-36" \
  "build-tools;36.0.0" \
  "cmake;3.31.6" \
  "ndk;30.0.16248370"
```

Then export absolute paths and build:

```sh
export ANDROID_NDK_HOME=/absolute/path/to/android-sdk/ndk/30.0.16248370
export ANDROID_SDK_ROOT=/absolute/path/to/android-sdk
export ANDROID_DEPS_PREFIX=/absolute/path/to/android-deps-armv7
export SDL2_ANDROID_HOME=/absolute/path/to/SDL-2.30.2
export GRADLE_BIN=/absolute/path/to/gradle-8.1.1/bin/gradle
export XRAY_QEMU_I386_STATIC=/absolute/path/to/qemu-i386-static
./android/build-apk-armv7.sh
```

The dependency prefix must provide `lib/cmake/SDL2/SDL2Config.cmake` and ARMv7
builds of SDL2, OpenAL Soft, JPEG, Ogg, Vorbis, Theora and LZO. The packaging
script checks the version and ABI, compares the packaged `libmain.so` with the
just-built engine, rejects duplicate assets, applies 16 KiB ZIP alignment and
verifies the debug signature.

## Install and run

Enable USB debugging, connect the device, then check its ABI support:

```sh
adb devices
adb shell getprop ro.product.cpu.abilist
adb install -r build/openxray-armv7-launcher-v0.9.24-debug.apk
adb shell am start -n org.openxray.stalker/org.openxray.app.LauncherActivity
```

If `adb install -r` reports an incompatible signature, the installed APK was
signed with another debug key. Back up anything important, then reinstall:

```sh
adb uninstall org.openxray.stalker
adb install build/openxray-armv7-launcher-v0.9.24-debug.apk
```

Uninstalling clears launcher preferences and private renderer support files,
but saves and `user.ltx` under the selected installation's `_appdata_` remain.

On Android 11 or newer, grant the launcher **All files access**. On Android
8–10, grant the requested storage permissions. Select a normal shared-storage
filesystem path such as `/storage/emulated/0/STALKER`, not a cloud/document
provider URI. The root must contain `fsgame.ltx` and the original game
directories or archives. The launcher will refuse to start if it cannot create
and write `<STALKER>/_appdata_`.

Launcher options are applied after reading `user.ltx` and before game startup:

- **Renderer:** Auto and OpenGL ES use GLES; Vulkan runs the probe then GLES.
- **Graphics:** Auto selects Low. Low through Extreme remain selectable.
- **3D resolution:** Auto keeps the display aspect ratio and caps internal
  width at 1280; lower fixed choices and native resolution are available.
- **FPS:** shows the engine counter in opaque red at the top center.
- **Touch controls:** optional overlay including Escape.

## Diagnostics

The diagnostics page reads bounded log tails on a background thread. Android
bootstrap/crash logs are normally available at:

```text
/storage/emulated/0/openxray/android.log
/storage/emulated/0/openxray/activity.log
```

The engine's normal log path remains `<STALKER>/_appdata_/logs/`. For a crash
report, start from a cleared logcat and collect all three sources:

```sh
adb logcat -c
adb shell am force-stop org.openxray.stalker
adb shell am start -n org.openxray.stalker/org.openxray.app.LauncherActivity
# Reproduce the problem, then run:
adb logcat -d -b all -v threadtime OpenXRay:I DEBUG:E '*:S' > openxray-logcat.txt
adb logcat -d -b crash -v threadtime > openxray-crash.txt
adb pull /sdcard/openxray/android.log openxray-engine.log
adb pull /sdcard/openxray/activity.log openxray-activity.log
adb pull /sdcard/STALKER/_appdata_/logs openxray-game-logs
```

The unstripped `bin/armv7-a/ReleaseMasterGold/libmain.so` from the exact same
commit is required for native symbolication. Activity diagnostics record the
APK version, version code and native git revision so stale installations can be
identified.

Platform boundaries are documented in [PORT_AUDIT.md](PORT_AUDIT.md). Launcher
behavior is summarized in [apk/README.md](apk/README.md).
