#ifndef CLUSTERED_LIGHTING_H
#define CLUSTERED_LIGHTING_H

struct GPULightData {
    float4 positionAndInvRangeSq;
    float4 colorAndRange;
    float4 directionAndSpotScale;
    float4 spotParamsAndType;
    float4x4 spotVP;
};

#include "shared/local_shadow.h"

uint LocalShadowSlot(GPULightData light, float3 worldPos)
{
    uint slot1 = (uint)(light.spotParamsAndType.w + 0.5f);
    if (slot1 == 0u)
        return 0xFFFFFFFFu;
    return LocalShadowCachedSlot(slot1 - 1u, light.spotParamsAndType.y < 0.5f, worldPos);
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

#ifdef CLUSTERED_LIGHTING_FORWARD
// These are bound in the forward pass
StructuredBuffer<GPULightData> g_LightData : register(t20);
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
        float3 lightPos = light.positionAndInvRangeSq.xyz;
        float invRangeSq = light.positionAndInvRangeSq.w;
        float3 lightColor = light.colorAndRange.xyz;
        float lightType = light.spotParamsAndType.y;

        float3 toLight = lightPos - worldPos;
        float distSq = dot(toLight, toLight);
        float3 L = normalize(toLight);
        float atten = PointLightAttenuation(distSq, invRangeSq);

        if (lightType > 0.5f)
        {
            uint texIdx = asuint(light.spotParamsAndType.z);
            if (texIdx != 0)
            {
                float4 projPos = mul(light.spotVP, float4(worldPos, 1.0));
                if (projPos.w > 0)
                {
                    float2 projUV = projPos.xy / projPos.w * 0.5 + 0.5;
                    projUV.y = 1.0 - projUV.y;
                    Texture2D spotTex = GetBindlessTexture(texIdx);
                    float4 texSample = spotTex.SampleLevel(smp_rtlinear, projUV, 0);
                    atten *= texSample.r;
                }
                else
                {
                    atten = 0;
                }
            }
            else
            {
                float3 spotDir = light.directionAndSpotScale.xyz;
                float spotScale = light.directionAndSpotScale.w;
                float spotOffset = light.spotParamsAndType.x;
                atten *= SpotLightAttenuation(toLight, spotDir, spotScale, spotOffset);
            }
        }

        if (atten <= 0.001f)
            continue;

        float2 shadow = float2(1.0, 0.0);
        uint shadowSlot = LocalShadowSlot(light, worldPos);
        if (shadowSlot != 0xFFFFFFFFu)
            shadow = LocalShadow(shadowSlot, worldPos, N, hudReceiver);

        float3 lc = lightColor * atten;
        if (foliage)
        {
            float transmit = shadow.x + (1.0 - shadow.x) * FoliageTransmittance(shadow.y, foliage_sss.w);
            totalLight += PBRDirectLighting(albedo, N, V, L, lc * shadow.x, 0.0, roughness, 1u)
                + FoliageTransmission(N, V, L, foliage_params2.x) * transmit * sssColor * lc;
        }
        else if (shadow.x > 0.001f)
        {
            totalLight += PBRDirectLighting(albedo, N, V, L, lc * shadow.x, metallic, roughness, diffuseMode);
        }
    }
    return totalLight;
}

#endif // CLUSTERED_LIGHTING_FORWARD

#endif // CLUSTERED_LIGHTING_H
