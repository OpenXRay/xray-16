#ifndef RT_INTEGRATOR_H
#define RT_INTEGRATOR_H

#include "rt_shading.h"
#ifdef RT_WORLD_CACHE
#include "rt_world_cache.h"
#endif

#define RT_INTEGRATOR_RR_DEPTH 3u
#define RT_INTEGRATOR_RR_CAP 0.95
#define RT_INTEGRATOR_STAGE_DONE 0u
#define RT_INTEGRATOR_STAGE_READY 1u
#define RT_INTEGRATOR_NO_CACHE 0xFFFFFFFFu

struct RTIntegratorSettings
{
    uint maxBounces;
    float coneWidth;
    float coneSpread;
    bool trackDiagnostics;
    bool trackSegmentMetrics;
    bool allowHudFirstRay;
    uint cacheBounce;
    uint cacheLife;
    bool cacheInsert;
    float roughnessFloor;
    bool indirectOnly;
    bool pixelPrimary;
};

RTIntegratorSettings RTIntegratorDefaultSettings()
{
    RTIntegratorSettings settings;
    settings.maxBounces = 1u;
    settings.coneWidth = 0.0;
    settings.coneSpread = 0.0;
    settings.trackDiagnostics = false;
    settings.trackSegmentMetrics = false;
    settings.allowHudFirstRay = false;
    settings.cacheBounce = RT_INTEGRATOR_NO_CACHE;
    settings.cacheLife = 0u;
    settings.cacheInsert = false;
    settings.roughnessFloor = 0.0;
    settings.indirectOnly = false;
    settings.pixelPrimary = true;
    return settings;
}

struct RTIntegratorResult
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
    uint scatteringDepth;
    float firstSegmentDistance;
    float firstDiffuseShare;
    float firstSpecularShare;
    bool firstSegmentDefined;
    bool valid;
    bool invalid;
    uint4 cacheEvents;
};

struct RTIntegratorState
{
    RTIntegratorSettings settings;
    RTIntegratorResult result;
    float3 origin;
    float3 direction;
    float3 throughput;
    float3 diffuseThroughput;
    float3 specularThroughput;
    float3 previousPosition;
    float previousPdf;
    float coneWidth;
    float coneSpread;
    float remainingReach;
    float diffuseLobeShare;
    float specularLobeShare;
    uint bounces;
    uint nullEvents;
    bool previousDelta;
    bool firstRayTraced;
    bool firstSurfaceRecorded;
    bool pendingFirstSegment;
};

RTIntegratorState RTIntegratorBegin(RTSceneParams scene, RTIntegratorSettings settings)
{
    RTIntegratorState s;
    s.settings = settings;
    s.result = (RTIntegratorResult)0;
    s.result.coverage = float3(0.0, 0.0, 1.0);
    s.origin = 0.0;
    s.direction = 0.0;
    s.throughput = 1.0;
    s.diffuseThroughput = 0.0;
    s.specularThroughput = 0.0;
    s.previousPosition = 0.0;
    s.previousPdf = 0.0;
    s.coneWidth = settings.coneWidth;
    s.coneSpread = settings.coneSpread;
    s.remainingReach = max(scene.rayDistance, 0.0);
    s.diffuseLobeShare = 0.0;
    s.specularLobeShare = 0.0;
    s.bounces = 0u;
    s.nullEvents = 0u;
    s.previousDelta = true;
    s.firstRayTraced = false;
    s.firstSurfaceRecorded = false;
    s.pendingFirstSegment = settings.trackSegmentMetrics;
    return s;
}

void RTIntegratorAddSource(inout RTIntegratorState s, float3 radiance)
{
    s.result.radiance += s.throughput * radiance;
    if (s.bounces == 0u)
    {
        s.result.emission += s.throughput * radiance;
    }
    else if (s.bounces == 1u)
    {
        s.result.directDiffuse += s.diffuseThroughput * radiance;
        s.result.directSpecular += s.specularThroughput * radiance;
    }
    else
    {
        s.result.indirectDiffuse += s.diffuseThroughput * radiance;
        s.result.indirectSpecular += s.specularThroughput * radiance;
    }
}

void RTIntegratorRecordSurface(inout RTIntegratorState s, RTHitSurface hit)
{
    s.result.albedo = hit.surface.albedo;
    s.result.normal = hit.surface.N;
    s.result.roughness = hit.surface.roughness;
    s.result.metallic = hit.surface.metallic;
    s.result.coverage = float3(0.0, 1.0, 0.0);
    if (hit.surface.shadingClass == SHADING_CLASS_FOLIAGE)
        s.result.coverage = float3(1.0, 1.0, 0.0);
    if ((hit.flags & MAT_FLAG_ALPHA_BLEND) != 0u)
        s.result.coverage = float3(0.0, 1.0, 1.0);
    if ((hit.flags & MAT_FLAG_WATER) != 0u)
        s.result.coverage = float3(1.0, 0.0, 1.0);
}

void RTIntegratorRecordSegment(inout RTIntegratorState s, float segmentDistance)
{
    if (!s.settings.trackSegmentMetrics || !s.pendingFirstSegment)
        return;
    s.pendingFirstSegment = false;
    s.result.firstSegmentDistance = isfinite(segmentDistance) ? max(segmentDistance, 0.0) : 0.0;
    s.result.firstDiffuseShare = s.diffuseLobeShare;
    s.result.firstSpecularShare = s.specularLobeShare;
    s.result.firstSegmentDefined = true;
}

struct RTIntegratorLightingContext
{
    bool apply;
    float3 V;
    MaterialSurface surface;
    float3 position;
    float3 geoNormal;
    float coneWidth;
    float coneSpread;
};

void RTIntegratorLightingResolve(RTIntegratorState s, RTHitSurface hit, RTHitGeometry geometry,
    float3 hitPosition, out RTIntegratorLightingContext lighting)
{
    lighting.apply = (hit.flags & MAT_FLAG_WATER) == 0u;
    lighting.V = -s.direction;
    lighting.surface = hit.surface;
    lighting.position = hitPosition;
    lighting.geoNormal = geometry.geoNormal;
    lighting.coneWidth = s.coneWidth;
    lighting.coneSpread = s.coneSpread;
}

bool RTIntegratorLightingBegin(inout RTIntegratorState s, RTHitSurface hit, RTHitGeometry geometry,
    float3 hitPosition, out RTIntegratorLightingContext lighting)
{
    RTIntegratorLightingResolve(s, hit, geometry, hitPosition, lighting);
    if (s.bounces == s.settings.maxBounces && lighting.apply)
        return false;
    if (lighting.apply)
    {
        s.coneSpread = max(s.coneSpread, max(hit.surface.roughness * hit.surface.roughness, 1.0 - hit.surface.metallic));
        lighting.coneSpread = s.coneSpread;
    }
    return true;
}

void RTIntegratorLightingApply(inout RTIntegratorState s, RTDirectTerms direct)
{
    s.result.invalid = s.result.invalid || direct.invalid;
    float3 directRadiance = direct.diffuse + direct.specular;
    s.result.radiance += s.throughput * directRadiance;
    if (s.bounces == 0u)
    {
        s.result.directDiffuse += s.throughput * direct.diffuse;
        s.result.directSpecular += s.throughput * direct.specular;
    }
    else
    {
        s.result.indirectDiffuse += s.diffuseThroughput * directRadiance;
        s.result.indirectSpecular += s.specularThroughput * directRadiance;
    }
}

bool RTIntegratorLightingStage(inout RTIntegratorState s, RTSceneParams scene, RTHitSurface hit,
    RTHitGeometry geometry, float3 hitPosition, inout uint rng)
{
    RTIntegratorLightingContext lighting;
    if (!RTIntegratorLightingBegin(s, hit, geometry, hitPosition, lighting))
        return false;
    if (!lighting.apply)
        return true;

    RTDirectTerms direct = RTDirectLightingTerms(scene, lighting.surface, lighting.position,
        lighting.geoNormal, lighting.V, lighting.coneWidth, lighting.coneSpread, true, rng,
        s.bounces == 0u && s.settings.pixelPrimary);
    RTIntegratorLightingApply(s, direct);
    return true;
}

bool RTIntegratorAdvanceStage(inout RTIntegratorState s, RTSceneParams scene, RTHitSurface hit,
    RTHitGeometry geometry, float3 hitPosition, float segmentDistance, inout uint rng)
{
    float3 V = -s.direction;
    bool passthrough;
    RTBSDFSample bounce = RTSampleSurface(hit, geometry, V, scene.diffuseMode, rng, passthrough);
    if (!bounce.valid)
    {
        s.result.invalid = s.result.invalid || !all(isfinite(bounce.weight)) || !isfinite(bounce.pdf);
        return false;
    }
    if (!passthrough && s.bounces == s.settings.maxBounces)
        return false;
    if (!passthrough && hit.surface.shadingClass != SHADING_CLASS_FOLIAGE &&
        dot(bounce.direction, geometry.geoNormal) <= 0.0)
        return false;

    if (passthrough)
    {
        s.throughput *= bounce.weight;
        s.diffuseThroughput *= bounce.weight;
        s.specularThroughput *= bounce.weight;
        s.remainingReach = max(0.0, s.remainingReach - max(segmentDistance, 0.0));
        if (++s.nullEvents >= scene.maxNullEvents)
        {
            s.result.invalid = true;
            return false;
        }
    }
    else
    {
        if (s.bounces == 0u)
        {
            s.diffuseThroughput = s.throughput * bounce.diffuseWeight;
            s.specularThroughput = s.throughput * bounce.specularWeight;
            float diffuseLuminance = Luminance(bounce.diffuseWeight);
            float specularLuminance = Luminance(bounce.specularWeight);
            float totalLuminance = diffuseLuminance + specularLuminance;
            if (s.settings.trackSegmentMetrics && totalLuminance > 0.0)
            {
                s.diffuseLobeShare = diffuseLuminance / totalLuminance;
                s.specularLobeShare = specularLuminance / totalLuminance;
            }
        }
        else
        {
            s.diffuseThroughput *= bounce.weight;
            s.specularThroughput *= bounce.weight;
        }
        s.throughput *= bounce.weight;
        s.previousPosition = hitPosition;
        s.previousPdf = bounce.pdf;
        s.previousDelta = bounce.delta;
        s.remainingReach = max(scene.rayDistance, 0.0);
        ++s.bounces;
        if (s.bounces >= RT_INTEGRATOR_RR_DEPTH)
        {
            float probability = min(max(s.throughput.r, max(s.throughput.g, s.throughput.b)), RT_INTEGRATOR_RR_CAP);
            if (!(probability > 0.0) || rand_float(rng) >= probability)
                return false;
            s.throughput /= probability;
            s.diffuseThroughput /= probability;
            s.specularThroughput /= probability;
        }
    }

    if (!all(isfinite(s.throughput)))
    {
        s.result.invalid = true;
        return false;
    }

    float side = dot(bounce.direction, geometry.geoNormal) >= 0.0 ? 1.0 : -1.0;
    s.origin = hitPosition + geometry.geoNormal * (RT_RAY_ORIGIN_OFFSET * side);
    s.direction = bounce.direction;
    return true;
}

#ifdef RT_WORLD_CACHE
float RTIntegratorCacheEligibility(RTHitSurface hit)
{
    if ((hit.flags & MAT_FLAG_WATER) != 0u)
        return 0.0;
    return RTWorldCacheRoughnessEligibility(hit.surface.roughness);
}

bool RTIntegratorCacheLobeAccepted(RTIntegratorState s)
{
    return s.bounces == 0u || (!s.previousDelta && s.previousPdf <= RT_WORLD_CACHE_MAX_LOBE_PDF);
}

void RTIntegratorCacheEvent(inout RTIntegratorState s, uint eventIndex)
{
    s.result.cacheEvents += uint4(eventIndex == 0u, eventIndex == 1u, eventIndex == 2u, eventIndex == 3u);
}

bool RTIntegratorCacheResolve(inout RTIntegratorState s, RTHitSurface hit, RTHitGeometry geometry,
    float3 hitPosition, inout uint rng, out RTWorldCacheLookup lookup)
{
    lookup = RTWorldCacheEmptyLookup();
    if (!RTWorldCacheEnabled() || s.bounces < s.settings.cacheBounce || s.bounces >= s.settings.maxBounces)
        return false;
    float eligibility = RTIntegratorCacheLobeAccepted(s) ? RTIntegratorCacheEligibility(hit) : 0.0;
    bool eligible = eligibility >= 1.0 || (eligibility > 0.0 && rand_float(rng) < eligibility);
    if (!eligible)
    {
        RTIntegratorCacheEvent(s, RT_WORLD_CACHE_EVENT_BYPASSED);
        return false;
    }
    lookup = RTWorldCacheQuery(hitPosition, geometry.geoNormal, s.settings.cacheLife, s.settings.cacheInsert,
        true, rng);
    if (lookup.state == RT_WORLD_CACHE_LOOKUP_VALID)
    {
        RTIntegratorCacheEvent(s, RT_WORLD_CACHE_EVENT_SUBSTITUTED);
        return true;
    }
    RTIntegratorCacheEvent(s, lookup.state == RT_WORLD_CACHE_LOOKUP_ABSENT ?
        RT_WORLD_CACHE_EVENT_ABSENT : RT_WORLD_CACHE_EVENT_UNSAMPLED);
    return false;
}

void RTIntegratorCacheApply(inout RTIntegratorState s, RTHitSurface hit, RTWorldCacheLookup lookup)
{
    float3 V = -s.direction;
    float NdotV = saturate(dot(hit.surface.N, V));
    float roughness = saturate(hit.surface.roughness);
    float3 F0 = CalculateF0(hit.surface.albedo, hit.surface.metallic);
    float3 diffuse = hit.surface.albedo * (1.0 - hit.surface.metallic) * lookup.radiance;
    float3 specular = EnvBRDFApprox(F0, roughness, NdotV) * lookup.radiance;
    if (!all(isfinite(diffuse)))
        diffuse = 0.0;
    if (!all(isfinite(specular)))
        specular = 0.0;
    float3 indirect = diffuse + specular;
    s.result.radiance += s.throughput * indirect;
    if (s.bounces == 0u)
    {
        s.result.indirectDiffuse += s.throughput * diffuse;
        s.result.indirectSpecular += s.throughput * specular;
    }
    else
    {
        s.result.indirectDiffuse += s.diffuseThroughput * indirect;
        s.result.indirectSpecular += s.specularThroughput * indirect;
    }
}
#endif

bool RTIntegratorContinue(inout RTIntegratorState s, RTSceneParams scene, RTHitSurface hit,
    RTHitGeometry geometry, float3 hitPosition, float segmentDistance, inout uint rng)
{
#ifdef RT_WORLD_CACHE
    if (s.settings.roughnessFloor > 0.0)
        hit.surface.roughness = max(hit.surface.roughness, s.settings.roughnessFloor);
    RTWorldCacheLookup cache;
    bool substitute = RTIntegratorCacheResolve(s, hit, geometry, hitPosition, rng, cache);
#endif
    if (!RTIntegratorLightingStage(s, scene, hit, geometry, hitPosition, rng))
        return false;
#ifdef RT_WORLD_CACHE
    if (substitute)
    {
        RTIntegratorCacheApply(s, hit, cache);
        s.settings.maxBounces = s.bounces + 1u;
    }
#endif
    return RTIntegratorAdvanceStage(s, scene, hit, geometry, hitPosition, segmentDistance, rng);
}

uint RTIntegratorTraceStage(inout RTIntegratorState s, RTSceneParams scene, inout uint rng,
    out RTSceneTrace trace, out float3 hitPosition)
{
    s.result.pathLength = s.bounces;
    uint rayMask = (s.settings.allowHudFirstRay && !s.firstRayTraced)
        ? (RT_RAY_MASK_WORLD | RT_RAY_MASK_HUD) : RT_RAY_MASK_WORLD;
    s.firstRayTraced = true;
    trace = RTTraceRay(scene, s.origin, s.direction, s.remainingReach, false, rng,
        s.coneWidth, s.coneSpread, s.previousPosition, s.previousPdf, s.previousDelta, rayMask);
    s.result.invalid = s.result.invalid || trace.exhausted;
    bool skipSources = s.settings.indirectOnly && s.bounces == 0u;
    if (!skipSources)
        RTIntegratorAddSource(s, trace.emissive);
    s.throughput *= trace.transmittance;
    s.diffuseThroughput *= trace.transmittance;
    s.specularThroughput *= trace.transmittance;
    s.nullEvents += trace.nullEvents;
    if (s.settings.trackSegmentMetrics && s.pendingFirstSegment)
        RTIntegratorRecordSegment(s, trace.hit ? trace.t : s.remainingReach);

    if (trace.exhausted || s.nullEvents >= scene.maxNullEvents)
    {
        s.result.invalid = true;
        return RT_INTEGRATOR_STAGE_DONE;
    }
    if (!trace.hit)
    {
        if (!skipSources)
            RTIntegratorAddSource(s, RTMissRadiance(scene, s.direction, s.previousPdf, s.previousDelta));
        return RT_INTEGRATOR_STAGE_DONE;
    }

    hitPosition = s.origin + s.direction * trace.t;
    return RT_INTEGRATOR_STAGE_READY;
}

uint RTIntegratorMaterialStage(inout RTIntegratorState s, RTSceneParams scene, RTSceneTrace trace,
    float3 hitPosition, out RTHitGeometry geometry, out RTHitSurface hit)
{
    geometry = RTFetchHitGeometry(scene, trace, s.direction);
    hit = RTResolveHitSurface(scene, trace, geometry);
    s.coneWidth += s.coneSpread * trace.t;
    if (!all(isfinite(hit.surface.albedo)) || !all(isfinite(hit.surface.N)) ||
        !isfinite(hit.surface.roughness) || !isfinite(hit.surface.metallic))
    {
        s.result.invalid = true;
        return RT_INTEGRATOR_STAGE_DONE;
    }
    if (s.settings.trackDiagnostics && !s.firstSurfaceRecorded)
    {
        RTIntegratorRecordSurface(s, hit);
        s.firstSurfaceRecorded = true;
    }

    if (any(hit.surface.emissive > 0.0) && !(s.settings.indirectOnly && s.bounces == 0u))
    {
        float emissionWeight = RTEmissionWeightFromArea(scene, trace.batchIdx, geometry.areaNormal,
            s.previousPosition, hitPosition, s.previousPdf, s.previousDelta);
        RTIntegratorAddSource(s, hit.surface.emissive * emissionWeight);
    }
    return RT_INTEGRATOR_STAGE_READY;
}

bool RTIntegratorStep(inout RTIntegratorState s, RTSceneParams scene, inout uint rng)
{
    RTSceneTrace trace;
    float3 hitPosition;
    if (RTIntegratorTraceStage(s, scene, rng, trace, hitPosition) != RT_INTEGRATOR_STAGE_READY)
        return false;
    RTHitGeometry geometry;
    RTHitSurface hit;
    if (RTIntegratorMaterialStage(s, scene, trace, hitPosition, geometry, hit) != RT_INTEGRATOR_STAGE_READY)
        return false;
    return RTIntegratorContinue(s, scene, hit, geometry, hitPosition, trace.t, rng);
}

RTIntegratorResult RTIntegratorFinish(inout RTIntegratorState s)
{
    if (!all(isfinite(s.result.radiance)))
    {
        s.result.invalid = true;
        s.result.radiance = 0.0;
    }
    s.result.scatteringDepth = s.result.pathLength;
    s.result.valid = !s.result.invalid;
    if (!isfinite(s.result.firstSegmentDistance))
    {
        s.result.firstSegmentDistance = 0.0;
        s.result.firstDiffuseShare = 0.0;
        s.result.firstSpecularShare = 0.0;
    }
    return s.result;
}

RTIntegratorResult RTIntegratorRunCamera(RTSceneParams scene, RTIntegratorSettings settings,
    float3 origin, float3 direction, inout uint rng)
{
    RTIntegratorState s = RTIntegratorBegin(scene, settings);
    s.origin = origin;
    s.direction = direction;
    s.previousPosition = origin;
    bool cont = true;
    while (cont)
        cont = RTIntegratorStep(s, scene, rng);
    return RTIntegratorFinish(s);
}

RTIntegratorResult RTIntegratorRunPrimary(RTSceneParams scene, RTIntegratorSettings settings,
    MaterialSurface primary, float3 position, float3 normal, float3 V, inout uint rng)
{
    RTIntegratorState s = RTIntegratorBegin(scene, settings);
    s.origin = position;
    s.direction = -V;
    s.previousPosition = position;
    RTHitSurface primaryHit = (RTHitSurface)0;
    primaryHit.surface = primary;
    primaryHit.surface.emissive = 0.0;
    RTHitGeometry primaryGeometry = (RTHitGeometry)0;
    primaryGeometry.normal = normal;
    primaryGeometry.geoNormal = normal;
    primaryGeometry.tangent = 0.0;
    primaryGeometry.bitangent = 0.0;
    primaryGeometry.uv = 0.0;
    primaryGeometry.uvDx = 0.0;
    primaryGeometry.uvDy = 0.0;
    bool cont = RTIntegratorContinue(s, scene, primaryHit, primaryGeometry, position, 0.0, rng);
    while (cont)
        cont = RTIntegratorStep(s, scene, rng);
    return RTIntegratorFinish(s);
}

#endif
