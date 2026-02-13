#include "bindless_common.h"

#define PARTITION_GROUP_SIZE 256

#define DRAW_ARGS_STRIDE 20

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

[numthreads(PARTITION_GROUP_SIZE, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint compactedCount = g_CompactCount.Load(0);
    if (dtID.x >= compactedCount)
        return;

    uint matID = g_CompactMaterialIDs[dtID.x];
    uint variant = g_Materials[matID].shaderVariant;
    if (variant >= g_VariantCount)
        variant = 0;

    uint writeIdx;
    InterlockedAdd(g_VariantCounts[variant], 1, writeIdx);
    if (writeIdx >= g_BinCapacity)
        return;

    uint outputIdx = variant * g_BinCapacity + writeIdx;

    uint srcOff = dtID.x * DRAW_ARGS_STRIDE;
    uint dstOff = outputIdx * DRAW_ARGS_STRIDE;
    g_ReorderedDrawArgs.Store(dstOff + 0,  g_CompactDrawArgs.Load(srcOff + 0));
    g_ReorderedDrawArgs.Store(dstOff + 4,  g_CompactDrawArgs.Load(srcOff + 4));
    g_ReorderedDrawArgs.Store(dstOff + 8,  g_CompactDrawArgs.Load(srcOff + 8));
    g_ReorderedDrawArgs.Store(dstOff + 12, g_CompactDrawArgs.Load(srcOff + 12));
    g_ReorderedDrawArgs.Store(dstOff + 16, outputIdx);

    g_ReorderedBatchIndices[outputIdx] = g_CompactBatchIndices[dtID.x];
    g_ReorderedMaterialIDs[outputIdx] = matID;
}
