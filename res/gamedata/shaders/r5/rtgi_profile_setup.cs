#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rtgi_profile_common.h"

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    RTGIProfileLane lane;
    if (!RTGIProfileResolveLane(dispatchID, lane))
        return;

    RTGIPrimarySurface primary = RTGIDecodePrimary(lane.pixel);
    RTSceneParams scene = RTGIBuildRawScene();
    RTIntegratorSettings settings = RTGIBuildRawSettings(primary);

    RTIntegratorState state = RTIntegratorBegin(scene, settings);
    if (primary.valid)
    {
        state.origin = primary.worldPos;
        state.direction = -primary.V;
        state.previousPosition = primary.worldPos;
    }

    uint rng = RTGISampleRng(lane.pixel, g_ProfileSampleIndex);
    RTGIProfileStoreState(u_ProfilePaths, lane.pathBase, state, primary.valid, primary.valid, rng);

    RTHitSurface primaryHit = (RTHitSurface)0;
    primaryHit.surface = primary.surface;
    primaryHit.surface.emissive = 0.0;
    RTHitGeometry primaryGeometry = (RTHitGeometry)0;
    primaryGeometry.geoNormal = primary.surface.N;
    RTGIProfileStoreMaterialHit(u_ProfileHits, lane.hitBase, primaryHit, primaryGeometry,
        primary.worldPos, 0.0);

    if (g_ProfileSampleIndex == 0u)
        RTGIProfileClearSums(u_ProfileSums, lane.sumBase);
}
