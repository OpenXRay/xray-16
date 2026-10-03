# Vulkan renderer: implementation through #69 and #72

This is a code and host verification checkpoint. It is not an Android device image-quality result; device scenarios remain at the end of the plan.

| Item | Implementation | Verification |
| --- | --- | --- |
| 66 | Decode `level.details` prototypes and slots from VFS, upload cutout meshes/materials, cache nearby placements and draw them within a camera radius. | Detail format fixtures, Debug/Release `xrRenderVK`. |
| 67 | Decode second UV and packed vertex color; retain paired diffuse/lightmap descriptors and compile lightmapped opaque/cutout variants. | Level format and descriptor fixtures, SPIR-V compilation and validation. |
| 68 | Own one sampled depth target and uniform per acquired swapchain image, draw visible static casters before G-buffer, recreate resources and descriptors on swapchain reset. | Debug/Release build, SPIR-V validation, source review of image/pass transitions. |
| 69 | Reconstruct world position from a separate sampled depth copy, project to the sun map and use 3×3 PCF plus bias in both weather and fallback deferred shaders. | Shader generation `--check`, `spirv-val`, descriptor fixture, full host CTest suite. |
| 72 | Pack interpolated weather fog color and range in spare push-constant components, reconstruct depth distance and blend fog before grading. | Fog parameter fixture, shader validation, full host CTest suite. |

The G-buffer depth image is copied after its pass into a separate per-image sampled depth image; the original remains a depth attachment for the final pass. This avoids sampling an image while it is bound as a writable depth attachment.

`cmake --build build-vulkan --target xrRenderVK` and `cmake --build build-vulkan-release --target xrRenderVK` succeeded. All 25 configured host CTests passed. `spirv-val --target-env vulkan1.0` passed for the new depth, weather and deferred fragments. No hardware frame or real game archive was used in this checkpoint. Local light shadow maps (#70–71) and water/reflection/refraction (#73–74) remain unimplemented.
