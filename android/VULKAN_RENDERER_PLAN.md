# Vulkan renderer status

OpenXRay does not currently render gameplay through Vulkan. The Android
launcher option named Vulkan runs the implemented probe and then explicitly
selects the OpenGL ES backend for gameplay.

## Implemented

| Part | Source | Current behavior |
|---|---|---|
| Loader and device setup | `src/Layers/xrRenderVK/VulkanHardware.*` | Loads Vulkan procedures, selects a physical device and graphics/present queue, and creates the logical device |
| Android surface and swapchain probe | `src/xrEngine/android_vulkan_smoke.cpp` | Uses SDL to create the surface and swapchain, records a clear render pass, submits it and presents one image |
| DDS decoding | `src/Layers/xrRenderVK/DdsTexture.*` | Reads 2D and cubemap DDS data, including mip chains; maps BC1/2/3 and RGBA/BGRA formats and can decode BC data to RGBA |
| Texture upload | `src/Layers/xrRenderVK/TextureUpload.*` | Stages decoded pixels into a device-local image and creates a sampled image view |
| Image state tracking | `src/Layers/xrRenderVK/ImageStateTracker.*` | Tracks layout/access state per aspect, mip and array layer on one externally synchronized queue |

The probe logs the selected device, queue, relevant limits, compression
features and attachment formats. These results are diagnostics, not a Vulkan
compatibility guarantee for gameplay.

## Not implemented

- compilation of the existing HLSL shaders to SPIR-V;
- descriptor layouts and descriptor allocation for engine resources;
- graphics and compute pipeline creation for renderer passes;
- vertex, index, constant and storage-buffer integration;
- render targets for the deferred G-buffer, lighting, shadows and
  post-processing;
- model, terrain, particle, UI and video draw paths;
- frame scheduling, fences and lifetime management for sustained rendering;
- swapchain recreation integrated with pause, resume, resize and surface loss;
- Win32 and Linux surface integration for `xrRenderVK`;
- Vulkan selection as an engine gameplay renderer.

Until those items exist, documentation and launcher text must use the words
"probe" or "smoke test", not "Vulkan renderer" without qualification.

## Required architecture

The eventual backend should keep these constraints:

1. `xrRenderVK` owns Vulkan objects; platform code only supplies a window and
   surface.
2. Existing game and mod shader sources remain the source of truth. Shader
   conversion must not require a parallel `shaders/vk` resource tree.
3. Resource formats are preserved on disk. Unsupported formats may be expanded
   in memory.
4. Feature selection is based on queried limits, formats and extensions, not a
   GPU-name allowlist.
5. Image and buffer transitions are centralized instead of being added as
   one-off barriers in individual passes.

## Next implementation steps

Work should proceed in dependencies-first order:

1. move the Android-only probe onto an API-neutral `xrRenderVK` frame context;
2. add buffer allocation, descriptor management and frame synchronization;
3. add the HLSL-to-SPIR-V compiler and reflection cache;
4. render a normal engine UI/static-geometry pass through Vulkan;
5. port deferred targets, lighting, shadows and post-processing;
6. integrate swapchain recreation and Android lifecycle handling;
7. add Windows and Linux surfaces and CI coverage;
8. remove the GLES fallback only after complete levels and representative mods
   run through Vulkan.

Useful host tests live in `tests/vulkan_dds.cpp` and
`tests/vulkan_image_state.cpp`. Device validation still requires the launcher's
Vulkan smoke test on real Android hardware. A successful one-frame probe does
not close any of the gameplay items above.
