#define SM_5_0
#include "common.h"

cbuffer CullParams : register(b5)
{
    float4x4 g_ViewProj;
    float4x4 g_PrevViewProj;
    float3 g_CameraPos;
    float g_MaxDistance;
    float4 g_FrustumPlanes[6];
    uint g_ObjectCount;
    uint g_HiZWidth;
    uint g_HiZHeight;
    uint g_HiZMipLevels;
    uint g_FrameId;
    uint3 g_Padding;
};

StructuredBuffer<uint> g_Visibility : register(t0);

RWByteAddressBuffer g_DrawArgs : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint objectIdx = dtID.x;
    if (objectIdx >= g_ObjectCount)
        return;

    uint visible = (g_Visibility[objectIdx] == g_FrameId) ? 1u : 0u;
    g_DrawArgs.Store(objectIdx * 20 + 4, visible);
}
