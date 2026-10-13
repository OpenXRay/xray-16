# Vulkan renderer interface audit (PDF task 9)

`particles_systems_library()` exposes `CPGDef` definitions decoded from the
VFS `particles.xr`. The library owns both definitions and iterator pointers;
`begin` is inclusive, `end` is one past the last group, and `next` advances
only a live iterator. Names are stored in `shared_str` and remain valid until
reload or device destruction. The host renderer module test covers an empty
catalog, two arbitrary names, traversal, ID listing, replacement and clear.
Callers must not retain a definition across a reload.

| Interface | Game caller / contract | Vulkan handling |
| --- | --- | --- |
| `BeforeWorldRender`, `AfterWorldRender` | `xrGame/Level.cpp` brackets the game scene. | No state transition here; `VulkanLevelRender::End` submits the accumulated frame. The R2 implementation also has empty hooks. |
| `overdrawBegin`, `overdrawEnd` | `xrEngine/Device_overdraw.cpp` asserts before dispatch. | Unreachable in a normal game build; debug overdraw visualization is outside this game renderer. |
| `updateGamma` | `xrEngine/device.cpp`, console command. | Setters retain gamma/brightness/contrast; `End` transfers all three to the postprocess constants for the next submitted frame. |
| `ImGui OnDeviceResetEnd` | UI device reset callback. | `OnDeviceResetBegin` invalidates atlas descriptors and marks textures for creation; `Frame/update_textures` recreates them when the new pass is used. |

This audit does not cover editor operations or visual acceptance of the
particle effects. It records the call boundary for the empty hooks above.
