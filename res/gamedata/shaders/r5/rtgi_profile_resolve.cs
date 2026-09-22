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
    RTGIPrimarySurface primary = RTGIDecodePrimary(lane.pixel);
    RTSceneParams scene = RTGIBuildRawScene(lane.pixel);

    if (validPrimary && active)
    {
        bool cont = true;
        while (cont)
            cont = RTIntegratorStep(state, scene, rng);
    }

    RTIntegratorResult result = RTIntegratorFinish(state);
    RTGIProfileStoreState(u_ProfilePaths, lane.pathBase, state, false, validPrimary, rng);

    RTGIAccumulation accumulation = (RTGIAccumulation)0;
    if (validPrimary)
    {
        accumulation = RTGIProfileLoadSums(u_ProfileSums, lane.sumBase);
        RTGIAccumulateSample(accumulation, result, g_GIIntensity);
        RTGIProfileStoreSums(u_ProfileSums, lane.sumBase, accumulation);
    }

    if (g_ProfileSampleIndex + 1u >= RTGISampleCount())
        RTGIWriteRawOutputs(lane.pixel, primary, accumulation);
}
