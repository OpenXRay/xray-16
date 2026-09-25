#ifndef RTGI_TRACE_COMMON_H
#define RTGI_TRACE_COMMON_H

#include "bindless_common.h"
#include "rtgi_raw_params.h"
#include "rt_integrator.h"

Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_Normal : register(t15);
Texture2D<float4> t_BaseColor : register(t16);
Texture2D<float2> t_Material : register(t17);
Texture2D<float4> t_SourceColor : register(t28);
Texture2D<float2> t_MotionVectors : register(t29);

RWTexture2D<float4> u_RawDiffuse : register(u0);
RWTexture2D<float4> u_RawSpecular : register(u1);
RWTexture2D<float4> u_Emission : register(u2);
RWTexture2D<float4> u_NormalRoughness : register(u3);
RWTexture2D<float4> u_AlbedoMetallic : register(u4);
RWTexture2D<float4> u_PathData : register(u5);
RWTexture2D<float4> u_SurfaceData : register(u6);
RWTexture2D<float2> u_Motion : register(u7);

float3 RTGIReconstructWorldPos(uint2 pixel, float depth)
{
    float2 uv = (float2(pixel) + 0.5) / float2(g_ScreenWidth, g_ScreenHeight);
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    if (depth >= 0.9)
        clip.z = (depth - 0.9) * 10.0;
    float4 world = mul(g_InvViewProj, clip);
    return world.xyz / world.w;
}

struct RTGIPrimarySurface
{
    bool valid;
    float3 worldPos;
    MaterialSurface surface;
    float3 V;
    float linearDistance;
    float coneWidth;
    float coneSpread;
    float depth;
    bool motionValid;
    float2 motion;
    float3 sourceColor;
};

RTGIPrimarySurface RTGIDecodePrimary(uint2 pixel)
{
    RTGIPrimarySurface primary = (RTGIPrimarySurface)0;
    float depth = t_Depth.Load(int3(pixel, 0));
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));
    float2 materialData = t_Material.Load(int3(pixel, 0));
    float3 sourceColor = t_SourceColor.Load(int3(pixel, 0)).rgb;
    float2 motion = t_MotionVectors.Load(int3(pixel, 0));
    bool primaryInputsFinite = isfinite(depth) && all(isfinite(normalData)) &&
        all(isfinite(baseColorData)) && all(isfinite(materialData));

    if (!all(isfinite(sourceColor)))
        sourceColor = 0.0;
    bool motionValid = all(isfinite(motion));
    if (!motionValid)
        motion = 0.0;
    if (!all(isfinite(normalData)))
        normalData = 0.0;
    if (!all(isfinite(baseColorData)))
        baseColorData = 0.0;
    if (!all(isfinite(materialData)))
        materialData = 0.0;
    if (!isfinite(depth))
        depth = 0.0;

    primary.sourceColor = sourceColor;
    primary.motion = motion;
    primary.motionValid = motionValid;
    primary.depth = depth;

    bool validPrimary = primaryInputsFinite && depth > 0.0 &&
        dot(normalData.xyz, normalData.xyz) >= 0.25 && g_RayDistance > 0.0;
    float3 worldPos = 0.0;
    if (validPrimary)
    {
        worldPos = RTGIReconstructWorldPos(pixel, depth);
        validPrimary = all(isfinite(worldPos));
    }
    primary.valid = validPrimary;
    if (!validPrimary)
        return primary;

    primary.worldPos = worldPos;
    MaterialSurface surface = GBufferMaterialSurface(normalData, baseColorData, materialData);
    primary.surface = surface;
    float3 V = RTSafeNormalize(g_CameraPos.xyz - worldPos, surface.N);
    primary.V = V;
    float3 worldRight = RTGIReconstructWorldPos(pixel + uint2(1u, 0u), depth);
    float3 worldUp = RTGIReconstructWorldPos(pixel + uint2(0u, 1u), depth);
    float linearDistance = length(g_CameraPos.xyz - worldPos);
    primary.linearDistance = linearDistance;
    float coneWidth = max(length(worldRight - worldPos), length(worldUp - worldPos));
    if (!isfinite(coneWidth))
        coneWidth = 0.0;
    float coneSpread = coneWidth / max(linearDistance, 0.001);
    if (!isfinite(coneSpread))
        coneSpread = 0.0;
    coneSpread = max(coneSpread, max(surface.roughness * surface.roughness, 1.0 - surface.metallic));
    primary.coneWidth = coneWidth;
    primary.coneSpread = coneSpread;
    return primary;
}

RTSceneParams RTGIBuildRawScene(uint2 pixel)
{
    RTSceneParams scene = RTBuildSceneParams(g_IdentityStaticCount, g_TerrainBatchCount,
        g_SkinnedBatchStart, g_GrassBatchStart, g_DetailAtlasIndex, g_RTLightCount,
        g_DiffuseMode, g_SunDir_Intensity, g_SunColor, g_EmissiveCount,
        g_MaxNullEvents, g_SunAngularRadius);
    scene.detailMeshBatchStart = g_DetailMeshBatchStart;
    scene.staticDetailBatchStart = g_StaticDetailBatchStart;
    scene.detailPbrIndex = g_DetailPbrIndex;
    scene.detailBumpIndex = g_DetailBumpIndex;
    scene.rayDistance = g_RayDistance;
    scene.rayMask = RT_RAY_MASK_WORLD;
    scene.clusterLights = g_ClusterLights;
    scene.clusterPixel = pixel;
    scene.lightRays = g_LightRays;
    return scene;
}

RTIntegratorSettings RTGIBuildRawSettings(RTGIPrimarySurface primary)
{
    RTIntegratorSettings settings = RTIntegratorDefaultSettings();
    settings.maxBounces = max(g_MaxBounces, 1u);
    settings.coneWidth = primary.coneWidth;
    settings.coneSpread = primary.coneSpread;
    settings.trackDiagnostics = false;
    settings.trackSegmentMetrics = true;
    settings.allowHudFirstRay = false;
    return settings;
}

uint RTGISampleCount()
{
    return max(g_SamplesPerPixel, 1u);
}

uint RTGISampleRng(uint2 pixel, uint sampleIndex)
{
    return pcg_hash(pixel.x + pixel.y * 1973u + (g_FrameIndex + sampleIndex * 48611u) * 26699u);
}

struct RTGIAccumulation
{
    float3 diffuseSum;
    float diffuseDistanceSum;
    float3 specularSum;
    float specularDistanceSum;
    float diffuseWeightSum;
    float specularWeightSum;
    uint validCount;
    uint depthSum;
};

void RTGIAccumulateSample(inout RTGIAccumulation accumulation, RTIntegratorResult path, float giIntensity)
{
    float3 diffuseSignal = path.directDiffuse + giIntensity * path.indirectDiffuse;
    float3 specularSignal = path.directSpecular + giIntensity * path.indirectSpecular;
    if (!all(isfinite(diffuseSignal)))
        diffuseSignal = 0.0;
    if (!all(isfinite(specularSignal)))
        specularSignal = 0.0;
    accumulation.diffuseSum += diffuseSignal;
    accumulation.specularSum += specularSignal;
    if (path.firstSegmentDefined)
    {
        accumulation.diffuseDistanceSum += path.firstDiffuseShare * path.firstSegmentDistance;
        accumulation.specularDistanceSum += path.firstSpecularShare * path.firstSegmentDistance;
        accumulation.diffuseWeightSum += path.firstDiffuseShare;
        accumulation.specularWeightSum += path.firstSpecularShare;
    }
    accumulation.validCount += path.valid ? 1u : 0u;
    accumulation.depthSum += path.scatteringDepth;
}

void RTGIWriteRawOutputs(uint2 pixel, RTGIPrimarySurface primary, RTGIAccumulation accumulation)
{
    float inverseSamples = 1.0 / float(RTGISampleCount());
    float3 rawDiffuse = 0.0;
    float3 rawSpecular = 0.0;
    float4 normalRoughness = 0.0;
    float4 albedoMetallic = 0.0;
    float4 pathData = 0.0;
    float4 surfaceData = 0.0;
    if (primary.valid)
    {
        float meanDiffuseDistance = accumulation.diffuseWeightSum > 0.0
            ? accumulation.diffuseDistanceSum / accumulation.diffuseWeightSum : 0.0;
        float meanSpecularDistance = accumulation.specularWeightSum > 0.0
            ? accumulation.specularDistanceSum / accumulation.specularWeightSum : 0.0;
        rawDiffuse = accumulation.diffuseSum * inverseSamples;
        rawSpecular = accumulation.specularSum * inverseSamples;
        pathData = float4(meanDiffuseDistance, meanSpecularDistance,
            float(accumulation.validCount) * inverseSamples, float(accumulation.depthSum) * inverseSamples);
        normalRoughness = float4(primary.surface.N, abs(primary.surface.roughness));
        albedoMetallic = float4(primary.surface.albedo, primary.surface.metallic);
        surfaceData = float4(primary.linearDistance, primary.depth, primary.motionValid ? 3.0 : 1.0,
            primary.depth >= 0.9 ? 1.0 : 0.0);
    }

    if (!all(isfinite(rawDiffuse)))
        rawDiffuse = 0.0;
    if (!all(isfinite(rawSpecular)))
        rawSpecular = 0.0;
    if (!all(isfinite(pathData)))
        pathData = 0.0;
    if (!all(isfinite(surfaceData)))
        surfaceData = 0.0;
    if (!all(isfinite(normalRoughness)))
        normalRoughness = 0.0;
    if (!all(isfinite(albedoMetallic)))
        albedoMetallic = 0.0;

    u_RawDiffuse[pixel] = float4(rawDiffuse, primary.valid ? 1.0 : 0.0);
    u_RawSpecular[pixel] = float4(rawSpecular, primary.valid ? 1.0 : 0.0);
    u_Emission[pixel] = float4(primary.sourceColor, 1.0);
    u_NormalRoughness[pixel] = normalRoughness;
    u_AlbedoMetallic[pixel] = albedoMetallic;
    u_PathData[pixel] = pathData;
    u_SurfaceData[pixel] = surfaceData;
    u_Motion[pixel] = primary.motion;
}

#endif
