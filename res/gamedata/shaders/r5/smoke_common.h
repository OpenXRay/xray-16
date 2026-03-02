// smoke_common.h
// Shared types and noise functions for GPU smoke simulation.
// Used by emit, simulate, and compact compute shaders.

#ifndef SMOKE_COMMON_H
#define SMOKE_COMMON_H

// ─────────────────────────────────────────────────────────
//  GPU SIMULATION POINT (must match SmokeSimPoint in C++)
//  48 bytes
// ─────────────────────────────────────────────────────────
struct SmokeSimPoint
{
    float px, py, pz;
    float lifetime;
    float vx, vy, vz;
    float age;
    float width;
    float seedF;
    float pad0;
    float pad1;
};

// ─────────────────────────────────────────────────────────
//  GPU TRAIL CONTROL POINT (must match GPUTrailControlPoint in C++)
//  32 bytes — output of compact CS, input to trail.vs at t10
//  Direction is PRE-SCALED to half-width (Stride parity).
// ─────────────────────────────────────────────────────────
struct TrailControlPoint
{
    float posX, posY, posZ;
    float dirX, dirY, dirZ;   // pre-scaled width direction (world-space, magnitude = halfWidth)
    float ageNorm;             // 0 = youngest (head), 1 = oldest (tail)
    float cumDist;             // cumulative distance from head
};

// ─────────────────────────────────────────────────────────
//  HASH / NOISE
// ─────────────────────────────────────────────────────────

float hash3(float3 p)
{
    p = frac(p * float3(443.8975f, 397.2973f, 491.1871f));
    p += dot(p.zxy, p.yxz + 19.19f);
    return frac(p.x * p.y * p.z);
}

float noise3(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    float3 u = f * f * (3.0f - 2.0f * f);

    float v000 = hash3(i + float3(0,0,0));
    float v100 = hash3(i + float3(1,0,0));
    float v010 = hash3(i + float3(0,1,0));
    float v110 = hash3(i + float3(1,1,0));
    float v001 = hash3(i + float3(0,0,1));
    float v101 = hash3(i + float3(1,0,1));
    float v011 = hash3(i + float3(0,1,1));
    float v111 = hash3(i + float3(1,1,1));

    return lerp(
        lerp(lerp(v000, v100, u.x), lerp(v010, v110, u.x), u.y),
        lerp(lerp(v001, v101, u.x), lerp(v011, v111, u.x), u.y),
        u.z);
}

float fbm3(float3 p)
{
    float v = 0.0f, a = 0.5f;
    [unroll] for (int i = 0; i < 3; i++, p *= 2.0f, a *= 0.5f)
        v += a * noise3(p);
    return v;
}

float3 curlNoise(float3 p)
{
    const float eps = 0.05f;
    float3 dx = float3(eps, 0, 0);
    float3 dy = float3(0, eps, 0);
    float3 dz = float3(0, 0, eps);

    float3 offset_x = float3(0.0f, 0.0f, 0.0f);
    float3 offset_y = float3(31.416f, 47.853f, 12.679f);
    float3 offset_z = float3(73.156f, 19.847f, 58.312f);

    float3 curlV;
    curlV.x = (fbm3(p + offset_z + dy) - fbm3(p + offset_z - dy))
            - (fbm3(p + offset_y + dz) - fbm3(p + offset_y - dz));
    curlV.y = (fbm3(p + offset_x + dz) - fbm3(p + offset_x - dz))
            - (fbm3(p + offset_z + dx) - fbm3(p + offset_z - dx));
    curlV.z = (fbm3(p + offset_y + dx) - fbm3(p + offset_y - dx))
            - (fbm3(p + offset_x + dy) - fbm3(p + offset_x - dy));
    return curlV / (2.0f * eps);
}

#endif // SMOKE_COMMON_H
