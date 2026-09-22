#ifndef DETAIL_BLADE_MATERIAL_H
#define DETAIL_BLADE_MATERIAL_H

#include "shared/common.h"
#include "shared/pbr_brdf.h"

static const float BLADE_ROUGHNESS_BASE = 0.85;
static const float BLADE_ROUGHNESS_TIP = 0.55;
static const float BLADE_AO_BASE = 0.35;
static const float BLADE_AO_TIP = 1.0;
static const float BLADE_AO_POWER = 0.6;
static const float BLADE_VIEW_FADE_RATE = 0.017;
static const float BLADE_COLOR_VEIN_STRENGTH = 0.15;
static const float BLADE_ROUGHNESS_VEIN_STRENGTH = 0.1;
static const float BLADE_AO_EDGE_MIN = 0.95;
static const float BLADE_AO_VEIN_MIN = 0.9;
static const float BLADE_COLOR_BLEND_LOW = 0.2;
static const float BLADE_COLOR_BLEND_HIGH = 0.8;
static const float BLADE_NORMAL_VARIANCE_LIMIT = 0.18;
static const float BLADE_NORMAL_MIN_LENGTH = 1e-4;
static const float4 BLADE_NEUTRAL_VEIN = float4(1.0, 1.0, 1.0, 0.0);

float3 BladeBaseAlbedo(float3 colorBase, float3 colorTip, float3 objectTint, float colorVariation, float bladeHash, float heightParam)
{
    float colorBlend = smoothstep(BLADE_COLOR_BLEND_LOW, BLADE_COLOR_BLEND_HIGH, heightParam);
    float3 albedo = lerp(colorBase, colorTip, colorBlend);
    albedo *= lerp(1.0 - colorVariation, 1.0 + colorVariation, bladeHash);
    return albedo * objectTint;
}

float3 BladeVeinAlbedo(float3 albedo, float4 vein, float veinValue)
{
    return lerp(albedo, albedo * vein.rgb, veinValue * BLADE_COLOR_VEIN_STRENGTH);
}

float3 BladeDistanceFade(float3 albedo, float3 colorBase, float3 colorTip, float3 worldPos)
{
    float viewDist = length(worldPos - eye_position);
    float distFade = exp(-viewDist * BLADE_VIEW_FADE_RATE);
    float3 midColor = (colorBase + colorTip) * 0.5;
    return lerp(midColor, albedo, distFade);
}

float BladeRoughness(float heightParam, float veinValue, float variance)
{
    float roughness = lerp(BLADE_ROUGHNESS_BASE, BLADE_ROUGHNESS_TIP, heightParam) + veinValue * BLADE_ROUGHNESS_VEIN_STRENGTH;
    return RoughnessWithVariance(saturate(roughness), variance);
}

float BladeAmbientOcclusion(float heightParam, float widthPercent, float veinValue)
{
    float heightAO = lerp(BLADE_AO_BASE, BLADE_AO_TIP, pow(saturate(heightParam), BLADE_AO_POWER));
    float edgeFactor = abs(widthPercent - 0.5) * 2.0;
    return heightAO * lerp(BLADE_AO_EDGE_MIN, 1.0, edgeFactor) * lerp(BLADE_AO_VEIN_MIN, 1.0, veinValue);
}

float BladeWidthNormalScale(float3 blendedNormal)
{
    return 1.0 / max(length(blendedNormal), BLADE_NORMAL_MIN_LENGTH);
}

float3 BladeWidthNormal(float3 normal1, float3 normal2, float widthPercent)
{
    return lerp(normal1, normal2, widthPercent);
}

float3 BladeWidthNormalDerivative(float3 deriv1, float3 deriv2, float3 normal1, float3 normal2, float widthPercent, float widthDeriv)
{
    return lerp(deriv1, deriv2, widthPercent) + (normal2 - normal1) * widthDeriv;
}

float BladeNormalVariance(float3 normalDx, float3 normalDy, float invLength)
{
    float3 dNdx = normalDx * invLength;
    float3 dNdy = normalDy * invLength;
    return min(0.5 * (dot(dNdx, dNdx) + dot(dNdy, dNdy)), BLADE_NORMAL_VARIANCE_LIMIT);
}

#endif
