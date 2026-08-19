#ifndef RT_SHADE_HIT_H
#define RT_SHADE_HIT_H

#include "bindless_common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"

float3 ShadeBakedFromHemi(float hemi, float3 albedo, float3 hemiColor)
{
    float h = saturate(hemi);
    return hemiColor * h * albedo * 0.55;
}

float3 ShadeBakedFromTerrainLmap(TerrainMaterialData tmat, float2 lmUV, float3 albedo, float3 hemiColor, float hemi)
{
    return ShadeBakedFromHemi(hemi, albedo, hemiColor);
}

float3 ShadeHitDirect(
    float3 albedo,
    float3 N,
    float3 V,
    float metallic,
    float roughness,
    float3 sunDir,
    float3 sunColor,
    float shadow,
    float3 baked)
{
    float3 Lo = baked;
    if (shadow > 0.001)
        Lo += PBRDirectLighting(albedo, N, V, sunDir, sunColor * shadow, metallic, roughness, 1);
    return Lo;
}

#endif
