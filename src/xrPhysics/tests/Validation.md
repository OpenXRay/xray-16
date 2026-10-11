# Local validation — 2026-09-30

Base: PR #2149, `650f204d459d40789b9c8f077b5b6b1d5b5e5449`.
Toolchain: Visual Studio 2022 / MSVC 19.44, Windows x64, CMake 4.4.3.
The physics/collision bridge builds were linked against the matching native
engine interfaces from the PR's Windows build. Only the new collision and
physics DLLs and fixture executables were rebuilt for this follow-up.

## Automated tests

Each listed build passed `xrCDB.queries`, `xrCDB.threaded_queries` and
`xrPhysics.backend` (3/3 tests):

| Configuration | Dynamics | Collision |
| --- | --- | --- |
| Release | ODE | OPCODE |
| Release | Jolt | Jolt |
| Release | Jolt | OPCODE |
| Debug | Jolt | Jolt |

The physics fixture checks physical invariants, not equality of complete
trajectories. See `BackendFixture.cpp` for assertions and tolerances. Jolt
fixture cases cover execution on the caller and with two worker threads.
The Debug runs found and fixed non-null world-handle requirements in the legacy
ODE settings getters. The threaded tests found and fixed an overflow caused by
squaring excessively large velocity limits.

The expanded GitHub workflow has not been run remotely; nothing was pushed.
Linux/macOS builds and native MSVC integration of the backend flag are untested.
The updated Windows test script was parsed successfully, but its complete
NuGet/native-rebuild orchestration was not rerun: local CMake bridge builds
reused existing compatible native DLLs and libraries.

## Gameplay

An isolated Call of Pripyat test root used the user's existing game archives.
The original installation and saves were not edited. As with the base PR's
smoke test, the old archive scripts needed two local compatibility overrides:
a guard for the absent `profiler.setup_hook` and a repaired Lua escape in
`gulag_general.script`. The supplied ALife version-6 save is incompatible with
this engine's version 7; tests used the previously generated version-7
`collision-start.scop` at Zaton's starting location.

Jolt dynamics + Jolt collision, Release, R2 renderer, 1280×720:

- Loaded the save and reached playable Zaton; startup logged
  `Physics dynamics backend: Jolt (legacy contact/joint bridge)`.
- Walked on terrain, sent jump/crouch input, and fired the weapon (ammo 30→28).
  This was a responsiveness check; obstacle traversal and jump height were not
  measured.
- Ran `run_script dynamics_encounter` with the committed script. The NPC came
  online alive, the ray hit the same ID, and a scripted lethal impulse produced
  a ragdoll. After five seconds, its position remained unchanged over another
  three seconds. The script logged `DYNAMICS_ENCOUNTER PASS`.
- Shut down with the console `quit` command and reached `KERNEL:QUIT` without
  a physics assertion.

The same committed encounter script also passed on the ODE + OPCODE Release
baseline: the NPC was hit, died, and its position stayed unchanged over the
final three seconds. Both backends logged the same
`SV:ge_destroy ... not found on server` warning during the scripted NPC death.
Renderer constant-buffer warnings were also present, as in the base PR tests.
An earlier test attempt overflowed the existing 256-byte `run_string` wrapper
and invoked the CRT invalid-parameter handler; using a script file avoids that
unrelated console limit. A first version of the probe also had a Lua variable
shadowing error, repaired in the committed script. These attempts are preserved
in local evidence and are not counted as successful encounters.

Logs, screenshots and build outputs are retained locally under
`C:/code/jolt-physics-gameplay` and the worktree's `build` directories.

This is a smoke test. Dialogue, real bullet impacts on NPCs, props, explosions,
doors, vehicles, breakage, multiplayer, level transitions and Clear Sky were
not validated. It does not establish gameplay parity or a performance gain.
