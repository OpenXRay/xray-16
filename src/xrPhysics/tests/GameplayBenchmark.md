# Comparing migrations in the running game

Use a factorial comparison: change collision and dynamics independently. This
separates the effect of PR #2149's query tree from the dynamics migration.

| Runtime | Collision | Dynamics | Purpose |
| --- | --- | --- | --- |
| OPCODE-ODE | OPCODE | ODE | Baseline |
| Jolt-ODE | Jolt | ODE | Collision migration |
| OPCODE-Jolt | OPCODE | Jolt bridge | Dynamics migration |
| Jolt-Jolt | Jolt | Jolt bridge | Combined |

## Build and run

Use the same commit, compiler, Release configuration and native engine DLLs.
The existing Windows builder can produce all four instrumented runtimes:

```powershell
./misc/windows/test-collision.ps1 -Configuration Release -Dynamics ODE -GameplayBenchmark
./misc/windows/test-collision.ps1 -Configuration Release -Dynamics Jolt -GameplayBenchmark
```

Alternatively set `XRAY_GAMEPLAY_BENCHMARK=ON` in the CMake bridge. This option
defaults to `OFF`; ordinary builds have neither the capture code nor its Lua
entry points. The native MSVC solution continues to build the legacy backend.

Create a JSON manifest listing the four runtime directories (each contains
`xrEngine.exe`, `xrCDB.dll`, and `xrPhysics.dll`):

```json
[
  {"name":"OPCODE-ODE","collision":"OPCODE","dynamics":"ODE","directory":"C:/build/OPCODE/ODE/Release/bin/Release"},
  {"name":"Jolt-ODE","collision":"Jolt","dynamics":"ODE","directory":"C:/build/Jolt/ODE/Release/bin/Release"},
  {"name":"OPCODE-Jolt","collision":"OPCODE","dynamics":"Jolt","directory":"C:/build/OPCODE/Jolt/Release/bin/Release"},
  {"name":"Jolt-Jolt","collision":"Jolt","dynamics":"Jolt","directory":"C:/build/Jolt/Jolt/Release/bin/Release"}
]
```

Use an isolated template with `fsgame.ltx`, `gamedata` compatibility overrides
if needed, `userdata/user.ltx` and an engine-compatible `.scop` save. Its
`$app_data_root$` should be `userdata/` and archive aliases should refer to the
root's `levels`, `resources`, etc. The supplied old ALife version-6 save cannot
be used with this engine; the local template uses `collision-start.scop`.

```powershell
./misc/windows/benchmark-gameplay.ps1 `
  -RuntimeManifest C:/bench/runtimes.json `
  -TemplateRoot C:/bench/template `
  -GameFiles 'C:/Games/S.T.A.L.K.E.R. Call of Pripyat' `
  -OutputRoot C:/bench/results -Repetitions 3

./misc/windows/summarize-gameplay-benchmark.ps1 -OutputRoot C:/bench/results
```

The runner creates separate game roots, links archives, copies the same save
and configuration, starts each runtime, enters gameplay and runs the script.
It reverses runtime order on alternate passes. It records machine information,
configuration and DLL hashes, raw CSVs, engine logs and process working set.
The original game files and save are not modified. Close other game instances;
keep the benchmark window in the foreground and avoid other CPU/GPU workloads.
The benchmark exits the game after each capture.

## Workloads and measurements

The committed `gameplay_benchmark.script` does the following:

1. Warms the loaded scene for ten seconds.
2. Captures ten seconds of an idle scene.
3. Runs fixed-seed nearest-ray, any-hit ray and full-box batches directly on
   the **real loaded level's collision mesh**, with one warm-up and seven timed
   batches of 4,096 queries each. Input coordinates and hit counts/range sums
   are checked across runs. These batches exclude Lua call overhead and dynamic
   object queries; they measure level-tree traversal and result generation.
4. Spawns twelve NPCs, waits for them to come online, kills them with scripted
   impulses, and captures twenty seconds of their ragdolls. Forces are applied
   once per fixed physics step, so their rate does not depend on rendering FPS.
   All registered shells are kept alive until capture ends. This is a controlled
   physics stress scene rather than a combat AI benchmark.

For each physics step, capture records raw wall times for collision generation,
solver/bridge execution and the complete step, plus active island/body/joint
and contact counts. It buffers samples in memory and writes only after each
phase. Artificial workload-force application occurs before the step timer;
frame intervals include it. The solver interval includes engine commander work,
island enumeration, integration and (for Jolt) synchronization and custom
constraint construction. It is the cost of the actual migration path, not just
a bare Jolt solver call.

Frame intervals are measured between consecutive physics `OnFrame` calls.
They include game/render scheduling and waits; they are not GPU timestamps or
display-present measurements. Capture requires `mt_physics off` to serialize
Lua phase control with sampling. The runner uses 800×600, VSync off, the engine's
maximum `rs_fps_limit 501`, stats overlay off, physics frequency 100 Hz and 18
iterations. It does not prove a CPU bottleneck or represent all threading modes.

The engine's normal HUD physics timers are unsuitable for exported comparisons:
they are disabled when stats are off and their frame results are smoothed.
The benchmark uses `steady_clock` samples independently of that HUD state.

## Interpreting results

- Compare Jolt-ODE against OPCODE-ODE for the collision migration, and
  OPCODE-Jolt against OPCODE-ODE for dynamics. Check the combined build too.
- Use mean cost per physics step for CPU budget, and p95 step/solver cost for
  spikes. At 100 Hz, 0.5 ms/step consumes about 50 ms of wall time per simulated
  second. It does not necessarily add 0.5 ms to every rendered frame.
- Summarize each process run first, then take the median of independent runs.
  The exported summary includes the range between runs. Individual frames and
  repeated query batches are not independent process trials.
- Check active bodies, contacts and joints alongside timings. Simulated poses
  and contact topology can differ between solvers despite identical forces.
  A lower cost from fewer active objects is not evidence of a faster solver.
- Verify query inputs and hit results before accepting a speed comparison.
- Treat frame improvements separately from subsystem improvements. Rendering,
  AI, frame limits, shader compilation and scheduling can hide or dominate a
  physics/query difference. Use an external presentation/GPU capture if the
  question is specifically about displayed frame rate or GPU bottlenecks.
- `model_reported_bytes` is the collision model's own estimate. Observed process
  working set includes the entire engine and loading; neither measures solely
  Jolt's allocator or establishes memory parity.

Three repetitions give an initial comparison. Expand to five or more if run
ranges overlap the claimed improvement. Add a fixed camera route in a populated
area, props/doors/explosions, a normal resolution and normal threading before
making a default-backend or general gameplay performance claim. Measure cold
level/cache construction separately; these captures intentionally exclude load
time and shader warm-up. No network action or push is part of this workflow.
