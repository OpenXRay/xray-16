# Initial in-game performance results — 2026-09-30

Twelve complete process runs: three repetitions of each of the four backend
combinations. All loaded the same version-7 Zaton save, ran the same script,
exported both capture phases and query batches, and exited normally. Query
coordinates, hit counts and nearest-ray range sums matched across every run.
The early pilot is excluded: its zero-damage impulses allowed ragdolls to sleep.

Hardware: AMD Ryzen 9 9955HX, NVIDIA GeForce RTX 5070 Ti Laptop GPU.
Windows x64 Release, MSVC 19.44, R2 renderer, 800×600, VSync off, engine frame
limit 501, `mt_physics off`, 100 Hz physics and 18 iterations. The engine and
native dependencies were identical; only `xrCDB.dll` and `xrPhysics.dll`
varied. Timing capture was enabled in all four builds. The ten-second warm-up
excluded loading/shader startup. Runtime order was forward, reverse, forward.

These results describe the current PR #2149 collision tree and experimental
dynamics **bridge**. They do not predict performance of a future native Jolt
shape/contact world or removal of the remaining ODE interfaces.

## Queries on the real loaded level

Times are microseconds/query: median of seven fixed-input batches within each
process, then median of the three processes. Each batch contained 4,096 probes.
Lua, dynamic object traversal, query generation and disk output are excluded.

| Collision / dynamics | Nearest ray | Any-hit ray | Full box |
| --- | ---: | ---: | ---: |
| OPCODE / ODE | 0.822 | 0.506 | 0.931 |
| Jolt / ODE | 0.663 | 0.443 | 1.588 |
| OPCODE / Jolt | 0.801 | 0.498 | 0.916 |
| Jolt / Jolt | 0.660 | 0.444 | 1.586 |

Holding dynamics at ODE, Jolt improved nearest-ray cost by **19.4%** and any-hit
ray cost by **12.5%**, but increased full-box cost by **70.5%**. The ranges
between process runs were separated for each of these comparisons. The same
direction appeared with Jolt dynamics, which is outside the timed query batch.
This is a useful collision-tree result for these probes on this level, not a
measurement of the game's complete query mix.

Collision model-reported memory was 118,187,388 bytes for OPCODE and 111,372,664
bytes for Jolt, about **5.8% less**. This is the model's estimate, not whole-engine
working set or Jolt allocator usage.

## Physics in the game

Values are medians of process-run means, in milliseconds per fixed 10 ms step.
The idle phase lasted ten seconds; the ragdoll phase lasted twenty seconds.
The twelve shells received the same force function once per physics step.

| Collision / dynamics | Idle whole step | Idle solver | Ragdoll whole step | Ragdoll solver | Active bodies | Contacts |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| OPCODE / ODE | 0.0611 | 0.0302 | 0.819 | 0.554 | 167.9 | 161.0 |
| Jolt / ODE | 0.0593 | 0.0294 | 0.618 | 0.404 | 168.6 | 139.9 |
| OPCODE / Jolt | 0.1076 | 0.0793 | 0.808 | 0.576 | 167.3 | 154.0 |
| Jolt / Jolt | 0.1007 | 0.0741 | 0.805 | 0.574 | 167.7 | 159.8 |

Idle body/joint counts matched exactly (27.02 bodies, 18.02 joints, zero
contacts). With OPCODE held constant, the bridge's idle solver interval was
**2.62×** ODE's, and its complete step cost was **76% higher**. The absolute
whole-step increase was only 0.0466 ms, about 4.66 ms per simulated second at
100 Hz. This is a clear fixed overhead regression, rather than evidence that
every rendered frame becomes 2.62× slower.

The stressed scene does **not** establish a general dynamics improvement.
Whole-step run means ranged from 0.673–0.832 ms with OPCODE/ODE and
0.808–0.810 ms with OPCODE/Jolt. Those ranges overlap; their medians differ by
only about 1.4%. Contacts differ even with comparable active-body counts.
Jolt/ODE ranged from 0.523–0.787 ms and had fewer contacts. Its apparently lower
physics cost includes a different contact workload and changed simulated
trajectories; it cannot be attributed solely to a faster collision tree.

The solver interval includes the production bridge's state synchronization,
mass/inertia updates and custom constraint creation. Profiling those stages is
the next useful investigation for the idle overhead; they have not been timed
separately here.

## Frame intervals

Median of each process's p95 frame interval, in milliseconds:

| Collision / dynamics | Idle p95 | Ragdoll p95 |
| --- | ---: | ---: |
| OPCODE / ODE | 3.529 | 4.442 |
| Jolt / ODE | 3.481 | 4.138 |
| OPCODE / Jolt | 3.611 | 4.573 |
| Jolt / Jolt | 3.471 | 4.412 |

The combined build's stressed p95 is close to the baseline. These intervals
include rendering, game scheduling and waits, and the test has only three
process trials per build. There is no reliable broad frame-rate improvement
claim. They are measured at physics `OnFrame`, not at display presentation.

## Reproducing and extending

See [the procedure](GameplayBenchmark.md). Committed summaries retain the run
ranges and workload counts: [physics](benchmark-results/2026-09-30-physics.csv)
and [queries](benchmark-results/2026-09-30-queries.csv).
Full CSVs, logs, DLL hashes, hardware/config metadata and per-run summaries are
retained locally under `C:/code/migration-benchmark/controlled`.

Before selecting a default backend, add normal-resolution runs with normal
threading, a fixed camera route in a populated area, and scaling scenes with
more props/ragdolls. Collect five or more trials for small differences, compare
contact workloads, and investigate the box-query regression and idle bridge
overhead. These initial results support keeping both migrations opt-in.
