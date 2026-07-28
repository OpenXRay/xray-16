#include "bindless_common.h"

#define PARTITION_GROUP_SIZE 256

#define DRAW_ARGS_STRIDE 20
#define MAX_VARIANTS 32

cbuffer PartitionParams : register(b5)
{
    uint g_BinCapacity;
    uint g_VariantCount;
    uint g_Padding0;
    uint g_Padding1;
};

ByteAddressBuffer g_CompactCount : register(t0);
ByteAddressBuffer g_CompactDrawArgs : register(t1);
StructuredBuffer<uint> g_CompactBatchIndices : register(t2);
StructuredBuffer<uint> g_CompactMaterialIDs : register(t3);

RWStructuredBuffer<uint> g_VariantCounts : register(u0);
RWByteAddressBuffer g_ReorderedDrawArgs : register(u1);
RWStructuredBuffer<uint> g_ReorderedBatchIndices : register(u2);
RWStructuredBuffer<uint> g_ReorderedMaterialIDs : register(u3);

groupshared uint gs_variant[PARTITION_GROUP_SIZE];
groupshared uint gs_running[MAX_VARIANTS];

[numthreads(PARTITION_GROUP_SIZE, 1, 1)]
void main(uint3 gtID : SV_GroupThreadID)
{
    uint tid = gtID.x;
    uint compactedCount = g_CompactCount.Load(0);

    if (tid < MAX_VARIANTS)
        gs_running[tid] = 0;
    GroupMemoryBarrierWithGroupSync();

    uint numChunks = (compactedCount + PARTITION_GROUP_SIZE - 1) / PARTITION_GROUP_SIZE;
    for (uint c = 0; c < numChunks; c++)
    {
        uint i = c * PARTITION_GROUP_SIZE + tid;
        uint variant = 0xffffffff;
        uint matID = 0;
        if (i < compactedCount)
        {
            matID = g_CompactMaterialIDs[i];
            variant = g_Materials[matID].shaderVariant;
            if (variant >= g_VariantCount)
                variant = 0;
        }
        gs_variant[tid] = variant;
        GroupMemoryBarrierWithGroupSync();

        if (i < compactedCount)
        {
            uint rank = 0;
            for (uint j = 0; j < tid; j++)
                rank += (gs_variant[j] == variant) ? 1 : 0;

            uint writeIdx = gs_running[variant] + rank;
            if (writeIdx < g_BinCapacity)
            {
                uint outputIdx = variant * g_BinCapacity + writeIdx;
                uint srcOff = i * DRAW_ARGS_STRIDE;
                uint dstOff = outputIdx * DRAW_ARGS_STRIDE;
                g_ReorderedDrawArgs.Store(dstOff + 0,  g_CompactDrawArgs.Load(srcOff + 0));
                g_ReorderedDrawArgs.Store(dstOff + 4,  g_CompactDrawArgs.Load(srcOff + 4));
                g_ReorderedDrawArgs.Store(dstOff + 8,  g_CompactDrawArgs.Load(srcOff + 8));
                g_ReorderedDrawArgs.Store(dstOff + 12, g_CompactDrawArgs.Load(srcOff + 12));
                g_ReorderedDrawArgs.Store(dstOff + 16, outputIdx);

                g_ReorderedBatchIndices[outputIdx] = g_CompactBatchIndices[i];
                g_ReorderedMaterialIDs[outputIdx] = matID;
            }
        }
        GroupMemoryBarrierWithGroupSync();

        if (tid < g_VariantCount)
        {
            uint cnt = 0;
            for (uint j = 0; j < PARTITION_GROUP_SIZE; j++)
                cnt += (gs_variant[j] == tid) ? 1 : 0;
            gs_running[tid] += cnt;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (tid < g_VariantCount)
        g_VariantCounts[tid] = min(gs_running[tid], g_BinCapacity);
}
