// batch_compact_count.cs
// Per-group prefix scan for visible batches.
// Outputs local prefix per batch and group-visible counts.
#define SM_5_0
#include "common.h"

#define COMPACT_GROUP_SIZE 256

cbuffer CompactParams : register(b5)
{
    uint g_BatchCount;
    uint g_FrameId;
    uint2 g_Padding;
};

StructuredBuffer<uint> g_Visibility : register(t0);  // Visibility from cull pass (frame stamp)

RWStructuredBuffer<uint> g_LocalPrefix : register(u0);  // Prefix within group (visible only)
RWStructuredBuffer<uint> g_GroupCounts : register(u1);  // Visible count per group

groupshared uint s_Prefix[COMPACT_GROUP_SIZE];

[numthreads(COMPACT_GROUP_SIZE, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID, uint3 gtID : SV_GroupThreadID, uint3 gID : SV_GroupID)
{
    uint batchIdx = dtID.x;
    uint isVisible = 0;

    if (batchIdx < g_BatchCount)
        isVisible = (g_Visibility[batchIdx] == g_FrameId) ? 1 : 0;

    s_Prefix[gtID.x] = isVisible;
    GroupMemoryBarrierWithGroupSync();

    // Inclusive scan within the group
    for (uint offset = 1; offset < COMPACT_GROUP_SIZE; offset <<= 1)
    {
        uint addValue = 0;
        if (gtID.x >= offset)
            addValue = s_Prefix[gtID.x - offset];
        GroupMemoryBarrierWithGroupSync();
        s_Prefix[gtID.x] += addValue;
        GroupMemoryBarrierWithGroupSync();
    }

    if (batchIdx < g_BatchCount)
    {
        g_LocalPrefix[batchIdx] = (isVisible != 0) ? (s_Prefix[gtID.x] - 1) : 0;
    }

    if (gtID.x == COMPACT_GROUP_SIZE - 1)
    {
        g_GroupCounts[gID.x] = s_Prefix[gtID.x];
    }
}
