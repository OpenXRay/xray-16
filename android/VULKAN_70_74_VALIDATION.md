# Vulkan renderer: code checkpoint #70–71 and #73–74

These items are closed against the code and host criteria in `VULKAN_RENDERER_PLAN.md`. The Android image-quality and validation scenarios are #118–119, #124, #131–136.

| Item | Implementation | Host verification |
| --- | --- | --- |
| 70 | Per-swapchain-image 24-layer depth array (four shadow slots, six faces each), spot and point projection matrices, visible static caster depth pass, aligned per-light uniforms. Image fences guard slot reuse; swapchain reset recreates the arrays. | Resource ownership and layer/pass fixtures, Debug/Release renderer builds. |
| 71 | Fullscreen additive local lighting reconstructs world position from G-buffer depth, applies spot/point attenuation and samples the appropriate depth layer with 3×3 PCF. Per-image descriptors and uniforms are released/rebound on reset; light removal drops the next frame's draw. | Descriptor/pipeline fixtures, SPIR-V validation, host CTest suite. |
| 73 | Transparent static level water and glass materials use animated normals, shore depth, alpha and a dedicated water pipeline after opaque lighting. | Shader validation and pipeline fixture. |
| 74 | The opaque lit color is captured between the lighting and transparent passes into per-image sampled targets. Water samples refraction and uses a bounded screen-space reflection ray march with mirrored-screen fallback. Swapchain recreation rebuilds images, descriptors and the overlay pass. | Image transition/reset fixtures, shader validation and builds. |

The local shadow budget is four casting lights and 64 additive lights per frame, ordered by camera distance. The depth pass currently records static level casters; the dynamic model and transparent lighting audit remains in #79/#102. Screen-space reflection only sees opaque content present in the captured scene; its appearance and the light budget must be checked on actual devices in #118–119 and #136. No device frame or new Android APK was generated for this checkpoint.

Verification: Debug and Release `xrRenderVK` builds succeeded, all 28 host CTests passed, both shader embedding generators passed `--check`, and generated local light/water SPIR-V passed `spirv-val --target-env vulkan1.0`.
