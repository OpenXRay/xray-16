// batch_compact.cs
// Compacts visible batches into a contiguous list for multi-draw indirect
// Input: Draw args with instanceCount set by culling (0 = culled, 1 = visible)
// Output: Only visible batches in compact array + their original indices + material IDs
#define SM_5_0
#include "common.h"

cbuffer CompactParams : register(b5)
{
    uint g_BatchCount;
    uint3 g_Padding;
};

// Input draw args is a raw buffer (for D3D11 DrawIndexedIndirect compatibility)
// Each IndirectDrawArgs is 20 bytes (5 x uint)
ByteAddressBuffer g_InputDrawArgs : register(t0);
StructuredBuffer<uint> g_InputMaterialIDs : register(t1);  // Material ID per batch

RWByteAddressBuffer g_OutputDrawArgs : register(u0);       // Raw buffer for indirect args
RWStructuredBuffer<uint> g_VisibleBatchIndices : register(u1);
RWByteAddressBuffer g_VisibleCount : register(u2);
RWStructuredBuffer<uint> g_OutputMaterialIDs : register(u3);  // Material ID per visible batch

// IndirectDrawArgs layout (20 bytes):
// offset 0:  indexCountPerInstance (uint)
// offset 4:  instanceCount (uint)
// offset 8:  startIndexLocation (uint)
// offset 12: baseVertexLocation (int)
// offset 16: startInstanceLocation (uint)

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint batchIdx = dtID.x;

    if (batchIdx >= g_BatchCount)
        return;

    // Read draw args from raw buffer (20 bytes per entry)
    uint byteOffset = batchIdx * 20;
    uint indexCount = g_InputDrawArgs.Load(byteOffset);
    uint instanceCount = g_InputDrawArgs.Load(byteOffset + 4);
    uint startIndex = g_InputDrawArgs.Load(byteOffset + 8);
    int baseVertex = asint(g_InputDrawArgs.Load(byteOffset + 12));
    uint startInstance = g_InputDrawArgs.Load(byteOffset + 16);

    // Only compact visible batches (instanceCount > 0)
    if (instanceCount > 0)
    {
        uint outputIdx;
        g_VisibleCount.InterlockedAdd(0, 1, outputIdx);

        // Write to output raw buffer (20 bytes per entry)
        uint outOffset = outputIdx * 20;
        g_OutputDrawArgs.Store(outOffset + 0, indexCount);
        g_OutputDrawArgs.Store(outOffset + 4, instanceCount);
        g_OutputDrawArgs.Store(outOffset + 8, startIndex);
        g_OutputDrawArgs.Store(outOffset + 12, asuint(baseVertex));
        // CRITICAL: Set startInstanceLocation to outputIdx for multi-draw
        // With instanceCount=1, SV_InstanceID in VS will equal outputIdx (draw index)
        g_OutputDrawArgs.Store(outOffset + 16, outputIdx);

        // Copy batch index and material ID for bindless rendering
        g_VisibleBatchIndices[outputIdx] = batchIdx;
        g_OutputMaterialIDs[outputIdx] = g_InputMaterialIDs[batchIdx];
    }
}
