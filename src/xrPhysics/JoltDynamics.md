# Experimental Jolt dynamics bridge

This local follow-up is based on PR #2149 at `650f204d4`. It adds an opt-in
simulation path in `xrPhysics` as an intermediate step toward issue #2139.
ODE remains a dependency. This change does not complete its removal or the
unification of gameplay physics and collision queries into one Jolt world.

## Build

Set `XRAY_USE_JOLT_PHYSICS=ON` in CMake to select Jolt dynamics. The default is
`OFF`, which preserves ODE stepping. `XRAY_USE_JOLT_CDB` independently selects
the collision tree. Both combinations of each flag are supported.

For Linux/macOS, use the normal top-level CMake build and enable
`XRAY_BUILD_PHYSICS_TESTS=ON` (and optionally `XRAY_BUILD_CDB_TESTS=ON`).
The workflow covers both settings of both backend flags; these platforms have
not been run locally for this follow-up.

For Windows, the native solution still builds the existing ODE backend. Use the
CMake bridge to build the replacement physics DLL and tests:

```powershell
./misc/windows/test-collision.ps1 -Configuration Release -Dynamics Jolt
```

The script restores/builds the native dependencies, builds both collision
backends with the selected dynamics backend, stages separate runtimes, and
runs `xrCDB.*` and `xrPhysics.*` tests. Use `-Dynamics ODE` for the baseline.
Runtime paths are `build/windows-collision/<collision>/<dynamics>/<config>/bin/<config>`.

The bridge accepts `XRAY_NATIVE_BUILD_ROOT` to import matching native libraries
from another worktree. The imported DLLs must have the same configuration and
compatible engine interfaces; this is not a way to use arbitrary engine builds.

## Simulation

- `CPHWorld::Step` submits every active merged engine island in one Jolt update.
  The inactive members of merged islands are skipped, so bodies advance once.
- Activation and camera collision call `CPHIsland::Step` outside the normal
  update. They use a separate Jolt system that advances only the requested
  island. It cannot advance other gameplay objects as a side effect.
- Jolt owns body mass/inertia, force integration, constraint scheduling and
  transform integration. Production systems currently execute on the calling
  physics thread; the fixture also exercises two Jolt worker threads.
- Existing ODE handles remain the gameplay state interface. Positions,
  orientations, velocities, mass, gravity flags and forces are synchronized
  before simulation. Transforms and velocities are written back afterward,
  attached geometry bounds are invalidated, and force/torque accumulators are
  cleared. `dxBodyNoUpdatePos` preserves the stored transform.
- The engine retains ownership of disabling, freezing, contacts and materials.
  Jolt sleeping and damping are disabled because game code handles them.

The bridge translates each existing contact/joint's Jacobian rows into a custom
Jolt `TwoBodyConstraint`. It preserves ERP/CFM, motor/stop bounds, restitution,
normal-dependent friction and `dJointFeedback`. ODE row generation remains;
neither `dWorldStep` nor `dWorldQuickStep` is called by the enabled backend.
Feedback is converted from impulses to forces before the existing damage and
breakage code reads it. Temporary contacts are released from Jolt before the
engine empties its contact group.

Jolt bodies use a dummy sphere with explicitly supplied mass/inertia. Its
collision filters reject every pair: collision response comes exclusively
from the engine's current collision/material callbacks. These dummy shapes
must not be used for gameplay queries. This preserves the legacy contact path
during solver migration, but adds synchronization and broadphase overhead.

This backend uses iterative constraints for every island, including islands
that prefer the legacy exact solver. Its rotation integration and constraint
order differ from ODE; numerical trajectories are not expected to be identical.
There is no performance claim and the backend stays experimental and opt-in.

## Validation

`xrPhysics.backend` runs the same physical invariants against either backend:
gravity, mass/force conversion, anisotropic inertia, accumulator clearing,
variable timesteps, isolated and merged islands, body retirement/recreation,
teleports, no-update-position mode, resting contacts, finite/infinite friction,
character rotation locking, restitution, fixed joints and action/reaction
feedback, slider motors/stops, and hinge motors/stops. Jolt cases also run with
two workers. The collision suite checks engine triangle contact generation.

Local Windows x64 runs passed the three CTest tests with Jolt dynamics + Jolt
collision in Debug and Release, with Jolt dynamics + OPCODE in Release,
and with ODE + OPCODE in Release. The gameplay
probe loaded Call of Pripyat/Zaton, spawned an NPC, hit it with a ray, applied
a lethal scripted impulse and ran its ragdoll. Movement and weapon firing were
also exercised. See [validation notes](tests/Validation.md) for details.

## Remaining work for #2139

1. Replace dummy bodies and legacy collision generation with real Jolt shapes,
   including level triangles, compound shells and character volumes. Carry
   triangle material identity, collision filtering and callbacks into Jolt
   contact handling before switching response paths.
2. Migrate persistent joint definitions to native Jolt constraints, preserving
   motors, limits, softness and force feedback used by breakage and damage.
3. Replace ODE body/joint/geometry handles and direct structure access throughout
   `xrPhysics`, including shell editing, activation, interpolation and network
   state. Remove the row bridge and then the ODE build/link dependency.
4. Connect gameplay ray/box/spatial queries to the actual physics world where
   required. Removing `xrCDB` is a separate API migration, not implied by
   enabling these two backend flags.
5. Expand testing to props, capture, explosions, doors, vehicles, breakable
   shells, save/reload, level transitions, multiplayer and Clear Sky; measure
   frame time/memory with real scenes before considering a default change.
