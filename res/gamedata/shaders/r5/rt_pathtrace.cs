#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rt_shading.h"

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
    float g_TransportPad;
};

RWTexture2D<float4> g_Accumulation : register(u0);
RWTexture2D<float4> g_Output : register(u1);

struct PTDiagnostics
{
    float3 radiance;
    float3 albedo;
    float3 normal;
    float roughness;
    float metallic;
    float3 directDiffuse;
    float3 directSpecular;
    float3 indirectDiffuse;
    float3 indirectSpecular;
    float3 emission;
    float3 coverage;
    uint pathLength;
    bool invalid;
};

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

void PTAddSource(inout PTDiagnostics result, float3 radiance, float3 throughput,
    float3 diffuseThroughput, float3 specularThroughput, uint bounces)
{
    result.radiance += throughput * radiance;
    if (bounces == 0u)
        result.emission += throughput * radiance;
    else if (bounces == 1u)
    {
        result.directDiffuse += diffuseThroughput * radiance;
        result.directSpecular += specularThroughput * radiance;
    }
    else
    {
        result.indirectDiffuse += diffuseThroughput * radiance;
        result.indirectSpecular += specularThroughput * radiance;
    }
}

float3 PTDiagnosticOutput(PTDiagnostics result)
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
    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_SampleIndex * 26699u);
    float3 origin;
    float3 direction = GenerateCameraRay(pixel, rng, origin);
    PTDiagnostics result = (PTDiagnostics)0;
    result.coverage = float3(0.0, 0.0, 1.0);
    float3 throughput = 1.0;
    float3 diffuseThroughput = 0.0;
    float3 specularThroughput = 0.0;
    float3 previousPosition = origin;
    float previousPdf = 0.0;
    bool previousDelta = true;
    float coneWidth = 0.0;
    float coneSpread = g_CameraConeSpread;
    uint bounces = 0u;
    uint nullEvents = 0u;
    bool firstSurface = true;

    while (bounces <= g_MaxBounces)
    {
        result.pathLength = bounces;
        RTSceneTrace trace = RTTraceRay(scene, origin, direction, RT_RAY_DISTANCE, false, rng,
            coneWidth, coneSpread, previousPosition, previousPdf, previousDelta);
        result.invalid = result.invalid || trace.exhausted;
        PTAddSource(result, trace.emissive, throughput, diffuseThroughput, specularThroughput, bounces);
        throughput *= trace.transmittance;
        diffuseThroughput *= trace.transmittance;
        specularThroughput *= trace.transmittance;
        nullEvents += trace.nullEvents;
        if (trace.exhausted || nullEvents >= scene.maxNullEvents)
        {
            result.invalid = true;
            break;
        }
        if (!trace.hit)
        {
            PTAddSource(result, RTMissRadiance(scene, direction, previousPdf, previousDelta),
                throughput, diffuseThroughput, specularThroughput, bounces);
            break;
        }
        float3 hitPosition = origin + direction * trace.t;
        RTHitGeometry geometry = RTFetchHitGeometry(scene, trace, direction);
        RTHitSurface hit = RTResolveHitSurface(scene, trace, geometry);
        coneWidth += coneSpread * trace.t;
        if (!all(isfinite(hit.surface.albedo)) || !all(isfinite(hit.surface.N)) ||
            !isfinite(hit.surface.roughness) || !isfinite(hit.surface.metallic))
        {
            result.invalid = true;
            break;
        }
        if (firstSurface)
        {
            result.albedo = hit.surface.albedo;
            result.normal = hit.surface.N;
            result.roughness = hit.surface.roughness;
            result.metallic = hit.surface.metallic;
            result.coverage = float3(0.0, 1.0, 0.0);
            if (hit.surface.shadingClass == SHADING_CLASS_FOLIAGE)
                result.coverage = float3(1.0, 1.0, 0.0);
            if ((hit.flags & MAT_FLAG_ALPHA_BLEND) != 0u)
                result.coverage = float3(0.0, 1.0, 1.0);
            if ((hit.flags & MAT_FLAG_WATER) != 0u)
                result.coverage = float3(1.0, 0.0, 1.0);
            firstSurface = false;
        }
        float emissionWeight = RTEmissionWeight(scene, trace.batchIdx, trace.info,
            trace.primitiveIndex, previousPosition, hitPosition, previousPdf, previousDelta);
        PTAddSource(result, hit.surface.emissive * emissionWeight, throughput,
            diffuseThroughput, specularThroughput, bounces);
        if (bounces == g_MaxBounces && (hit.flags & MAT_FLAG_WATER) == 0u)
            break;
        float3 V = -direction;
        if ((hit.flags & MAT_FLAG_WATER) == 0u)
        {
            coneSpread = max(coneSpread, max(hit.surface.roughness * hit.surface.roughness, 1.0 - hit.surface.metallic));
            RTDirectTerms direct = RTDirectLightingTerms(scene, hit.surface, hitPosition,
                geometry.geoNormal, V, coneWidth, coneSpread, true, rng);
            result.invalid = result.invalid || direct.invalid;
            float3 directRadiance = direct.diffuse + direct.specular;
            result.radiance += throughput * directRadiance;
            if (bounces == 0u)
            {
                result.directDiffuse += throughput * direct.diffuse;
                result.directSpecular += throughput * direct.specular;
            }
            else
            {
                result.indirectDiffuse += diffuseThroughput * directRadiance;
                result.indirectSpecular += specularThroughput * directRadiance;
            }
        }
        bool passthrough;
        RTBSDFSample bounce = RTSampleSurface(hit, geometry, V, scene.diffuseMode, rng, passthrough);
        if (!bounce.valid)
        {
            result.invalid = result.invalid || !all(isfinite(bounce.weight)) || !isfinite(bounce.pdf);
            break;
        }
        if (!passthrough && bounces == g_MaxBounces)
            break;
        if (!passthrough && hit.surface.shadingClass != SHADING_CLASS_FOLIAGE &&
            dot(bounce.direction, geometry.geoNormal) <= 0.0)
            break;
        if (passthrough)
        {
            throughput *= bounce.weight;
            diffuseThroughput *= bounce.weight;
            specularThroughput *= bounce.weight;
            if (++nullEvents >= scene.maxNullEvents)
            {
                result.invalid = true;
                break;
            }
        }
        else
        {
            if (bounces == 0u)
            {
                diffuseThroughput = throughput * bounce.diffuseWeight;
                specularThroughput = throughput * bounce.specularWeight;
            }
            else
            {
                diffuseThroughput *= bounce.weight;
                specularThroughput *= bounce.weight;
            }
            throughput *= bounce.weight;
            previousPosition = hitPosition;
            previousPdf = bounce.pdf;
            previousDelta = bounce.delta;
            ++bounces;
            if (bounces >= 3u)
            {
                float probability = min(max(throughput.r, max(throughput.g, throughput.b)), 0.95);
                if (!(probability > 0.0) || rand_float(rng) >= probability)
                    break;
                throughput /= probability;
                diffuseThroughput /= probability;
                specularThroughput /= probability;
            }
        }
        if (!all(isfinite(throughput)))
        {
            result.invalid = true;
            break;
        }
        float side = dot(bounce.direction, geometry.geoNormal) >= 0.0 ? 1.0 : -1.0;
        origin = hitPosition + geometry.geoNormal * (RT_RAY_ORIGIN_OFFSET * side);
        direction = bounce.direction;
    }
    if (!all(isfinite(result.radiance)))
    {
        result.invalid = true;
        result.radiance = 0.0;
    }
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
