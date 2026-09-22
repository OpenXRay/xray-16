#ifndef RTGI_PROFILE_LIGHTING_H
#define RTGI_PROFILE_LIGHTING_H

#include "rtgi_profile_common.h"

#if !defined(RTGI_PROFILE_LIGHTING_SUN) && !defined(RTGI_PROFILE_LIGHTING_LOCAL_LIGHTS) && \
    !defined(RTGI_PROFILE_LIGHTING_ENVIRONMENT) && !defined(RTGI_PROFILE_LIGHTING_EMISSIVE)
#error "rtgi_profile_lighting.h requires one RTGI_PROFILE_LIGHTING_* group define"
#endif

void RTGIProfileLightingStage(uint3 dispatchID)
{
    RTGIProfileLane lane;
    if (!RTGIProfileResolveLane(dispatchID, lane))
        return;

    uint rng;
    bool active;
    bool validPrimary;
    RTIntegratorState state = RTGIProfileLoadState(u_ProfilePaths, lane.pathBase, rng, active, validPrimary);
    if (!active)
        return;

    RTSceneParams scene = RTGIBuildRawScene(lane.pixel);
    RTHitSurface hit;
    RTHitGeometry geometry;
    float3 hitPosition;
    float segmentDistance;
    RTGIProfileLoadMaterialHit(u_ProfileHits, lane.hitBase, hit, geometry, hitPosition, segmentDistance);

    RTIntegratorLightingContext lighting;
    RTDirectTerms terms = (RTDirectTerms)0;
#if defined(RTGI_PROFILE_LIGHTING_SUN)
    if (!RTIntegratorLightingBegin(state, hit, geometry, hitPosition, lighting))
    {
        RTGIProfileStoreState(u_ProfilePaths, lane.pathBase, state, false, validPrimary, rng);
        return;
    }
    if (lighting.apply)
    {
        RTDirectLightingSun(scene, lighting.surface, lighting.position, lighting.geoNormal, lighting.V,
            lighting.coneWidth, lighting.coneSpread, true, rng, terms);
    }
#else
    RTIntegratorLightingResolve(state, hit, geometry, hitPosition, lighting);
    if (!lighting.apply)
        return;
    terms = RTGIProfileLoadDirectTerms(u_ProfilePaths, lane.pathBase);
#if defined(RTGI_PROFILE_LIGHTING_LOCAL_LIGHTS)
    RTDirectLightingLocalLights(scene, lighting.surface, lighting.position, lighting.geoNormal,
        lighting.V, lighting.coneWidth, lighting.coneSpread, true, terms, state.bounces == 0u);
#elif defined(RTGI_PROFILE_LIGHTING_ENVIRONMENT)
    RTDirectLightingEnvironment(scene, lighting.surface, lighting.position, lighting.geoNormal,
        lighting.V, lighting.coneWidth, lighting.coneSpread, true, rng, terms);
#elif defined(RTGI_PROFILE_LIGHTING_EMISSIVE)
    RTDirectLightingEmissive(scene, lighting.surface, lighting.position, lighting.geoNormal,
        lighting.V, lighting.coneWidth, lighting.coneSpread, true, rng, terms);
    RTDirectLightingSanitize(terms);
    RTIntegratorLightingApply(state, terms);
#endif
#endif

    if (lighting.apply)
        RTGIProfileStoreDirectTerms(u_ProfilePaths, lane.pathBase, terms);
    RTGIProfileStoreState(u_ProfilePaths, lane.pathBase, state, true, validPrimary, rng);
}

#endif
