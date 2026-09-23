#define RT_WORLD_CACHE 1
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

    uint debugMode = RTWorldCacheDebugMode();
    if (debugMode != 0u && RTWorldCacheEnabled())
    {
        RTGIAccumulation debug = (RTGIAccumulation)0;
        debug.validCount = RTGISampleCount();
        if (primary.depth < 0.9)
        {
            uint rng = RTGISampleRng(pixel, 0u);
            RTWorldCacheLookup lookup = RTWorldCacheQuery(primary.worldPos, primary.surface.N, g_WorldCacheLifetime,
                true, false, rng);
            float3 diffuseAlbedo = primary.surface.albedo * (1.0 - primary.surface.metallic);
            debug.diffuseSum = RTWorldCacheDebugColor(lookup, debugMode, diffuseAlbedo) * float(RTGISampleCount());
        }
        RTGIWriteRawOutputs(pixel, primary, debug);
        return;
    }

    RTSceneParams scene = RTGIBuildRawScene(pixel);
    RTIntegratorSettings settings = RTGIBuildRawSettings(primary);
    if (RTWorldCacheEnabled())
    {
        settings.cacheBounce = g_WorldCacheBounce;
        settings.cacheLife = g_WorldCacheLifetime;
        settings.maxBounces = max(settings.maxBounces, settings.cacheBounce + 1u);
    }

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
