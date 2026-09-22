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

    RTSceneParams scene = RTGIBuildRawScene(lane.pixel);
    RTHitSurface hit;
    RTHitGeometry geometry;
    float3 hitPosition;
    float segmentDistance;
    RTGIProfileLoadMaterialHit(u_ProfileHits, lane.hitBase, hit, geometry, hitPosition, segmentDistance);

    bool cont = RTIntegratorAdvanceStage(state, scene, hit, geometry, hitPosition, segmentDistance, rng);
    RTGIProfileStoreState(u_ProfilePaths, lane.pathBase, state, cont, validPrimary, rng);
}
