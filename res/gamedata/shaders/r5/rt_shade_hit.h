#ifndef RT_SHADE_HIT_H
#define RT_SHADE_HIT_H

#include "bindless_common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"

float3 ShadeBakedFromHemi(float hemi, float3 albedo, float3 hemiColor)
{
    float h = max(saturate(hemi), 0.18);
    return hemiColor * h * albedo * 0.7;
}

float3 ShadeBakedFromTerrainLmap(TerrainMaterialData tmat, float2 lmUV, float3 albedo, float3 hemiColor, float hemi)
{
    if ((tmat.flags & MAT_FLAG_HAS_LMAP) != 0 && tmat.lmapIndex != INVALID_TEXTURE_INDEX) {
        float2 luv = (dot(lmUV, lmUV) > 1e-8) ? lmUV : float2(0.5, 0.5);
        float3 L = GetBindlessTexture(tmat.lmapIndex).SampleLevel(smp_linear, luv, 0).rgb;
        return L * albedo;
    }
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
