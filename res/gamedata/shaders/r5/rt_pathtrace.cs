#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rt_shading.h"

cbuffer PathTracerParams : register(b5) {
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
};

RWTexture2D<float4> g_Accumulation : register(u0);
RWTexture2D<float4> g_Output : register(u1);

static const float3 RT_WATER_TINT = float3(0.7, 0.85, 0.8);
static const uint RT_MAX_WATER_EVENTS = 4;

float3 GenerateCameraRay(uint2 pixel, inout uint rng, out float3 origin)
{
    float2 jitter = float2(rand_float(rng), rand_float(rng));
    float2 uv = (float2(pixel) + jitter) / float2(g_ScreenWidth, g_ScreenHeight);
    float4 clip = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    clip.y = -clip.y;

    float4 nearWorld = mul(g_InvViewProj, clip);
    nearWorld.xyz /= nearWorld.w;

    float4 farClip = float4(clip.xy, 0.0, 1.0);
    float4 farWorld = mul(g_InvViewProj, farClip);
    farWorld.xyz /= farWorld.w;

    origin = g_CameraPos.xyz;
    return normalize(farWorld.xyz - nearWorld.xyz);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenWidth || pixel.y >= (uint)g_ScreenHeight)
        return;

    RTSceneParams scene = RTBuildSceneParams(g_IdentityStaticCount, g_TerrainBatchCount,
        g_SkinnedBatchStart, g_GrassBatchStart, g_DetailAtlasIndex, g_RTLightCount,
        g_DiffuseMode, g_SunDir_Intensity, g_SunColor_SkyWeight);

    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_SampleIndex * 26699u);

    float3 origin;
    float3 direction = GenerateCameraRay(pixel, rng, origin);

    float3 radiance = 0;
    float3 throughput = 1;
    uint bounces = 0;
    uint waterEvents = 0;

    while (bounces <= g_MaxBounces)
    {
        RTSceneTrace trace = RTTraceRay(scene, origin, direction, RT_RAY_DISTANCE, false, rng);
        radiance += throughput * trace.emissive;
        throughput *= trace.transmittance;

        if (!trace.hit)
        {
            radiance += throughput * SampleRTSky(scene, direction);
            break;
        }

        float3 hitPos = origin + direction * trace.t;
        RTHitGeometry geometry = RTFetchHitGeometry(scene, trace, direction);
        RTHitSurface hit = RTResolveHitSurface(scene, trace, geometry);

        if ((hit.flags & MAT_FLAG_WATER) != 0)
        {
            float3 faceN = dot(direction, geometry.geoNormal) < 0.0 ? geometry.normal : -geometry.normal;
            float cosI = saturate(dot(-direction, faceN));
            float fresnel = 0.02 + 0.98 * pow(1.0 - cosI, 5.0);

            if (rand_float(rng) < fresnel)
            {
                origin = hitPos + faceN * RT_RAY_ORIGIN_OFFSET;
                direction = reflect(direction, faceN);
            }
            else
            {
                throughput *= RT_WATER_TINT;
                origin = hitPos - faceN * RT_RAY_ORIGIN_OFFSET;
            }

            if (++waterEvents >= RT_MAX_WATER_EVENTS)
                break;
            continue;
        }

        float3 viewDir = -direction;

        radiance += throughput * hit.surface.emissive;
        if (bounces == g_MaxBounces)
            break;
        radiance += throughput * RTDirectLighting(scene, hit.surface, hitPos, geometry.geoNormal, viewDir);

        RTBSDFSample bounce = RTSampleBSDF(hit.surface, viewDir, scene.diffuseMode, rng);
        if (!bounce.valid)
            break;
        if (hit.surface.shadingClass != SHADING_CLASS_FOLIAGE && dot(bounce.direction, geometry.geoNormal) <= 0.0)
            break;
        throughput *= bounce.weight;

        bounces += 1;
        if (bounces >= 3)
        {
            float p = min(max(throughput.r, max(throughput.g, throughput.b)), 0.95);
            if (p <= 0.0 || rand_float(rng) >= p)
                break;
            throughput /= p;
        }

        if (!all(isfinite(throughput)))
            break;

        float biasSide = dot(bounce.direction, geometry.geoNormal) >= 0.0 ? 1.0 : -1.0;
        origin = hitPos + geometry.geoNormal * (RT_RAY_ORIGIN_OFFSET * biasSide);
        direction = bounce.direction;
    }

    if (!all(isfinite(radiance)))
        radiance = 0.0;

    float4 newSample = float4(radiance, 1.0);

    if (g_SampleIndex == 0) {
        g_Accumulation[pixel] = newSample;
    } else {
        float4 prev = g_Accumulation[pixel];
        g_Accumulation[pixel] = prev + (newSample - prev) / float(g_SampleIndex + 1);
    }

    g_Output[pixel] = float4(g_Accumulation[pixel].rgb, 1.0);
}
