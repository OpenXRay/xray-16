// Native X-Ray contact feedback extension for the pinned Jolt revision.
#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/SubShapeID.h>

JPH_NAMESPACE_BEGIN
// Called by solver workers. The callback must only collect results; gameplay
// code consumes them after PhysicsSystem::Update has released its locks.
using XRayContactImpulseCallback = void (*)(void*, const Body&, const SubShapeID&,
    const Body&, const SubShapeID&, RVec3Arg, RVec3Arg, Vec3Arg, Vec3Arg);
// Set or clear only while no PhysicsSystem::Update is running.
void SetXRayContactImpulseCallback(void* context, XRayContactImpulseCallback callback);
JPH_NAMESPACE_END
