#ifndef CLUSTERED_LIGHTING_H
#define CLUSTERED_LIGHTING_H

struct GPULightData {
    float4 positionAndInvRangeSq;
    float4 colorAndRange;
    float4 directionAndSpotScale;
    float4 spotParamsAndType;
    float4x4 spotVP;
    float4 areaRightAndRadius;
    float4 areaLength;
    uint4 areaShadowSlots0;
    uint4 areaShadowSlots1;
};

#include "shared/local_shadow.h"

#define CAPSULE_LIGHT_SAMPLES 8u
#define CAPSULE_LIGHT_SAMPLE_WEIGHT 0.125f

bool IsSpotLight(GPULightData light)
{
    return light.spotParamsAndType.y > 0.5f && light.spotParamsAndType.y < 1.5f;
}

bool IsHudSpotLight(GPULightData light)
{
    return IsSpotLight(light) && light.areaLength.y > 0.5f;
}

bool IsCapsuleLight(GPULightData light)
{
    return light.spotParamsAndType.y > 1.5f;
}

float3 LightInfluenceCenter(GPULightData light)
{
    float3 center = light.positionAndInvRangeSq.xyz;
    if (IsCapsuleLight(light))
        center += light.directionAndSpotScale.xyz * (0.5f * light.areaLength.x);
    return center;
}

float LightInfluenceRadius(GPULightData light)
{
    float radius = light.colorAndRange.w;
    if (IsCapsuleLight(light))
        radius += 0.5f * light.areaLength.x + light.areaRightAndRadius.w;
    return radius;
}

bool LightHasShadow(GPULightData light)
{
    return light.spotParamsAndType.w != 0.0f
        || any(light.areaShadowSlots0) || any(light.areaShadowSlots1);
}

uint LocalShadowSlot(GPULightData light, float3 worldPos)
{
    uint slot1 = (uint)(light.spotParamsAndType.w + 0.5f);
    if (slot1 == 0u)
        return 0xFFFFFFFFu;
    return LocalShadowCachedSlot(slot1 - 1u, light.spotParamsAndType.y < 0.5f, worldPos);
}

uint CapsuleShadowSlot(GPULightData light, uint sampleIndex, float3 worldPos)
{
    uint slot1 = sampleIndex < 4u ? light.areaShadowSlots0[sampleIndex] : light.areaShadowSlots1[sampleIndex - 4u];
    if (slot1 == 0u)
        return 0xFFFFFFFFu;
    return LocalShadowCachedSlot(slot1 - 1u, true, worldPos);
}

float2 CapsuleLightRasterSample(uint sampleIndex)
{
    return float2((float(sampleIndex) + 0.5f) * 0.125f, frac(float(sampleIndex) * 0.61803398875f));
}

void SampleCapsuleLight(GPULightData light, float2 u, out float3 position, out float3 normal)
{
    float3 start = light.positionAndInvRangeSq.xyz;
    float3 axis = light.directionAndSpotScale.xyz;
    float3 right = light.areaRightAndRadius.xyz;
    float radius = light.areaRightAndRadius.w;
    float capsuleLength = light.areaLength.x;
    float3 up = cross(axis, right);

    float h = u.x * (capsuleLength + 2.0f * radius) - radius;
    float a = h < 0.0f ? h / radius : (h > capsuleLength ? (h - capsuleLength) / radius : 0.0f);
    float radial = sqrt(max(0.0f, 1.0f - a * a));
    float phi = 6.28318530718f * u.y;

    normal = axis * a + (right * cos(phi) + up * sin(phi)) * radial;
    position = start + axis * clamp(h, 0.0f, capsuleLength) + radius * normal;
}

// Point light distance attenuation (smooth window function)
float PointLightAttenuation(float distSq, float invRangeSq)
{
    float factor = saturate(1.0f - distSq * invRangeSq);
    return factor * factor;
}

// Spot light angular attenuation
float SpotLightAttenuation(float3 toLight, float3 spotDir, float scale, float offset)
{
    float cosAngle = dot(normalize(-toLight), spotDir);
    return saturate(cosAngle * scale + offset);
}

float CapsuleLightAttenuation(GPULightData light, float3 worldPos, float3 samplePosition, float3 sampleNormal, out float3 L, out float dist)
{
    float3 toSample = samplePosition - worldPos;
    float distSq = max(dot(toSample, toSample), 1e-6f);
    float invDist = rsqrt(distSq);
    dist = distSq * invDist;
    L = toSample * invDist;
    float cosEmitter = max(dot(sampleNormal, -L), 0.0f);
    return 4.0f * cosEmitter / distSq * PointLightAttenuation(distSq, light.positionAndInvRangeSq.w);
}

#ifdef CLUSTERED_LIGHTING_FORWARD
#define CLUSTERED_LIGHTING_PUNCTUAL
#endif

#ifdef CLUSTERED_LIGHTING_PUNCTUAL
StructuredBuffer<GPULightData> g_LightData : register(t20);

float PunctualLightAttenuation(GPULightData light, float3 worldPos, out float3 L, out float dist)
{
    float3 toLight = light.positionAndInvRangeSq.xyz - worldPos;
    float distSq = dot(toLight, toLight);
    dist = sqrt(distSq);
    L = distSq > 1e-12f ? toLight * rsqrt(distSq) : float3(0.0f, 1.0f, 0.0f);

    float atten = PointLightAttenuation(distSq, light.positionAndInvRangeSq.w);
    if (IsSpotLight(light))
    {
        uint texIdx = asuint(light.spotParamsAndType.z);
        if (texIdx != 0u)
        {
            float4 projPos = mul(light.spotVP, float4(worldPos, 1.0));
            if (projPos.w > 0.0f)
            {
                float2 projUV = projPos.xy / projPos.w * 0.5f + 0.5f;
                projUV.y = 1.0f - projUV.y;
                atten *= GetBindlessTexture(texIdx).SampleLevel(smp_rtlinear, projUV, 0).r;
            }
            else
            {
                atten = 0.0f;
            }
        }
        else
        {
            atten *= SpotLightAttenuation(toLight, light.directionAndSpotScale.xyz, light.directionAndSpotScale.w, light.spotParamsAndType.x);
        }
    }
    return atten;
}
#endif

#ifdef CLUSTERED_LIGHTING_FORWARD
StructuredBuffer<uint2> g_ClusterGrid : register(t21);
StructuredBuffer<uint> g_LightIndexList : register(t22);
#endif

// Compute cluster index from screen position and linear depth
uint GetClusterIndex(float2 screenPos, float linearDepth,
    float3 gridDims, float4 depthParams)
{
    uint tileX = (uint)(screenPos.x / depthParams.w);
    uint tileY = (uint)(screenPos.y / depthParams.w);

    // Clamp to grid bounds
    tileX = min(tileX, (uint)gridDims.x - 1);
    tileY = min(tileY, (uint)gridDims.y - 1);

    // Exponential depth slice
    float zNear = depthParams.x;
    float logRatio = depthParams.z;
    uint slice = (uint)(log2(max(linearDepth / zNear, 1.0f)) * logRatio);
    slice = min(slice, (uint)gridDims.z - 1);

    return tileX + tileY * (uint)gridDims.x + slice * (uint)gridDims.x * (uint)gridDims.y;
}

// Linearize depth from projection matrix depth value
float LinearizeDepth(float ndcDepth, float zNear, float zFar)
{
    return zNear * zFar / (zNear + ndcDepth * (zFar - zNear));
}

#ifdef CLUSTERED_LIGHTING_FORWARD
#include "shared/pbr_brdf.h"

float3 EvaluateCapsuleLight(
    GPULightData light,
    float3 worldPos,
    float3 N,
    float3 V,
    float3 albedo,
    float metallic,
    float roughness,
    bool hudReceiver,
    uint diffuseMode,
    bool foliage,
    float3 sssColor)
{
    float3 lightColor = light.colorAndRange.xyz;
    float3 total = 0;
    for (uint s = 0; s < CAPSULE_LIGHT_SAMPLES; ++s)
    {
        float3 samplePosition;
        float3 sampleNormal;
        SampleCapsuleLight(light, CapsuleLightRasterSample(s), samplePosition, sampleNormal);

        float3 L;
        float dist;
        float atten = CapsuleLightAttenuation(light, worldPos, samplePosition, sampleNormal, L, dist) * CAPSULE_LIGHT_SAMPLE_WEIGHT;
        if (atten <= 0.0001f)
            continue;
        if (!foliage && dot(N, L) <= 0.0f)
            continue;

        float2 shadow = float2(1.0, 0.0);
        uint shadowSlot = CapsuleShadowSlot(light, s, worldPos);
        if (shadowSlot != 0xFFFFFFFFu)
            shadow = LocalShadow(shadowSlot, worldPos, N, hudReceiver);

        float3 lc = lightColor * atten;
        if (foliage)
        {
            float transmit = shadow.x + (1.0 - shadow.x) * FoliageTransmittance(shadow.y, foliage_sss.w);
            total += PBRDirectLighting(albedo, N, V, L, lc * shadow.x, 0.0, roughness, 1u)
                + FoliageTransmission(N, V, L, foliage_params2.x) * transmit * sssColor * lc;
        }
        else if (shadow.x > 0.001f)
        {
            total += PBRDirectLighting(albedo, N, V, L, lc * shadow.x, metallic, roughness, diffuseMode);
        }
    }
    return total;
}

float3 EvaluateClusteredLights(
    float3 worldPos,
    float3 N,
    float3 V,
    float3 albedo,
    float metallic,
    float roughness,
    float2 screenPos,
    float linearDepth,
    bool hudReceiver,
    uint diffuseMode,
    uint shadingClass = SHADING_CLASS_STANDARD,
    float3 sssColor = 0.0)
{
    uint numLights = (uint)cluster_params.w;
    if (numLights == 0)
        return 0;

    float4 depthParams = cluster_scales;
    uint clusterIdx = GetClusterIndex(screenPos, linearDepth, cluster_params.xyz, depthParams);
    uint2 clusterData = g_ClusterGrid[clusterIdx];
    uint lightOffset = clusterData.x;
    uint lightCount = clusterData.y;

    bool foliage = shadingClass == SHADING_CLASS_FOLIAGE;
    float3 totalLight = 0;
    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIdx = i;
        if (lightOffset != 0xFFFFFFFFu)
            lightIdx = g_LightIndexList[lightOffset + i];
        GPULightData light = g_LightData[lightIdx];

        if (IsCapsuleLight(light))
        {
            totalLight += EvaluateCapsuleLight(light, worldPos, N, V, albedo, metallic, roughness, hudReceiver, diffuseMode, foliage, sssColor);
            continue;
        }
        float3 lightColor = light.colorAndRange.xyz;

        float3 lightPos = worldPos;
        float3 lightN = N;
        float3 lightV = V;
        if (hudReceiver && IsHudSpotLight(light))
        {
            lightPos = mul(m_HudWarp, float4(worldPos, 1.0)).xyz;
            lightN = normalize(mul(N, float3x3(m_HudUnwarp[0].xyz, m_HudUnwarp[1].xyz, m_HudUnwarp[2].xyz)));
            lightV = normalize(eye_position - lightPos);
        }

        float3 L;
        float dist;
        float atten = PunctualLightAttenuation(light, lightPos, L, dist);

        if (atten <= 0.001f)
            continue;

        float2 shadow = float2(1.0, 0.0);
        uint shadowSlot = LocalShadowSlot(light, lightPos);
        if (shadowSlot != 0xFFFFFFFFu)
            shadow = LocalShadow(shadowSlot, lightPos, lightN, hudReceiver);

        float3 lc = lightColor * atten;
        if (foliage)
        {
            float transmit = shadow.x + (1.0 - shadow.x) * FoliageTransmittance(shadow.y, foliage_sss.w);
            totalLight += PBRDirectLighting(albedo, lightN, lightV, L, lc * shadow.x, 0.0, roughness, 1u)
                + FoliageTransmission(lightN, lightV, L, foliage_params2.x) * transmit * sssColor * lc;
        }
        else if (shadow.x > 0.001f)
        {
            totalLight += PBRDirectLighting(albedo, lightN, lightV, L, lc * shadow.x, metallic, roughness, diffuseMode);
        }
    }
    return totalLight;
}

#endif // CLUSTERED_LIGHTING_FORWARD

#endif // CLUSTERED_LIGHTING_H
