// smoke_trail_compact.cs
// Compact: collect live sim points into TrailControlPoint array for trail.vs.
// Parallel read to groupshared, thread 0 compacts + computes directions + writes draw args.
// Turbulence params passed through CB for trail.vs (not used here).

#include "smoke_common.h"

cbuffer SmokeCompactCB : register(b5)
{
    // Row 0
    uint  g_MaxPoints;
    uint  g_Subdivisions;
    float g_MaxWidth;
    float g_TurbAmount;

    // Row 1
    float g_TurbFrequency;
    float g_TurbEvolution;
    float g_SphereRadius;
    float _pad0;

    // Row 2
    float g_SphereCenterX;
    float g_SphereCenterY;
    float g_SphereCenterZ;
    float _pad1;
};

RWStructuredBuffer<TrailControlPoint>   g_CompactBuffer : register(u0);
RWByteAddressBuffer                     g_StateBuffer   : register(u1);
RWByteAddressBuffer                     g_DrawArgs      : register(u2);
RWStructuredBuffer<SmokeSimPoint>       g_SimBuffer     : register(u3);

#define MAX_PTS 256

groupshared float3 s_pos[MAX_PTS];
groupshared float  s_ageNorm[MAX_PTS];
groupshared float  s_width[MAX_PTS];
groupshared bool   s_alive[MAX_PTS];
groupshared uint   s_ringCount;
groupshared float  s_compactWidth[MAX_PTS];

[numthreads(MAX_PTS, 1, 1)]
void main(uint gtid : SV_GroupIndex)
{
    if (gtid == 0)
    {
        uint totalSpawned = g_StateBuffer.Load(4);
        s_ringCount = min(totalSpawned, g_MaxPoints);
    }
    GroupMemoryBarrierWithGroupSync();

    uint ringCount = s_ringCount;

    // Parallel phase: each thread reads one sim point
    s_alive[gtid] = false;

    if (gtid < ringCount)
    {
        uint totalSpawned = g_StateBuffer.Load(4);
        uint oldestSpawn = (totalSpawned > ringCount) ? (totalSpawned - ringCount) : 0;
        uint slot = (oldestSpawn + gtid) % g_MaxPoints;

        SmokeSimPoint p = g_SimBuffer[slot];
        if (p.lifetime > 0.0f && p.age < p.lifetime)
        {
            float3 basePos = float3(p.px, p.py, p.pz);
            s_alive[gtid]   = true;
            s_pos[gtid]     = basePos;
            s_ageNorm[gtid] = saturate(p.age / p.lifetime);
            s_width[gtid]   = p.width;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    if (gtid != 0)
        return;

    // Thread 0: compact live points
    uint liveCount = 0;
    float totalDist = 0.0f;
    float3 prevPos = float3(0, 0, 0);

    for (uint i = 0; i < ringCount; i++)
    {
        if (!s_alive[i])
            continue;

        float3 pos = s_pos[i];

        if (liveCount > 0)
            totalDist += length(pos - prevPos);

        TrailControlPoint cp;
        cp.posX = pos.x;
        cp.posY = pos.y;
        cp.posZ = pos.z;
        cp.dirX = 0;
        cp.dirY = 0;
        cp.dirZ = 0;
        cp.ageNorm = s_ageNorm[i];
        cp.cumDist = totalDist;

        g_CompactBuffer[liveCount] = cp;
        s_compactWidth[liveCount] = s_width[i];
        prevPos = pos;
        liveCount++;
    }

    // Direction pass: tangent from neighbors → cross with up → scale to halfWidth
    for (uint k = 0; k < liveCount; k++)
    {
        float3 curr = float3(g_CompactBuffer[k].posX, g_CompactBuffer[k].posY, g_CompactBuffer[k].posZ);
        float3 tangent;

        if (liveCount < 2)
            tangent = float3(0, 1, 0);
        else if (k == 0)
            tangent = float3(g_CompactBuffer[1].posX, g_CompactBuffer[1].posY, g_CompactBuffer[1].posZ) - curr;
        else if (k == liveCount - 1)
            tangent = curr - float3(g_CompactBuffer[k-1].posX, g_CompactBuffer[k-1].posY, g_CompactBuffer[k-1].posZ);
        else
            tangent = float3(g_CompactBuffer[k+1].posX, g_CompactBuffer[k+1].posY, g_CompactBuffer[k+1].posZ)
                    - float3(g_CompactBuffer[k-1].posX, g_CompactBuffer[k-1].posY, g_CompactBuffer[k-1].posZ);

        float tLen = length(tangent);
        tangent = (tLen > 0.0001f) ? tangent / tLen : float3(0, 1, 0);

        float3 up = (abs(tangent.y) < 0.9f) ? float3(0, 1, 0) : float3(1, 0, 0);
        float3 dir = normalize(cross(tangent, up));

        float hw = s_compactWidth[k];

        g_CompactBuffer[k].dirX = dir.x * hw;
        g_CompactBuffer[k].dirY = dir.y * hw;
        g_CompactBuffer[k].dirZ = dir.z * hw;
    }

    // State buffer
    g_StateBuffer.Store(8, liveCount);
    g_StateBuffer.Store(12, asuint(totalDist));

    // Draw args (non-indexed) — no subdivision, 1 quad per segment
    uint segments = (liveCount >= 2) ? (liveCount - 1) : 0;
    uint vertexCount = segments * 6;

    g_DrawArgs.Store(0,  vertexCount);
    g_DrawArgs.Store(4,  128);   // instances, offset in VS via SV_InstanceID
    g_DrawArgs.Store(8,  0);
    g_DrawArgs.Store(12, 0);
}
