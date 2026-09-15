#ifndef GPU_PARTICLE_COLLISION_H
#define GPU_PARTICLE_COLLISION_H

struct GpuPapiCollisionNode
{
    float3 minimum;
    uint escape;
    float3 maximum;
    uint first;
    uint count;
    uint pad0;
    uint pad1;
    uint pad2;
};

struct GpuPapiCollisionTriangle
{
    float4 a;
    float4 b;
    float4 c;
};

struct GpuPapiCollisionShape
{
    float4 row0;
    float4 row1;
    float4 row2;
    float4 data0;
    float4 data1;
    uint type;
    uint triangleFirst;
    uint triangleCount;
    uint pad;
};

struct GpuPapiCollisionObject
{
    float4 spatialSphere;
    float4 sphere;
    float4 row0;
    float4 row1;
    float4 row2;
    uint first;
    uint count;
    uint mesh;
    uint pad;
};

StructuredBuffer<uint4> g_GpuCollisionCounts;
StructuredBuffer<GpuPapiCollisionNode> g_GpuCollisionNodes;
StructuredBuffer<GpuPapiCollisionTriangle> g_GpuCollisionStaticTriangles;
StructuredBuffer<GpuPapiCollisionTriangle> g_GpuCollisionDynamicTriangles;
StructuredBuffer<GpuPapiCollisionShape> g_GpuCollisionShapes;
StructuredBuffer<GpuPapiCollisionObject> g_GpuCollisionObjects;
StructuredBuffer<GpuPapiCollisionNode> g_GpuCollisionBlocks;

float3 GpuCollisionTransform(float3 point, float w, float4 row0, float4 row1, float4 row2)
{
    float4 p = float4(point, w);
    return float3(dot(p, row0), dot(p, row1), dot(p, row2));
}

bool GpuCollisionBounds(float3 origin, float3 direction, float3 minimum, float3 maximum,
    float limit, out float entry)
{
    entry = -3.402823466e+38;
    float exit = limit;
    for (uint axis = 0; axis != 3; ++axis)
    {
        if (direction[axis] == 0.0)
        {
            if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis])
                return false;
        }
        else
        {
            float a = (minimum[axis] - origin[axis]) / direction[axis];
            float b = (maximum[axis] - origin[axis]) / direction[axis];
            entry = max(entry, min(a, b));
            exit = min(exit, max(a, b));
            if (exit < entry)
                return false;
        }
    }
    return exit >= 0.0;
}

bool GpuCollisionSphere(float3 origin, float3 direction, float4 sphere, float limit, bool cull,
    out float distance)
{
    float3 relative = origin - sphere.xyz;
    float b = dot(relative, direction);
    float c = dot(relative, relative) - sphere.w * sphere.w;
    float discriminant = b * b - c;
    distance = limit;
    if (discriminant <= 0.0)
        return false;
    float root = sqrt(discriminant);
    float entry = -b - root;
    float exit = -b + root;
    if (exit < 0.0 || (cull && entry < 0.0))
        return false;
    distance = entry < 0.0 ? exit : entry;
    return (!cull && entry < 0.0) || distance < limit;
}

bool GpuCollisionTriangle(float3 origin, float3 direction, GpuPapiCollisionTriangle triangle,
    float limit, bool rejectBehind, out float distance)
{
    float3 e1 = triangle.b.xyz - triangle.a.xyz;
    float3 e2 = triangle.c.xyz - triangle.a.xyz;
    float3 p = cross(direction, e2);
    float determinant = dot(e1, p);
    distance = limit;
    if (determinant < 0.00001)
        return false;
    float3 t = origin - triangle.a.xyz;
    float u = dot(t, p);
    if (u < 0.0 || u > determinant)
        return false;
    float3 q = cross(t, e1);
    float v = dot(direction, q);
    if (v < 0.0 || u + v > determinant)
        return false;
    distance = dot(e2, q) / determinant;
    return (!rejectBehind || distance > 0.0) && distance < limit;
}

bool GpuCollisionCylinder(float3 origin, float3 direction, float4 centerRadius,
    float4 axisHeight, float limit, out float distance)
{
    float3 axis = axisHeight.xyz;
    float3 up = abs(axis.x) >= abs(axis.y)
        ? float3(-axis.z, 0.0, axis.x) * rsqrt(axis.x * axis.x + axis.z * axis.z)
        : float3(0.0, axis.z, -axis.y) * rsqrt(axis.y * axis.y + axis.z * axis.z);
    float3 right = cross(up, axis);
    float3 relative = origin - centerRadius.xyz;
    float height = dot(relative, axis);
    float axialDirection = dot(direction, axis);
    float2 radial = float2(dot(relative, up), dot(relative, right));
    float2 radialDirection = float2(dot(direction, up), dot(direction, right));
    float radiusSquared = centerRadius.w * centerRadius.w;
    float halfHeight = axisHeight.w * 0.5;
    float a = dot(radialDirection, radialDirection);
    float b = dot(radial, radialDirection);
    float c = dot(radial, radial) - radiusSquared;
    float entry = -3.402823466e+38;
    float exit = 3.402823466e+38;
    distance = limit;
    if (a <= 1e-24)
    {
        if (c > 0.0)
            return false;
    }
    else
    {
        float discriminant = b * b - a * c;
        if (discriminant < 0.0)
            return false;
        float root = sqrt(discriminant);
        entry = (-b - root) / a;
        exit = (-b + root) / a;
    }
    if (abs(axialDirection) <= 1e-12)
    {
        if (abs(height) > halfHeight)
            return false;
    }
    else
    {
        float cap0 = (-halfHeight - height) / axialDirection;
        float cap1 = (halfHeight - height) / axialDirection;
        entry = max(entry, min(cap0, cap1));
        exit = min(exit, max(cap0, cap1));
    }
    if (entry < 0.0 || exit < entry || entry >= limit)
        return false;
    distance = entry;
    return true;
}

bool GpuCollisionShape(float3 origin, float3 direction, GpuPapiCollisionShape shape,
    float limit, out float distance)
{
    distance = limit;
    if (shape.type == 1)
    {
        float3 localOrigin = GpuCollisionTransform(origin, 1.0, shape.row0, shape.row1, shape.row2);
        float3 localDirection = GpuCollisionTransform(direction, 0.0, shape.row0, shape.row1, shape.row2);
        if (all(localOrigin >= -shape.data0.xyz) && all(localOrigin <= shape.data0.xyz))
            return false;
        float entry;
        if (!GpuCollisionBounds(localOrigin, localDirection, -shape.data0.xyz,
            shape.data0.xyz, 3.402823466e+38, entry) || entry < 0.0)
            return false;
        distance = length(localDirection * entry);
        return distance < limit;
    }
    if (shape.type == 2)
        return GpuCollisionSphere(origin, direction, shape.data0, limit, true, distance);
    return GpuCollisionCylinder(origin, direction, shape.data0, shape.data1, limit, distance);
}

bool GpuParticleRayPick(float3 origin, float3 direction, float limit, bool dynamicCollision, bool hitOnly,
    out float3 normal, out uint hitSource)
{
    bool hit = false;
    normal = float3(0.0, 1.0, 0.0);
    hitSource = 0;
    uint nodeIndex = 0;
    uint staticTriangle = 0xffffffff;
    uint4 counts = g_GpuCollisionCounts[0];
    while (nodeIndex < counts.x)
    {
        GpuPapiCollisionNode node = g_GpuCollisionNodes[nodeIndex];
        float entry;
        if (!GpuCollisionBounds(origin, direction, node.minimum, node.maximum, limit, entry))
        {
            nodeIndex = node.escape;
            continue;
        }
        for (uint offset = 0; offset < node.count; ++offset)
        {
            GpuPapiCollisionTriangle triangle = g_GpuCollisionStaticTriangles[node.first + offset];
            float distance;
            if (GpuCollisionTriangle(origin, direction, triangle, limit, true, distance))
            {
                if (hitOnly)
                {
                    hitSource = 1;
                    return true;
                }
                limit = distance;
                staticTriangle = node.first + offset;
                hitSource = 1;
                hit = true;
            }
        }
        ++nodeIndex;
    }
    for (uint objectIndex = 0; dynamicCollision && objectIndex < counts.y; ++objectIndex)
    {
        if ((objectIndex & 15u) == 0)
        {
            GpuPapiCollisionNode block = g_GpuCollisionBlocks[objectIndex >> 4];
            float3 relativeBounds = max(abs(origin - block.minimum), abs(origin - block.maximum));
            float padding = max(relativeBounds.x, max(relativeBounds.y, relativeBounds.z)) * 0.002;
            float blockEntry;
            if (!GpuCollisionBounds(origin, direction, block.minimum - padding, block.maximum + padding,
                max(limit, 0.0), blockEntry))
            {
                objectIndex = block.first + block.count - 1;
                continue;
            }
        }
        GpuPapiCollisionObject object = g_GpuCollisionObjects[objectIndex];
        float distance;
        if (!GpuCollisionSphere(origin, direction, object.spatialSphere, limit, false, distance)
            || !GpuCollisionSphere(origin, direction, object.sphere, limit, false, distance))
            continue;
        uint nearestShape = 0xffffffff;
        float nearestDistance = limit;
        for (uint offset = 0; offset < object.count; ++offset)
        {
            uint shapeIndex = object.first + offset;
            if (GpuCollisionShape(origin, direction, g_GpuCollisionShapes[shapeIndex], nearestDistance, distance))
            {
                nearestDistance = distance;
                nearestShape = shapeIndex;
            }
        }
        if (nearestShape == 0xffffffff)
            continue;
        if (object.mesh != 0)
        {
            GpuPapiCollisionShape shape = g_GpuCollisionShapes[nearestShape];
            float3 localOrigin = GpuCollisionTransform(origin, 1.0, object.row0, object.row1, object.row2);
            float3 localDirection = GpuCollisionTransform(direction, 0.0, object.row0, object.row1, object.row2);
            bool meshHit = false;
            for (uint triangleIndex = 0; triangleIndex < shape.triangleCount; ++triangleIndex)
            {
                if (GpuCollisionTriangle(localOrigin, localDirection,
                    g_GpuCollisionDynamicTriangles[shape.triangleFirst + triangleIndex], limit, false, distance))
                {
                    nearestDistance = distance;
                    meshHit = true;
                    break;
                }
            }
            if (!meshHit)
                continue;
        }
        if (hitOnly)
        {
            hitSource = 2;
            return true;
        }
        limit = nearestDistance;
        staticTriangle = 0xffffffff;
        hitSource = 2;
        hit = true;
    }
    if (staticTriangle != 0xffffffff)
    {
        GpuPapiCollisionTriangle triangle = g_GpuCollisionStaticTriangles[staticTriangle];
        normal = normalize(cross(triangle.b.xyz - triangle.a.xyz, triangle.c.xyz - triangle.a.xyz));
    }
    return hit;
}

uint GpuParticleCollision(inout float3 position, float3 previousPosition, inout float3 velocity,
    float dt, uint definitionFlags, float friction, float resilience, float cutoff, inout bool dead)
{
    uint result = 0;
    for (uint iteration = 0; iteration != 2; ++iteration)
    {
        float3 segment = position - previousPosition;
        float distance = length(segment);
        if (distance < 0.00001)
        {
            position = previousPosition;
            return result;
        }
        float3 normal;
        uint hitSource;
        if (!GpuParticleRayPick(previousPosition, segment / distance, distance,
            (definitionFlags & (1u << 19)) != 0, (definitionFlags & (1u << 17)) != 0, normal, hitSource))
            return result;
        result = hitSource;
        if ((definitionFlags & (1u << 17)) != 0)
        {
            dead = true;
            return result;
        }
        float3 normalVelocity = normal * dot(velocity, normal);
        float3 tangentVelocity = velocity - normalVelocity;
        velocity = tangentVelocity * (dot(tangentVelocity, tangentVelocity) <= cutoff ? 1.0 : friction)
            - normalVelocity * resilience;
        position = previousPosition + velocity * dt;
    }
    return result;
}

#endif
