# Vulkan implementation review, steps 86–96

All eleven code and host gates are complete. The Debug and Release `xrRenderVK`
targets, the ARMv7 native engine and a debug-signed ARMv7 APK with the official
validation layer build. The 31 registered CTest cases and four shader Python
tests pass. An Xvfb/host Vulkan run created a fresh VUID log through the
renderer callback. Android gameplay frames remain the separate #104–145 gates.

| Step | Completed code/host evidence |
| --- | --- |
| 86 | `GpuModel::record_animated` uploads per-instance poses for 1–4 weights, checks bone bounds and progressive windows. The OGF asset fixture records both world and HUD variants for all four layouts; packaged SPIR-V passes manifest validation. |
| 87 | HUD is recorded after the world with independent cleared depth. The asset fixture switches model instances, records two HUD draws and checks buffer lifecycle. |
| 88 | Rigid bone queries, visibility and callbacks are exercised after a live pose change; invisible bone picks fail. |
| 89 | Instance reset clears bone, track and blend callbacks. Three respawn cycles release pose buffers and shared GPU resources, including after menu/level teardown. |
| 90 | Explicit `-vk_validation` works in ReleaseMasterGold native code used by the debug APK. ARMv7 official Khronos layer is packaged and ABI-checked. Xvfb/host Vulkan emitted a deliberate VUID and persisted it through `VulkanWindowDevice`'s callback. |
| 91 | Capture tool requires recorded `.xrdemo`, level save, effects file, frame, APK, log and matching build manifest/commit. Positive and mismatched-commit fixtures pass. |
| 92 | Upload fences retain staging buffers until completion; delayed `VK_NOT_READY` fixture checks retirement. Texture reload waits for idle frames and uploads before descriptor rebind and old-view disposal; failed and successful reload fixtures check atomicity and resource counts. |
| 93 | A mock `FrameContext` executes acquire, submit, present, out-of-date, suboptimal, surface-lost, recreate and destroy. It checks per-frame and per-image sync creation/destruction and idle retirement. |
| 94 | Probe and device selection check required limits, sampled/attachment color and depth formats. BC data is decoded when unsupported. Positive/negative capability tests and Debug/Release builds pass. |
| 95 | SDL app lifecycle pauses submissions, resume requests reset and five repeated host cycles restore the state. Device observation is #137. |
| 96 | Zero extent postpones swapchain creation, nonzero resize/rotation recreates it; repeated host sequences pass. Device observation is #137. |

Build inputs: Android SDK platform/build-tools 36, NDK r30, SDL2 2.30.12,
OpenAL Soft 1.24.2, libjpeg-turbo 3.1.0, Ogg 1.3.5, Vorbis 1.3.7,
Theora 1.1.1, LZO 2.10, and Khronos Android validation 1.4.363.0.
The 16 KiB aligned, signed APK has SHA-256
`ab9644e06c4145ff6be9223eb43140cb61d19b927f948056d9e6a35cb6a15037`.
These are local build inputs and outputs, not committed third-party binaries.

For a reproducible level capture, load a named game save, use the existing
`demo_record route_name` console command to record a camera route (the console
also supports `demo_set_cam_position x,y,z`), and save the weather/effects
commands in a text file. Preserve the resulting `.xrdemo`, the game save,
the effects file, the APK, the log and frame images. Run:

```sh
python3 tools/vulkan_capture_session.py --scenario soc_day --apk game.apk \
  --log game.log --frame frame.png --camera-demo route_name.xrdemo \
  --level-save level.sav --effects-config effects.txt \
  --build-commit <full-native-build-git-sha> \
  --build-manifest build/android-apk-armv7/build-manifest.txt \
  --device 'model; Android; GPU; driver' --output capture-soc-day
```

The new output directory contains SHA-256 hashes for every input and records
the build commit separately from the capture checkout. The script preserves
test inputs; it does not control the in-game camera itself.
