# Vulkan #64–65: OGF paths and level visibility

These are code and host checks. A real GPU frame and comparison against GLES remain in #113–114.

## #64: OGF 6–12

| Type | Source and rendering path |
| --- | --- |
| 6 | Level OGF impostor: decode eight facets, choose a camera-facing facet for distant LOD, otherwise submit its children. Standalone model uses the same GPU mesh path. |
| 7, 11 | Decode the tree transform into level vertices and submit to the level pass; 11 selects a validated sliding window. Standalone trees use `GpuModel`. |
| 8, 9 | Runtime particle effect/group created by `model_CreateParticles` from `particles.xr`; submitted by `add_Visual` to the particle draw queue. Neither is a `level.geom` container. |
| 10 | Skeletal rigid visual with the skeletal path already checked in #63. |
| 12 | `MT_3DFLUIDVOLUME` is not instantiated by the GLES `CModelPool` and has no level OGF loader. Only the DX11 loader's `Load3DFluid` uses a separate `level.fog_vol` file under `USE_DX11`. Thus the separate path required for a GLES-equivalent Vulkan renderer is the particle library for 8/9; a DX11 volumetric-fluid feature is outside the GLES feature set. Static OGF type 12 fails atomically instead of silently disappearing. |

`vulkan_visual_catalog` parses the 0–12 header table, while `vulkan_level_models` checks 6/7/11 decoding, fast and normal windows, and rejection of 8/9/12 in the static table. `vulkan_particle_catalog` and `vulkan_particle_visual` exercise the runtime particle path.

## #65: visibility and LOD

Sector traversal now commits its roots only after all traversed portal data is valid. A malformed later portal leaves no partial root set; the caller renders all roots. Valid traversal additionally includes roots that do not belong to any sector, so outdoors or independently attached hierarchies survive portal selection. Child visuals select their own distance LOD instead of inheriting the parent hierarchy's LOD. Window decoding checks index and active vertex ranges before GPU upload. `vulkan_level_visibility` checks a visible neighboring sector, an out-of-frustum portal, unsectored roots, and failure after a partial traversal. `vulkan_slide_windows_test` and `vulkan_level_models` check normal/fast ranges.

Host Debug and Release `xrRenderVK` and all 23 configured CTests passed. ARMv7 native and debug APK builds, shader assets, and Vulkan route/package checks passed. These tests do not establish actual visibility or image quality on a device; those scenarios are #113–114. #66–74 remain open because they require new detail, baked-light, shadow, fog, and water passes.
