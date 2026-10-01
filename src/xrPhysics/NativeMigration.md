# Native Jolt physics migration

The results below describe the initial native port. See
[NativePerformance.md](NativePerformance.md) for the local performance followup
and its separate comparisons.

This branch, `followup/jolt-native-physics`, is stacked on
`followup/jolt-physics` at `888ab2d02`. It is a separate worktree at
`C:/code/xray-jolt-native`. Nothing from this branch has been pushed.

## Source attribution

The initial native bodies, shapes, characters, constraints, and gameplay physics
adaptation come from xxcactussell/unxray at
`c3e09c9f1c5c7b1f4674aa460181bafc9e755eb2`, licensed under the repository's
existing license. This work adapts that implementation to the PR #2149 branch,
uses the PR's pinned Jolt 5.6 revision, retains the game save/network encoding,
and does not enable the fork's added active-ragdoll gameplay feature.

## Completion requirements

- Native Jolt bodies, compound shapes, level mesh, collision generation, and
  persistent constraints; no ODE solver, collision implementation, or DLL link.
- Character movement, activation and camera checks, collision filtering,
  materials, contact callbacks, shell splitting, damage, joint feedback, motors,
  limits, save/load, and level teardown must work with the native backend.
- Keep the same scenario workloads available to the baseline and native path.
- Test invariants before timing; a faster incorrect simulation is not a win.
- Record complete-step CPU time, frame intervals, memory and workload counts;
  native Jolt collision and solving run inside one update and cannot be directly
  compared with the bridge's separately timed collision/solver columns.
- Compare single-thread Jolt and multiple worker counts with ODE, using repeated
  process runs with alternating order, fixed time steps, and identical assets.
- Include ordinary gameplay and heavier physics workloads. Report both wins and
  regressions, behavior differences, and limits of what the measurements show.

## Implementation

`xrPhysicsCore` owns the Jolt world, native bodies, compound shapes, persistent
constraints, virtual characters, worker pool, and 32 MiB temporary allocator.
`xrPhysics` adapts the existing gameplay operations and retains the Lua API and
save/network encoding. Native C++ contact callback signatures change, so the
engine/game and native plugins must be rebuilt together.
The ODE bridge, islands, primitive colliders, and triangle collider are removed
from the native physics build. Native game binaries do not link or load ODE or
OPCODE. ODE remains available only as an opt-in benchmark comparator.

Level CDB queries and physics use the same compressed Jolt `MeshShape` BVH.
Original triangle IDs remain 32 bits; material metadata retains its 14 bits.
The CDB API retains its original query results, cache behavior, degenerate
triangle handling, and coincident triangle IDs. The existing game spatial
registry remains the interface for locating gameplay objects.

Response-changing game contact callbacks execute on the simulation caller's
thread before Jolt's worker jobs. Ordinary impact effects can execute after
the workers with pre-solver snapshots, as described in the performance followup.
Collision rejection, friction, and static-environment
responses are then read by the native contact listener. The adapter preserves
material friction, restitution, passable/actor-obstacle policies, water and
slowdown effects, damage, sound/particle/wallmark effects, and missile callbacks.
Solved contact and constraint impulses provide fracture/capture force feedback.

The port restores behavior missing from the reference fork: weighted bone mass
centers and inertia, fracture mass/pose/velocity preservation, joint anchors
after a mass-center shift, actor/NPC restriction cylinders, forced character
steps, ladder gravity changes, ragdoll friction changes, normal impact energy,
wheel friction and suspension, animated targets, anchored doors, and capture
motor brakes. Prepared death shells stay outside simulation until activation.
Bodies, constraints, shared meshes, and temporary gameplay fixtures retire
without leaving native world entries behind.

## Validation

Verified on Windows x64 Release, 2026-10-01:

- CMake engine/game/native physics builds; Visual Studio project builds for
  `xrPhysicsCore`, `xrPhysics`, and `xrCDB`.
- CDB query, cache, construction, spatial, frustum, degenerate/coincident
  geometry, material, shared-mesh lifetime, and threaded query checks.
- Native physics checks with 0, 1, and 4 workers: motion, inertia, compound
  transforms, contacts/impulses, friction changes, static-environment responses,
  constraints, suspension compression/reactions, mass-center changes, motors,
  doors, animated/capture targets, placement, character movement/restrictions,
  collision ownership, passable materials, and world teardown.
- Checks inside the loaded CoP save for bone masses, fracture decisions and
  splitting, preserved geometry/velocities/inertia, NPC spacing, forced motion,
  ladder gravity, character save encoding, and impact damage calculations.
- Scripted creation and death of 12 NPCs, sustained ragdoll forces, actual game
  saving, process shutdown, and reloading the resulting save. Reload checks
  require all 12 dead NPCs, their original shell topology/mass, and physical root
  bone positions within 1 m of their pre-save snapshots. This permits
  simulation frames around asynchronous saving/loading; it is not an exact
  byte-for-byte state comparison. A pilot maximum was 0.087 m for native Jolt;
  ODE's third final capture moved one root bone 0.551 m during reconstruction,
  failing the initial 0.5 m bound. That failure and its timing capture are
  retained, and the fixture uses a 1 m placement bound with actual distances
  reported. The NPC proxy's approximately one metre
  bind-pose offset after reload occurs on both backends; the fixture compares
  the physical bone instead.

The game checks use an isolated copy of the supplied save/configuration and
junctions to the user's CoP archives. They do not modify the original game/save.
Compilation is stopped during timing. CDB test hooks are disabled in the final
timing build. Runtime hashes, machine/configuration metadata, CSVs, engine logs,
reload logs, and the generated validation saves are retained with the results.

## Benchmark method

Synthetic stacks and ball-jointed chains contain 128, 512, or 2,048 dynamic
2 kg unit boxes. Both backends use gravity 9.81, 10 ms steps, 18 velocity
iterations, matching friction/restitution and 0.05 linear/angular damping, and
the same forces. Each process warms up for 100 steps and records 500 steps.
The native floor retains the boxes' 0.7 friction and 0.1 restitution when made
static, so its combined contact properties match the ODE fixture. An earlier
capture used Jolt's default static-body friction of 0.2; it is retained separately
and excluded from the results below.
Finite positions, floor penetration, and scene bounds are checked on every
step. Three processes per configuration alternate backend order: 72 runs in
total, comparing ODE with Jolt at 0, 1, and 4 worker threads.

The game comparison uses the same engine executable, archives, save, 800x600
windowed R2 renderer, VSync off, 100 Hz physics, 18 iterations, and
`mt_physics off`. Native Jolt's own worker pool is measured at 0 and 4 workers.
The baseline is the parent PR's Jolt CDB with ODE dynamics. Each process captures
10 seconds idle and 20 seconds with 12 driven ragdolls, plus identical static
ray/box batches. Three runs per backend alternate order. Tables use medians of
the three process means, with per-process p95 values and workload counts
retained separately.

Synthetic `total_ms` includes native collision/solving or ODE collision,
island construction, solving, and cleanup. The game `total_ms` includes the
entire physics step. Native preparation, integration, and feedback are reported
separately. Solver columns from ODE and Jolt are not interchangeable. Worker CPU
time and synthetic validation/output costs are also reported; parallel wall-time
wins do not imply equivalent total CPU savings.
The synthetic fixture does not register gameplay callbacks, so it skips the
native adapter's contact preparation pass. The loaded-game measurements include
that pass and the gameplay effects it preserves.

### Synthetic results

Ryzen 9 9955HX, 16 cores / 32 logical processors. Milliseconds per step:

| Scene | Bodies | ODE | Jolt 0 workers | Jolt 1 worker | Jolt 4 workers | ODE / Jolt 4 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Stacks | 128 | 0.351 | 0.169 | 0.119 | 0.083 | 4.23x |
| Stacks | 512 | 1.392 | 0.677 | 0.433 | 0.235 | 5.94x |
| Stacks | 2,048 | 5.723 | 2.907 | 1.644 | 0.796 | 7.19x |
| Chains | 128 | 0.208 | 0.353 | 0.225 | 0.135 | 1.54x |
| Chains | 512 | 0.958 | 1.505 | 0.881 | 0.441 | 2.17x |
| Chains | 2,048 | 5.767 | 5.751 | 3.616 | 1.877 | 3.07x |

All 72 runs passed their invariants. The single-thread chain cases show that
native Jolt can still lose on small constrained workloads. Different contact
manifold/solver algorithms produce different contact counts and trajectories;
these are matched workloads, not identical simulations. For 2,048 stacks the
median peak working set is 61.2 MiB for Jolt 4 and 55.5 MiB for ODE; median
private bytes are 169.7 and 108.6 MiB respectively.

For 2,048 stacks, median process CPU per recorded step is 3.28 ms for Jolt 4
versus 5.75 ms for ODE. For 2,048 chains it is 7.88 versus 5.78 ms: the wall-time
gain costs more total CPU in that case. These process counters include
validation/output and worker CPU, and have coarser resolution than wall timings.

### Game results

All nine final processes completed both timed phases, NPC death checks, saving,
and reloading. The six native processes each passed all 35 loaded-game checks.
The original stricter ODE reload failure is retained separately, including its
timing CSVs; the resumed run and subsequent captures use the documented 1 m
placement bound. Maximum accepted reload root displacements were 0.394 m for
ODE, 0.062 m for Jolt 0, and 0.031 m for Jolt 4.

Complete physics step times in milliseconds; p95 is the median of the three
per-process p95 values:

| Phase | ODE mean | Jolt 0 mean | Jolt 4 mean | ODE p95 | Jolt 4 p95 | ODE body count | Jolt 4 active count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Idle | 0.060 | 1.046 | 0.860 | 0.068 | 1.064 | 27.0 | 126.0 |
| Driven ragdolls | 0.869 | 2.238 | 1.755 | 0.995 | 2.108 | 166.9 | 265.3 |

Jolt 4's three ragdoll means range from 1.744 to 1.814 ms; ODE's range from
0.747 to 0.915 ms. In this saved scene the native port takes 14.3 times the idle
physics time and 2.02 times the ragdoll physics time. The reported active workload
is larger, but these counters have different definitions: ODE counts bodies in
active islands; native Jolt counts active rigid bodies plus active virtual
characters. They do not establish an exact difference in awake rigid bodies.
A per-object activation/sleep audit is needed to attribute the extra work.
This measures the behavior and cost of the complete port rather than equal-body
solver throughput. Contact counts and the ODE/native constraint counters are
not directly equivalent either.

Jolt 4 spends about 0.523 ms idle / 1.112 ms ragdolls in contact preparation,
and 0.201 / 0.415 ms in native integration. Preparation accounts for roughly
61-63% of the entire physics step. Native worker scaling helps integration, but
the simulation-thread prepass remains. Its repeated narrow-phase queries and
game callbacks dominate this workload.

Frame p95 is 3.416 / 4.335 ms for ODE's idle/ragdoll phases, and 4.390 / 5.903 ms
for Jolt 4. These frame intervals include the renderer and game; they are
specific to this machine and configuration. Median sampled process working set
is 1,406 MiB for ODE and 1,410 MiB for Jolt 4.

Static query results, hit counts, ranges, and origins matched across all runs.
Median microseconds per query:

| Query | Parent Jolt CDB + ODE | Shared native mesh, Jolt 4 |
| --- | ---: | ---: |
| Nearest ray | 0.647 | 1.186 |
| Any ray | 0.433 | 0.786 |
| Full box | 1.560 | 1.943 |

The shared mesh's reported static-model memory falls from 111,372,664 to
96,087,846 bytes (13.7%), but these query batches regress. The memory accounting
is for the CDB model, not the whole game or a sum of independent physics/CDB
allocations.

### Assessment of issue #2139

The premise is supported for the measured synthetic workloads, particularly
large stacks with worker threads. It is not supported as an in-game performance
upgrade by this implementation. The native architecture and worker scaling are
useful results, but the port should remain a local experimental followup.

The next performance work is to eliminate duplicate contact discovery while
preserving callback rejection/response timing, reconcile sleeping/activation
behavior with the baseline, and optimize exact CDB queries on the shared mesh.
Re-run these comparisons after those changes. Reducing reported solver time
alone would omit the dominant adapter cost.

## Scope and remaining limitations

This replaces the physics backend; it does not establish complete gameplay
parity or better frame rate for every level, machine, renderer, or mod. The
loaded-save tests cover the encounters above. Vehicle, multiplayer, complete
campaign traversal, Linux, and Debug runtime behavior have not been exercised.
Vehicle suspension, doors, and capture are covered by native constraint
fixtures and compilation, rather than a CoP vehicle driving session.

ODE-specific per-contact CFM/ERP and motor fudge factors do not have exact Jolt
equivalents. Native material contacts use Jolt's rigid contact solver; hinge2
compliance maps to a native suspension spring, and translational full-control
compliance maps to SixDOF spring limits. Hinge/slider springs are native limits;
SixDOF angular limits use Jolt's angular constraint behavior. Tuning and motion
will differ from ODE even when gameplay interfaces and invariants are preserved.

The contact callback prepass repeats narrow-phase work so gameplay code can
change collision response before native worker jobs. Its cost, and differences
in activation and character representation in a loaded scene, must be included when judging
[issue #2139](https://github.com/OpenXRay/xray-16/issues/2139)'s performance
premise. A local native migration is not a recommendation to ship it solely
because the standalone solver benchmarks win.

## Reproduce

Build `misc/windows/collision` using the Windows imported-runtime prerequisites
from the parent followup, with `XRAY_NATIVE_BUILD_ROOT` pointing to that build.
Enable `XRAY_USE_JOLT_CDB`, `XRAY_BUILD_NATIVE_GAME`,
`XRAY_BUILD_PHYSICS_TESTS`, `XRAY_BUILD_PHYSICS_BENCHMARKS`, and
`XRAY_GAMEPLAY_BENCHMARK`. Run CDB checks with `XRAY_BUILD_CDB_TESTS=ON`, then
disable that option and rebuild before timing.

Use `misc/windows/benchmark-native-physics.ps1` and
`misc/windows/summarize-native-physics.ps1` for the synthetic comparison.
Use `misc/windows/benchmark-gameplay.ps1 -ValidateReload` with a runtime
manifest and isolated template, followed by
`misc/windows/summarize-gameplay-benchmark.ps1`, for the loaded-save comparison.
Pass `-ScratchRoot <isolated-game-directory>` to reuse one workspace and its
collision cache across runtimes and comparisons. Use a benchmark workspace,
since its configuration, scripts, logs and validation save are overwritten.
Captured logs, CSVs and saved states remain under each comparison's separate
`OutputRoot`. Keep runtime DLLs and executables without copying PDBs into each
candidate directory; reuse one candidate runtime while keeping the baseline
frozen. Build in the existing build directory rather than creating another
complete checkout/build for each candidate.

For Clear Sky, pass `-GameMode cs -Save clear-sky-benchmark-start`; the runner
uses `-cs` for both processes and `.sav` rather than CoP's `.scop`. Use a
separate template and scratch directory with junctions to the Clear Sky
assets. `tests/clear_sky_benchmark_prepare.script` creates the common save
from `server(all/single/alife/new)` on the ODE reference runtime, outside
timing. The starting-base fixture has 23 live stalkers and uses the same
12-NPC benchmark script as CoP.

The supplied Clear Sky 1.5.10 scripts need compatibility adjustments with the
current Lua runtime. In the isolated template only, extract `_g.script`,
`gulag_general.script`, `xr_logic.script` and `bind_smart_cover.script` from
`patches/xpatch_10.db`. Guard the initial `_g.script` profiler call with
`profiler.setup_hook ~= nil`; in `gulag_general.script`, replace the invalid
Lua string `"\scripts\\"` with `"scripts\\"`. Use the extracted Clear Sky
`xr_logic` and `bind_smart_cover` instead of the CoP versions in `res/gamedata`.
Keep the same template for every backend. These edits are benchmark setup,
not changes to the installed game or an assertion of full Clear Sky support.
Local final artifact directories are
`C:/code/migration-benchmark/native-scenes-matched-20261001` and
`C:/code/migration-benchmark/native-game-final-20261001`.
The earlier synthetic capture with mismatched floor friction remains at
`C:/code/migration-benchmark/native-scenes-final-20261001` for comparison.
