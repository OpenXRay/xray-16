#define RT_WORLD_CACHE 1
#define RT_WORLD_CACHE_UPDATE 1
#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rtgi_raw_params.h"
#include "rt_integrator.h"

#define RT_WORLD_CACHE_MIN_HISTORY 8.0
#define RT_WORLD_CACHE_FAST_ALPHA 0.25
#define RT_WORLD_CACHE_CHANGE_SIGMA 2.0
#define RT_WORLD_CACHE_CHANGE_FLOOR 0.02
#define RT_WORLD_CACHE_CHANGE_INPUT_SIGMA 3.0
#define RT_WORLD_CACHE_CLAMP_RATIO 4.0
#define RT_WORLD_CACHE_CLAMP_SIGMA 2.0
#define RT_WORLD_CACHE_CLAMP_FLOOR 0.5

RTSceneParams RTWorldCacheBuildScene()
{
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
    scene.clusterLights = g_ClusterLights;
    scene.clusterPixel = uint2(0u, 0u);
    scene.lightRays = max(g_LightRays, 2u);
    return scene;
}

float2 RTWorldCacheStratifiedSample(uint cell, uint frame)
{
    uint index = frame & 4095u;
    float2 rotation = float2(float(pcg_hash(cell) & 0xFFFFu), float(pcg_hash(cell + 0x9E3779B9u) & 0xFFFFu)) / 65536.0;
    return frac(float2(0.7548776662, 0.5698402909) * float(index) + rotation);
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint cell = dispatchID.x;
    if (cell > g_WorldCacheCapacityMask || !RTWorldCacheEnabled())
        return;
    uint life = u_WorldCacheLife[cell];
    if (life == 0u || life >= g_WorldCacheLifetime)
        return;

    float4 old = t_WorldCacheRadianceInput[cell];
    if (!all(isfinite(old)))
        old = 0.0;
    uint rng = pcg_hash(cell * 7919u + g_WorldCacheFrame * 48611u + 1u);
    uint liveCells = max(u_WorldCacheStats[RT_WORLD_CACHE_STAT_LIVE], 1u);
    float priority = (life + 1u >= g_WorldCacheLifetime ? 1.0 : 0.5) * (old.a < RT_WORLD_CACHE_MIN_HISTORY ? 2.0 : 1.0);
    float acceptance = float(g_WorldCacheUpdateTarget) / float(liveCells) * priority;
    if (acceptance < 1.0 && rand_float(rng) >= acceptance)
        return;

    float4 positionFast = u_WorldCachePosition[cell];
    float4 normalMoment = u_WorldCacheNormal[cell];
    float3 normal = normalMoment.xyz;
    if (!all(isfinite(positionFast)) || !all(isfinite(normalMoment)) || dot(normal, normal) < 0.5)
        return;
    normal = normalize(normal);

    RTSceneParams scene = RTWorldCacheBuildScene();
    RTIntegratorSettings settings = RTIntegratorDefaultSettings();
    settings.maxBounces = 1u;
    settings.coneWidth = RTWorldCacheCellSize(RTWorldCacheLod(positionFast.xyz, false, rng)) * 0.5;
    settings.coneSpread = 1.0;
    settings.cacheBounce = 0u;
    settings.cacheLife = life;
    settings.cacheInsert = false;
    settings.indirectOnly = true;
    settings.roughnessFloor = RT_WORLD_CACHE_SPECULAR_ROUGHNESS_MAX;
    settings.pixelPrimary = false;

    float3 direction = CosineWeightedHemisphere(RTWorldCacheStratifiedSample(cell, g_WorldCacheFrame), normal);
    float3 origin = positionFast.xyz + normal * RT_RAY_ORIGIN_OFFSET;
    RTIntegratorResult path = RTIntegratorRunCamera(scene, settings, origin, direction, rng);
    if (!path.valid || !all(isfinite(path.radiance)))
        return;
    float3 sample = max(path.radiance, 0.0);
    float sampleLuminance = Luminance(sample);

    float maxSamples = max(float(g_WorldCacheMaxSamples), 1.0);
    float slowLuminance = Luminance(old.rgb);
    float sigma = sqrt(max(normalMoment.w - slowLuminance * slowLuminance, 0.0));
    if (old.a >= 1.0)
    {
        float ceiling = max(RT_WORLD_CACHE_CLAMP_RATIO * (slowLuminance + RT_WORLD_CACHE_CLAMP_SIGMA * sigma),
            RT_WORLD_CACHE_CLAMP_FLOOR);
        if (sampleLuminance > ceiling)
        {
            sample *= ceiling / sampleLuminance;
            sampleLuminance = ceiling;
        }
    }

    bool established = old.a >= RT_WORLD_CACHE_MIN_HISTORY;
    float changeBand = RT_WORLD_CACHE_CHANGE_INPUT_SIGMA * sigma + 2.0 * RT_WORLD_CACHE_CHANGE_FLOOR;
    float changeLuminance = established
        ? clamp(sampleLuminance, slowLuminance - changeBand, slowLuminance + changeBand) : sampleLuminance;
    float fastLuminance = old.a > 0.0 ? lerp(positionFast.w, changeLuminance, RT_WORLD_CACHE_FAST_ALPHA) : sampleLuminance;
    float count = min(old.a + 1.0, maxSamples);
    if (established &&
        abs(fastLuminance - slowLuminance) > RT_WORLD_CACHE_CHANGE_SIGMA * sigma + RT_WORLD_CACHE_CHANGE_FLOOR)
        count = RT_WORLD_CACHE_MIN_HISTORY;
    float blend = 1.0 / count;
    float3 blended = lerp(old.rgb, sample, blend);
    float moment = lerp(normalMoment.w, sampleLuminance * sampleLuminance, blend);

    u_WorldCacheRadiance[cell] = float4(blended, count);
    u_WorldCachePosition[cell] = float4(positionFast.xyz, fastLuminance);
    u_WorldCacheNormal[cell] = float4(normalMoment.xyz, moment);
}
