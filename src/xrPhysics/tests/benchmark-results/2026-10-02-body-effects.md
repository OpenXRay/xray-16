# Body synchronization and contact effects, 2026-10-02

Local continuation of [issue #2139](https://github.com/OpenXRay/xray-16/issues/2139)
and the [wake investigation](2026-10-01-wake-attribution.md). Production source:
`82307c47d`, followed by the inactive-read shortcut in `a2d963eef`. No changes
from this investigation have been pushed.

## Changes

Element synchronization previously acquired a body read lock repeatedly for
activity, velocities, the disabling vote, position/rotation interpolation,
mass and drag accumulators. A single coherent state read now supplies those
values. Its lock is released before any velocity, force or torque writes.
Local velocities follow scaling/clamping writes; interpolation still uses the
center-of-mass transform, and disabling retains its original counters, motion
thresholds and whole-shell vote. No object layout or virtual interface changes
are required. Inactive bodies still exit without applying drag.

The follow-on shortcut reads activity under that same lock and avoids deriving
the unused transform, velocities, mass and accumulators for inactive elements.
The default state API still reads a complete state, including static/sleeping
bodies. Its optional active-only mode explicitly returns default remaining
fields for an inactive body; caller synchronization already exits in that case.

Contact effect dispatch now skips material-pair resolution when every actual
effect eligibility check fails. Continuous passable-material sounds remain
eligible at zero impact energy; wallmarks retain their triangle requirement
and have no new camera-distance limit. Sound and particle distance constants
come from the existing game code. Valid native material tags avoid a linear
name search only when the resolved material pointer matches that index;
callback changes and untagged contacts retain the original name lookup.
Effects that qualify retain their existing execution path and timing.

The timestep, solver iterations, collision quality, sleep thresholds, force
application and driving workload are unchanged. The Jolt library itself is
unchanged from the previous comparison. These are adapter changes; the older
synthetic solver measurements do not independently measure their contribution.

## Comparison protocol

Three runs each of ODE, frozen previous native `7d973f106` and optimized native
`82307c47d`, in Clear Sky and Call of Pripyat. Both native versions use four
workers. Runtime order reverses on the second pass. Profiling is disabled,
and no build runs during timing. All use the same engine, configuration,
shader compiler and game-specific starting save. The ODE reference is the
parent Jolt collision-query bridge, not the original retail executable.

Settings and scripts are those in the wake report: 800x600 R2, VSync off,
501 FPS cap, 100 Hz, 18 iterations, `mt_physics off`, 10-second warmup,
10-second idle and 20-second driven-ragdoll capture with 12 additional NPCs.
Native captures execute 35 loaded-world checks before timing; all runtimes
validate twelve saved/reloaded shells outside timing. Hardware: Ryzen 9
9955HX, RTX 5070 Ti Laptop GPU, Windows x64, MSVC Release.

The frozen previous runtime hardlinks unchanged files from the existing
immutable runtime and copies only the three changed physics/collision DLLs.
The candidate reuses its existing directory. Game assets remain junctions,
and no PDBs or another complete build tree are copied.

Raw captures under `C:/code/migration-benchmark`:

- `clear-sky-body-effects-final-20261002`.
- `cop-body-effects-final-20261002`.
- Manifest: `body-effects-runtimes.json`.
- Release build: `body-effects-build-20261002.log`.
- Core checks: `body-effects-tests-{0,1,4}-20261002.log`.

## First comparison: body state and effects

Medians of three per-run mean physics times, milliseconds. Ranges are per-run
ragdoll means. This set uses `82307c47d`, before the inactive-read shortcut.

| Game / runtime | Idle | Driven ragdolls | Ragdoll range |
| --- | ---: | ---: | ---: |
| Clear Sky / ODE | 0.03272 | 0.86400 | 0.69200–0.95024 |
| Clear Sky / previous native | 0.14522 | 1.09085 | 1.03848–1.20933 |
| Clear Sky / optimized native | 0.14495 | 0.87002 | 0.85752–0.95295 |
| CoP / ODE | 0.06082 | 0.68217 | 0.53687–0.68625 |
| CoP / previous native | 0.10250 | 0.76868 | 0.76612–0.82884 |
| CoP / optimized native | 0.09629 | 0.61532 | 0.46446–0.71615 |

The native ragdoll median falls 20.2% in Clear Sky and 20.0% in CoP versus the
frozen previous integration. The CoP optimized median is 9.8% below ODE;
Clear Sky is 0.7% above ODE. Native/ODE run ranges overlap in both scenes.
Native integration time also varies, so the entire median difference cannot
be assigned to a single changed adapter function.

The measured native feedback-phase median falls from 0.11630 to 0.02242 ms
in Clear Sky (80.7%) and from 0.12376 to 0.02577 ms in CoP (79.2%). This
phase includes deferred rigid effects, joint/character feedback and activation
dispatch, not just material lookup. CoP integration is roughly unchanged
(0.41024 to 0.41137 ms), while Clear Sky integration also falls (0.35531 to
0.27244 ms). Treat those solver differences as workload/timing variation,
not as proof that the unchanged Jolt kernel was optimized by this patch.

Native idle still loses to ODE. Ragdoll frame p95 also remains worse: 3.5742
vs 3.0329 ms in Clear Sky and 4.3595 vs 4.1549 ms in CoP. A physics-step
median win in one set is not a repeatable overall frame-time win.

Evidence: [Clear Sky physics](2026-10-02-body-effects-clear-sky-physics.csv),
[queries](2026-10-02-body-effects-clear-sky-query.csv),
[CoP physics](2026-10-02-body-effects-cop-physics.csv),
[queries](2026-10-02-body-effects-cop-query.csv),
[failed attempt metadata](2026-10-02-body-effects-exclusion.json).

## Repeat with inactive-read shortcut

The second set uses `a2d963eef` and compares the final native build against
ODE, three captures each in each game. Report it separately from the first
set; do not select the more favorable ODE or native timing across sets.

| Game / runtime | Idle | Driven ragdolls | Ragdoll range |
| --- | ---: | ---: | ---: |
| Clear Sky / ODE | 0.03338 | 0.68422 | 0.67137–0.68726 |
| Clear Sky / final native | 0.14750 | 0.96643 | 0.93518–1.03827 |
| CoP / ODE | 0.06395 | 0.70598 | 0.59308–0.73648 |
| CoP / final native | 0.09906 | 0.64529 | 0.61083–0.67438 |

The CoP native physics median is 8.6% lower than ODE, following the 9.8%
advantage in the first set. The run ranges still overlap; these small samples
do not establish a precise statistically significant speedup. Clear Sky is
41.2% slower in this repeat, with non-overlapping run ranges; its apparent
near-tie in the first set did not hold. Idle remains 4.42x ODE in Clear Sky
and 1.55x in CoP. The inactive-read shortcut does not establish an idle gain.

| Game / runtime | Ragdoll mean frame, ms | Frame p95, ms |
| --- | ---: | ---: |
| Clear Sky / ODE | 1.7173 | 2.7736 |
| Clear Sky / final native | 2.2227 | 3.5876 |
| CoP / ODE | 3.1174 | 4.2229 |
| CoP / final native | 3.2476 | 4.3660 |

Both sets favor ODE in whole-frame measurements. A repeated CoP physics
median advantage has emerged, but a full-migration frame-time advantage has
not. Box queries remain faster, any-hit rays slower; nearest-ray results
depend on the scene. The static-query code and model accounting are unchanged
by these patches, and all origins/hit signatures/distances match.

Evidence: [Clear Sky physics](2026-10-02-body-effects-active-clear-sky-physics.csv),
[queries](2026-10-02-body-effects-active-clear-sky-query.csv),
[CoP physics](2026-10-02-body-effects-active-cop-physics.csv),
[queries](2026-10-02-body-effects-active-cop-query.csv),
[accepted-run checks](2026-10-02-body-effects-validation.csv),
[binary/config/source inventory](2026-10-02-body-effects-inputs.csv),
[starting-save/script hashes and asset/template paths](2026-10-02-body-effects-fixture.json).
Raw outputs: `clear-sky-body-effects-active-final-20261002` and
`cop-body-effects-active-final-20261002`; manifest:
`body-effects-active-runtimes.json`, under `C:/code/migration-benchmark`.

## Component attribution, excluded from timing

One Clear Sky diagnostic per native version at four workers, using
`XRAY_JOLT_CONTACT_PROFILE=1`, after all timing runs. The existing phase
parser retains nine idle and nineteen ragdoll windows per field after dropping
the first potentially straddling window. These are instrumented means, not
independent measurements of whole-frame speedups. Timing scopes overlap.

| Ragdoll component, ms/step | Previous native | Final native | Change |
| --- | ---: | ---: | ---: |
| World data synchronization | 0.16998 | 0.13358 | 21.4% lower |
| Core rigid effect dispatch | 0.09993 | 0.01772 | 82.3% lower |
| Contact preparation | 0.44639 | 0.44690 | roughly unchanged |
| Integration | 0.39831 | 0.29204 | varies despite unchanged kernel |

Synchronization and effect savings agree with the targeted code changes.
Remaining Clear Sky preparation is substantially larger than either changed
component. Idle data costs (0.05379 vs 0.05496 ms) do not show a gain. These
profiles attribute measured costs, but do not isolate every source change as
a noise-free causal percentage or eliminate physical-trajectory variation.

Both diagnostics pass 35 loaded checks, twelve reload checks and zero exits;
they contribute no timed-comparison runs. Raw directory:
`clear-sky-body-effects-profile-20261002`; manifest:
`body-effects-profile-runtimes.json`. Compact data:
[previous](2026-10-02-body-effects-PreviousNative4-profile.csv),
[final](2026-10-02-body-effects-OptimizedNative4-profile.csv).

## Assessment of the migration premise

The investigation found integration waste and removed substantial measured
cost. CoP now has lower native physics medians in two sets, while Clear Sky
and whole-frame measurements still favor ODE. Earlier matched synthetic
benchmarks already show Jolt winning large parallel stacks/chains; those
fixtures omit gameplay adapters and are not rerun here because the kernel
and build features are unchanged. These results support a workload-dependent
throughput advantage, not a blanket frame-time benefit from full migration.

Remaining targets include the large Clear Sky preparatory contact budget and
virtual-character queries outside the rigid-body step. Jolt's
[architecture guidance](https://github.com/jrouwe/JoltPhysics/blob/master/Docs/Architecture.md)
supports early filtering, batch insertion and simpler rigid AI characters.
Changing controller behavior requires separate movement/contact validation;
batch insertion primarily addresses loading/streaming, not a demonstrated
steady-state win after this fixture's warmup. Jolt also supports
[CPU-specific SIMD builds](https://github.com/jrouwe/JoltPhysics#required-cpu-features),
but this experiment retains baseline x64 SIMD and does not test AVX2 or Jolt
interprocedural optimization. Those are unmeasured followups, not claimed gains.

## Validation

The Release build and native tests pass with 0, 1 and 4 workers. Added checks
cover rotated offset compounds, mass-center state, velocities, accumulators,
sleeping/static bodies and invalid/retired handles. A write immediately after
the snapshot verifies that its read lock has been released. Contact tests
cover exact effect/distance thresholds, continuous passable sounds, distant
wallmarks, missing triangles and changed/untagged material fallback.
The inactive-read variant also passes all three worker configurations.
All 30 accepted timing captures and their reloads exit zero; all 18 native
captures pass 35 loaded checks, and every capture passes twelve reload checks.
The two rejected timing attempts and diagnostic exception below remain part
of the evidence. Passing replacements do not erase the reliability problem.

These tests and save fixtures do not establish campaign-wide gameplay parity.
The previously observed shader-compiler shutdown heap error still has no
identified corrupting write and is not claimed fixed.

The first attempt at CoP optimized run 2 completed the timed main process,
35 loaded-world checks and twelve reload checks, but the reload process exited
with `0xc0000005` after `GAMEPLAY_RELOAD DONE` and `KERNEL:QUIT`. The failed
capture is excluded and retained under the CoP output root's
`excluded-captures/OptimizedNative4-run-2-reload-failure`, including timing
CSVs, both logs, save, reload status and exclusion metadata. The main exit
code was not persisted before the reload exception, so it is not inferred
from its quit log. The slot is rerun without a debugger. This is another
shutdown failure; its precise exception location is not established by these
logs and is not assumed to match the previous dumps.

The inactive-read repeat also rejects the first CoP optimized main capture:
its main process completes the benchmark/checks but exits `0xc0000005`;
its separate reload passes with exit zero. The complete failed capture,
including persisted main exit and binary hashes, is retained under that
root's `excluded-captures/OptimizedNative4-run-1-main-failure`.
See [failed main metadata](2026-10-02-body-effects-active-exclusion.json).

One subsequent diagnostic reload under ProcDump reproduces first-chance
`C0000005` after validation/quit. The symbolized stack matches the earlier
candidate and frozen first-optimization diagnostics: `RtlpCoalesceFreeBlocks`
during D3DCompiler_47's `CDiaSymbol::rgpropinfo` exit destructor, via
`LdrShutdownProcess`. This establishes a repeated detection location, not
the corrupting write or the innocence of migration code. The diagnostic is
excluded from timing. Its approximately 50 MiB minidump and logs are under
`body-effects-shutdown-dumps/reload-1`; the compact
[symbolized stack](2026-10-02-body-effects-shutdown-stack.txt) is retained here.
An initial diagnostic invocation used the wrong startup-script flag and was
stopped after its loaded game did not run the reload script; its log remains
at the dump root and it contributes no benchmark or validation result.
