#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rt_integrator.h"

cbuffer RTGIRawParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_SkyWeight;
    float g_ScreenWidth, g_ScreenHeight, g_GIIntensity; uint g_FrameIndex;
    uint g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart;
    uint g_DetailAtlasIndex, g_DiffuseMode, g_RTLightCount, g_EmissiveCount;
    uint g_MaxNullEvents, g_MaxBounces, g_SamplesPerPixel; float g_RayDistance;
    float g_EnvironmentRotation, g_SunAngularRadius, g_CameraConeSpread; uint g_Pad;
    uint g_DetailMeshBatchStart, g_StaticDetailBatchStart, g_DetailPbrIndex, g_DetailBumpIndex;
};

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

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= uint(g_ScreenWidth) || pixel.y >= uint(g_ScreenHeight))
        return;

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

    float3 rawDiffuse = 0.0;
    float3 rawSpecular = 0.0;
    float4 normalRoughness = 0.0;
    float4 albedoMetallic = 0.0;
    float4 pathData = 0.0;
    float4 surfaceData = 0.0;

    bool validPrimary = primaryInputsFinite && depth > 0.0 &&
        dot(normalData.xyz, normalData.xyz) >= 0.25 && g_RayDistance > 0.0;
    float3 worldPos = 0.0;
    if (validPrimary)
    {
        worldPos = RTGIReconstructWorldPos(pixel, depth);
        validPrimary = all(isfinite(worldPos));
    }

    if (validPrimary)
    {
        MaterialSurface primary = GBufferMaterialSurface(normalData, baseColorData, materialData);
        float3 V = RTSafeNormalize(g_CameraPos.xyz - worldPos, primary.N);
        float3 worldRight = RTGIReconstructWorldPos(pixel + uint2(1u, 0u), depth);
        float3 worldUp = RTGIReconstructWorldPos(pixel + uint2(0u, 1u), depth);
        float linearDistance = length(g_CameraPos.xyz - worldPos);
        float coneWidth = max(length(worldRight - worldPos), length(worldUp - worldPos));
        if (!isfinite(coneWidth))
            coneWidth = 0.0;
        float coneSpread = coneWidth / max(linearDistance, 0.001);
        if (!isfinite(coneSpread))
            coneSpread = 0.0;
        coneSpread = max(coneSpread, max(primary.roughness * primary.roughness, 1.0 - primary.metallic));

        RTSceneParams scene = RTBuildSceneParams(g_IdentityStaticCount, g_TerrainBatchCount,
            g_SkinnedBatchStart, g_GrassBatchStart, g_DetailAtlasIndex, g_RTLightCount,
            g_DiffuseMode, g_SunDir_Intensity, g_SunColor_SkyWeight, g_EmissiveCount,
            g_MaxNullEvents, g_EnvironmentRotation, g_SunAngularRadius);
        scene.detailMeshBatchStart = g_DetailMeshBatchStart;
        scene.staticDetailBatchStart = g_StaticDetailBatchStart;
        scene.detailPbrIndex = g_DetailPbrIndex;
        scene.detailBumpIndex = g_DetailBumpIndex;
        scene.rayDistance = g_RayDistance;
        scene.rayMask = RT_RAY_MASK_WORLD;

        RTIntegratorSettings settings;
        settings.maxBounces = max(g_MaxBounces, 1u);
        settings.coneWidth = coneWidth;
        settings.coneSpread = coneSpread;
        settings.trackDiagnostics = false;
        settings.trackSegmentMetrics = true;
        settings.allowHudFirstRay = false;

        uint samples = max(g_SamplesPerPixel, 1u);
        float3 diffuseSum = 0.0;
        float3 specularSum = 0.0;
        float diffuseDistanceSum = 0.0;
        float specularDistanceSum = 0.0;
        float diffuseWeightSum = 0.0;
        float specularWeightSum = 0.0;
        uint validCount = 0u;
        uint depthSum = 0u;

        for (uint sample = 0u; sample < samples; ++sample)
        {
            uint rng = pcg_hash(pixel.x + pixel.y * 1973u + (g_FrameIndex + sample * 48611u) * 26699u);
            RTIntegratorResult path = RTIntegratorRunPrimary(scene, settings, primary, worldPos,
                primary.N, V, rng);
            float3 diffuseSignal = path.directDiffuse + g_GIIntensity * path.indirectDiffuse;
            float3 specularSignal = path.directSpecular + g_GIIntensity * path.indirectSpecular;
            if (!all(isfinite(diffuseSignal)))
                diffuseSignal = 0.0;
            if (!all(isfinite(specularSignal)))
                specularSignal = 0.0;
            diffuseSum += diffuseSignal;
            specularSum += specularSignal;
            if (path.firstSegmentDefined)
            {
                diffuseDistanceSum += path.firstDiffuseShare * path.firstSegmentDistance;
                specularDistanceSum += path.firstSpecularShare * path.firstSegmentDistance;
                diffuseWeightSum += path.firstDiffuseShare;
                specularWeightSum += path.firstSpecularShare;
            }
            validCount += path.valid ? 1u : 0u;
            depthSum += path.scatteringDepth;
        }

        float inverseSamples = 1.0 / float(samples);
        float meanDiffuseDistance = diffuseWeightSum > 0.0 ? diffuseDistanceSum / diffuseWeightSum : 0.0;
        float meanSpecularDistance = specularWeightSum > 0.0 ? specularDistanceSum / specularWeightSum : 0.0;
        rawDiffuse = diffuseSum * inverseSamples;
        rawSpecular = specularSum * inverseSamples;
        pathData = float4(meanDiffuseDistance, meanSpecularDistance,
            float(validCount) * inverseSamples, float(depthSum) * inverseSamples);
        normalRoughness = float4(primary.N, abs(primary.roughness));
        albedoMetallic = float4(primary.albedo, primary.metallic);
        surfaceData = float4(linearDistance, depth, motionValid ? 3.0 : 1.0, depth >= 0.9 ? 1.0 : 0.0);
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

    u_RawDiffuse[pixel] = float4(rawDiffuse, validPrimary ? 1.0 : 0.0);
    u_RawSpecular[pixel] = float4(rawSpecular, validPrimary ? 1.0 : 0.0);
    u_Emission[pixel] = float4(sourceColor, 1.0);
    u_NormalRoughness[pixel] = normalRoughness;
    u_AlbedoMetallic[pixel] = albedoMetallic;
    u_PathData[pixel] = pathData;
    u_SurfaceData[pixel] = surfaceData;
    u_Motion[pixel] = motion;
}
