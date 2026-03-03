#include "noise4d.h"

cbuffer Perlin4DGenParams : register(b0)
{
    float g_time;
    float g_tileScale;
    uint  g_textureSize;
    float g_pad0;
};

RWTexture3D<float4> g_output : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (any(dtid >= g_textureSize))
        return;

    float3 uvw = float3(dtid) / float(g_textureSize);
    float4 p = float4(uvw * g_tileScale, g_time);

    float dx = fbm4D(p);
    float dy = fbm4D(p + float4(31.416, -47.853, 12.679, 0.0));
    float dz = fbm4D(p + float4(-17.236, 53.147, -28.912, 0.0));

    g_output[dtid] = float4(dx * 0.5 + 0.5,
                             dy * 0.5 + 0.5,
                             dz * 0.5 + 0.5,
                             1.0);
}
