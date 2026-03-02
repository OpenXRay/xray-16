#include "smoke_common.h"

cbuffer SmokeCompactCB : register(b5)
{
    uint  g_MaxPoints;
    uint  g_Subdivisions;
    float g_MaxWidth;
    float g_Pad0;
};

RWStructuredBuffer<TrailControlPoint>   g_CompactBuffer : register(u0);
RWByteAddressBuffer                     g_StateBuffer   : register(u1);
RWByteAddressBuffer                     g_DrawArgs      : register(u2);
RWStructuredBuffer<SmokeSimPoint>       g_SimBuffer     : register(u3);

#define MAX_PTS 256

struct SharedPoint
{
    float3 pos;
    float3 vel;
    float  ageNorm;
    float  halfWidth;
    bool   alive;
};

groupshared SharedPoint s_points[MAX_PTS];
groupshared uint        s_ringCount;

groupshared float3      s_rawDir[MAX_PTS];

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

    SharedPoint sp;
    sp.alive = false;
    sp.pos = float3(0, 0, 0);
    sp.vel = float3(0, 0, 0);
    sp.ageNorm = 0;
    sp.halfWidth = 0;

    if (gtid < ringCount)
    {
        uint totalSpawned = g_StateBuffer.Load(4);
        uint oldestSpawn = (totalSpawned > ringCount) ? (totalSpawned - ringCount) : 0;
        uint spawnId = oldestSpawn + gtid;
        uint slot = spawnId % g_MaxPoints;

        SmokeSimPoint p = g_SimBuffer[slot];
        if (p.lifetime > 0.0f && p.age < p.lifetime)
        {
            float ageNorm = saturate(p.age / p.lifetime);
            float initial  = smoothstep(0.0f, 0.15f, ageNorm);
            float spread   = 1.0f + smoothstep(0.5f, 1.0f, ageNorm);

            sp.alive = true;
            sp.pos = float3(p.px, p.py, p.pz);
            sp.vel = float3(p.vx, p.vy, p.vz);
            sp.ageNorm = ageNorm;
            sp.halfWidth = max(0.0008f, g_MaxWidth * initial * spread * 0.70f);
        }
    }
    s_points[gtid] = sp;
    GroupMemoryBarrierWithGroupSync();

    if (gtid != 0)
        return;

    uint liveCount = 0;
    float totalDist = 0.0f;
    float3 prevPos = float3(0, 0, 0);

    for (uint i = 0; i < ringCount; i++)
    {
        SharedPoint pt = s_points[i];
        if (!pt.alive)
            continue;

        float vLen = length(pt.vel);
        float3 tangent = (vLen > 0.0001f) ? pt.vel / vLen : float3(0, 0, 1);
        float3 up = (abs(tangent.y) < 0.9f) ? float3(0, 1, 0) : float3(1, 0, 0);
        float3 dir = normalize(cross(tangent, up));

        if (liveCount > 0)
            totalDist += length(pt.pos - prevPos);

        s_rawDir[liveCount] = dir * pt.halfWidth;

        TrailControlPoint cp;
        cp.posX = pt.pos.x;
        cp.posY = pt.pos.y;
        cp.posZ = pt.pos.z;
        cp.dirX = 0;
        cp.dirY = 0;
        cp.dirZ = 0;
        cp.ageNorm = pt.ageNorm;
        cp.cumDist = totalDist;

        g_CompactBuffer[liveCount] = cp;
        prevPos = pt.pos;
        liveCount++;
    }

    static const int R = 4;
    for (uint k = 0; k < liveCount; k++)
    {
        float3 sum = float3(0, 0, 0);
        int lo = max(0, (int)k - R);
        int hi = min((int)liveCount - 1, (int)k + R);
        for (int n = lo; n <= hi; n++)
            sum += s_rawDir[n];
        sum /= (float)(hi - lo + 1);

        g_CompactBuffer[k].dirX = sum.x;
        g_CompactBuffer[k].dirY = sum.y;
        g_CompactBuffer[k].dirZ = sum.z;
    }

    g_StateBuffer.Store(8, liveCount);
    g_StateBuffer.Store(12, asuint(totalDist));

    uint smoothCount = (liveCount >= 2)
        ? ((liveCount - 1) * g_Subdivisions + 1)
        : 0;
    uint vertexCount = (smoothCount >= 2) ? ((smoothCount - 1) * 6) : 0;

    g_DrawArgs.Store(0,  vertexCount);
    g_DrawArgs.Store(4,  1);
    g_DrawArgs.Store(8,  0);
    g_DrawArgs.Store(12, 0);
}
