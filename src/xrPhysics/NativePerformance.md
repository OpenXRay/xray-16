# Native Jolt performance followup

The first optimization is published in draft PR #2167 at `25ad9bb2e`.
Further optimization on `followup/jolt-native-physics` remains local.
The initial migration's measurements remain in
[NativeMigration.md](NativeMigration.md); this report describes the optimized
implementation and its separate captures.

## Changes

- Cache joint motor and limit settings. Repeating the same settings preserves
  warm starts and sleep timers; changing a setting still wakes the bodies. A
  powered motor with zero target velocity remains a brake. Unchanged animated
  transforms also avoid waking bodies. The previous hinge path reset warm
  starts and activated bodies on every resistance update.
- Classify contact response requirements on the simulation thread before
  worker jobs. Collision-changing callbacks and fluid forces keep their
  pre-integration timing. Ordinary impact sound, particle and wallmark effects
  execute after worker jobs, using captured pre-solver velocities and masses.
  Uniform ragdoll skin friction is read directly by native contact listeners;
  mixed geometry friction retains the preparatory callback path.
- Skip contact queries for bodies with no nearby response-changing contact.
  Query ordinary active body pairs once, retaining directional CCD sweeps.
  Reuse collectors, body lists and contact buffers; resolve material/triangle
  metadata together. Buffers retain capacity, but this is not a claim of zero
  allocation in every step.
- Restrict fluid preparation using per-material bounds and a conservative
  nearby-leaf check on the existing shared mesh BVH. Disconnected water regions
  otherwise made aggregate bounds cover most of Zaton. This adds material
  bounds, not a duplicate level BVH.
- Test four shared mesh child bounds together for ray/box traversal and
  vectorize the exact triangle/box edge-axis tests. Preserve triangle IDs,
  result order, material/cache behavior and degenerate triangle fallbacks.

The 32 MiB temporary allocator, timestep, solver iterations, Lua interfaces and
save/network encoding are unchanged. The native C++ interface changes require
rebuilding the game and physics DLLs together.

## Validation and measurement

Windows x64 Release: CDB queries and threaded queries, native physics tests
with 0/1/4 workers, CMake game/physics builds, and Visual Studio project builds
for `xrPhysicsCore`, `xrCDB` and `xrPhysics` passed before timing. New fixtures
check deferred callbacks on the caller thread with pre-solver snapshots,
uniform and persistent friction changes, immediate rejection and fluid
callback timing, sleeping under repeated motor/limit/transform updates, and
waking/driving when the motor changes.

Game captures use the same engine executable, original isolated CoP save,
archive junctions, 800x600 R2 renderer, 100 Hz physics, 18 iterations, VSync off,
and `mt_physics off`. Compare the parent PR's Jolt CDB + ODE, the original
native port with 4 workers, and the optimized port with 0/1/4 workers. Each
configuration runs three times, reversing process order on alternate passes. Each capture
includes idle, 12 driven ragdolls, static queries, saving and process reload.
Native captures must also pass all 35 loaded-game checks. Reload placement uses
the same documented 1 m physical-root bound as the initial migration.

Synthetic stacks/chains retain the matched 0.7 friction/0.1 restitution floor,
2 kg boxes, 0.05 damping, 100 warmup + 500 measured steps, 10 ms timestep and
18 iterations. Run ODE and native 0/1/4 workers at 128/512/2048 bodies, three
times each (72 processes). This fixture has no gameplay callbacks.

No builds run during timing; contact profiling and CDB test hooks are disabled.
Total game physics time includes deferred effect dispatch in the feedback
phase. ODE island-body counters and native active-body/character counters have
different definitions and do not prove a particular awake-body discrepancy.
The two solvers produce different contacts and trajectories.

## Artifacts

Published CSV summaries and their provenance are listed in
[the benchmark index](tests/benchmark-results/2026-10-01-native.md).

- Game: `C:/code/migration-benchmark/native-game-performance-final-20261001`
- Synthetic: `C:/code/migration-benchmark/native-scenes-performance-final-20261001`
- Frozen runtimes: `native-game-bin-performance-final` and
  `native-scenes-bin-performance-final` under the same benchmark directory.
- Runtime manifest: `native-performance-final-game-runtimes.json`.
- Configure/build/test logs: `native-performance-gate-*`,
  `native-performance-timing-*`, and `native-performance-msbuild-*`.

### Excluded captures and shutdown limitation

The first optimized zero-worker game process completed its timed phases,
all 35 loaded-game checks, saving and its quit log, but returned `0xc0000005`
(access violation). Its separate reload process passed. The failure is
retained under the game results' `excluded-captures` directory. A subsequent
zero-worker capture was attached to ProcDump and exited normally; it is also
excluded from timing because a debugger was attached. Both timing slots are
rerun without a debugger. Ten additional zero-worker reload/shutdown processes
under exception monitoring exited normally and produced no crash dump.

The cause of that original exit failure has not been identified. These passing
reruns do not establish that shutdown is fixed; it remains a reliability risk.
The benchmark runner now rejects a nonzero main-process exit after retaining
the logs, CSVs and exit status. Previously, only the summarizer rejected it.
Diagnostic logs are `native-performance-procdump.log` and
`native-performance-shutdown-check-1.log` through `-10.log`.

## Game results

Ryzen 9 9955HX, 16 cores / 32 logical processors. Medians of three process
means, milliseconds for the complete physics step:

| Phase | Parent ODE | Original native, 4 workers | Optimized, 0 workers | Optimized, 1 worker | Optimized, 4 workers |
| --- | ---: | ---: | ---: | ---: | ---: |
| Idle | 0.063 | 0.853 | 0.495 | 0.391 | 0.367 |
| Driven ragdolls | 0.611 | 1.906 | 0.974 | 0.764 | 0.777 |

Compared with the original native port at four workers, idle physics is 57.0%
lower and ragdoll physics is 59.2% lower (2.45x faster). The three original
ragdoll means span 1.727-1.975 ms; optimized four-worker means span
0.745-0.836 ms. The optimized port still takes 5.83x ODE's idle physics time
and 1.27x its ragdoll physics time in this saved scene. ODE's ragdoll means
span 0.548-0.717 ms. One worker slightly beats four for this workload; more
workers are not automatically faster.

At four workers, preparation falls from 0.522 to 0.130 ms idle and from
1.179 to 0.158 ms ragdolls. Integration falls from 0.199 to 0.108 ms idle and
from 0.501 to 0.373 ms ragdolls. Deferred effects increase the separately
reported feedback phase from about 0.003 to 0.049/0.120 ms; their cost is
included in total time. Within the native backend's same counter definition,
active rigid bodies plus virtual characters fall from 126.8 to 63.6 idle and
266.3 to 141.6 ragdolls. This supports the sleeping fix; comparing these counts
directly with ODE island counts would not.

Frame p95 falls from 4.259/6.047 ms idle/ragdolls in the original port to
3.843/4.563 ms optimized. ODE measures 3.406/4.118 ms. Median sampled peak
working sets are 1,425 MiB original, 1,421 MiB optimized, and 1,409 MiB ODE;
these are whole-process samples rather than physics allocator totals.

All 15 accepted captures have zero exit status and validated saving/reloading.
The 12 native captures each pass all 35 loaded-game checks. The excluded
shutdown failure above remains part of the retained evidence.

### Shared static mesh queries

Identical query origins, hit counts and ranges match across all accepted runs.
Median microseconds per query, using the four-worker native configurations
(these query batches themselves run on the caller thread):

| Query | Parent Jolt CDB + ODE | Original shared mesh | Optimized shared mesh | Reduction from original |
| --- | ---: | ---: | ---: | ---: |
| Nearest ray | 0.654 | 1.181 | 0.959 | 18.8% |
| Any ray | 0.443 | 0.781 | 0.630 | 19.3% |
| Full box | 1.575 | 1.920 | 0.892 | 53.6% |

Full box queries now beat the parent CDB path by 43.4%. Ray queries still cost
about 42-47% more than that path. Reported static model bytes are 96,089,734,
compared with 96,087,846 before optimization and 111,372,664 for the parent
(13.7% lower). The material bounds add 1,888 reported bytes. This accounting
covers the CDB model and does not sum independent whole-game allocations.

## Synthetic results

All 72 processes pass their invariants. Medians of three process means,
milliseconds per complete physics step:

| Scene | Bodies | ODE | Native 0 workers | Native 1 worker | Native 4 workers | ODE / Native 4 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Stacks | 128 | 0.353 | 0.172 | 0.123 | 0.082 | 4.29x |
| Stacks | 512 | 1.406 | 0.682 | 0.436 | 0.232 | 6.06x |
| Stacks | 2,048 | 5.774 | 2.810 | 1.651 | 0.812 | 7.11x |
| Chains | 128 | 0.208 | 0.357 | 0.227 | 0.139 | 1.50x |
| Chains | 512 | 0.960 | 1.515 | 0.895 | 0.452 | 2.13x |
| Chains | 2,048 | 5.788 | 5.813 | 3.583 | 1.847 | 3.13x |

Four-worker times differ by no more than about 3.1% from the initial port's
matched synthetic captures, with both increases and decreases. This followup
primarily improves the gameplay adapter and shared static queries; it does
not demonstrate a substantial improvement to isolated solver throughput.
Single-thread small chains still lose to ODE, while parallel heavy stacks
support the issue's throughput premise.

For 2,048 stacks, four-worker Jolt's median process CPU per recorded step is
3.63 ms versus 5.75 ms for ODE. For 2,048 chains it is 8.03 versus 5.75 ms:
parallel wall-time gains can use more total CPU. These counters include
validation/output and have coarser resolution than wall timings. Jolt's median
stack peak working set/private bytes are 61.2/169.7 MiB versus ODE's
55.5/108.7 MiB. Chain values are 63.0/171.1 versus 55.8/108.8 MiB.

## Assessment of issue #2139

The requested adapter, sleeping and shared-query improvements materially
improve the native migration. They do not establish that full migration is
an in-game speed upgrade: this save still favors ODE, despite much smaller
native overhead. The remaining measured costs include caller-thread body and
policy snapshots, native integration and effect dispatch. Ray query throughput
also still trails the parent path. The synthetic fixture isolates native
collision/solver throughput and cannot substitute for these gameplay costs.

Keep the migration experimental until the unexplained shutdown exit is
resolved and gameplay coverage is broader. Full campaign, vehicles,
multiplayer, Linux and Debug runtime coverage remain outside these tests.

## Further local optimization

This change builds on `25ad9bb2e` and remains local:

- Retain mesh-local regions proven to contain no slowdown material. Each step
  checks the current transformed, swept/speculative body bounds against the
  cached region and the same mesh shape. A successful containment test avoids
  revisiting mesh leaves. Only negative results are retained; expansion of a
  dry region cannot add preparatory contacts for the current body pair.
- Compare the sorted set of slowdown-material indices every step, invalidating
  exclusions when flags change even if the material count stays the same.
  Clear exclusions on world clearing/body retirement. Shape references protect
  against pointer reuse; no second mesh BVH is created. Worker jobs read the
  already prepared pair list and do not access the exclusion map.
- Test ray/triangle intersection using the original vertex pointers, constructing
  and copying a complete CDB result only after a hit. Intersection arithmetic,
  triangle order, equal-distance behavior and result metadata are preserved.
- Add `-ScratchRoot` to the gameplay runner so different comparisons can share
  one isolated game workspace/cache. Reuse the existing build and one candidate
  runtime, keeping the baseline frozen and avoiding further PDB copies.

The fluid regression fixture checks dry contacts across steps, movement into
water, a mesh moving beneath a stationary body, and changed material flags.
It passes with 0/1/4 workers. CDB query and threaded-query tests pass with the
lazy ray-result change. Release core/CDB/physics builds pass. These changes do
not alter the native public interface or the isolated solver benchmark path,
so the earlier synthetic measurements are retained rather than repeated.

### Local comparison and exit-status exclusion

`native-second-final-20261001` contains three repetitions each of the first
optimization and the new candidate, at one and four workers. All twelve
accepted captures pass all 35 loaded-game checks, saving, process reload and
zero main-process exit. Contact profiling/test hooks are disabled; builds do
not run during timing. Query origins, hit counts and ranges match. The engine,
game DLL, save, settings and scratch workspace are shared.

One earlier candidate-four-worker reload reached `GAMEPLAY_RELOAD DONE` and
the quit log, but the runner rejected its process exit. Its numeric exit code
was not retained, so this is an ambiguous rejection, not an established crash.
Logs, save and CSVs are preserved in the comparison's `excluded-captures`
directory. The replacement exits normally. The runner now retains the process
handle, waits for exited processes, records `reload-status.json`, includes the
exit code in reload errors and retains CSVs even if reload fails. This does
not establish a cause for the rejection or fix the previously confirmed
shutdown access violation described above.

The targeted four-worker repeat later captured a confirmed reload exit of
`0xc0000005` after successful validation, retained and excluded separately.
ProcDump first-chance minidumps reproduce the failure with both the candidate
and the frozen first-optimization baseline. Matching Microsoft symbols show
`D3DCompiler_47.dll`'s `CDiaSymbol::rgpropinfo` exit destructor freeing memory,
with the access violation detected in `ntdll!RtlpCoalesceFreeBlocks` during
`LdrShutdownProcess`. This locates detection; it does not identify the write
that damaged the heap or exonerate all migration code.

The copied compiler is version `10.0.26100.7705`. A diagnostic substitution of
the installed `10.0.26100.9457` compiler also fails, so it is not a workaround.
The candidate compiler is restored from the frozen baseline before continuing
timing. The runner now checks that effective shader-compiler hashes match
across runtimes and records them for new captures. Three roughly 50 MiB
minidumps and the matching 6 MiB Microsoft symbols are retained, without
installing a debugger package or making further runtime/PDB copies.

Diagnostic artifacts are `native-second-shutdown-dumps`, including candidate,
`baseline` and `current-compiler` captures. The interrupted timing repeat and
its retained failure are under `native-second-four-repeat-20261001`.

### Further game measurements

Medians of three process means in each capture set, milliseconds per complete
step. The second set was added to investigate the variable four-worker
ragdoll result; it is reported separately instead of selecting a favorable run.

| Set | Workers | Phase | Frozen baseline | Local candidate | Reduction |
| --- | ---: | --- | ---: | ---: | ---: |
| First | 1 | Idle | 0.402 | 0.379 | 5.6% |
| First | 1 | Driven ragdolls | 0.778 | 0.726 | 6.7% |
| First | 4 | Idle | 0.369 | 0.346 | 6.4% |
| First | 4 | Driven ragdolls | 0.689 | 0.765 | -11.1% |
| Repeat | 4 | Idle | 0.376 | 0.352 | 6.6% |
| Repeat | 4 | Driven ragdolls | 0.842 | 0.813 | 3.4% |

Idle preparation falls from 0.126-0.127 to 0.102-0.106 ms across these
comparisons. The first one-worker ragdoll preparation median falls from 0.189
to 0.108 ms. Four-worker ragdoll preparation is roughly unchanged, while
integration varies substantially. Its complete-step run ranges overlap:
first baseline 0.682-0.946 vs candidate 0.671-0.827 ms; repeat baseline
0.704-0.874 vs candidate 0.692-0.814 ms. The change of direction between sets
does not support a consistent four-worker ragdoll speedup or regression.

Nearest rays fall from 0.954-0.961 to 0.883-0.894 microseconds per query
(6.3-7.8%); any rays from 0.628-0.631 to 0.591-0.595 (5.4-6.3%). Full-box
queries are roughly unchanged, with differences of at most 1.2%. Reported
static model bytes remain 96,089,734; that counter does not include the small
physics exclusion map. Whole-process peak working-set differences vary in
direction and do not demonstrate a memory reduction.

All eighteen accepted captures pass all 35 loaded-game checks, saving and
process reload, with zero main exit and no contact-profile output. The
rejected captures and three diagnostic minidumps remain part of the evidence;
the shutdown limitation remains unresolved. No fresh ODE or isolated-solver
comparison is included in this experiment, so these adapter/query gains do
not establish that native migration now beats ODE in gameplay.

The local CSV summaries are indexed in
[the benchmark index](tests/benchmark-results/2026-10-01-native.md).
Candidate core SHA256:
`83B1F5666EF1E5E977569FCB246FDA815C43FBF723445392EBF17251331D7BBA`;
CDB SHA256:
`BAA789F219ADF9E300618D235F078567EAF1A7FDF0A0E56EEB6F57D052161236`.
Runtime manifests are `native-second-final-runtimes.json` and
`native-second-four-runtimes.json`. Captures are
`native-second-final-20261001` and `native-second-four-repeat-20261001` under
`C:/code/migration-benchmark`; baseline binaries remain frozen in
`native-game-bin-performance-final`. New records include matched compiler
SHA256 `A05F99734F7C4822FEFC12B367AF21FD0976ED6608752FB1E1E80B6ECE7ECBBB`.

### Clear Sky cross-game check

The additional [Clear Sky starting-base comparison](tests/benchmark-results/2026-10-01-clear-sky.md)
reuses these native binaries and the ODE reference, with three captures each
at one native worker. The local changes lower idle step time from 0.746 to
0.716 ms (4.0%), nearest/any rays by 5.3%/6.7%; ragdoll ranges overlap.
All nine captures and reloads pass, including all 35 checks in each native run.
ODE remains faster at 0.030 ms idle / 0.853 ms ragdolls vs new native's
0.716 / 1.098 ms. This is a second measured scene where full native migration
has not established an in-game speedup. Shared-mesh full-box queries are
about 3.0 times faster than the parent CDB here; query gains depend on scene.

### Disk usage

The new comparisons reuse `native-game-bin-second-profile` and the existing
`native-second-profile-20261001/games/ProfileNative4` game workspace; only result
files are added. C: remained near 25.1 GiB free during the first comparison and
has about 24.9 GiB free after the minidumps and symbols. Older
diagnostic runtime copies and old build intermediates still occupy substantial
space. Automatic approval review rejected cleanup with only "blocked by
policy"; no files were deleted and no reclamation is claimed.
