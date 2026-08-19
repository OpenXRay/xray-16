#include "common.h"

cbuffer MotionVectorParams : register(b5) {
    float4x4 g_ViewProj;
    float4x4 g_PrevViewProj;
    float4x4 g_InvViewProj;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    float4 g_CameraPos;
    float4 g_PrevCameraPos;
    uint g_HasPrevCamera;
    float g_CurrJitterX;
    float g_CurrJitterY;
    float g_PrevJitterX;
    float g_PrevJitterY;
    float g_Pad0;
    float g_Pad1;
    float g_Pad2;
};

Texture2D<float> t_Depth : register(t0);
RWTexture2D<float4> u_MotionVectors : register(u0);

float2 ProjectToUv(float4x4 viewProj, float3 worldPos)
{
    float4 clip = mul(viewProj, float4(worldPos, 1.0));
    float2 ndc = clip.xy / max(abs(clip.w), 1e-5);
    ndc.y = -ndc.y;
    return ndc * 0.5 + 0.5;
}

float3 ReconstructWorldPos(uint2 pixel, float depth)
{
    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float4 clip = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip.y = -clip.y;
    float4 world = mul(g_InvViewProj, clip);
    return world.xyz / max(world.w, 1e-6);
}

float3 ReconstructFarWorld(float2 uv)
{
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 farH = mul(g_InvViewProj, float4(ndc, 0.0, 1.0));
    return farH.xyz / max(farH.w, 1e-6);
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixel = dtid.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float depth = t_Depth.Load(int3(pixel, 0));

    if (g_HasPrevCamera == 0) {
        u_MotionVectors[pixel] = 0;
        return;
    }

    if (depth <= 1e-7) {
        float3 farW = ReconstructFarWorld(uv);
        float3 dir = normalize(farW - g_CameraPos.xyz);
        float3 prevPt = g_PrevCameraPos.xyz + dir * 1e5;
        float2 currUnjit = ProjectToUv(g_ViewProj, prevPt);
        float2 prevUnjit = ProjectToUv(g_PrevViewProj, prevPt);
        float3 dualPt = prevPt + (g_CameraPos.xyz - g_PrevCameraPos.xyz);
        float2 dual = ProjectToUv(g_PrevViewProj, dualPt) - currUnjit;
        u_MotionVectors[pixel] = float4(prevUnjit - currUnjit, dual);
        return;
    }

    float3 worldPos = ReconstructWorldPos(pixel, depth);
    float2 currUnjit = ProjectToUv(g_ViewProj, worldPos);
    float2 prevUnjit = ProjectToUv(g_PrevViewProj, worldPos);
    float3 dualPos = worldPos + (g_CameraPos.xyz - g_PrevCameraPos.xyz);
    float2 dual = ProjectToUv(g_PrevViewProj, dualPos) - currUnjit;
    u_MotionVectors[pixel] = float4(prevUnjit - currUnjit, dual);
}
