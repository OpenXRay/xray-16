#define SM_5_0
#include "common.h"

cbuffer CompactParams : register(b5)
{
    uint g_BatchCount;
    uint3 g_Padding;
};

ByteAddressBuffer g_InputDrawArgs : register(t0);
StructuredBuffer<uint> g_InputMaterialIDs : register(t1);
StructuredBuffer<uint> g_Visibility : register(t2);

RWByteAddressBuffer g_OutputDrawArgs : register(u0);
RWStructuredBuffer<uint> g_VisibleBatchIndices : register(u1);
RWByteAddressBuffer g_VisibleCount : register(u2);
RWStructuredBuffer<uint> g_OutputMaterialIDs : register(u3);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint batchIdx = dtID.x;

    if (batchIdx >= g_BatchCount)
        return;

    uint visible = g_Visibility[batchIdx];

    if (visible > 0)
    {
        uint byteOffset = batchIdx * 20;
        uint indexCount = g_InputDrawArgs.Load(byteOffset);
        uint instanceCount = g_InputDrawArgs.Load(byteOffset + 4);
        uint startIndex = g_InputDrawArgs.Load(byteOffset + 8);
        int baseVertex = asint(g_InputDrawArgs.Load(byteOffset + 12));

        uint outputIdx;
        g_VisibleCount.InterlockedAdd(0, 1, outputIdx);

        uint outOffset = outputIdx * 20;
        g_OutputDrawArgs.Store(outOffset + 0, indexCount);
        g_OutputDrawArgs.Store(outOffset + 4, instanceCount);
        g_OutputDrawArgs.Store(outOffset + 8, startIndex);
        g_OutputDrawArgs.Store(outOffset + 12, asuint(baseVertex));
        g_OutputDrawArgs.Store(outOffset + 16, outputIdx);

        g_VisibleBatchIndices[outputIdx] = batchIdx;
        g_OutputMaterialIDs[outputIdx] = g_InputMaterialIDs[batchIdx];
    }
}
