#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rtgi_profile_common.h"

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
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

    RTSceneParams scene = RTGIBuildRawScene();
    float3 hitPosition;
    RTSceneTrace trace = RTGIProfileLoadTraceHit(u_ProfileHits, lane.hitBase, hitPosition);
    RTHitGeometry geometry;
    RTHitSurface hit;
    uint stage = RTIntegratorMaterialStage(state, scene, trace, hitPosition, geometry, hit);
    RTGIProfileStoreState(u_ProfilePaths, lane.pathBase, state, stage == RT_INTEGRATOR_STAGE_READY,
        validPrimary, rng);
    if (stage != RT_INTEGRATOR_STAGE_READY)
        return;

    RTGIProfileStoreMaterialHit(u_ProfileHits, lane.hitBase, hit, geometry, hitPosition, trace.t);
}
