#define RT_WORLD_CACHE 1
#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rtgi_raw_params.h"
#include "rt_integrator.h"

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

[numthreads(64, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint cell = dispatchID.x;
    if (cell > g_WorldCacheCapacityMask || !RTWorldCacheEnabled())
        return;
    uint life = u_WorldCacheLife[cell];
    if (life == 0u || life >= g_WorldCacheLifetime)
        return;

    uint rng = pcg_hash(cell * 7919u + g_WorldCacheFrame * 48611u + 1u);
    uint liveCells = max(u_WorldCacheStats[0], 1u);
    if (g_WorldCacheUpdateTarget < liveCells && rand_float(rng) * float(liveCells) >= float(g_WorldCacheUpdateTarget))
        return;

    float4 positionDelta = u_WorldCachePosition[cell];
    float3 normal = u_WorldCacheNormal[cell].xyz;
    if (!all(isfinite(positionDelta)) || !all(isfinite(normal)) || dot(normal, normal) < 0.5)
        return;
    normal = normalize(normal);

    RTSceneParams scene = RTWorldCacheBuildScene();
    RTIntegratorSettings settings = RTIntegratorDefaultSettings();
    settings.maxBounces = 1u;
    settings.coneWidth = RTWorldCacheCellSize(RTWorldCacheLod(positionDelta.xyz, false, rng)) * 0.5;
    settings.coneSpread = 1.0;
    settings.cacheBounce = 0u;
    settings.cacheLife = life;
    settings.indirectOnly = true;
    settings.pixelPrimary = false;

    float3 direction = CosineWeightedHemisphere(float2(rand_float(rng), rand_float(rng)), normal);
    float3 origin = positionDelta.xyz + normal * RT_RAY_ORIGIN_OFFSET;
    RTIntegratorResult path = RTIntegratorRunCamera(scene, settings, origin, direction, rng);
    if (!path.valid || !all(isfinite(path.radiance)))
        return;
    float3 sample = max(path.radiance, 0.0);

    float4 old = u_WorldCacheRadiance[cell];
    if (!all(isfinite(old)))
        old = 0.0;
    float maxSamples = max(float(g_WorldCacheMaxSamples), 1.0);
    float count = min(old.a + 1.0, maxSamples);
    float oldLuminance = Luminance(old.rgb);
    float3 blended = lerp(old.rgb, sample, 1.0 / count);
    float delta = lerp(positionDelta.w, Luminance(blended) - oldLuminance, 0.125);

    u_WorldCacheRadiance[cell] = float4(blended, count);
    u_WorldCachePosition[cell] = float4(positionDelta.xyz, delta);
}
