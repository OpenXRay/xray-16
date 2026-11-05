#pragma once
#include "IPhysicsCore.h"

// One coherent read for immediate caller-thread synchronization. No locks are
// held after this call, and callers update local velocities after any writes.
struct NativeBodyState
{
    Fmatrix transform = Fidentity;
    Fvector linear_velocity{}, angular_velocity{}, force{}, torque{};
    float mass = 0;
    bool active = false;
};
// With active_only, a valid inactive body returns true with active=false and
// the remaining fields at their defaults. This avoids work discarded by sync.
PHYSICS_CORE_API bool GetPhysicsBodyState(BodyHandle body, NativeBodyState& state, bool active_only = false);
