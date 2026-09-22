#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rtgi_trace_common.h"

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= uint(g_ScreenWidth) || pixel.y >= uint(g_ScreenHeight))
        return;

    RTGIPrimarySurface primary = RTGIDecodePrimary(pixel);
    if (!primary.valid)
    {
        RTGIWriteRawOutputs(pixel, primary, (RTGIAccumulation)0);
        return;
    }

    RTSceneParams scene = RTGIBuildRawScene(pixel);
    RTIntegratorSettings settings = RTGIBuildRawSettings(primary);

    uint samples = RTGISampleCount();
    RTGIAccumulation accumulation = (RTGIAccumulation)0;
    for (uint sample = 0u; sample < samples; ++sample)
    {
        uint rng = RTGISampleRng(pixel, sample);
        RTIntegratorResult path = RTIntegratorRunPrimary(scene, settings, primary.surface, primary.worldPos,
            primary.surface.N, primary.V, rng);
        RTGIAccumulateSample(accumulation, path, g_GIIntensity);
    }
    RTGIWriteRawOutputs(pixel, primary, accumulation);
}
