# Building OpenXRay for Android

The Android port is maintained in this fork and is not an upstream OpenXRay
release target. It builds a debug APK containing the launcher and a native
engine. It does not contain S.T.A.L.K.E.R. game data.

Device test (APK 0.9.101, Xiaomi 23122PCD1G, Android 16) loaded a level and
entered gameplay with the 64-bit Vulkan engine. The earlier ARMv7 Vulkan build
ran out of process address space during level loading. For now, treat
`arm64-v8a` as a requirement for Vulkan gameplay and use an ARM64-only APK;
this is device evidence, not a proven requirement of Vulkan itself. The
current Vulkan image and frame rate still need work: missing dynamic objects,
flat sky, visual artifacts and roughly 11–12 FPS were reported in that test.

For Vulkan on a 64-bit device, build with `XRAY_ANDROID_ARM64_ONLY=ON`
and `./android/build-apk-dual.sh` using an ARM64 dependency prefix. ARMv7 gameplay
can exhaust its 32-bit address space while loading a large level; switching
the packaged engine to `arm64-v8a` removes that address-space ceiling. An APK
with both ABIs can be built by omitting `XRAY_ANDROID_ARM64_ONLY`; Android
selects the native ABI supported by the device, so a dual-ABI APK is not a
reliable way to force the 64-bit Vulkan engine. The build kit can include an
optional `ANDROID_DEPS_PREFIX_ARM64` alongside its ARMv7 prefix.

## Supported configuration

| Component | Value |
|---|---|
| Host | x86-64 Linux |
| Android version | API 26 (Android 8.0) or newer |
| ABI | `armeabi-v7a`; optional `arm64-v8a` via `build-apk-dual.sh` |
| Native API level | 26 |
| Compile and target SDK | 36 |
| NDK used by the current scripts | r30 (`30.0.16248370`) |
| CMake | 3.23 or newer |
| Java | JDK 17 |
| Gameplay renderer | OpenGL ES 3.1 |

An ARMv7-only APK requires device support for 32-bit applications. The OpenGL
ES driver must expose at least four draw buffers and four color attachments.
The ARMv7 engine remains subject to the 32-bit address-space limit.

Call of Pripyat is the intended game profile. The SoC and CS entries in the
launcher only pass `-soc` or `-cs`; they do not make those games supported by
this port.

## What the repository does and does not provide

The checked-in scripts configure OpenXRay, assemble the SDL Android project,
package native libraries and sign a debug APK. They do **not** download or
build third-party libraries. No Android APK or ready-made Android build kit is
published by the repository workflow.

You must provide:

- Android SDK platform 36, platform-tools and build-tools;
- Android NDK r30;
- an SDL2 2.0.18-or-newer source tree containing `android-project/gradlew`;
- an ARMv7/API 26 dependency prefix containing SDL2, OpenAL Soft, JPEG, Ogg,
  Vorbis, Theora and LZO.

Use the same SDL2 source revision for `SDL2_ANDROID_HOME` and for the SDL2
library in the dependency prefix. The repository does not pin that external
source, so record the revision with your build artifacts.

## 1. Install host and Android tools

On Debian or Ubuntu:

```sh
sudo apt update
sudo apt install git openjdk-17-jdk python3 cmake ninja-build \
  unzip zip qemu-user-static
```

Install Google's Android command-line tools, then install the packages used by
the project:

```sh
export ANDROID_SDK_ROOT="$HOME/Android/Sdk"

"$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager" --licenses
"$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager" \
  'platform-tools' \
  'platforms;android-36' \
  'build-tools;36.0.0' \
  'ndk;30.0.16248370'

export ANDROID_NDK_HOME="$ANDROID_SDK_ROOT/ndk/30.0.16248370"
export XRAY_QEMU_I386_STATIC=/usr/bin/qemu-i386-static
```

The APK script uses the SDL project's Gradle wrapper by default. `GRADLE_BIN`
is optional and should only be set when you deliberately want to override that
wrapper.

## 2. Prepare the source tree

```sh
git clone --recurse-submodules https://github.com/r0shn1ch/xray-16.git
cd xray-16
git switch dev
git submodule update --init --recursive
```

`android/prepare-source.sh` performs the same submodule check before every
native build. A source archive must contain the recursive submodule contents;
a GitHub-generated source ZIP is not sufficient by itself.

## 3. Provide the ARMv7 dependency prefix

Build every native dependency with the same NDK, ABI and API level:

```sh
-DANDROID_ABI=armeabi-v7a
-DANDROID_PLATFORM=android-26
-DANDROID_STL=c++_shared
```

Set `ANDROID_DEPS_PREFIX` to the install prefix. The build scripts expect these
files:

```text
lib/cmake/SDL2/SDL2Config.cmake
lib/libopenal.so
lib/libjpeg.a
lib/libogg.a
lib/libvorbis.a
lib/libvorbisenc.a
lib/libvorbisfile.a
lib/libtheora.a
lib/libtheoradec.a
lib/libtheoraenc.a
lib/liblzo2.a
```

The SDL2 CMake package must define the static `SDL2::SDL2` target. Matching
headers must be under `include/`. OpenAL is the only shared third-party library
copied into the APK; the codec libraries are linked into `libmain.so`.

Check the prefix before configuring OpenXRay:

```sh
export ANDROID_DEPS_PREFIX=/absolute/path/to/android-deps-armv7-api26

for file in \
  lib/cmake/SDL2/SDL2Config.cmake \
  lib/libopenal.so lib/libjpeg.a lib/libogg.a \
  lib/libvorbis.a lib/libvorbisenc.a lib/libvorbisfile.a \
  lib/libtheora.a lib/libtheoradec.a lib/libtheoraenc.a lib/liblzo2.a
do
  test -f "$ANDROID_DEPS_PREFIX/$file" || {
    echo "missing: $ANDROID_DEPS_PREFIX/$file" >&2
    exit 1
  }
done
```

If you build a CMake-based dependency yourself, this is the common toolchain
part of its configure command; project-specific feature switches still depend
on that library:

```sh
cmake -S /path/to/source -B /tmp/dependency-armv7 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=armeabi-v7a \
  -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$ANDROID_DEPS_PREFIX"
cmake --build /tmp/dependency-armv7
cmake --install /tmp/dependency-armv7
```

This is intentionally not presented as a universal dependency recipe: options
such as shared/static output and disabled tools differ between SDL2, OpenAL
Soft and the codec libraries.

## 4. Build

For the native engine only:

```sh
./android/build-armv7.sh
```

For the APK, point `SDL2_ANDROID_HOME` at the same SDL2 source revision used in
the dependency prefix:

```sh
export SDL2_ANDROID_HOME=/absolute/path/to/SDL
./android/build-apk-armv7.sh
```

The useful outputs are:

```text
bin/armv7-a/ReleaseMasterGold/libmain.so
build/openxray-armv7-launcher-v<version>-debug.apk
build/android-apk-armv7/build-manifest.txt
build/android-apk-armv7/android-update.json
```

The copy of `libmain.so` under `bin/` is unstripped and is required to
symbolicate native crashes. The APK is debug-signed. Reproducible distribution
requires a stable keystore supplied through the `ANDROID_DEBUG_KEYSTORE`,
`ANDROID_DEBUG_KEYSTORE_PASS`, `ANDROID_DEBUG_KEY_PASS` and
`ANDROID_DEBUG_KEY_ALIAS` variables.

Optional build switches:

```sh
XRAY_ANDROID_ARM_MODE=thumb ./android/build-apk-armv7.sh
XRAY_ANDROID_ENABLE_LTO=ON ./android/build-apk-armv7.sh
```

The default is ARM mode with LTO disabled.

## 5. Install and run

```sh
adb devices
adb shell getprop ro.product.cpu.abilist
adb install -r "build/openxray-armv7-launcher-v$(head -n1 android/PORT_VERSION)-debug.apk"
adb shell am start -n org.openxray.stalker/org.openxray.app.LauncherActivity
```

The ABI list must contain `armeabi-v7a`. If `adb install -r` reports a
signature mismatch, the installed APK was signed with another key. Uninstall
it before installing the new build; the uninstall removes launcher preferences
but does not remove game data stored outside the app:

```sh
adb uninstall org.openxray.stalker
adb install "build/openxray-armv7-launcher-v$(head -n1 android/PORT_VERSION)-debug.apk"
```

On Android 11 and newer, grant **All files access**. On Android 8-10, grant the
requested storage permissions. Select a normal shared-storage directory such
as `/storage/emulated/0/STALKER`; document-provider and cloud URIs are not
supported. The directory must contain `fsgame.ltx` and the original game
resources. The launcher also verifies that `<STALKER>/_appdata_` is writable.

The launcher does not rewrite archives, textures, shaders or `fsgame.ltx`.
Runtime data uses the normal desktop paths:

```text
<STALKER>/_appdata_/user.ltx
<STALKER>/_appdata_/savedgames/
<STALKER>/_appdata_/screenshots/
<STALKER>/_appdata_/logs/
```

## Renderer status

The launcher can request the separate Vulkan gameplay module. `Auto` selects it
only when the Vulkan probe and required game shaders pass, then selects GLES
otherwise. Explicit Vulkan requests fail if those requirements are missing.
Android hardware and representative gameplay have not yet been validated. See
[VULKAN_RENDERER_PLAN.md](VULKAN_RENDERER_PLAN.md) for the implemented Vulkan
pieces and remaining work.

The launcher can run GLES and Vulkan smoke tests without game data. The Vulkan
test draws depth-tested indexed, lit geometry and a textured UI overlay, reads back one pixel
from each draw and presents three frames.
A passing smoke test verifies a small render path only; it does not prove that
a level can be loaded or rendered correctly.

## Diagnostics

Start with a clean logcat, reproduce the problem, then collect Android and game
logs:

```sh
adb logcat -c
adb shell am force-stop org.openxray.stalker
adb shell am start -n org.openxray.stalker/org.openxray.app.LauncherActivity
# Reproduce the problem.
adb logcat -d -b all -v threadtime OpenXRay:I DEBUG:E '*:S' > openxray-logcat.txt
adb logcat -d -b crash -v threadtime > openxray-crash.txt
adb pull /sdcard/STALKER/_appdata_/logs openxray-game-logs
```

Replace `/sdcard/STALKER` with the directory selected in the launcher. Android
session logs are named `android_<timestamp>_<pid>_<id>.log` and
`activity_<timestamp>_<pid>_<id>.log`. Symbolicate a native crash with the
unstripped `libmain.so` from the exact APK build.

## Maintainer checks

```sh
python3 tests/test_android_version.py
python3 android/apk/generate-options.py --check
```

`android/PORT_VERSION` contains `versionName` and `versionCode`. Developers who
want automatic increments can enable the optional hook with
`git config core.hooksPath .githooks`. API-based commits must run
`python3 android/update-version.py --pending` explicitly.

`android/create-build-kit.sh` only archives an already prepared SDK, NDK,
dependency prefix, SDL source tree and Gradle cache. It does not create or
download any of them.

Launcher behavior is described in [apk/README.md](apk/README.md). Port-specific
resource and platform boundaries are recorded in [PORT_AUDIT.md](PORT_AUDIT.md).
