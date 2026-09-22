#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rt_integrator.h"

cbuffer PathTracerParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_SkyWeight;
    float g_ScreenWidth;
    float g_ScreenHeight;
    uint g_SampleIndex;
    uint g_MaxBounces;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_TransparentBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint g_DiffuseMode;
    uint g_RTLightCount;
    uint g_DiagnosticMode;
    uint g_MaxNullEvents;
    uint g_EmissiveCount;
    uint g_MaxSamples;
    float g_EnvironmentRotation;
    float g_SunAngularRadius;
    float g_CameraConeSpread;
    uint g_ClusterLights;
    uint g_DetailMeshBatchStart;
    uint g_StaticDetailBatchStart;
    uint g_DetailPbrIndex;
    uint g_DetailBumpIndex;
};

RWTexture2D<float4> g_Accumulation : register(u0);
RWTexture2D<float4> g_Output : register(u1);

float3 GenerateCameraRay(uint2 pixel, inout uint rng, out float3 origin)
{
    float2 jitter = float2(rand_float(rng), rand_float(rng));
    float2 uv = (float2(pixel) + jitter) / float2(g_ScreenWidth, g_ScreenHeight);
    float4 clip = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    clip.y = -clip.y;
    float4 nearWorld = mul(g_InvViewProj, clip);
    nearWorld.xyz /= nearWorld.w;
    float4 farWorld = mul(g_InvViewProj, float4(clip.xy, 0.0, 1.0));
    farWorld.xyz /= farWorld.w;
    origin = g_CameraPos.xyz;
    return RTSafeNormalize(farWorld.xyz - nearWorld.xyz, float3(0.0, 0.0, 1.0));
}

float3 PTDiagnosticOutput(RTIntegratorResult result)
{
    switch (g_DiagnosticMode)
    {
    case 1u: return result.albedo;
    case 2u: return result.normal * 0.5 + 0.5;
    case 3u: return result.roughness.xxx;
    case 4u: return result.metallic.xxx;
    case 5u: return result.directDiffuse;
    case 6u: return result.directSpecular;
    case 7u: return result.indirectDiffuse;
    case 8u: return result.indirectSpecular;
    case 9u: return result.emission;
    case 10u: return float3(float(result.pathLength) / 16.0, float(g_MaxBounces) / 16.0, 0.0);
    case 11u: return result.invalid ? float3(1.0, 0.0, 1.0) : float3(0.0, 0.0, 0.0);
    case 13u: return result.coverage;
    default: return result.radiance;
    }
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= uint(g_ScreenWidth) || pixel.y >= uint(g_ScreenHeight))
        return;
    if (g_MaxSamples > 0u && g_SampleIndex >= g_MaxSamples)
    {
        g_Output[pixel] = float4(g_Accumulation[pixel].rgb, 1.0);
        return;
    }

    RTSceneParams scene = RTBuildSceneParams(g_IdentityStaticCount, g_TerrainBatchCount,
        g_SkinnedBatchStart, g_GrassBatchStart, g_DetailAtlasIndex, g_RTLightCount,
        g_DiffuseMode, g_SunDir_Intensity, g_SunColor_SkyWeight, g_EmissiveCount,
        g_MaxNullEvents, g_EnvironmentRotation, g_SunAngularRadius);
    scene.detailMeshBatchStart = g_DetailMeshBatchStart;
    scene.staticDetailBatchStart = g_StaticDetailBatchStart;
    scene.detailPbrIndex = g_DetailPbrIndex;
    scene.detailBumpIndex = g_DetailBumpIndex;
    scene.clusterLights = g_ClusterLights;
    scene.clusterPixel = pixel;
    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_SampleIndex * 26699u);
    float3 origin;
    float3 direction = GenerateCameraRay(pixel, rng, origin);

    RTIntegratorSettings settings;
    settings.maxBounces = g_MaxBounces;
    settings.coneWidth = 0.0;
    settings.coneSpread = g_CameraConeSpread;
    settings.trackDiagnostics = true;
    settings.trackSegmentMetrics = false;
    settings.allowHudFirstRay = true;
    RTIntegratorResult result = RTIntegratorRunCamera(scene, settings, origin, direction, rng);

    float3 diagnostic = PTDiagnosticOutput(result);
    if (!all(isfinite(diagnostic)))
        diagnostic = 0.0;
    float4 newSample = float4(diagnostic, 1.0);
    if (g_SampleIndex > 0u)
    {
        float4 previous = g_Accumulation[pixel];
        newSample = previous + (newSample - previous) / float(g_SampleIndex + 1u);
    }
    g_Accumulation[pixel] = newSample;
    g_Output[pixel] = float4(newSample.rgb, 1.0);
}
