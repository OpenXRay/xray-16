#define SM_6_0
#define CLUSTERED_LIGHTING_FORWARD
#define SUN_SHADOW_RECEIVER
#include "common.h"
#include "bindless_common.h"

Texture2D<float> g_GBufferDepth : register(t30);
Texture2D<float4> g_GBufferNormal : register(t31);
Texture2D<float4> g_GBufferBaseColor : register(t32);
RWTexture2D<float4> g_SceneColor : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint2 p = dtid.xy;
    uint width, height;
    g_GBufferDepth.GetDimensions(width, height);
    if (p.x >= width || p.y >= height)
        return;

    float4 n = g_GBufferNormal[p];
    if (dot(n.xyz, n.xyz) < 0.25)
        return;

    float depth = g_GBufferDepth[p];

    float4 bc = g_GBufferBaseColor[p];
    float4 c = g_SceneColor[p];
    float2 pixel = float2(p) + 0.5;
    float3 worldPos = reconstruct_world_pos(pixel, depth);
    float3 lit = c.rgb + shade_pbr(bc.rgb, normalize(n.xyz), worldPos, bc.a, abs(n.w), c.a, float4(pixel, depth, 1.0), -1.0);
    if (dev_param_3.y > 0.5)
        lit = SunShadowDebugColor(lit, worldPos);
    g_SceneColor[p] = float4(lit, 1.0);
}
