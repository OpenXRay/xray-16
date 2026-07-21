#include "common.h"

cbuffer MotionVectorParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevViewProj;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
};

Texture2D<float> t_Depth : register(t0);
RWTexture2D<float2> u_MotionVectors : register(u0);

float3 ReconstructWorldPos(uint2 pixel, float depth)
{
    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float4 clip = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip.y = -clip.y;
    float4 world = mul(g_InvViewProj, clip);
    return world.xyz / world.w;
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixel = dtid.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));

    if (depth <= 0.0) {
        u_MotionVectors[pixel] = float2(0, 0);
        return;
    }

    float3 worldPos = ReconstructWorldPos(pixel, depth);

    float4 prevClip = mul(g_PrevViewProj, float4(worldPos, 1.0));
    float2 prevNDC = prevClip.xy / prevClip.w;
    prevNDC.y = -prevNDC.y;
    float2 prevUV = prevNDC * 0.5 + 0.5;

    float2 currUV = (float2(pixel) + 0.5) * g_InvScreenSize;

    u_MotionVectors[pixel] = prevUV - currUV;
}
