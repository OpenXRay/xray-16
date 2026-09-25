#include "shared/sky_source.h"

cbuffer SkyDownsampleParams
{
    uint g_TargetSize;
    uint3 g_SkyDownsamplePad;
};

TextureCube<float4> g_Source;
SamplerState smp_rtlinear;
RWTexture2DArray<float4> g_Target;

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_TargetSize || id.y >= g_TargetSize || id.z >= 6u)
        return;

    float2 uv = (float2(id.xy) + 0.5) / float(g_TargetSize) * 2.0 - 1.0;
    g_Target[id] = float4(g_Source.SampleLevel(smp_rtlinear, SkyCubeFaceDirection(id.z, uv), 0).rgb, 1.0);
}
