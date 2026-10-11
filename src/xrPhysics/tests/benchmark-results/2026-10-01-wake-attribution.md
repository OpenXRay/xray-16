# Native wake and contact cost investigation, 2026-10-01

Local continuation of [issue #2139](https://github.com/OpenXRay/xray-16/issues/2139),
following the [Clear Sky comparison](2026-10-01-clear-sky.md).
Production changes are in `7d973f106`, following the first optimization in
draft PR #2167. Measurements were collected locally before publication.

## What caused the excess work

`character_shell_control::UpdateFrictionAndJointResistanse` continually ramps
the resistance cap on death shells, with a target speed of zero. The previous
adapter represented this as a velocity motor and called `ActivateBody` on both
dynamic endpoints whenever its cap changed. In the pinned Jolt revision,
`BodyManager::ActivateBodies` resets the sleep timer even for an already active
body. Exact-setting caches did not help with these legitimately changing caps.

The diagnostic found valid brake updates, with no non-finite inputs or driven
motors in these phases. The original instrumented Clear Sky idle interval
averages 342,242 motor activation calls per 100 physics steps, including 387
calls on previously inactive bodies. Those counters are API calls; they do not
claim that every call changed a body's active state. Explicit activation,
velocity writes and actual driving remain enabled.

The adapter now uses native joint friction for zero-speed resistance on hinges,
sliders and SixDOF rotation axes. Changing a passive cap preserves friction warm
starts and does not wake a resting body. Starting/changing a drive or stopping
a previously driven motor still wakes its endpoints. Resistance has not been
removed: tests impose motion, verify braking, wait for sleep, then start a
drive and verify waking/motion for all three joint types.

## Attribution runs, excluded from performance conclusions

`XRAY_JOLT_CONTACT_PROFILE=1` adds caller-thread phase timings, potential wake
sources, character update timing and shell sleep votes. Use `mt_physics off`.
`misc/windows/summarize-native-profile.ps1` selects only the `idle`/`ragdolls`
BEGIN/END intervals and drops the first sample per field in each interval,
since an averaged window can straddle BEGIN. There are nine retained windows
for idle and nineteen for driven ragdolls in each diagnostic. Loading, checks,
save/reload and post-benchmark rest are excluded.

The core/world/character timings overlap. Character calls can occur outside a
world step, and the world timing includes the core. Do not sum these columns
as independent whole-frame costs. Contact snapshot/query/dispatch fields are
single-step samples once per 100 steps; core/world fields are 100-step means.

Clear Sky idle diagnostic means, milliseconds per physics step unless stated:

| Measurement | Original adapter | Passive resistance | Also batch snapshots / filter queries |
| --- | ---: | ---: | ---: |
| Native active rigid bodies (sampled) | 136.7 | 0 | 0 |
| Motor activation calls / 100 steps | 342,242 | 0 | 0 |
| Contact preparation | 0.1636 | 0.1148 | 0.0851 |
| Native integration | 0.2738 | 0.00350 | 0.00338 |
| Rigid contact effects | 0.0739 | 0.000071 | 0.000064 |
| World data update | 0.1765 | 0.0533 | 0.0546 |
| Character update cost / 100 steps, divided by 100 | 0.4801 | 0.4074 | 0.4345 |

The causal finding is the repeated timer reset and its removal: changing brake
caps no longer prevent resting rigid bodies from sleeping. The diagnostic
phase means describe where work disappears; they are not independent,
noise-free estimates of every optimization's contribution. In particular,
these runs do not establish a character-controller speedup.

The remaining idle contact prepass still snapshots roughly 631 bodies even
with no active rigid bodies. A single `BodyLockMultiRead` now protects those
snapshots rather than hundreds of separate lock/unlock operations. Locks
remain in place for external writers. Character/placement body filtering also
uses Jolt's `ShouldCollideLocked`, which supplies the already locked body,
instead of acquiring a separate lock in `ShouldCollide`.

Driven-ragdoll preparation remains the largest adapter cost: diagnostic means
are 0.5130 ms originally, 0.4967 ms with passive resistance and 0.4608 ms with
the further changes. Filtering lowers sampled dispatch from 0.0666 to 0.0439
ms, but native rigid effects increase from 0.0722 to 0.1045 ms. This shift does
not establish a complete-step gain from triangle filtering alone.

The pinned vanilla `ShapeFilter` does not visit individual mesh triangles.
The generated mesh translation unit now supports an optional, nested,
thread-local filter immediately before convex/sphere triangle overlap/sweep
tests. Only preparatory queries use it, selecting slowdown triangles of plain
tagged static meshes. Normal solver collisions keep their ordinary traversal.
Dry contacts still dispatch their effects using pre-solver snapshots. Custom
response-changing policies, moving/compound meshes and unresolved materials
retain the conservative preparatory path. Fluid forces keep first-step timing.

Raw diagnostics, all under `C:/code/migration-benchmark`:

- `clear-sky-full-attribution-20261001`: original motor behavior plus profiling.
- `clear-sky-passive-attribution-20261001`: passive resistance, individual snapshot locks.
- `clear-sky-fluid-attribution-20261001`: batch snapshots, query lock reuse and triangle filtering.
- `clear-sky-motor-probe.log`: short input-validity diagnostic, outside timing.
- `clear-sky-passive-pilot-20261001` and `cop-passive-pilot-20261001`: single-run setup/pilots, outside the final comparison.

Compact diagnostic data:
[original profile](2026-10-01-wake-original-profile.csv),
[passive profile](2026-10-01-wake-passive-profile.csv),
[further profile](2026-10-01-wake-filtered-profile.csv).
The third diagnostic predates the conservative moving-mesh/unresolved-material
fallback added before final tests. The static tagged level-mesh path is the same.

## Matched gameplay comparison after the wake fixes

The final comparison contains three runs per runtime per game, 18 captures in
total, alternating forward/reverse runtime order. Profiling is disabled. These
are medians of per-run mean physics times, in milliseconds; ranges are the
minimum and maximum run means. All variants use 100 Hz, 18 iterations and the
same engine executable, configuration, shader compiler and starting save.
Twelve ragdolls receive forces throughout the ragdoll phase, so sleeping cannot
explain away that workload. These tests use the conservative final filters in
`7d973f106`, not the earlier diagnostic binaries.

| Game / runtime | Idle | Driven ragdolls | Ragdoll range |
| --- | ---: | ---: | ---: |
| Clear Sky / ODE | 0.03137 | 0.76986 | 0.58805–0.78486 |
| Clear Sky / native, 1 worker | 0.14721 | 1.13512 | 1.05256–1.15207 |
| Clear Sky / native, 4 workers | 0.14162 | 0.96995 | 0.90126–0.97401 |
| Call of Pripyat / ODE | 0.06153 | 0.54994 | 0.53465–0.55406 |
| Call of Pripyat / native, 1 worker | 0.09866 | 0.71279 | 0.70748–0.74801 |
| Call of Pripyat / native, 4 workers | 0.10037 | 0.64338 | 0.60092–0.74492 |

Native with four workers remains 26.0% slower in Clear Sky and 17.0% slower in
Call of Pripyat during driven ragdolls. Earlier single-run pilots appeared to
beat ODE; those gains did not survive repetition. The native idle improvement
against historical second-optimization measurements is large (79.4% in Clear
Sky and 74.0% in Call of Pripyat with one worker), but those historical captures
were not interleaved here and do not isolate each optimization's contribution.

| Game / runtime | Nearest ray, µs | Any ray, µs | Box, µs |
| --- | ---: | ---: | ---: |
| Clear Sky / ODE | 1.2340 | 0.5046 | 19.7914 |
| Clear Sky / native, 4 workers | 1.2110 | 0.7033 | 6.5907 |
| Call of Pripyat / ODE | 0.6590 | 0.4410 | 1.5832 |
| Call of Pripyat / native, 4 workers | 0.9068 | 0.6039 | 0.8952 |

Box queries improve, while any-hit rays lose and nearest rays do not establish
a general gain. Query origins, hit signatures and hit distances match across
the backends. Native static model accounting is 13.5% lower in Clear Sky and
13.7% lower in Call of Pripyat; it excludes allocations not reported by the
model and does not represent whole-process memory. Loading-inclusive median
peak working sets are higher for native in these captures.

All 18 main runs and reloads exit successfully. All 12 native runs pass 35
loaded-world checks; every runtime passes 12 ragdoll save/reload checks.
There are no profile markers or fatal/script failures in the timed captures.
Remaining native costs include contact preparation, effect dispatch and world
data synchronization. No complete-step performance win has been established.

Evidence: [Clear Sky physics](2026-10-01-wake-clear-sky-physics.csv),
[queries](2026-10-01-wake-clear-sky-queries.csv),
[CoP physics](2026-10-01-wake-cop-physics.csv),
[queries](2026-10-01-wake-cop-queries.csv),
[validation](2026-10-01-wake-validation.csv),
[runtime hashes](2026-10-01-wake-inputs.csv).
Raw captures are `clear-sky-attribution-final-20261001` and
`cop-attribution-final-20261001` under `C:/code/migration-benchmark`.

## What the external research supports

The relevant primary guidance is [Jolt's architecture documentation](https://github.com/jrouwe/JoltPhysics/blob/master/Docs/Architecture.md):
allow sleeping, batch insertion, reject irrelevant collisions early, and use
the correct locking interface. Contact listeners run during worker updates
and must not mutate bodies. Simple AI can use rigid `Character` controllers;
`CharacterVirtual` provides more involved query-based movement.

The migration already lets Jolt's ragdoll implementation batch insertion.
Broadphase optimization on world clear does not establish good insertion for
every other shell path. Moving all AI to rigid characters, batching remaining
shell insertion, and a CPU-feature/build experiment remain separate work with
their own behavior and performance checks. Jolt uses Release `/O2` and
baseline x64 SIMD; its AVX2/interprocedural optimization options are disabled.
Some engine libraries already use MSVC `/GL` and link-time code generation.
No solver
iteration, timestep, damping, sleep-threshold or collision-quality reduction
is used to make these timings better.

[Jolt's author benchmark](https://www.jrouwe.nl/jolt/JoltPhysicsMulticoreScaling.pdf)
compares PhysX and Bullet, not ODE. The author also cautions against broad
claims from one timing in [the PhysX comparison discussion](https://github.com/jrouwe/JoltPhysics/discussions/327).
An actual [ODE-to-Jolt port account](https://www.froyok.fr/blog/2024-11-ombre-dev-blog-2/)
reports that switching from Debug to Release improved performance, but is not
a matched numerical ODE/Jolt benchmark of X-Ray workloads. The search did not
provide a controlled ODE/Jolt comparison that could replace our engine tests.

## Validation

The final Release core and native tests pass with 0, 1 and 4 workers. New cases
cover changing passive caps, braking external motion, starting/stopping drives,
owner/camera query filtering, and box/sphere contacts with dry and slowdown
triangles in one mesh leaf. They check retained first-step fluid sweeps,
ordinary dry effects without duplicate fluid effects, and the immediate
callback fallback. Existing fluid-region movement/material-flag, friction,
capture, suspension, joint-feedback, character and collision-owner checks pass.
The CDB build passes; final game query comparisons exercise the loaded meshes.

This remains an experimental draft. The previously observed shader-compiler exit
heap exception has no identified corrupting write and is not claimed fixed.
