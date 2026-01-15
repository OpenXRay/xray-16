// terrain_apply_visibility.cs
// Copies visibility buffer → instanceCount in terrain draw args
// This allows terrain to use the same visibility buffer pattern as regular geometry
// without needing full compaction
#define SM_5_0
#include "common.h"

cbuffer ApplyVisibilityParams : register(b5)
{
    uint g_ObjectCount;
    uint g_FrameId;
    uint2 g_Padding;
};

// Input: Visibility buffer from cull pass (frame stamp)
StructuredBuffer<uint> g_Visibility : register(t0);

// Output: Draw args buffer (we write instanceCount field)
// Layout: 20 bytes per entry, instanceCount at offset 4
RWByteAddressBuffer g_DrawArgs : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_ObjectCount)
        return;

    // Read visibility (frame stamp match = visible)
    uint visible = (g_Visibility[idx] == g_FrameId) ? 1 : 0;

    // Write to instanceCount field of draw args
    // DrawIndexedIndirectArguments layout (20 bytes):
    //   offset 0:  indexCountPerInstance
    //   offset 4:  instanceCount <- we write here
    //   offset 8:  startIndexLocation
    //   offset 12: baseVertexLocation
    //   offset 16: startInstanceLocation
    g_DrawArgs.Store(idx * 20 + 4, visible);
}
