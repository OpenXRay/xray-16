// smoke_trail_emit.cs
// Emit: place points at muzzle position with upward velocity, constant width.

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

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= g_EmitCount)
        return;

    float3 prevPos = float3(g_PrevPosX, g_PrevPosY, g_PrevPosZ);
    float3 currPos = float3(g_CurrPosX, g_CurrPosY, g_CurrPosZ);

    // Distribute along prev→curr muzzle segment
    float t = (g_EmitCount > 1u)
        ? ((float(dtid.x) + 0.5f) / float(g_EmitCount))
        : 0.5f;
    float3 emitPos = lerp(prevPos, currPos, t);

    // Atomic ring buffer write
    uint oldHead;
    g_StateBuffer.InterlockedAdd(0, 1, oldHead);
    uint slot = oldHead % g_MaxPoints;

    uint spawnId;
    g_StateBuffer.InterlockedAdd(4, 1, spawnId);

    SmokeSimPoint p;
    p.px = emitPos.x;
    p.py = emitPos.y;
    p.pz = emitPos.z;
    p.lifetime = g_BaseLifetime;

    p.vx = 0.0f;
    p.vy = 0.18f;
    p.vz = 0.0f;
    p.age = 0.0f;

    p.width = 0.001f;
    p.seedF = 0.0f;
    p.pad0 = 0.0f;
    p.pad1 = 0.0f;

    g_SimBuffer[slot] = p;
}
