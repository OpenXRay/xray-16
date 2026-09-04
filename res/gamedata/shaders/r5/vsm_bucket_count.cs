#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#include "visbuffer_common.h"

cbuffer VsmBucketParams : register(b5)
{
    uint g_EntryCount;
    uint g_Level;
    uint g_ItemBase;
    uint g_ItemCap;
    int4 g_Shift;
    float g_ErrK;
    float g_Pw;
    uint2 g_BucketPad;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t0);
RWStructuredBuffer<uint> g_BucketCount : register(u0);

bool bucketRect(uint idx, out int2 p0, out int2 p1)
{
    p0 = int2(0, 0);
    p1 = int2(-1, -1);
    ClusterEntry e = g_Entries[idx];
    if ((e.flags & 2u) == 0u)
    {
        float errB = vsm_level[g_Level].z / float(VSM_VIRTUAL_RES) * g_ErrK;
        if (e.selfError > errB || e.parentError <= errB)
            return false;
    }
    float2 lp = mul(vsm_view, float4(e.sphere.xyz, 1.0)).xy;
    float R = e.sphere.w;
    int2 lo = int2(floor((lp - float2(R, R)) / g_Pw)) + g_Shift.xy;
    int2 hi = int2(floor((lp + float2(R, R)) / g_Pw)) + g_Shift.xy;
    if (hi.x < 0 || hi.y < 0 || lo.x >= VSM_BUCKET_AXIS || lo.y >= VSM_BUCKET_AXIS)
        return false;
    p0 = clamp(lo, int2(0, 0), int2(VSM_BUCKET_AXIS - 1, VSM_BUCKET_AXIS - 1));
    p1 = clamp(hi, int2(0, 0), int2(VSM_BUCKET_AXIS - 1, VSM_BUCKET_AXIS - 1));
    return true;
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_EntryCount)
        return;
    int2 p0;
    int2 p1;
    if (!bucketRect(idx, p0, p1))
        return;
    uint baseB = g_Level * uint(VSM_BUCKETS_PER_LVL);
    for (int y = p0.y; y <= p1.y; ++y)
    for (int x = p0.x; x <= p1.x; ++x)
    {
        uint d;
        InterlockedAdd(g_BucketCount[baseB + uint(y * VSM_BUCKET_AXIS + x)], 1u, d);
    }
}
