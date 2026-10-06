#define RT_WORLD_CACHE 1
#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rtgi_trace_common.h"

void RTGIRecordWorldCacheEvents(uint4 events)
{
    if (!RTWorldCacheEnabled())
        return;
    for (uint index = 0u; index < RT_WORLD_CACHE_EVENT_COUNT; ++index)
    {
        uint count = WaveActiveSum(events[index]);
        if (count != 0u && WaveIsFirstLane())
            InterlockedAdd(u_WorldCacheStats[RT_WORLD_CACHE_STAT_EVENTS + index], count);
    }
}

uint4 RTGITracePixel(uint2 pixel)
{
    RTGIPrimarySurface primary = RTGIDecodePrimary(pixel);
    if (!primary.valid)
    {
        RTGIWriteRawOutputs(pixel, primary, (RTGIAccumulation)0);
        return 0u;
    }

    uint debugMode = RTWorldCacheDebugMode();
    if (debugMode != 0u && RTWorldCacheEnabled())
    {
        RTGIAccumulation debug = (RTGIAccumulation)0;
        debug.validCount = RTGISampleCount();
        if (!primary.hud)
        {
            uint rng = RTGISampleRng(pixel, 0u);
            RTWorldCacheLookup lookup = RTWorldCacheQuery(primary.worldPos, primary.surface.N, g_WorldCacheLifetime,
                true, false, rng);
            float3 diffuseAlbedo = primary.surface.albedo * (1.0 - primary.surface.metallic);
            debug.diffuseSum = RTWorldCacheDebugColor(lookup, debugMode, diffuseAlbedo,
                RTWorldCacheRoughnessEligibility(primary.surface.roughness)) * float(RTGISampleCount());
        }
        RTGIWriteRawOutputs(pixel, primary, debug);
        return 0u;
    }

    RTSceneParams scene = RTGIBuildRawScene(pixel);
    RTIntegratorSettings settings = RTGIBuildRawSettings(primary);
    if (RTWorldCacheEnabled())
    {
        settings.cacheBounce = g_WorldCacheBounce;
        settings.cacheLife = g_WorldCacheLifetime;
        settings.cacheInsert = true;
        settings.maxBounces = max(settings.maxBounces, settings.cacheBounce + 1u);
    }

    uint samples = RTGISampleCount();
    RTGIAccumulation accumulation = (RTGIAccumulation)0;
    uint4 cacheEvents = 0u;
    for (uint sample = 0u; sample < samples; ++sample)
    {
        uint rng = RTGISampleRng(pixel, sample);
        RTIntegratorResult path = RTIntegratorRunPrimary(scene, settings, primary.surface, primary.worldPos,
            primary.surface.N, primary.V, rng, primary.hud);
        RTGIAccumulateSample(accumulation, path, g_GIIntensity);
        cacheEvents += path.cacheEvents;
    }
    RTGIWriteRawOutputs(pixel, primary, accumulation);
    return cacheEvents;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    uint4 cacheEvents = 0u;
    if (pixel.x < uint(g_ScreenWidth) && pixel.y < uint(g_ScreenHeight))
        cacheEvents = RTGITracePixel(pixel);
    RTGIRecordWorldCacheEvents(cacheEvents);
}
