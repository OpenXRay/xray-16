#define SM_5_0
#include "common.h"

#define COMPACT_GROUP_SIZE 256

cbuffer CompactParams : register(b5)
{
    uint g_BatchCount;
    uint g_FrameId;
    uint2 g_Padding;
};

ByteAddressBuffer g_InputDrawArgs : register(t0);
StructuredBuffer<uint> g_InputMaterialIDs : register(t1);
StructuredBuffer<uint> g_Visibility : register(t2);
StructuredBuffer<uint> g_LocalPrefix : register(t3);
StructuredBuffer<uint> g_GroupOffsets : register(t4);

RWByteAddressBuffer g_OutputDrawArgs : register(u0);
RWStructuredBuffer<uint> g_VisibleBatchIndices : register(u1);
RWStructuredBuffer<uint> g_OutputMaterialIDs : register(u2);

[numthreads(COMPACT_GROUP_SIZE, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint batchIdx = dtID.x;

    if (batchIdx >= g_BatchCount)
        return;

    if (g_Visibility[batchIdx] != g_FrameId)
        return;

    uint groupId = batchIdx / COMPACT_GROUP_SIZE;
    uint outputIdx = g_GroupOffsets[groupId] + g_LocalPrefix[batchIdx];

    uint byteOffset = batchIdx * 20;
    uint indexCount = g_InputDrawArgs.Load(byteOffset);
    uint instanceCount = g_InputDrawArgs.Load(byteOffset + 4);
    uint startIndex = g_InputDrawArgs.Load(byteOffset + 8);
    int baseVertex = asint(g_InputDrawArgs.Load(byteOffset + 12));

    uint outOffset = outputIdx * 20;
    g_OutputDrawArgs.Store(outOffset + 0, indexCount);
    g_OutputDrawArgs.Store(outOffset + 4, instanceCount);
    g_OutputDrawArgs.Store(outOffset + 8, startIndex);
    g_OutputDrawArgs.Store(outOffset + 12, asuint(baseVertex));
    g_OutputDrawArgs.Store(outOffset + 16, outputIdx);

    g_VisibleBatchIndices[outputIdx] = batchIdx;
    g_OutputMaterialIDs[outputIdx] = g_InputMaterialIDs[batchIdx];
}
