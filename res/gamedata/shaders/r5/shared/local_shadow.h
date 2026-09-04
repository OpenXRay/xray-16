#ifndef SHARED_LOCAL_SHADOW_H
#define SHARED_LOCAL_SHADOW_H

#include "local_shadow_common.h"

#ifdef LOCAL_SHADOW_RECEIVER

StructuredBuffer<LocalShadowTile> g_LocalShadowTiles : register(t34);
Texture2D<float> g_LocalShadowStatic : register(t35);
Texture2D<float> g_LocalShadowDyn : register(t36);

float LocalShadowVisibility(uint slot, float3 wp, float3 N)
{
    LocalShadowTile t = g_LocalShadowTiles[slot];
    if (t.zparams.w < 0.5)
        return 1.0;
    float4 c0 = mul(t.viewProj, float4(wp, 1.0));
    if (c0.w <= t.zparams.x)
        return 1.0;
    float4 c = mul(t.viewProj, float4(wp + N * (c0.w * t.zparams.z), 1.0));
    if (c.w <= 0.0)
        return 1.0;
    float2 ndc = c.xy / c.w;
    if (any(abs(ndc) > 1.0))
        return 1.0;
    float2 pl = (ndc * 0.5 + 0.5) * t.rect.z;
    float dRef = c.w - t.rect.w;
    float n = t.zparams.x;
    float f = t.zparams.y;
    float lit = 0.0;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            float2 uv = (t.rect.xy + clamp(pl + float2(dx, dy), 1.5, t.rect.z - 1.5)) / LOCAL_SHADOW_ATLAS;
            float z = max(g_LocalShadowStatic.SampleLevel(smp_nofilter, uv, 0),
                          g_LocalShadowDyn.SampleLevel(smp_nofilter, uv, 0));
            float dOcc = n * f / (n + z * (f - n));
            lit += (dOcc >= dRef) ? 1.0 : 0.0;
        }
    }
    return lit * (1.0 / 9.0);
}

#else

float LocalShadowVisibility(uint slot, float3 wp, float3 N)
{
    return 1.0;
}

#endif

#endif
