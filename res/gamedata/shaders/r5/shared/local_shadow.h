#ifndef SHARED_LOCAL_SHADOW_H
#define SHARED_LOCAL_SHADOW_H

#include "local_shadow_common.h"

#ifdef LOCAL_SHADOW_RECEIVER

StructuredBuffer<LocalShadowView> g_LocalShadowTiles : register(t34);
Texture2DArray<float> g_LocalShadowStatic : register(t35);
Texture2DArray<float> g_LocalShadowDyn : register(t36);
Texture2D<float> g_LocalShadowHud : register(t38);

uint LocalShadowCachedSlot(uint baseSlot, bool pointLight, float3 worldPos)
{
    LocalShadowView base = g_LocalShadowTiles[baseSlot];
    if (base.zparams.w < 0.5)
        return 0xFFFFFFFFu;
    if (!pointLight)
        return baseSlot;
    float3 d = worldPos - base.lightPos.xyz;
    float3 a = abs(d);
    uint face = (a.x >= a.y && a.x >= a.z) ? (d.x >= 0.0 ? 0u : 1u)
              : (a.y >= a.z) ? (d.y >= 0.0 ? 2u : 3u) : (d.z >= 0.0 ? 4u : 5u);
    return baseSlot + face;
}

float2 LocalShadowHudLayer(LocalShadowView t, float3 wp, float3 N)
{
    float4 c0 = mul(t.hudViewProj, float4(wp, 1.0));
    if (c0.w <= t.hudZ.x)
        return float2(1.0, 0.0);
    float4 c = mul(t.hudViewProj, float4(wp + N * (c0.w * t.hudZ.z), 1.0));
    if (c.w <= 0.0)
        return float2(1.0, 0.0);
    float2 ndc = c.xy / c.w;
    if (any(abs(ndc) > 1.0))
        return float2(1.0, 0.0);
    float2 pl = (ndc * 0.5 + 0.5) * t.hud.w;
    float dRef = c.w - t.hudZ.w;
    float dSurf = c0.w - t.hudZ.w;
    float n = t.hudZ.x;
    float f = t.hudZ.y;
    float lit = 0.0;
    float thickSum = 0.0;
    float thickCnt = 0.0;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            float2 uv = (t.hud.yz + clamp(pl + float2(dx, dy), 1.5, t.hud.w - 1.5)) / LOCAL_SHADOW_HUD_ATLAS;
            float z = g_LocalShadowHud.SampleLevel(smp_nofilter, uv, 0);
            float dOcc = n * f / (n + z * (f - n));
            if (dOcc >= dRef)
            {
                lit += 1.0;
            }
            else
            {
                thickSum += max(dSurf - dOcc, 0.0);
                thickCnt += 1.0;
            }
        }
    }
    return float2(lit * (1.0 / 9.0), thickCnt > 0.5 ? thickSum / thickCnt : 0.0);
}

float2 LocalShadow(uint slot, float3 wp, float3 N, bool hudReceiver)
{
    LocalShadowView t = g_LocalShadowTiles[slot];
    if (t.zparams.w < 0.5)
        return float2(1.0, 0.0);
    uint hudFlags = uint(t.hud.x + 0.5);
    bool hudLayer = hudReceiver ? (hudFlags & LOCAL_SHADOW_HUD_CASTERS) != 0u : (hudFlags & LOCAL_SHADOW_HUD_TO_WORLD) != 0u;
    float2 hud = hudLayer ? LocalShadowHudLayer(t, wp, N) : float2(1.0, 0.0);
    float4 c0 = mul(t.viewProj, float4(wp, 1.0));
    if (c0.w <= t.zparams.x)
        return hud;
    float4 c = mul(t.viewProj, float4(wp + N * (c0.w * t.zparams.z), 1.0));
    if (c.w <= 0.0)
        return hud;
    float2 ndc = c.xy / c.w;
    if (any(abs(ndc) > 1.0))
        return hud;
    float2 pl = (ndc * 0.5 + 0.5) * t.rect.z;
    float dRef = c.w - t.rect.w;
    float dSurf = c0.w - t.rect.w;
    float n = t.zparams.x;
    float f = t.zparams.y;
    float lit = 0.0;
    float thickSum = 0.0;
    float thickCnt = 0.0;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            float2 uv = (t.rect.xy + clamp(pl + float2(dx, dy), 1.5, t.rect.z - 1.5)) / LOCAL_SHADOW_ATLAS;
            float3 uvw = float3(uv, t.shape.y);
            float z = max(g_LocalShadowStatic.SampleLevel(smp_nofilter, uvw, 0),
                          g_LocalShadowDyn.SampleLevel(smp_nofilter, uvw, 0));
            float dOcc = n * f / (n + z * (f - n));
            if (dOcc >= dRef)
            {
                lit += 1.0;
            }
            else
            {
                thickSum += max(dSurf - dOcc, 0.0);
                thickCnt += 1.0;
            }
        }
    }
    return float2(min(lit * (1.0 / 9.0), hud.x), max(thickCnt > 0.5 ? thickSum / thickCnt : 0.0, hud.y));
}

#else

uint LocalShadowCachedSlot(uint baseSlot, bool pointLight, float3 worldPos)
{
    return 0xFFFFFFFFu;
}

float2 LocalShadow(uint slot, float3 wp, float3 N, bool hudReceiver)
{
    return float2(1.0, 0.0);
}

#endif

#endif
