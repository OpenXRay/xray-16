#define SM_5_0
#include "common.h"

cbuffer ExposureParams : register(b5)
{
    float g_min_log_luminance;
    float g_log_luminance_range;
    uint g_screen_width;
    uint g_screen_height;
};

Texture2D<float4> g_scene_color : register(t0);
RWStructuredBuffer<uint> g_histogram : register(u0);

groupshared uint gs_histogram[64];

float ComputeLuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

uint ComputeBinIndex(float luminance)
{
    float lum = max(luminance, 0.00001);

    float logLum = log2(lum);

    float normalized = saturate((logLum - g_min_log_luminance) / g_log_luminance_range);

    return (uint)(normalized * 63.0);
}

[numthreads(16, 16, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    if (group_index < 64)
    {
        gs_histogram[group_index] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    bool inBounds = (dispatch_id.x < g_screen_width) && (dispatch_id.y < g_screen_height);

    if (inBounds)
    {
        float4 color = g_scene_color.Load(int3(dispatch_id.xy, 0));

        float luminance = ComputeLuminance(color.rgb);

        if (luminance > 0.0001)
        {
            uint binIndex = ComputeBinIndex(luminance);

            InterlockedAdd(gs_histogram[binIndex], 1);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index < 64)
    {
        uint localCount = gs_histogram[group_index];
        if (localCount > 0)
        {
            InterlockedAdd(g_histogram[group_index], localCount);
        }
    }
}
