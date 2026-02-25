#define SM_5_0
#include "common.h"

cbuffer PaintParams : register(b5)
{
    float2 g_hitUV;
    float g_radius;
    float g_opacity;
    float4 g_tintColor;
    uint g_overlayWidth;
    uint g_overlayHeight;
    uint g_flags;
    uint g_pad;
};

RWTexture2D<float4> g_Overlay : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= g_overlayWidth || dtid.y >= g_overlayHeight)
        return;

    float2 pixelUV = (float2(dtid.xy) + 0.5) / float2(g_overlayWidth, g_overlayHeight);
    float2 delta = pixelUV - g_hitUV;

    float dist = length(delta);
    if (dist > g_radius)
        return;

    float fade = 1.0 - smoothstep(g_radius * 0.6, g_radius, dist);
    float4 stamp = g_tintColor * fade * g_opacity;

    float4 existing = g_Overlay[dtid.xy];
    float4 result;

    if (g_flags == 0)
        result = existing + stamp;
    else if (g_flags == 1)
        result = lerp(existing, stamp, stamp.a);
    else
        result = existing * lerp(1.0, stamp, stamp.a);

    g_Overlay[dtid.xy] = saturate(result);
}
