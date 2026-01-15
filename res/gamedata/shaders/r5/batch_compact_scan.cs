// batch_compact_scan.cs
// Prefix scan over per-group counts to produce group offsets and total visible count.
#define SM_5_0
#include "common.h"

#define COMPACT_GROUP_SIZE 256

cbuffer CompactParams : register(b5)
{
    uint g_BatchCount;
    uint g_FrameId;
    uint2 g_Padding;
};

StructuredBuffer<uint> g_GroupCounts : register(t0);

RWStructuredBuffer<uint> g_GroupOffsets : register(u0);
RWByteAddressBuffer g_VisibleCount : register(u1);

groupshared uint s_Scan[COMPACT_GROUP_SIZE];

[numthreads(COMPACT_GROUP_SIZE, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint groupCount = (g_BatchCount + COMPACT_GROUP_SIZE - 1) / COMPACT_GROUP_SIZE;
    uint idx = dtID.x;

    uint value = (idx < groupCount) ? g_GroupCounts[idx] : 0;
    s_Scan[idx] = value;
    GroupMemoryBarrierWithGroupSync();

    // Inclusive scan across the group counts
    for (uint offset = 1; offset < COMPACT_GROUP_SIZE; offset <<= 1)
    {
        uint addValue = 0;
        if (idx >= offset)
            addValue = s_Scan[idx - offset];
        GroupMemoryBarrierWithGroupSync();
        s_Scan[idx] += addValue;
        GroupMemoryBarrierWithGroupSync();
    }

    if (idx < groupCount)
    {
        uint exclusive = (idx == 0) ? 0 : s_Scan[idx - 1];
        g_GroupOffsets[idx] = exclusive;
    }

    if (idx + 1 == groupCount)
    {
        g_VisibleCount.Store(0, s_Scan[idx]);
    }
}
