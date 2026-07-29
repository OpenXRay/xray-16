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
    uint3 g_Pad;
};

Texture2D<float> t_Depth : register(t0);
Texture2D<float4> t_WorldPos : register(t1);
RWTexture2D<float2> u_MotionVectors : register(u0);

float2 ProjectToUv(float4x4 viewProj, float3 worldPos)
{
    float4 clip = mul(viewProj, float4(worldPos, 1.0));
    float2 ndc = clip.xy / max(clip.w, 1e-5);
    ndc.y = -ndc.y;
    return ndc * 0.5 + 0.5;
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
    if (IsSkyDepth(depth))
    {
        if (g_HasPrevCamera == 0)
        {
            u_MotionVectors[pixel] = 0.0;
            return;
        }

        float3 farW = ReconstructFarWorld(uv);
        float3 dir = normalize(farW - g_CameraPos.xyz);
        float3 currPt = g_CameraPos.xyz + dir * 1e5;
        float3 prevPt = g_PrevCameraPos.xyz + dir * 1e5;
        float2 currUV = ProjectToUv(g_ViewProj, currPt);
        float2 prevUV = ProjectToUv(g_PrevViewProj, prevPt);
        u_MotionVectors[pixel] = prevUV - currUV;
        return;
    }

    float3 worldPos = t_WorldPos.Load(int3(pixel, 0)).xyz;
    float2 prevUV = ProjectToUv(g_PrevViewProj, worldPos);
    float2 currUV = ProjectToUv(g_ViewProj, worldPos);
    u_MotionVectors[pixel] = prevUV - currUV;
}
