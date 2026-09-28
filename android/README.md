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

## Building from installed components

Use a Linux x86_64 host, JDK 17, Python 3, CMake 3.31 or newer, Ninja, Git,
Gradle 8.1.1, and `qemu-user-static`. On Debian/Ubuntu:

```sh
sudo apt update
sudo apt install git openjdk-17-jdk python3 ninja-build cmake unzip zip zstd qemu-user-static autoconf automake libtool pkg-config
```

Install the Android SDK command-line tools from Google, set `ANDROID_SDK_ROOT`,
and install platform tools, platform 36, build tools 36.0.0 and NDK r30:

```sh
export ANDROID_SDK_ROOT="$HOME/Android/Sdk"
"$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager" --licenses
"$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager" \
  'platform-tools' 'platforms;android-36' 'build-tools;36.0.0' 'ndk;30.0.16248370'
export ANDROID_NDK_HOME="$ANDROID_SDK_ROOT/ndk/30.0.16248370"
```

Install Gradle 8.1.1 and set `GRADLE_BIN` to its executable. Obtain SDL 2.30.2
sources and set `SDL2_ANDROID_HOME` to the source directory (which contains
`android-project/gradlew`). The Gradle project builds the SDL Android activity.

The native engine also needs an **ARMv7 / API 26** prefix containing `libSDL2.a`,
`libopenal.so`, `libjpeg.a`, `libogg.a`, `libvorbis.a`, `libvorbisenc.a`,
`libvorbisfile.a`, `libtheora.a`, `libtheoradec.a`, `libtheoraenc.a` and
`liblzo2.a` with matching headers. Sources are SDL2, OpenAL Soft,
libjpeg-turbo, libogg, libvorbis, libtheora and LZO. Each must be compiled for
`armeabi-v7a` with the same NDK and API level. For a CMake dependency (SDL2,
OpenAL Soft or libjpeg-turbo), the pattern is:

```sh
export ANDROID_DEPS_PREFIX="$HOME/openxray-android-deps/armv7"
cmake -S /path/to/dependency -B /tmp/dependency-armv7 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$ANDROID_DEPS_PREFIX" \
  -DBUILD_SHARED_LIBS=OFF
cmake --build /tmp/dependency-armv7
cmake --install /tmp/dependency-armv7
```

Build OpenAL Soft as a shared library; SDL2 and the other dependencies must
provide the static libraries listed above. For Autotools projects (Ogg,
Vorbis, Theora and LZO), use the NDK host compiler and build in dependency
order:

```sh
export TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin"
export CC="$TOOLCHAIN/armv7a-linux-androideabi26-clang"
export CXX="$TOOLCHAIN/armv7a-linux-androideabi26-clang++"
export AR="$TOOLCHAIN/llvm-ar" RANLIB="$TOOLCHAIN/llvm-ranlib"
export PKG_CONFIG_LIBDIR="$ANDROID_DEPS_PREFIX/lib/pkgconfig"
export CPPFLAGS="-I$ANDROID_DEPS_PREFIX/include"
export LDFLAGS="-L$ANDROID_DEPS_PREFIX/lib"
# Run each source project's ./configure --host=arm-linux-androideabi \
#   --prefix="$ANDROID_DEPS_PREFIX" --disable-shared --enable-static,
# then make -j$(nproc) && make install.
```

The LuaJIT cross build also needs an executable `qemu-i386-static`; set
`XRAY_QEMU_I386_STATIC` to its path. `ANDROID_DEPS_PREFIX` must contain
`lib/cmake/SDL2/SDL2Config.cmake`. Clone the repository and its submodules:

```sh
git clone --recursive https://github.com/r0shn1ch/xray-16.git
cd xray-16
git switch dev
git submodule update --init --recursive
```

Build the native engine or the APK directly with the repository scripts:

```sh
export SDL2_ANDROID_HOME=/path/to/SDL2-2.30.2
export GRADLE_BIN=/path/to/gradle-8.1.1/bin/gradle
export XRAY_QEMU_I386_STATIC=/usr/bin/qemu-i386-static
./android/build-armv7.sh
./android/build-apk-armv7.sh
```

The APK is written to `build/openxray-armv7-launcher-v<version>-debug.apk`;
the version comes from `android/PORT_VERSION`. The second line of that file is
Android's monotonically increasing `versionCode`. Enable the repository's
commit hook once per clone with `git config core.hooksPath .githooks`. Each
subsequent commit then increments both values in the same commit. The hook runs locally; no GitHub Actions job is needed. Re-running a failed
commit does not increment the version twice. For commits prepared through an API,
run `python3 android/update-version.py --pending` and include `android/PORT_VERSION`
in the same commit. `--check` verifies an already committed version.
The Gradle build, launcher display,
APK filename and archive scripts all read this file.

`XRAY_ANDROID_ARM_MODE=thumb` and `XRAY_ANDROID_ENABLE_LTO=ON` can be set for
comparison builds; normal builds use ARM mode and no LTO. The build manifest
in `build/android-apk-armv7/build-manifest.txt` records the source commit and
toolchain paths.

## Install and run

Enable USB debugging, connect the device, then check its ABI support:

```sh
adb devices
adb shell getprop ro.product.cpu.abilist
adb install -r "build/openxray-armv7-launcher-v$(head -n1 android/PORT_VERSION)-debug.apk"
adb shell am start -n org.openxray.stalker/org.openxray.app.LauncherActivity
```

If `adb install -r` reports an incompatible signature, the installed APK was
signed with another debug key. Back up anything important, then reinstall:

```sh
adb uninstall org.openxray.stalker
adb install "build/openxray-armv7-launcher-v$(head -n1 android/PORT_VERSION)-debug.apk"
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
<game>/_appdata_/logs/android_<timestamp>_<pid>_<id>.log
<game>/_appdata_/logs/activity_<timestamp>_<pid>_<id>.log
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
adb pull /sdcard/STALKER/_appdata_/logs openxray-game-logs
```

The unstripped `bin/armv7-a/ReleaseMasterGold/libmain.so` from the exact same
commit is required for native symbolication. Activity diagnostics record the
APK version, version code and native git revision so stale installations can be
identified.

Platform boundaries are documented in [PORT_AUDIT.md](PORT_AUDIT.md). Launcher
behavior is summarized in [apk/README.md](apk/README.md).

### Frame scheduling and visibility diagnostics

`rs_sheduler_budget` sets the maximum adaptive scheduling budget in milliseconds
(3–66; Android default 10, desktop default 66). Real-time updates and precaching
are unaffected. A running object callback cannot be interrupted; pending objects
retain their due-time order. Lower budgets can delay AI updates under load.
`[scheduler-profile]` records callback time, the current budget and maximum
lateness of processed objects.

On Android, `[sector-audit]` compares downward and upward camera rays against
both the collision tree and an independent double-precision triangle scan.
It captures the camera position and frame before scanning, so movement during
an audit does not change the reference ray. The two directions share a single scan per model, with an independent XZ
interval rejection before triangle intersection. Work is limited to 32768
triangles and a 1 ms target per frame, checked every 64 triangles. Up to eight samples
are collected per level, with at least five seconds between completed samples.
The audit only reports results; it does not override visibility.

For ordinary performance measurements, leave HOM and GPU occlusion enabled
and disable detailed OpenGL output. The launcher provides a reset button for
these diagnostic options. The sector and scheduler summaries are available
without enabling them.

Local checks:

```sh
python3 tests/test_android_version.py
python3 android/apk/generate-options.py --check
c++ -std=c++17 -O2 -Isrc tests/ray_query_audit.cpp -o /tmp/ray-query-audit
/tmp/ray-query-audit
```

Use `python3 android/analyze-sector-audit.py <timestamped-engine-log>` to check the capture.
Exit code 0 means all recorded audits completed and their ray results agree;
this does not prove rendering correctness. Exit code 1 reports differing ray
results. Exit code 2 indicates missing or incomplete audit evidence. Hit triangle
coordinates and metadata are recorded for offline reproduction, including when
the tree and reference agree. Progress reports identify long-running scans.

### Rendering measurements

`[render-quality]` records effective shadow-map size, reflections, sun, SSAO,
texture LOD and anisotropy. The original game preset is loaded first; generated
`quality_*.ltx` files apply the renderer-specific settings from
`android/apk/launcher-options.json`. Restart the engine after changing quality.

`[render-phases]` reports CPU average/maximum times for frame phases, including
GPU synchronization. `[gpu-time]` reports asynchronous elapsed-query samples
when supported; disjoint results are discarded and unavailable results never
block the CPU. GPU elapsed time can include gaps in command submission.
Enable **Проверка коллизий и секторов** only when investigating visibility;
the independent full-mesh audit is disabled during normal gameplay.

Saves, `user.ltx`, screenshots and logs follow the installation's `fsgame.ltx`
paths. Russian save names cross the Android filesystem boundary as UTF-8;
the original game's UI continues to use Windows-1251.
