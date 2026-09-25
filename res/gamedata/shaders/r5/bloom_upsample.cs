#include "bloom_common.h"

float3 BloomTap(float2 uv)
{
    return t_Source.SampleLevel(smp_rtlinear, uv, 0).rgb;
}

[numthreads(BLOOM_GROUP_SIZE, BLOOM_GROUP_SIZE, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadId.xy;
    if (any(pixel >= g_TargetSize))
        return;

    float2 uv = BloomTargetUV(pixel);
    float2 texel = g_SourceTexelSize;
    float3 blurred = BloomTap(uv) * 4.0;
    blurred += (BloomTap(uv + float2(-texel.x, 0.0)) + BloomTap(uv + float2(texel.x, 0.0))
        + BloomTap(uv + float2(0.0, -texel.y)) + BloomTap(uv + float2(0.0, texel.y))) * 2.0;
    blurred += BloomTap(uv - texel) + BloomTap(uv + texel)
        + BloomTap(uv + float2(-texel.x, texel.y)) + BloomTap(uv + float2(texel.x, -texel.y));

    u_Target[pixel] = float4(u_Target[pixel].rgb + blurred * (1.0 / 16.0), 1.0);
}
