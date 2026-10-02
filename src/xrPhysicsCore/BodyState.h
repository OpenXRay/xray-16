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
PHYSICS_CORE_API bool GetPhysicsBodyState(BodyHandle body, NativeBodyState& state);
