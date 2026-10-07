# Vulkan gameplay contract

This is an inventory of the engine-facing contracts, not a claim that they are
implemented. `renderer_vulkan` stays unavailable while any required game path
below has no Vulkan implementation. The independent smoke test does not cover
these contracts.

## Startup and ownership

1. `CEngineAPI::SelectRenderer` calls `CheckGameRequirements`, then `SetupEnv`.
   `SetupEnv` owns `GEnv.Render`, `GEnv.RenderFactory`, `GEnv.UIRender`,
   `GEnv.DU`, and, in debug builds, `GEnv.DRender`. `ClearEnv` removes only
   pointers installed by this module after all engine users have been torn
   down. A Vulkan request must never select the GLES implementation.
2. `CRenderDevice::Initialize` asks `ObtainRequiredWindowFlags` before creating
   the SDL window. `CRenderDevice::Create` calls `IRender::Create`, then
   `SetupStates` and `OnDeviceCreate`; it immediately creates an `IImGuiRender`
   through `IRenderFactory`. Fonts can be created before any level is loaded.
3. Each frame calls `GetDeviceState`, possibly `Reset`, then `Begin`, engine
   render callbacks and `End`. The level's render callback calls `Calculate`
   and `Render`. The UI and ImGui paths may draw around these callbacks.
4. `IGame_Level::Load` passes an engine-owned `IReader*` to `level_Load`.
   `GpuLevel` owns copied/decompressed geometry and Vulkan allocations, not
   this reader. `getVisual` pointers remain valid until `level_Unload`.
5. Dynamic `IRenderVisual` instances belong to the model pool. A duplicate
   shares immutable GPU geometry but has independent pose and callback state.
   The pool retires GPU resources only after the last submitted frame.
6. On reset and shutdown, submissions complete before releasing descriptors,
   pipelines, images and the device. Swapchain-dependent resources are
   recreated together. Paused/lost surfaces cannot submit new draws.

## `IRender` methods

| Area | Methods | Resource owner and invocation | Vulkan completion condition |
|---|---|---|---|
| Feature and options | `GetGeneration`, `GetBackendAPI`, `is_sun_static`, `get_dx_level`, `getShaderPath`, `HWSupportsShaderYUV2RGB` | Renderer; queried during configuration and gameplay | Return a consistent capability profile and shader root, including a distinct Vulkan API value where consumers require it |
| Device initialization | `create`, `destroy`, `Create`, `Destroy`, `OnDeviceCreate`, `OnDeviceDestroy`, `ObtainRequiredWindowFlags`, `SetupStates`, `Reset`, `reset_begin`, `reset_end` | Renderer/device, from engine startup, resize and teardown | SDL Vulkan window flags, exactly one live device, rebuild and release in dependency order |
| Frame and camera | `GetDeviceState`, `Begin`, `Clear`, `ClearTarget`, `End`, `SetCacheXform`, `OnCameraUpdated`, `GetCurrentContext`, `MakeContextCurrent` | Renderer and frame context, each game frame | Acquire, geometry, lighting, UI and present use one Vulkan frame and current camera; reset and surface loss are reported to engine |
| Level | `level_Load`, `level_Unload`, `getVisual` | Renderer owns `GpuLevel`, called from `IGame_Level` | Complete supported visual table and resources survive until unload; repeat loads work |
| Model pool | `model_Create`, `model_CreateChild`, `model_Duplicate`, `model_Delete`, `model_Logging`, `models_Prefetch`, `models_Clear`, `model_CreateParticles` | Model pool owns base models and per-object instances | Every engine visual type has a correct instance lifetime and cast interface; no GL allocation |
| Scene submission | `add_Visual`, `Calculate`, `Render`, `RenderMenu`, `BeforeWorldRender`, `AfterWorldRender` | Renderer queue per context/frame | Static and dynamic visuals, world transforms, HUD and menu draws reach the Vulkan command buffer once per frame |
| Visibility | `occ_visible(vis_data&)`, `occ_visible(Fbox&)`, `occ_visible(sPoly&)` | Scene visibility/HOM state, during traversal | Conservative, stable results with no missing visible object; culling can be refined after correct rendering |
| Lights and objects | `light_create`, `light_destroy`, `glow_create`, `glow_destroy`, `ros_create`, `ros_destroy` | Renderer resources referenced by gameplay objects | Valid reference-counted lights/glows and object lighting for lifetime of owners; updates affect lighting passes |
| Wallmarks | Both `add_StaticWallmark` overloads, `add_SkeletonWallmark`, `clear_static_wallmarks` | Renderer, material and model pool | Decals survive their intended level/object lifetime and draw with depth and alpha |
| Shaders and images | `shader_compile`, `GetImGuiTextureId`, `Screenshot` | Shader cache and texture/readback service | Required permutations compile or load with useful errors; ImGui IDs have valid descriptors; all screenshot modes work |
| Visual settings | `setGamma`, `setBrightness`, `setContrast`, `updateGamma`, `SetPostProcessParams` | Per-device/postprocess state | Gameplay settings affect the presented frame and survive reset |
| Resources | `DeferredLoad`, `ResourcesDeferredUpload`, `ResourcesDeferredUnload`, `ResourcesGetMemoryUsage`, `ResourcesDestroyNecessaryTextures`, `ResourcesStoreNecessaryTextures`, `ResourcesDumpMemoryUsage`, `OnAssetsChanged` | Texture/model/shader managers | Precache, archive/mod reload and memory reporting do not invalidate in-flight descriptors |
| Diagnostics | `DumpStatistics`, `GetForceGPU_REF`, `GetCacheStatCalls`, `GetCacheStatPolys`, `overdrawBegin`, `overdrawEnd` | Renderer counters/debug modes | Calls are safe in release and debug; counters reflect submitted work and debug modes are explicit |

`IRender::BackendAPI` currently lists D3D9/10/11 and OpenGL only. Add Vulkan
and audit every consumer before binding the backend. A method may be explicitly
unsupported only after confirming that no selected game or engine path calls
it; it must not silently report success or substitute an OpenGL object.

## Factory, UI and auxiliary interfaces

| Contract | First use / ownership | Required behavior |
|---|---|---|
| `IUIShader`, `IUIRender` | Factory creates shaders; `GEnv.UIRender` is set at `SetupEnv` | Atlas DDS, resolution, copy/destroy, TL and LIT vertices, world transform, scissor, culling, alpha reference, frame ownership |
| `IFontRender` | `CGameFont` may be constructed before level load | Existing `.ini` atlas metrics, multibyte and action-binding text, alignment, gradient, scaling and Vulkan UI draw |
| `IImGuiRender` | Created immediately after `OnDeviceCreate` | Context, fonts, texture descriptors, draw lists, reset and shutdown without OpenGL backend calls |
| `IUISequenceVideoItem` | Factory during scripted UI | Play/sync/stop, decoded frame upload and captured texture lifetime |
| `IStatGraphRender`, `IWallMarkArray` | Engine/debug UI and gameplay decals | Graphics and material instances with correct copy and cleanup |
| `IEnvironmentRender`, `IEnvDescriptorRender` | Persistent environment and level startup | Sky/clouds, descriptor interpolation and particle systems library |
| `IRainRender`, `IFlareRender`, `ILensFlareRender`, `IThunderboltRender`, `IThunderboltDescRender` | Weather and effect instances | Per-frame geometry, textures, transforms and lifetime |
| `IObjectSpaceRender` (debug), `IDebugRender`, `IDrawUtils` | Debug builds and engine utility rendering | Own Vulkan debug primitives and state; never bind GL globals |

All products declared by `IRenderFactory` must have matching create/destroy
implementations. `GEnv` objects must be usable both before and after a level
load, and freed after their last user.

## OGF model types

| Type | Name | Current Vulkan state | Required ownership/interface |
|---|---|---|---|
| 0 | normal | Level container and standalone common FVF decoded | Base mesh, material and per-object transform |
| 1 | hierarchy | Level children and standalone embedded children decoded | Stable linked/embedded children, duplication rules |
| 2 | progressive | First sliding window decoded | All LOD windows and screen-space selection |
| 3 | skeleton animated | Embedded child geometry decoded | `IKinematics` plus `IKinematicsAnimated`, motions/blends and independent poses |
| 4 | skeleton progressive mesh | 1–4 weight vertices and first window decoded | Parent bone table, LOD windows, skinning and collision/bone queries |
| 5 | skeleton static mesh | 1–4 weight vertices decoded | Parent bone table and skinning |
| 6 | LOD | Metadata only | Correct impostor/LOD draw and material |
| 7, 11 | static/progressive tree | Metadata only | Tree geometry, wind, lighting and LOD |
| 8, 9 | particle effect/group | Metadata only | `IParticleCustom`, update, child effects and render |
| 10 | skeleton rigid | Embedded child geometry decoded | Kinematics and rigid bone transforms |
| 12 | fluid volume | Metadata only | Explicit engine support decision based on actual game usage |

## Integration checks

- Boot SoC, CS and CoP to menu and level through `renderer_vulkan`, without a
  GLES context; inspect the actual selected renderer and Vulkan submissions.
- Create/destroy every factory type before and after a level load; render
  fonts, ImGui, textured UI and video through device reset.
- Load and delete representative standalone OGF records for each type, animate
  NPC and HUD weapon, duplicate them and change level without stale pointers.
- Exercise restart, resize, Android pause/resume, device/surface loss and
  repeated model/material reload with Vulkan validation enabled.
