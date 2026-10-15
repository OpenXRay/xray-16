#pragma once

#include "xrPhysicsCore/IPhysicsCore.h"

IC void spatialParsFromShape(PhysicsShapeHandle shape, Fvector& center, Fvector& AABB, float& radius)
{
    if (!shape)
    {
        center.set(0.f, 0.f, 0.f);
        AABB.set(0.f, 0.f, 0.f);
        radius = 0.f;
        return;
    }

    GetPhysicsCore()->GetCDBModelBounds(shape, center, AABB);

    float sq_len = (AABB.x * AABB.x) + (AABB.y * AABB.y) + (AABB.z * AABB.z);
    if (sq_len < 0.0001f)
    {
        radius = 100.f;
        return;
    }

    radius = _sqrt(sq_len);
}
