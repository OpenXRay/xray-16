#ifndef RT_INTEGRATOR_H
#define RT_INTEGRATOR_H

#include "rt_shading.h"

#define RT_INTEGRATOR_RR_DEPTH 3u
#define RT_INTEGRATOR_RR_CAP 0.95

struct RTIntegratorSettings
{
    uint maxBounces;
    float coneWidth;
    float coneSpread;
    bool trackDiagnostics;
    bool trackSegmentMetrics;
    bool allowHudFirstRay;
};

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

bool RTIntegratorContinue(inout RTIntegratorState s, RTSceneParams scene, RTHitSurface hit,
    RTHitGeometry geometry, float3 hitPosition, float segmentDistance, inout uint rng)
{
    if (s.bounces == s.settings.maxBounces && (hit.flags & MAT_FLAG_WATER) == 0u)
        return false;

    float3 V = -s.direction;
    if ((hit.flags & MAT_FLAG_WATER) == 0u)
    {
        s.coneSpread = max(s.coneSpread, max(hit.surface.roughness * hit.surface.roughness, 1.0 - hit.surface.metallic));
        RTDirectTerms direct = RTDirectLightingTerms(scene, hit.surface, hitPosition,
            geometry.geoNormal, V, s.coneWidth, s.coneSpread, true, rng);
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

bool RTIntegratorStep(inout RTIntegratorState s, RTSceneParams scene, inout uint rng)
{
    s.result.pathLength = s.bounces;
    uint rayMask = (s.settings.allowHudFirstRay && !s.firstRayTraced)
        ? (RT_RAY_MASK_WORLD | RT_RAY_MASK_HUD) : RT_RAY_MASK_WORLD;
    s.firstRayTraced = true;
    RTSceneTrace trace = RTTraceRay(scene, s.origin, s.direction, s.remainingReach, false, rng,
        s.coneWidth, s.coneSpread, s.previousPosition, s.previousPdf, s.previousDelta, rayMask);
    s.result.invalid = s.result.invalid || trace.exhausted;
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
        return false;
    }
    if (!trace.hit)
    {
        RTIntegratorAddSource(s, RTMissRadiance(scene, s.direction, s.previousPdf, s.previousDelta));
        return false;
    }

    float3 hitPosition = s.origin + s.direction * trace.t;
    RTHitGeometry geometry = RTFetchHitGeometry(scene, trace, s.direction);
    RTHitSurface hit = RTResolveHitSurface(scene, trace, geometry);
    s.coneWidth += s.coneSpread * trace.t;
    if (!all(isfinite(hit.surface.albedo)) || !all(isfinite(hit.surface.N)) ||
        !isfinite(hit.surface.roughness) || !isfinite(hit.surface.metallic))
    {
        s.result.invalid = true;
        return false;
    }
    if (s.settings.trackDiagnostics && !s.firstSurfaceRecorded)
    {
        RTIntegratorRecordSurface(s, hit);
        s.firstSurfaceRecorded = true;
    }

    float emissionWeight = RTEmissionWeight(scene, trace.batchIdx, trace.info, trace.primitiveIndex,
        s.previousPosition, hitPosition, s.previousPdf, s.previousDelta);
    RTIntegratorAddSource(s, hit.surface.emissive * emissionWeight);
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
