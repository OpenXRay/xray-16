// smoke_trail_emit.cs
// Emit compute shader for GPU smoke trail.
// Writes new SmokeSimPoint entries into ring buffer.

#include "smoke_common.h"

cbuffer SmokeEmitCB : register(b5)
{
    float g_PrevPosX, g_PrevPosY, g_PrevPosZ, g_CurrPosX;
    float g_CurrPosY, g_CurrPosZ, g_EmitDirX, g_EmitDirY;
    float g_EmitDirZ, g_BaseLifetime, g_LifetimeVariance, g_MaxWidth;
    uint  g_EmitCount, g_MaxPoints;
    float g_FrameSeed, g_Pad0;
};

RWStructuredBuffer<SmokeSimPoint> g_SimBuffer   : register(u0);
RWByteAddressBuffer               g_StateBuffer : register(u1);

uint pcg(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float randFloat(inout uint seed)
{
    seed = pcg(seed);
    return float(seed) / 4294967296.0f;
}

float3 randInSphere(inout uint seed)
{
    [unroll] for (int i = 0; i < 8; i++)
    {
        float3 v = float3(
            randFloat(seed) * 2.0f - 1.0f,
            randFloat(seed) * 2.0f - 1.0f,
            randFloat(seed) * 2.0f - 1.0f
        );
        if (dot(v, v) <= 1.0f)
            return v;
    }
    return float3(0, 1, 0);
}

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= g_EmitCount)
        return;

    uint seed = pcg(dtid.x + asuint(g_FrameSeed) * 1664525u);

    float3 prevPos = float3(g_PrevPosX, g_PrevPosY, g_PrevPosZ);
    float3 currPos = float3(g_CurrPosX, g_CurrPosY, g_CurrPosZ);
    float3 emitDir = float3(g_EmitDirX, g_EmitDirY, g_EmitDirZ);

    // Distribute points along prev→curr muzzle segment
    float t = (g_EmitCount > 1u)
        ? ((float(dtid.x) + 0.5f) / float(g_EmitCount))
        : 0.5f;

    float3 scatter = randInSphere(seed);
    float3 emitPos = lerp(prevPos, currPos, t) + scatter * 0.001f;

    // Atomic ring buffer write
    uint oldHead;
    g_StateBuffer.InterlockedAdd(0, 1, oldHead);
    uint slot = oldHead % g_MaxPoints;

    uint spawnId;
    g_StateBuffer.InterlockedAdd(4, 1, spawnId);

    float lifeVar = (randFloat(seed) * 2.0f - 1.0f) * g_LifetimeVariance;

    SmokeSimPoint p;
    p.px = emitPos.x;
    p.py = emitPos.y;
    p.pz = emitPos.z;
    p.lifetime = max(0.05f, g_BaseLifetime * (1.0f + lifeVar));

    // Minimal initial velocity — turbulence handles curling over time
    p.vx = emitDir.x * 0.02f + scatter.x * 0.003f;
    p.vy = 0.08f + scatter.y * 0.005f;
    p.vz = emitDir.z * 0.02f + scatter.z * 0.003f;
    p.age = 0.0f;

    p.width = g_MaxWidth;
    p.seedF = frac(float(spawnId) * 0.61803398875f);
    p.pad0 = 0.0f;
    p.pad1 = 0.0f;

    g_SimBuffer[slot] = p;
}
