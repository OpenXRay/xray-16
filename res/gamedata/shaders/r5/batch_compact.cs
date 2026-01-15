// batch_compact.cs
// Scatter pass for visible batches into compact arrays (no per-item atomics).
// Requires batch_compact_count.cs and batch_compact_scan.cs to run first.
// Input: Static draw args + visibility + prefix buffers
// Output: Visible batches in compact arrays + their original indices + material IDs
#define SM_5_0
#include "common.h"

#define COMPACT_GROUP_SIZE 256

cbuffer CompactParams : register(b5)
{
    uint g_BatchCount;
    uint g_FrameId;
    uint2 g_Padding;
};

// Input draw args is a raw buffer (STATIC - uploaded once at level load)
// Each IndirectDrawArgs is 20 bytes (5 x uint)
ByteAddressBuffer g_InputDrawArgs : register(t0);
StructuredBuffer<uint> g_InputMaterialIDs : register(t1);  // Material ID per batch
StructuredBuffer<uint> g_Visibility : register(t2);        // Visibility from cull pass (frame stamp)
StructuredBuffer<uint> g_LocalPrefix : register(t3);       // Prefix within group
StructuredBuffer<uint> g_GroupOffsets : register(t4);      // Prefix offset per group

RWByteAddressBuffer g_OutputDrawArgs : register(u0);       // Raw buffer for indirect args
RWStructuredBuffer<uint> g_VisibleBatchIndices : register(u1);
RWStructuredBuffer<uint> g_OutputMaterialIDs : register(u2);  // Material ID per visible batch

// IndirectDrawArgs layout (20 bytes):
// offset 0:  indexCountPerInstance (uint)
// offset 4:  instanceCount (uint)
// offset 8:  startIndexLocation (uint)
// offset 12: baseVertexLocation (int)
// offset 16: startInstanceLocation (uint)

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

    // Read static draw args from raw buffer (20 bytes per entry)
    uint byteOffset = batchIdx * 20;
    uint indexCount = g_InputDrawArgs.Load(byteOffset);
    uint instanceCount = g_InputDrawArgs.Load(byteOffset + 4);
    uint startIndex = g_InputDrawArgs.Load(byteOffset + 8);
    int baseVertex = asint(g_InputDrawArgs.Load(byteOffset + 12));

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
