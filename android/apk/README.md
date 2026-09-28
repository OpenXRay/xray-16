# Android launcher APK

`build-apk-armv7.sh` packages the ARMv7 OpenXRay engine, SDL2 activity and
launcher into a debug APK. No proprietary S.T.A.L.K.E.R. resources are
included. Required packages, pinned toolchain versions and full build/install
commands are in [../README.md](../README.md).

Quick build and install:

```sh
export XRAY_ANDROID_KIT_ROOT=/absolute/path/to/openxray-android-build-kit-v0.8.0
./android/build-harness.sh --apk
adb install -r build/openxray-armv7-launcher-v0.9.24-debug.apk
```

The application ID is `org.openxray.stalker`; the main activity is
`org.openxray.app.LauncherActivity`.

## Launcher workflow

1. Grant storage access. Android 11+ opens the system **All files access**
   page; Android 8–10 uses runtime storage permissions.
2. Select the installation root containing `fsgame.ltx` and the original game
   resources. Call of Pripyat is the intended profile.
3. Configure renderer, graphics preset, internal 3D resolution, controls and
   FPS display under **Settings**.
4. Optionally run the GLES smoke test or Vulkan probe without game files.
5. Start the game. Before launch, the app verifies a real write to
   `<STALKER>/_appdata_` and refuses to continue if Android denies it.

Auto renderer and explicit OpenGL ES both use the GLES gameplay backend.
Vulkan currently runs the surface/device/swapchain probe and then uses GLES; it
does not select a native Vulkan gameplay renderer.

Graphics choices are applied in memory after loading
`<STALKER>/_appdata_/user.ltx`. Auto graphics maps to Low. Auto resolution
preserves the physical aspect ratio and caps internal width at 1280 unless the
display is smaller. The physical landscape surface remains native and receives
the scaled final frame.

The engine keeps the desktop filesystem layout:

```text
<STALKER>/_appdata_/user.ltx
<STALKER>/_appdata_/savedgames/
<STALKER>/_appdata_/screenshots/
<STALKER>/_appdata_/logs/
```

These files survive APK uninstall as long as the selected game directory is
not removed. The private application directory contains only launcher state
and the OpenXRay-owned renderer fallback data shipped in the APK.

The launcher is portrait and the engine activity is landscape. The FPS counter
is opaque red at the top center. Touch controls include Escape.

## Running-engine controls

While the `:engine` process exists, **Start game** changes to **Return to
running game**. The adjacent stop button force-terminates a stuck engine after
confirmation. These controls cannot restore a process that Android has already
killed or that has crashed natively.

## Logs

The diagnostics page polls bounded 32 KiB tails on a background executor, so
it does not load whole growing files on the UI thread. Primary Android paths:

```text
/storage/emulated/0/openxray/android.log
/storage/emulated/0/openxray/activity.log
```

Normal engine logs are also written under `<STALKER>/_appdata_/logs/`.
**Clear** only removes the Android diagnostic files; it does not delete saves,
screenshots, game files or `user.ltx`.

Useful collection commands:

```sh
adb logcat -c
adb shell am force-stop org.openxray.stalker
adb shell am start -n org.openxray.stalker/org.openxray.app.LauncherActivity
adb logcat -d -b all -v threadtime OpenXRay:I DEBUG:E '*:S' > openxray-logcat.txt
adb logcat -d -b crash -v threadtime > openxray-crash.txt
adb pull /sdcard/openxray/android.log openxray-engine.log
adb pull /sdcard/openxray/activity.log openxray-activity.log
adb pull /sdcard/STALKER/_appdata_/logs openxray-game-logs
```

Native crashes must be symbolicated against the unstripped `libmain.so` built
from the same commit as the APK.
