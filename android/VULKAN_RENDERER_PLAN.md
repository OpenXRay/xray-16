# Portable Vulkan renderer plan

The target is one `xrRenderVK` implementation shared by Android, Linux, and
Windows. It must consume original PC resources and mod overrides without
requiring Android-specific shader or asset copies.

## Upstream research

OpenXRay issue [#447](https://github.com/OpenXRay/xray-16/issues/447) defines
Vulkan as a separate renderer and lists hardware, backend, resource, state, and
shader work. Its shader requirement is to cross-compile the existing HLSL
sources. Issue [#258](https://github.com/OpenXRay/xray-16/issues/258) tracks the
type-aware compiler options.

The old [PR #558](https://github.com/OpenXRay/xray-16/pull/558) was reviewed and
must not be revived as the implementation: it registers an R5-shaped stub but
does not create a Vulkan instance, device, swapchain, resources, pipelines, or
synchronization. The unmerged DX12 [PR #1620](https://github.com/OpenXRay/xray-16/pull/1620)
is a large Windows/CryEngine-derived backend and is not a portable base.

NVRHI was also evaluated because it is mentioned by OpenXRay maintainers. Its
current supported targets are Windows and Linux on 64-bit CPUs; it does not
cover Android or ARMv7. It therefore cannot be the common foundation for this
port. LLGL supports Vulkan on all three requested platforms, but adopting it
does not remove the hardest compatibility task: compiling and reflecting the
existing X-Ray HLSL shader model. The initial design consequently keeps the
Vulkan backend inside OpenXRay and limits third-party components to focused,
replaceable libraries such as Vulkan headers/loader, memory allocation, and a
shader compiler.

## Non-negotiable architecture

1. `xrRenderVK` owns Vulkan objects only. Game and renderer-generation code
   must not contain Android/Win32/X11 Vulkan branches.
2. SDL supplies the native window and creates the `VkSurfaceKHR`; all later
   instance/device/swapchain/resource code is common.
3. Shaders have one source path. Existing HLSL plus engine defines is compiled
   to SPIR-V in memory, reflected for bindings, and cached by source, includes,
   defines, compiler version, and target capabilities. No `shaders/vk` asset
   fork is allowed.
4. GPU selection uses queried Vulkan features, formats, limits, and extensions.
   Unsupported devices receive a precise requirement report. No vendor or
   model allowlist is allowed.
5. Surface loss, resize, rotation, pause/resume, and `VK_ERROR_OUT_OF_DATE_KHR`
   recreate only swapchain-dependent objects. Game state and long-lived GPU
   resources survive when the device remains valid.
6. Packed PC resource formats and shader semantics are preserved. Any format
   expansion or transcoding happens in engine-owned memory.

## Delivery gates

| Gate | Required result |
|---|---|
| VK0: platform bootstrap | The same source creates instance, physical/logical device, queues, SDL surface, and swapchain on Android, Linux, and Windows; reports missing features clearly |
| VK1: shader pipeline | Existing HLSL includes/defines compile to SPIR-V with reflection and deterministic cache invalidation; a mod override follows the same path |
| VK2: backend primitives | Vertex/index/constant/storage buffers, textures, samplers, render targets, descriptors, graphics/compute pipelines, queries, and barriers pass validation |
| VK3: renderer integration | UI, static geometry, models, particles, deferred G-buffer, lighting, shadows, post-processing, and video paths render without resource edits |
| VK4: lifecycle | Repeated pause/resume, focus loss, resize, and swapchain recreation do not freeze, black-screen, leak, or restart the game |
| VK5: compatibility | Clean PC resources for CoP, CS, and SoC mode plus representative shader/texture mods run from read-only directories on all three platforms |
| VK6: release | Validation layers are clean in debug builds; release builds include capability diagnostics, reproducible shaders, and cross-platform CI artifacts |

## Minimum-capability policy

The minimum version and feature set must be derived from the first complete
render pass set, not guessed from a single test triangle. The probe records at
least API version, queue families, swapchain support, descriptor and attachment
limits, texture formats/compression, geometry/tessellation support, and shader
numeric features. Optional effects are enabled by capability; a feature used by
core resources either has an engine fallback or becomes an explicit launch
requirement.

## Migration order

- First isolate API-neutral render resource descriptions and shader reflection
  from the current GL/DX implementation.
- Add a validation-enabled Vulkan smoke executable that uses the future backend
  objects, not a parallel sample implementation.
- Port resource management and the UI/static passes before deferred lighting.
- Introduce render-pass/resource dependency tracking before the full deferred
  pipeline so barriers are generated centrally rather than patched per GPU.
- Keep GLES available during development and remove no working backend.

Version 0.9.0 adds an Android VK0 bring-up probe. The probe dynamically loads
`libvulkan.so`, creates the SDL-provided Android surface, selects a graphics
queue with swapchain support, creates a swapchain, acquires an image and
presents it through the selected queue. The launcher exposes this as “Vulkan + GLES
fallback”; gameplay remains on the proven GLES renderer when the device or
SDL build does not provide Vulkan. Gates VK1–VK6 are intentionally not claimed
complete until the probe is replaced by the common `xrRenderVK` backend.

Version 0.9.8 also runs that probe when Vulkan is selected for gameplay and
logs the physical device, Vulkan/driver versions, queue, device-local memory,
descriptor/attachment limits, BC/ETC2/ASTC support and the color/depth formats
needed by the deferred path. Gameplay still falls back explicitly to GLES;
these diagnostics narrow VK1/VK2 implementation decisions but do not satisfy
those gates.
