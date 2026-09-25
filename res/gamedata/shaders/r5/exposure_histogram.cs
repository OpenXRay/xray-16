#include "shared/color_space.h"
#include "exposure_common.h"

Texture2D<float4> t_Scene;
RWByteAddressBuffer u_Histogram;

cbuffer ExposureHistogramParams
{
    uint2 g_SceneSize;
    uint2 g_HistogramPad;
};

groupshared uint gs_Bins[EXPOSURE_HISTOGRAM_BINS];

uint LuminanceBin(float luminance)
{
    if (!(luminance > exp2(EXPOSURE_MIN_LOG2_LUMINANCE)))
        return 0u;
    float t = saturate((log2(luminance) - EXPOSURE_MIN_LOG2_LUMINANCE) / EXPOSURE_LOG2_LUMINANCE_RANGE);
    return 1u + uint(t * float(EXPOSURE_HISTOGRAM_BINS - 2) + 0.5);
}

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    gs_Bins[groupIndex] = 0u;
    GroupMemoryBarrierWithGroupSync();

    uint2 pixel = dispatchThreadId.xy * 2u;
    if (all(pixel < g_SceneSize))
    {
        float luminance = LinearLuminance(t_Scene.Load(int3(pixel, 0)).rgb);
        if (isfinite(luminance))
            InterlockedAdd(gs_Bins[LuminanceBin(luminance)], 1u);
    }
    GroupMemoryBarrierWithGroupSync();

    uint count = gs_Bins[groupIndex];
    if (count != 0u)
        u_Histogram.InterlockedAdd(groupIndex * 4u, count);
}
