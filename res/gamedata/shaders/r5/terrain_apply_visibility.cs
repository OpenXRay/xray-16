#define SM_5_0
#include "common.h"

cbuffer ApplyVisibilityParams : register(b5)
{
    uint g_ObjectCount;
    uint g_FrameId;
    uint2 g_Padding;
};

StructuredBuffer<uint> g_Visibility : register(t0);

RWByteAddressBuffer g_DrawArgs : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_ObjectCount)
        return;

    uint visible = (g_Visibility[idx] == g_FrameId) ? 1 : 0;

    g_DrawArgs.Store(idx * 20 + 4, visible);
}
