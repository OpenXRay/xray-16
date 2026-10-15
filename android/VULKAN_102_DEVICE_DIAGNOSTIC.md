# Device diagnostic for N1–N4 (APK 0.9.146)

APK 0.9.145 accidentally gated N1–N4 messages behind `-vk_export_shaders_xr`,
which the launcher does not enable by default. Absence of these messages in a
0.9.145 log proves nothing about the four paths. APK 0.9.146 emits the following
bounded messages without an extra launch flag. They report observations, not
a visual acceptance verdict:

| Finding | Log key | What the log can establish |
| --- | --- | --- |
| N1 named matrix | `N1.matrix first/second`, `N1.model-draw` | Two evaluated row-0 samples of the same nonstatic matrix at least three game seconds apart, with mode, slot, frame, time and delta. Priority 2/3 OGF matrix use is also reported every 300 frames, with shader, phase, LOD and active slots. A zero delta may be expected for some modes or camera positions; a missing second sample means the tracked matrix was not drawn twice sufficiently far apart. The screenshot remains necessary to judge UV mapping. |
| N2 priority | `N2.priority` | Every 300 frames: sorted transparent reference count by priority and level/model/particle source, plus a check that priority increases and distance decreases within a priority. This checks queue order, not correct pixel overlap. |
| N3 PN/HM | `N3.level-tess` and `OGF PN/HM tessellation` | First level mesh for each active PN/HM mode and every OGF conversion report input/output triangles. Missing HM means the scene did not exercise HM; a loaded conversion does not prove correct displacement or LOD appearance. |
| N4 world PARTICLE SET | `N4.particle-set` | First successful world SET draw in the G-buffer, particle count, serialized alpha threshold, first tint and priority. Missing message means no such draw was observed; it does not mean the path is broken. Visual alpha/depth still require a screenshot. |

On the device, use CoP Extreme at the same resolution as the previous log, load
the save near the ship and campfire, remain in the world for at least 30 seconds,
move the camera around the ship and fire, then exit through the menu. Preserve
the complete engine/activity logs and capture the ship and fire at two different
game times. No archive is needed. This single run samples the routes used by
that scene. It cannot cover unavailable material modes, HM if absent, all three
game profiles, or the project's 36-run device acceptance matrix.
