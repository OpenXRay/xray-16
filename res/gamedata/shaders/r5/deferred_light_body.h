#ifndef DEFERRED_LIGHT_BODY_H
#define DEFERRED_LIGHT_BODY_H

#define SM_6_0
#if TILE_LIGHTS
#define CLUSTERED_LIGHTING_FORWARD
#define LOCAL_SHADOW_RECEIVER
#endif
#if TILE_SUN_MIXED
#define SUN_SHADOW_RECEIVER
#endif
#include "common.h"
#include "bindless_common.h"
#include "deferred_tiles.h"

Texture2D<float> g_GBufferDepth : register(t30);
Texture2D<float4> g_GBufferNormal : register(t31);
Texture2D<float4> g_GBufferBaseColor : register(t32);
StructuredBuffer<uint> g_TileList : register(t33);
RWTexture2D<float4> g_SceneColor : register(u0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
    uint tile = g_TileList[listBase + gid.x];
    uint2 p = uint2(tile % tilesX, tile / tilesX) * TILE_SIZE + gtid.xy;
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
#if TILE_SUN_MIXED
    float sunVis = -1.0;
#else
    float sunVis = 1.0;
#endif
    float3 lit = c.rgb + shade_pbr(bc.rgb, normalize(n.xyz), worldPos, bc.a, abs(n.w), c.a, float4(pixel, depth, 1.0), sunVis);
#if TILE_SUN_MIXED
    if (dev_param_3.y > 0.5)
        lit = SunShadowDebugColor(lit, worldPos);
#endif
#if TILE_LIGHTS
    if (localShadowDebug != 0u)
    {
        float linearDepth = mul(m_V, float4(worldPos, 1.0)).z;
        float4 dbg = LocalShadowDebug(worldPos, normalize(n.xyz), pixel, linearDepth);
        if (dbg.w > 0.5)
            lit = dbg.rgb;
    }
#endif
    g_SceneColor[p] = float4(lit, 1.0);
}

#endif
