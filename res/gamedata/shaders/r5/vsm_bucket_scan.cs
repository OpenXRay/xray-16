#define SM_5_0
#include "common.h"
#include "vsm_common.h"

cbuffer VsmBucketScanParams : register(b5)
{
    uint g_Level;
    uint g_ItemCap;
    uint2 g_ScanPad;
};

RWStructuredBuffer<uint> g_BucketCount : register(u0);
RWStructuredBuffer<uint> g_BucketStart : register(u1);
RWStructuredBuffer<uint> g_BucketEnd : register(u2);
RWStructuredBuffer<uint> g_BucketCursor : register(u3);

#define SCAN_THREADS 256
#define SCAN_PER_THREAD 9

groupshared uint gs_sum[SCAN_THREADS];

[numthreads(SCAN_THREADS, 1, 1)]
void main(uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    uint baseB = g_Level * uint(VSM_BUCKETS_PER_LVL);
    uint local[SCAN_PER_THREAD];
    uint sum = 0u;
    for (uint i = 0u; i < uint(SCAN_PER_THREAD); ++i)
    {
        uint b = t * uint(SCAN_PER_THREAD) + i;
        uint c = g_BucketCount[baseB + b];
        local[i] = c;
        sum += c;
    }
    gs_sum[t] = sum;
    GroupMemoryBarrierWithGroupSync();
    for (uint off = 1u; off < uint(SCAN_THREADS); off <<= 1u)
    {
        uint v = (t >= off) ? gs_sum[t - off] : 0u;
        GroupMemoryBarrierWithGroupSync();
        gs_sum[t] += v;
        GroupMemoryBarrierWithGroupSync();
    }
    uint run = gs_sum[t] - sum;
    for (uint j = 0u; j < uint(SCAN_PER_THREAD); ++j)
    {
        uint b = baseB + t * uint(SCAN_PER_THREAD) + j;
        uint s = min(run, g_ItemCap);
        uint e = min(run + local[j], g_ItemCap);
        g_BucketStart[b] = s;
        g_BucketEnd[b] = e;
        g_BucketCursor[b] = run;
        g_BucketCount[b] = 0u;
        run += local[j];
    }
}
