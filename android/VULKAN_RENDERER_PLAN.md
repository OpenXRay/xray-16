# Vulkan renderer status

OpenXRay does not currently render gameplay through Vulkan. The Android
launcher option named Vulkan runs the implemented probe and then explicitly
selects the OpenGL ES backend for gameplay.

## Implemented

| Part | Source | Current behavior |
|---|---|---|
| Loader and device setup | `src/Layers/xrRenderVK/VulkanHardware.*` | Loads Vulkan procedures, selects a physical device and graphics/present queue, and creates the logical device |
| Frame context | `src/Layers/xrRenderVK/FrameContext.*` | Creates the swapchain and clear pass with an optional per-image depth attachment, tracks two frames in flight, detects lost surfaces, and provides the command buffer, render pass, framebuffer and extent to a frame recorder |
| SDL Vulkan device | `src/Layers/xrRenderVK/VulkanWindowDevice.*`, `VulkanProbe.*` | Uses SDL's platform Vulkan loader and surface API instead of a hard-coded Android/Linux shared-library name; owns the instance, selected device and frame context and can recreate the surface after Android replaces its native window |
| Android lifecycle | `src/xrEngine/x_ray.cpp`, `src/xrEngine/Render.h`, `src/Layers/xrRenderVK/VulkanLevelRender.*` | Forwards app pause/resume to the renderer, waits for submitted Vulkan work before suspension, and recreates the surface and swapchain after resume |
| Android Vulkan smoke | `src/xrEngine/android_vulkan_smoke.cpp`, `src/Layers/xrRenderVK/SmokeTrianglePass.*` | Creates an SDL Vulkan surface, draws the triangle plus lit indexed geometry and a colored UI overlay without game assets, checks pixels from geometry and UI and presents three frames |
| DDS decoding | `src/Layers/xrRenderVK/DdsTexture.*` | Reads 2D and cubemap DDS data, including mip chains; maps BC1/2/3 and RGBA/BGRA formats and can decode BC data to RGBA |
| Texture upload | `src/Layers/xrRenderVK/TextureUpload.*` | Stages decoded pixels into a device-local image and creates a sampled image view |
| Image state tracking | `src/Layers/xrRenderVK/ImageStateTracker.*` | Tracks layout/access state per aspect, mip and array layer on one externally synchronized queue |
| Buffer allocation | `src/Layers/xrRenderVK/BufferResource.*` | Owns buffer allocations, selects a compatible memory type, and supports bounded writes to host-visible coherent memory |
| Buffer upload | `src/Layers/xrRenderVK/BufferUpload.*` | Copies host data into a device-local buffer asynchronously, inserts a transfer-to-use memory barrier, and retires staging resources by fence |
| Shader module | `src/Layers/xrRenderVK/ShaderModule.*` | Creates and owns a Vulkan shader module from precompiled SPIR-V bytes |
| Screen-copy pass | `src/Layers/xrRenderVK/ScreenCopyPass.*` | Creates a sampled-image descriptor set and graphics pipeline, and records a fullscreen draw through the frame callback |
| Indexed scene and UI | `src/Layers/xrRenderVK/ScenePass.*`, `SceneShaders.h` | Records depth-tested indexed geometry with per-fragment directional light and an alpha-blended sampled-texture UI pass with alpha reference. The no-game smoke uploads a tiny UI texture and reads back pixels from both draws |
| Engine DDS bridge | `src/Layers/xrRenderVK/EngineTextureSource.*`, `src/xrEngine/android_vulkan_smoke.cpp` | Decodes bytes from the mounted engine VFS and uploads the standard fallback DDS into a sampled image when the probe has a mounted VFS |
| OGF catalogue and level bytes | `src/Layers/xrRenderVK/VisualCatalog.*`, `EngineLevelModels.*`, `LevelModels.*` | Reads all OGF visual type headers and retains their chunks, child links and standalone model files; decodes static and progressive container geometry and hierarchy children. Skeletal, particles, trees and other types have metadata only and cannot yet be drawn |
| Deferred primitives | `src/Layers/xrRenderVK/DeferredPass.*`, `DeferredShaderFactory.*` | Creates a two-color/depth G-buffer render pass and separate geometry and fullscreen directional-light pipelines |
| Deferred frame and targets | `src/Layers/xrRenderVK/GBufferTargets.*`, `DeferredFrame.*`, `FrameContext.*` | Allocates a G-buffer triplet per swapchain image and records an offscreen geometry pass before fullscreen lighting and UI in the present pass. The no-game Android smoke runs this path and reads back the lit pixel |
| GPU level visuals | `src/Layers/xrRenderVK/GpuLevel.*`, `VulkanVisual.*`, `VulkanLevelRender.*` | Uploads supported static and progressive OGF geometry, preserves level visual IDs and hierarchy, exposes bounds and child visuals through `IRenderVisual`. The abstract `VulkanLevelRender` implements `IRender::level_Load`, `level_Unload` and `getVisual`; no concrete gameplay renderer inherits it yet, and unsupported model types still fail loading |
| Game frame and UI | `src/Layers/xrRenderVK/VulkanGameDevice.*`, `VulkanUIRender.*`, `VulkanUIShader.*` | Creates Vulkan frame, G-buffer, deferred and UI resources; batches engine `IUIRender` vertices and sampled UI shaders. These are not yet bound into `GEnv` by a concrete `IRender` and `IRenderFactory` |
| Camera and render contexts | `src/Layers/xrRenderVK/VulkanCameraState.h`, `VulkanRenderContextState.h`, `VulkanLevelRender.*` | Caches `OnCameraUpdated` and `SetCacheXform` matrices for scene visibility, level visuals and queued draws. Engine context scopes resolve to Vulkan's single primary frame-recording context without creating or switching an OpenGL context |
| Device resources and callback order | `VulkanDeviceResourceState.h`, `VulkanFramePhaseState.h`, `VulkanLevelRender.*` | Enforces device → `SetupStates` → `OnDeviceCreate` before Vulkan font/UI shader products or level loading, and checks world/menu frame ordering. Binding them through the engine `IRenderFactory` remains pending |
| Portal visibility | `LevelVisibility.*`, `GpuLevel.*`, `VulkanLevelRender.*` | Traverses camera-visible sector portals and submits sector roots; invalid or missing visibility data falls back to every level root |
| Game texture descriptors | `src/Layers/xrRenderVK/GameTextureFactory.*` | Loads DDS assets through the mounted VFS, caches images and creates sampled descriptors for deferred materials and game UI, including texture dimensions |
| Renderer registration | `src/Layers/xrRenderVK/VulkanRendererModule.cpp` | Owns the `renderer_vulkan` mode independently of GLES and refuses game initialization until Vulkan implementations of the engine render interfaces exist |
| Offline HLSL compiler | `tools/compile_vulkan_shader.py` | Invokes a host DXC executable on an existing game/mod HLSL file, with entry point, include roots and defines, and atomically writes checked SPIR-V output |

The screen-copy pass takes compiled vertex and fragment modules, a sampled
image view, and a sampler. The caller must transition the image to shader-read
layout, keep resources alive through submitted frames, and rebuild the pipeline
when the render pass changes. With mounted game data, the probe can upload an
engine DDS and, if both compiled screen-copy shaders are present in the game VFS,
draw it as a fullscreen triangle. The no-game smoke test uses embedded SPIR-V
made from the GLSL sources in `src/Layers/xrRenderVK/smoke`, creates graphics
pipelines, verifies geometry and UI pixels by copying from the swapchain to a
mapped buffer, and presents three frames. To regenerate the original triangle
header, run `python3 tools/embed_vulkan_smoke_shaders.py --glslang glslangValidator`.

To compile an existing game or mod HLSL file on a host with DXC installed:

```sh
python3 tools/compile_vulkan_shader.py --dxc dxc \
  --source path/to/game/shaders/r3/editor.vs --stage vs --entry main \
  --include path/to/game/shaders/r3 --output build/shaders/r3/editor.vs.spv
```

Use `--define NAME=VALUE` for each shader variant. Compilation failures leave
the previous output intact. DXC is not shipped with the engine, and legacy
shader syntax or features may require porting. This tool does not yet compile
the complete game shader set or package SPIR-V into game resources.

For a group of named variants, the host can compile
`tools/vulkan_shader_variants.json` with:

```sh
python3 tools/compile_vulkan_shader.py --dxc dxc \
  --manifest tools/vulkan_shader_variants.json --output-dir build/vulkan-shaders
```

The paths in the manifest are relative to its own directory. Its entries cover
the editor vertex shader and the screen-copy pair, including an alpha-tested
pixel variant. The compiler keeps the previous outputs if any variant fails and
prints the failed source, stage, defines and DXC diagnostics. More gameplay
shader families and the engine `IRender::shader_compile` bridge remain to be
implemented before the shader-variant plan item can be closed.

For the diagnostic image draw, compile the supplied HLSL pair on a host with
DXC and install both outputs under the game's `$game_shaders$/r3/` directory:

```sh
python3 tools/compile_vulkan_shader.py --source res/gamedata/shaders/r3/screen_copy_vk.vs \
  --stage vs --entry main --output GAME_ROOT/gamedata/shaders/r3/screen_copy_vk.vs.spv
python3 tools/compile_vulkan_shader.py --source res/gamedata/shaders/r3/screen_copy_vk.ps \
  --stage ps --entry main --output GAME_ROOT/gamedata/shaders/r3/screen_copy_vk.ps.spv
```

The paths above are an example for a loose-file game installation. The engine
also resolves files from mounted archives and mod overrides. Both outputs must
be available for the image draw; if neither is present, the probe clears and
presents as before.

The scene diagnostic shaders can be regenerated with
`python3 tools/embed_vulkan_scene_shaders.py --glslc PATH/TO/glslc`.
The probe logs the selected device, queue, relevant limits, compression
features and attachment formats. These results are diagnostics, not a Vulkan
compatibility guarantee for gameplay.

## Not implemented

The Vulkan model geometry path now decodes standalone OGF static/progressive
meshes and skeleton child meshes with 1–4 bone weights. `GpuModel` owns
per-frame vertex buffers, can skin from an existing `IKinematics` pose and
can be queued into the deferred geometry pass. This does not yet make model
creation available through the game's `IRender::model_Create`: the concrete
`IRender` model pool, skeletal object/animation lifetime and scene submission
are still missing. Do not enable `renderer_vulkan` on this basis.

- runtime compilation, shader permutation coverage and reflection for the existing HLSL shaders;
- descriptor layouts and descriptor allocation for engine resources;
- graphics and compute pipelines for gameplay passes;
- engine integration for vertex, index, constant and storage buffers, with descriptors and pipelines;
- full engine scene geometry/material shader permutations, scene traversal,
  shadows and post-processing;
- skeletal/progressive/tree model, terrain, particle, game UI and video draw paths;
- recording engine draw commands and managing engine resource lifetimes;
- Win32 and Linux surface integration for `xrRenderVK`;
- Vulkan implementations of `IRender`, `IRenderFactory`, `IUIRender`,
  `IRenderDeviceRender` and the debug renderer; the separately registered
  Vulkan module rejects gameplay until these are present.

Pause/resume and surface-loss recovery still need validation on Android hardware.
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

1. expand the host HLSL-to-SPIR-V compiler to cover game shader permutations and add reflection;
2. connect buffer uploads to engine allocations, descriptor allocation and graphics pipelines;
3. validate the image draw on Android hardware, then render UI and static
   geometry through Vulkan;
4. port deferred targets, lighting, shadows and post-processing;
5. integrate swapchain recreation and Android lifecycle handling (implemented; device validation remains);
6. route Vulkan WSI through SDL's platform API and include `xrRenderVK` in desktop CMake builds (implemented; Linux/macOS matrix and a focused Windows MSVC build now cover compilation, CI results pending);
7. register desktop Vulkan modes and run Windows/Linux surface smoke tests;
8. enable Vulkan gameplay selection only after complete levels and representative
   mods run through Vulkan.

Useful host tests live in `tests/vulkan_probe.cpp`, `tests/vulkan_dds.cpp`,
`tests/vulkan_image_state.cpp`, `tests/vulkan_buffer_resource.cpp`,
`tests/vulkan_buffer_upload.cpp`, `tests/vulkan_shader_module.cpp`,
`tests/vulkan_screen_copy_pass.cpp`, `tests/vulkan_frame_lifecycle.cpp`, and
`tests/vulkan_smoke_triangle.cpp`. Device validation still requires the
launcher's Vulkan smoke test on real Android hardware. A successful triangle
smoke test does not close any of the gameplay items above.

The lifecycle, camera, callback-order, portal-visibility and deferred G-buffer
host tests can be enabled and run with:

```sh
cmake -S . -B build/vulkan-host -DXRAY_BUILD_VULKAN_TESTS=ON
cmake --build build/vulkan-host
ctest --test-dir build/vulkan-host --output-on-failure
```

These tests validate state transitions, conservative visibility fallback,
G-buffer attachments and geometry command setup. They do not replace a GPU
render or gameplay run on an Android device.
