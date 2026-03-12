#include "common.h"
#include "cull_utils.h"
#include "shared/clustered_lighting.h"

cbuffer LightHiZCullParams : register(b5)
{
    float4x4 cb_prevViewProj;
    float4x4 cb_curViewProj;
    float4 cb_cameraPos;
    uint cb_numLights;
    uint cb_hizWidth;
    uint cb_hizHeight;
    uint cb_hizMipLevels;
};

StructuredBuffer<GPULightData> g_Lights : register(t0);
Texture2D<float> g_HiZPyramid : register(t1);

RWStructuredBuffer<uint> g_VisibleLightIndices : register(u0);
RWByteAddressBuffer g_VisibleLightCount : register(u1);

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint lightIdx = dtid.x;
    if (lightIdx >= cb_numLights)
        return;

    GPULightData ld = g_Lights[lightIdx];
    float3 lightPos = ld.positionAndInvRangeSq.xyz;
    float range = ld.colorAndRange.w;

    float3 toLight = lightPos - cb_cameraPos.xyz;
    if (dot(toLight, toLight) <= range * range)
    {
        uint idx;
        g_VisibleLightCount.InterlockedAdd(0, 1, idx);
        if (idx < 1024)
            g_VisibleLightIndices[idx] = lightIdx;
        return;
    }

    bool visible = HiZTestSphereTemporal(
        lightPos, range, cb_cameraPos.xyz,
        cb_curViewProj, cb_prevViewProj,
        g_HiZPyramid, smp_nofilter,
        cb_hizWidth, cb_hizHeight, cb_hizMipLevels);

    if (visible)
    {
        uint idx;
        g_VisibleLightCount.InterlockedAdd(0, 1, idx);
        if (idx < 1024)
            g_VisibleLightIndices[idx] = lightIdx;
    }
}
