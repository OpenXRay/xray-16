# Vulkan host verification, 2026-10-08

Build source: local commit `89519931` (tree `7ca14b9b5aa94a8e11711fa92aea8e4c46ecb629`), identical to PR commit `b35cdec4b60bb8112e94e05b852b385aefb5a79d`. Android NDK r30, build kit 0.9.104, Build Tools 36.0.0. Native builds used LTO=OFF. Debug used `-O0 -g1` for both C and C++ to fit the temporary build volume. ARMv7 LuaJIT host tools used a static glibc i386 compiler and QEMU because Android's 32-bit libc aborts when the host PID exceeds 65535.

| ABI | Configuration | ELF | Unstripped `libmain.so` SHA-256 |
| --- | --- | --- | --- |
| arm64-v8a | Debug | ELF64/AArch64 | `d016530f1d4da6acea2fa29b5445bce4471bb9fefe38f1c9ef1ce0933581e90a` |
| armeabi-v7a | Debug | ELF32/ARM | `1ca5756edd7fe5213e182c4e91dcd03bcfa956a40322be5291ed62cb72084b88` |
| armeabi-v7a | ReleaseMasterGold | ELF32/ARM | `e391a171196b74b8539056a5b7de99750d008bd68489aeb0b7675e9f5b1db5fa` |
| arm64-v8a | ReleaseMasterGold | ELF64/AArch64 | `d2567b1fc500aa6f070b976be44e1491e5e10da74f0a83722e4aef6d3a412eed` |

The stripped ReleaseMasterGold libraries used for the package have SHA-256 `71456358e282eb9845db08d772adcd38d706424c7cc08589a4a4d801288cc146` (ARMv7) and `1a01d9a5aecc51bebba55d6d9ffa47f4ec5d99b33df1e9b7609445ad37e6c86e` (ARM64). The signed dual-ABI debug launcher `openxray-universal-launcher-v0.9.128-debug.apk` is 106462080 bytes and has SHA-256 `dbd8a23f17e34867e5cca1ddbf1d08a2e3a9a0bd563bc432a525984e88555b19`. The APK build script passed both ABI Vulkan route checks, packaged-library byte comparisons, shader asset validation, 16 KiB native ZIP alignment and `apksigner verify`.

Host checks: 33/33 CTest Vulkan tests passed, 9/9 Python shader tests passed, 38/38 bundled SPIR-V files passed `spirv-val`, and the Android version test passed. The test-only CMake link dependencies for `xrAPI` and `xrMaterialSystem` were corrected after the native builds; this change does not alter engine sources or the packaged native libraries.

## Remaining before closing plan items

- **102:** The renderer still rejects standard `S_SET` ADD and ALPHA-ADD (and other MUL/scale variants), and several legacy blenders with additional texture/pass semantics are reduced to one surface mode. These need matching Vulkan pipeline/pass behavior. Real SoC, CS, CoP and mod `shaders.xr`, geometry and shader assets are absent from this repository; synthetic format fixtures alone cannot validate their combinations. Unsupported geometry/material paths still need a complete audit of file, class/version, material/visual ID and shader diagnostics.
- **103:** The host build and package matrix above passed for the recorded source tree. Its explicit prerequisite 102 is still open. Re-run the matrix on the final 102 commit before checking 103, and retain the corresponding artifact hashes.

No device correctness or performance claim follows from these host checks.
