# Vulkan renderer interface audit (PDF task 9)

`particles_systems_library()` exposes `CPGDef` definitions decoded from the
VFS `particles.xr`. The library owns both definitions and iterator pointers;
`begin` is inclusive, `end` is one past the last group, and `next` advances
only an element address in the current array. A foreign or end iterator is not
advanced; unrelated pointers are never compared with relational operators.
Names are stored in `shared_str` and remain valid until
reload or device destruction. The host renderer module test covers an empty
catalog, two arbitrary names, traversal, ID listing, replacement and clear.
Callers must not retain a definition across a reload.

| Interface | Game caller / contract | Vulkan handling |
| --- | --- | --- |
| `BeforeWorldRender`, `AfterWorldRender` | `xrGame/Level.cpp` brackets the game scene. | No state transition here; `VulkanLevelRender::End` submits the accumulated frame. The R2 implementation also has empty hooks. |
| `overdrawBegin`, `overdrawEnd` | `xrEngine/Device_overdraw.cpp` asserts before dispatch. | Unreachable in a normal game build; debug overdraw visualization is outside this game renderer. |
| `updateGamma` | `xrEngine/device.cpp`, console command. | Copies the clamped gamma/brightness/contrast into postprocess constants immediately; `End` transfers them for the next submitted frame. |
| `ImGui OnDeviceResetEnd` | UI device reset callback. | `OnDeviceResetBegin` invalidates atlas descriptors and marks textures for creation; End restores context and renderer backend flags, then `Render/update_textures` recreates the atlas when the new pass is used. The reset sequence is exercised by `vulkan_renderer_module`. |
| Lens flare `Copy` and device callbacks | `CLensFlare` owns each flare and gradient descriptor; reset recreates their textures. | `VulkanFlareRender::Copy` reacquires its own texture lease. The parent checks device identity and owns no texture or buffers. Its device callbacks leave child lifetime to `CreateShader`/`DestroyShader`. |
| Thunderbolt `Copy` | Weather effects may copy their render companion. | Checks device identity and discards transient CPU geometry. Every `Render` reads the current effect and descriptor; it never aliases another renderer's in-flight buffers. |
| Particle effect/group `OnDeviceCreate` | Effects are created by `model_CreateParticles`, which resolves the catalog and initializes textures. | Per-visual device-create hook owns no additional resource; visual destruction and renderer device teardown retire textures and buffers. |

Editor operations and device image acceptance are separate from this game-path
inventory. Particle effects still require visual acceptance after the new APK
is installed.
The exact synthetic format, diagnostic and fixture evidence for 8–11 is in
`VULKAN_102_8_11_ACCEPTANCE.md`.
