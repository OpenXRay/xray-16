# AGENT_COMMANDS.md

## Run Ozz Tests
- Context: Keep parity checks green while iterating on the runtime façade/visual.
- Workflow:
  1. Build tests with the standard tools cache: `cmake --build ozz_utils --target ozz_kinematics_tests xrAnimation_converter_tests -j`.
  2. Run the full suite: `ctest --test-dir ozz_utils --output-on-failure`.
  3. Execute a single suite when iterating: `ozz_utils/bin/Debug/ozz_kinematics_tests --gtest_filter=OzzKinematicsParity.*` (adjust path/config as needed).

## Enable Ozz Visual Bundles
- Context: Smoke-test `.ozzx` bundles inside the engine without touching legacy assets.
- Workflow:
  1. Copy or convert bundles under `gamedata/meshes` (a sample `actors/dev_stalker.ozzx` is provided).
  2. Toggle the developer flag: `g_use_ozz_visuals 1`.
  3. Spawn or reload actors/HUD items; the loader will prefer `.ozzx` when present and fall back to `.ogf` otherwise.

## Legacy Bind Pose Toggle
- Context: Compare `.ogf` visuals against ozz output by forcing the legacy runtime to keep bind-pose matrices.
- Workflow:
  1. Enable the switch: `rs_skeleton_force_bind_pose 1`.
  2. Inspect visuals (legacy skeletons render at bind pose and foot IK stays disabled until you toggle it off).
  3. Restore normal animation: `rs_skeleton_force_bind_pose 0`.
  4. Use `debug_dump_ozz_palette` to log legacy and ozz skeleton palettes on demand.

## Ozz In-Game Smoke Test
- Context: Validate `.ozzx` bundles can hydrate animations on demand via legacy motion refs.
- Workflow:
  1. Generate fresh test assets (produces `.ozz/.ozzx` under `src/xrAnimation/tests/testdata`):
     `cmake --build ozz_utils --target xray_to_ozz_converter xrAnimation_converter_tests -j`
     `bin/x86_64/Mixed/xrAnimation_converter_tests --gtest_filter=ConverterIntegration.*`
  2. Mirror the converted bundle into the runtime search path. Example (Developer build, Linux):
     `install -D src/xrAnimation/tests/testdata/stalker_hero.ozzx bin/x86_64/Mixed/gamedata/meshes/actors/dev_stalker.ozzx`
     `install -D src/xrAnimation/tests/testdata/critical_hit_grup_1.ozz bin/x86_64/Mixed/gamedata/anims/critical_hit_grup_1.ozz`
  3. Launch the engine with developer flags (replace `-fsltx` with your configuration).
  4. In the in-game console:
     - Enable Ozz visuals: `g_use_ozz_visuals 1`
     - Swap player model: `g_dev_ozz_actor 1`
     - List discoverable legacy motions converted from OMF: `g_dev_ozz_animation_list`
     - Play a motion by name: `g_dev_ozz_animation actors\stalker_animation`
     - Stop playback if needed: `g_dev_ozz_animation_stop`
  5. Optional: capture palette output for debug parity via `debug_dump_ozz_palette` / `debug_dump_ozz_palette_toggle`.

## Bone Rest Pose Dump (Blender)
- Context: Needed the rest-pose transforms for the `stalker_hero_1.ogf` armature in Blender.
- Workflow:
  1. Retrieve the armature object by name and ensure it is an `ARMATURE`.
  2. Force rest pose by setting `armature.data.pose_position = 'REST'`.
  3. Iterate the `armature.data.bones`, decompose each `matrix_local`, convert rotations to XYZ Euler degrees, and print a table.
- Blender Python snippet:
```python
import bpy
from math import degrees

armature_name = "stalker_hero_1.ogf"
armature = bpy.data.objects.get(armature_name)
if armature is None or armature.type != 'ARMATURE':
    raise RuntimeError(f"Armature '{armature_name}' not found or not an armature object")

armature.data.pose_position = 'REST'

rows = []
for bone in armature.data.bones:
    matrix = bone.matrix_local
    loc, rot, _scale = matrix.decompose()
    euler = rot.to_euler('XYZ')
    rows.append({
        "name": bone.name,
        "pos": (loc.x, loc.y, loc.z),
        "rot": (degrees(euler.x), degrees(euler.y), degrees(euler.z))
    })

name_width = max(len("Bone"), *(len(row["name"]) for row in rows))
coord_headers = ["PosX", "PosY", "PosZ", "RotX", "RotY", "RotZ"]
col_widths = [max(len(header), 10) for header in coord_headers]

header = f"{ 'Bone'.ljust(name_width) }  " + "  ".join(h.ljust(w) for h, w in zip(coord_headers, col_widths))
separator = f"{'-' * name_width}  " + "  ".join('-' * w for w in col_widths)
print(header)
print(separator)
for row in rows:
    pos = row['pos']
    rot = row['rot']
    data = [f"{pos[0]: .6f}", f"{pos[1]: .6f}", f"{pos[2]: .6f}",
            f"{rot[0]: .3f}", f"{rot[1]: .3f}", f"{rot[2]: .3f}"]
    line = f"{row['name'].ljust(name_width)}  " + "  ".join(val.rjust(w) for val, w in zip(data, col_widths))
    print(line)
```

## Ozz Bind Pose Dump (ozz_animation_viewer)
- Context: Need bind-pose translations and Euler rotations directly from ozz runtime without legacy debug binaries.
- Workflow:
  1. Reconfigure debug build if needed: `cmake -S xray-16 -B ozz_utils -DCMAKE_BUILD_TYPE=Debug`.
  2. Rebuild the viewer: `cmake --build ozz_utils --target ozz_animation_viewer -j` (target skipped when using Visual Studio generators).
  3. Visual Studio 2022 workflow:
     - Initialise the GLFW submodule: `git submodule update --init Externals/glfw`.
     - Generate GLFW binaries once: `cmake -S Externals/glfw -B Externals/glfw/build -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_DOCS=OFF`.
     - Open either the standalone `src/xrAnimation/tools/ozz_animation_viewer.sln` or the top-level `src/engine.sln` (the viewer lives under **utils/ozz_animation_viewer**), ensure the `VULKAN_SDK` env var points at your LunarG install (or set the MSBuild property `GlslcExecutable` to the glslc path), and build the `ozz_animation_viewer` project (shaders are compiled automatically into `bin/<Cfg>/shaders/`).
  4. Run the viewer headless (ensure `LD_LIBRARY_PATH` points at the build bin dir):
     `LD_LIBRARY_PATH=ozz_utils/bin/Debug ozz_utils/bin/Debug/ozz_animation_viewer --bundle=asset_tests/stalker_hero.ozzx --render=false --max_idle_loops=1`.
  5. Capture the `=== OZZ BIND POSE TABLE ===` output for comparisons.

## Animation JSON Dump (ozz_animation_viewer)
- Context: Need frame-by-frame world transforms straight from ozz to diff against Blender exports.
- Workflow:
  1. Build the viewer (`cmake --build ozz_utils --target ozz_animation_viewer -j`, not available with Visual Studio generators).
  2. Run with skeleton/animation arguments plus `--dump-animation-json=<path>`; for example:
    `./ozz_utils/bin/Debug/ozz_animation_viewer --bundle=asset_tests/stalker_hero.ozzx --animation=asset_tests/critical_hit_grup_1.ozz --render=false --max_idle_loops=1 --dump-animation-json=xray-16/res/testdata/npc/critical_hit_grup_1_world.json`.
  3. The viewer samples every animation time point (including first/last frame) and writes a JSON blob with per-joint translation, rotation (quaternion), and scale, ready for diffing.

## Blender Bind Pose Dump (execute_blender_code)
- Context: Export Blender’s world-space bind pose transforms in the same format as the ozz viewer table.
- Workflow:
  1. Ensure `stalker_hero_1.ogf` is the active armature.
  2. Run `mcp__blender__execute_blender_code` with the Python snippet below, which sets rest pose, decomposes each bone’s world matrix, and prints a formatted table.
- Blender Python snippet:
```python
import bpy
from math import degrees

armature_name = "stalker_hero_1.ogf"
armature = bpy.data.objects.get(armature_name)
if armature is None or armature.type != 'ARMATURE':
    raise RuntimeError(f"Armature '{armature_name}' not found or not an armature object")

armature.data.pose_position = 'REST'

rows = []
for bone in armature.data.bones:
    matrix = armature.matrix_world @ bone.matrix_local
    loc, rot, _scale = matrix.decompose()
    euler = rot.to_euler('XYZ')
    rows.append({
        "name": bone.name,
        "pos": (loc.x, loc.y, loc.z),
        "rot": (degrees(euler.x), degrees(euler.y), degrees(euler.z))
    })

name_width = max(len("Bone"), *(len(row["name"]) for row in rows))
coord_headers = ["PosX", "PosY", "PosZ", "RotX", "RotY", "RotZ"]
col_widths = [max(len(header), 10) for header in coord_headers]

header = f"{ 'Bone'.ljust(name_width) }  " + "  ".join(h.ljust(w) for h, w in zip(coord_headers, col_widths))
separator = f"{'-' * name_width}  " + "  ".join('-' * w for w in col_widths)
print(header)
print(separator)
for row in rows:
    pos = row['pos']
    rot = row['rot']
    data = [f"{pos[0]: .6f}", f"{pos[1]: .6f}", f"{pos[2]: .6f}",
            f"{rot[0]: .3f}", f"{rot[1]: .3f}", f"{rot[2]: .3f}"]
    line = f"{row['name'].ljust(name_width)}  " + "  ".join(val.rjust(w) for val, w in zip(data, col_widths))
    print(line)
```
