# Android launcher

Build and installation commands are in [../README.md](../README.md). This file
describes the APK that those scripts produce.

| Property | Value |
|---|---|
| Application ID | `org.openxray.stalker` |
| Launcher activity | `org.openxray.app.LauncherActivity` |
| Engine activity | `org.openxray.app.XRayActivity` |
| Engine process | `org.openxray.stalker:engine` |
| ABI | `armeabi-v7a` |
| Minimum Android version | API 26 |
| Orientation | portrait launcher, landscape engine |

The APK contains the launcher, native engine, OpenAL, the C++ runtime and
OpenXRay-owned support files. It does not contain proprietary S.T.A.L.K.E.R.
archives or other game assets.

## Starting a game

1. Grant storage access.
2. Select the installation directory containing `fsgame.ltx` and the original
   game resources.
3. Select the game profile and renderer settings.
4. Run a smoke test if you are checking a new device.
5. Start the engine.

The selected path must be a filesystem directory in shared storage, for
example `/storage/emulated/0/STALKER`. Android document-provider and cloud URIs
are not supported. Before launching, the app creates the usual `_appdata_`
subdirectories and verifies them with an actual write/delete test.

The engine uses the desktop filesystem layout:

```text
<STALKER>/_appdata_/user.ltx
<STALKER>/_appdata_/savedgames/
<STALKER>/_appdata_/screenshots/
<STALKER>/_appdata_/logs/
```

These files are outside the APK's private data directory and normally survive
an APK uninstall. The launcher does not edit `fsgame.ltx`, game archives,
textures or shader sources.

## Renderer choices

- **Auto** probes Vulkan gameplay requirements and selects Vulkan when they pass;
  otherwise it selects OpenGL ES.
- **OpenGL ES** explicitly selects the GLES gameplay backend.
- **Vulkan** explicitly selects the separate Vulkan gameplay module. An
  unavailable Vulkan device or missing shader asset produces a launch error.
  Game compatibility and Android hardware behavior still need validation.
- **GLES smoke test** creates the real SDL/EGL context, compiles a minimal GLES
  shader and verifies pixel readback without loading game data.
- **Vulkan smoke test** draws a triangle through a Vulkan graphics pipeline,
  reads back its center pixel, and presents three frames without game files.
  The separate gameplay selection probe can also upload an engine DDS.

A smoke-test pass is limited to those operations. It does not validate level
loading, all shaders or sustained gameplay.

Debug Vulkan validation can be requested with engine argument `-vk_validation`
or environment `XRAY_VK_VALIDATION=1`. `android/build-apk-armv7.sh` can include
an ARMv7 `libVkLayer_khronos_validation.so` supplied by the Android Vulkan SDK
through `XRAY_ANDROID_VALIDATION_LAYER_ARMV7`. The layer is optional: when it is
unavailable the renderer logs that fact and continues without validation.
When available, synchronization validation is enabled if its instance extension
is supported. VUID messages go to SDL logging and the app private preference
directory `OpenXRay/validation/vulkan-vuid.log`.

Graphics presets and internal render resolution are applied after `user.ltx`
has been read. `Auto` graphics maps to `Low`. `Auto` resolution keeps the
display aspect ratio and defaults to half of the current display dimensions.

## Process and lifecycle controls

The launcher and engine use separate Android processes. While the engine
process is alive, the start button returns to its activity instead of creating
a second engine. The stop action sends the engine control broadcast and then
terminates a stuck process after confirmation. It cannot recover a process
that Android has already killed or a native process that has crashed.

## Logs

Each launch creates separate Android session logs under the selected game's
`_appdata_/logs` directory:

```text
android_<timestamp>_<pid>_<id>.log
activity_<timestamp>_<pid>_<id>.log
android_<timestamp>_<pid>_<id>.log.native-exit.txt
android_<timestamp>_<pid>_<id>.log.native-tombstone.pb
```

The last two files appear when available. **Save Android crash reports** is
enabled by default. On Android 11 and later, returning to the launcher
stores the engine's exit information in the same log directory. On Android 12
and later, a native crash also stores Android's protobuf tombstone if the
system provides it. Sharing diagnostics includes these saved files once.
The diagnostics tab reads a bounded tail of the latest session. **Clear**
removes Android diagnostic files only while the engine is stopped; it does not
remove saves, screenshots, `user.ltx` or game resources.

For native crashes, collect `logcat`, the Android crash buffer and the game log
directory as described in [../README.md](../README.md). Symbolication requires
the unstripped `libmain.so` from the same build.

## Update mechanism

The launcher checks published, non-prerelease GitHub releases. A release is an
Android update only when it contains both:

- `android-update.json`;
- the APK named by that manifest.

`android/build-apk-armv7.sh` writes the manifest to
`build/android-apk-armv7/android-update.json`. It records the application ID,
version, minimum SDK, APK size and SHA-256 digest. The launcher verifies those
fields before handing the APK to Android's installer.

The repository workflow currently publishes Windows nightly files, not Android
updates. To publish Android builds, attach the manifest and its APK to a normal
published release. Do not use a draft or prerelease. Every update must be
signed with the same certificate as the installed APK; a newly generated debug
key is not a release-signing strategy.
