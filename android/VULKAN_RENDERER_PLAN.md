# Vulkan renderer status

OpenXRay does not currently render gameplay through Vulkan. The Android
launcher option named Vulkan runs the implemented probe and then explicitly
selects the OpenGL ES backend for gameplay.

## Implemented

| Part | Source | Current behavior |
|---|---|---|
| Loader and device setup | `src/Layers/xrRenderVK/VulkanHardware.*` | Loads Vulkan procedures, selects a physical device and graphics/present queue, and creates the logical device |
| Frame context | `src/Layers/xrRenderVK/FrameContext.*` | Creates the swapchain and clear pass, tracks two frames in flight, and provides the command buffer, render pass, framebuffer and extent to a frame recorder |
| Android Vulkan smoke | `src/xrEngine/android_vulkan_smoke.cpp`, `src/Layers/xrRenderVK/SmokeTrianglePass.*` | Creates an SDL Vulkan surface, draws a bundled SPIR-V triangle without game assets, copies its center pixel to coherent host memory, checks the pixel and presents three frames |
| DDS decoding | `src/Layers/xrRenderVK/DdsTexture.*` | Reads 2D and cubemap DDS data, including mip chains; maps BC1/2/3 and RGBA/BGRA formats and can decode BC data to RGBA |
| Texture upload | `src/Layers/xrRenderVK/TextureUpload.*` | Stages decoded pixels into a device-local image and creates a sampled image view |
| Image state tracking | `src/Layers/xrRenderVK/ImageStateTracker.*` | Tracks layout/access state per aspect, mip and array layer on one externally synchronized queue |
| Buffer allocation | `src/Layers/xrRenderVK/BufferResource.*` | Owns buffer allocations, selects a compatible memory type, and supports bounded writes to host-visible coherent memory |
| Buffer upload | `src/Layers/xrRenderVK/BufferUpload.*` | Copies host data into a device-local buffer asynchronously, inserts a transfer-to-use memory barrier, and retires staging resources by fence |
| Shader module | `src/Layers/xrRenderVK/ShaderModule.*` | Creates and owns a Vulkan shader module from precompiled SPIR-V bytes |
| Screen-copy pass | `src/Layers/xrRenderVK/ScreenCopyPass.*` | Creates a sampled-image descriptor set and graphics pipeline, and records a fullscreen draw through the frame callback |
| Engine DDS bridge | `src/Layers/xrRenderVK/EngineTextureSource.*`, `src/xrEngine/android_vulkan_smoke.cpp` | Decodes bytes from the mounted engine VFS and uploads the standard fallback DDS into a sampled image when the probe has a mounted VFS |
| Renderer registration | `src/Layers/xrRenderVK/VulkanRendererModule.cpp` | Owns the `renderer_vulkan` mode independently of GLES and refuses game initialization until Vulkan implementations of the engine render interfaces exist |
| Offline HLSL compiler | `tools/compile_vulkan_shader.py` | Invokes a host DXC executable on an existing game/mod HLSL file, with entry point, include roots and defines, and atomically writes checked SPIR-V output |

The screen-copy pass takes compiled vertex and fragment modules, a sampled
image view, and a sampler. The caller must transition the image to shader-read
layout, keep resources alive through submitted frames, and rebuild the pipeline
when the render pass changes. With mounted game data, the probe can upload an
engine DDS and, if both compiled screen-copy shaders are present in the game VFS,
draw it as a fullscreen triangle. The no-game smoke test uses embedded SPIR-V
made from `src/Layers/xrRenderVK/smoke/triangle.vert` and `.frag`, creates a
graphics pipeline, verifies a center pixel by copying it from the swapchain
to a mapped buffer, and presents three frames. To regenerate the embedded
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

The probe logs the selected device, queue, relevant limits, compression
features and attachment formats. These results are diagnostics, not a Vulkan
compatibility guarantee for gameplay.

## Not implemented

- runtime compilation, shader permutation coverage and reflection for the existing HLSL shaders;
- descriptor layouts and descriptor allocation for engine resources;
- graphics and compute pipelines for gameplay passes;
- engine integration for vertex, index, constant and storage buffers, with descriptors and pipelines;
- render targets for the deferred G-buffer, lighting, shadows and
  post-processing;
- model, terrain, particle, UI and video draw paths;
- recording engine draw commands and managing engine resource lifetimes;
- swapchain recreation integrated with pause, resume, resize and surface loss;
- Win32 and Linux surface integration for `xrRenderVK`;
- Vulkan implementations of `IRender`, `IRenderFactory`, `IUIRender`,
  `IRenderDeviceRender` and the debug renderer; the separately registered
  Vulkan module rejects gameplay until these are present.

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
5. integrate swapchain recreation and Android lifecycle handling;
6. add Windows and Linux surfaces and CI coverage;
7. enable Vulkan gameplay selection only after complete levels and representative
   mods run through Vulkan.

Useful host tests live in `tests/vulkan_dds.cpp`,
`tests/vulkan_image_state.cpp`, `tests/vulkan_buffer_resource.cpp`,
`tests/vulkan_buffer_upload.cpp`, `tests/vulkan_shader_module.cpp`, and
`tests/vulkan_screen_copy_pass.cpp`, and `tests/vulkan_smoke_triangle.cpp`. Device validation still requires the
launcher's Vulkan smoke test on real Android hardware. A successful triangle
smoke test does not close any of the gameplay items above.
