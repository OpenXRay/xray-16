#ifndef CLUSTERED_LIGHTING_H
#define CLUSTERED_LIGHTING_H

struct GPULightData {
    float4 positionAndInvRangeSq;
    float4 colorAndRange;
    float4 directionAndSpotScale;
    float4 spotParamsAndType; // .z cookie idx (0xFFFFFFFF=none); .w shadow idx/want (0xFFFFFFFF=none)
    float4x4 spotVP;
    float4 localShadowRect;
};

struct GPUShadowData {
    float4 atlasOffsetScale;
    float4x4 viewProj;
};

// Point light distance attenuation (smooth window function)
float PointLightAttenuation(float distSq, float invRangeSq)
{
    float factor = saturate(1.0f - distSq * abs(invRangeSq));
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
StructuredBuffer<GPUShadowData> g_ShadowData : register(t50);
#endif

// Compute cluster index from pixel coords (top-left origin, Y down) + positive view-Z.
uint GetClusterIndex(float2 screenPos, float linearDepth,
    float3 gridDims, float4 depthParams)
{
    uint tileX = (uint)(screenPos.x / depthParams.w);
    uint tileY = (uint)(screenPos.y / depthParams.w);

    tileX = min(tileX, (uint)gridDims.x - 1);
    tileY = min(tileY, (uint)gridDims.y - 1);

    float zNear = depthParams.x;
    float logRatio = depthParams.z;
    // linearDepth must be positive view-space Z (use abs at call site)
    uint slice = (uint)(log2(max(linearDepth / zNear, 1.0f)) * logRatio);
    slice = min(slice, (uint)gridDims.z - 1);

    return tileX + tileY * (uint)gridDims.x + slice * (uint)gridDims.x * (uint)gridDims.y;
}

// Same screen-space convention as cluster_light_assign.cs / NVRHI DX viewport
// (NDC +Y up → pixel Y down). Prefer SV_Position for shading pixels — it is the
// authoritative tile; reconstructing from worldPos can drift a few pixels and
// cross 64px tile boundaries (square flicker).
float2 ClusterWorldToScreen(float3 worldPos)
{
    float4 clipPos = mul(m_VP, float4(worldPos, 1.0));
    if (clipPos.w <= 1e-5)
        return float2(0, 0);
    float3 ndc = clipPos.xyz / clipPos.w;
    float2 screenPos;
    screenPos.x = (ndc.x * 0.5 + 0.5) * screen_res.x;
    screenPos.y = (0.5 - ndc.y * 0.5) * screen_res.y;
    return screenPos;
}

uint GetClusterIndexFromWorld(float3 worldPos, float linearDepth)
{
    return GetClusterIndex(
        ClusterWorldToScreen(worldPos),
        linearDepth,
        cluster_params.xyz,
        cluster_scales);
}

uint GetClusterIndexFromPixel(float2 pixelPos, float linearDepth)
{
    return GetClusterIndex(pixelPos, linearDepth, cluster_params.xyz, cluster_scales);
}

// r_cluster_debug: 1 = tile XY checker, 2 = depth slice heatmap
// Prefer pixelPos (SV_Position.xy) so overlay matches EvaluateClusteredLights.
float3 ClusterDebugColor(float2 pixelPos, float linearDepth, int mode)
{
    float tileSize = cluster_scales.w;
    uint tileX = (uint)(pixelPos.x / tileSize);
    uint tileY = (uint)(pixelPos.y / tileSize);
    uint slice = (uint)(log2(max(linearDepth / cluster_scales.x, 1.0f)) * cluster_scales.z);
    slice = min(slice, (uint)cluster_params.z - 1);
    if (mode == 2)
    {
        float t = (float)slice / max(cluster_params.z - 1.0, 1.0);
        return float3(t, 1.0 - t, 0.25);
    }
    float rx = (float)(tileX & 1u);
    float ry = (float)(tileY & 1u);
    return float3(rx, ry, 1.0 - abs(rx - ry));
}

float3 ClusterDebugColor(float3 worldPos, float linearDepth, int mode)
{
    return ClusterDebugColor(ClusterWorldToScreen(worldPos), linearDepth, mode);
}

// Linearize depth from projection matrix depth value
float LinearizeDepth(float ndcDepth, float zNear, float zFar)
{
    return zNear * zFar / (zFar - ndcDepth * (zFar - zNear));
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
    float2 pixelPos,
    float linearDepth,
    uint diffuseMode,
    float localShadowMask = 1.0)
{
    uint numLights = (uint)cluster_params.w;
    if (numLights == 0)
        return 0;

    // Pixel position is authoritative (matches rasterizer / NVRHI DX viewport).
    uint clusterIdx = GetClusterIndexFromPixel(pixelPos, linearDepth);
    uint2 clusterData = g_ClusterGrid[clusterIdx];
    uint lightOffset = clusterData.x;
    uint lightCount = clusterData.y;

    float3 totalLight = 0;
    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIdx = g_LightIndexList[lightOffset + i];
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
            if (texIdx != 0xFFFFFFFFu)
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

        if (atten > 0.001f)
        {
            float shadow = 1.0;
            if (light.localShadowRect.w > 0.5)
            {
                float soft = (lightType > 0.5f)
                    ? max(dev_param_1.z, 0.1)
                    : max(dev_param_1.y, 0.1);
                shadow = SampleLocalShadow(
                    worldPos, N, light.spotVP,
                    light.localShadowRect, soft);
            }

            float3 litColor = PBRDirectLighting(
                albedo, N, V, L,
                lightColor * atten * shadow,
                metallic, roughness, diffuseMode);
            totalLight += litColor;
        }
    }
    return totalLight;
}
#endif // CLUSTERED_LIGHTING_FORWARD

#endif // CLUSTERED_LIGHTING_H
