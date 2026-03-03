// perlin4d_gen.cs
// Compute shader: generates 3-channel 4D Perlin fBm into a 3D volume texture.
// Each frame, g_time evolves the 4th noise dimension, animating the field.
// Output channels are remapped from [-1,1] to [0,1] for texture storage.
// Consumers decode: disp = sample.rgb * 2.0 - 1.0

#include "noise4d.h"

cbuffer Perlin4DGenParams : register(b0)
{
    float g_time;         // 4th dimension (evolution)
    float g_tileScale;    // spatial periods across texture (e.g. 4.0)
    uint  g_textureSize;  // 64
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

    // 3 independent fBm channels (same offsets as turbulenceDisplace in noise4d.h)
    float dx = fbm4D(p);
    float dy = fbm4D(p + float4(31.416, -47.853, 12.679, 0.0));
    float dz = fbm4D(p + float4(-17.236, 53.147, -28.912, 0.0));

    // Remap [-1,1] → [0,1] for texture storage
    g_output[dtid] = float4(dx * 0.5 + 0.5,
                             dy * 0.5 + 0.5,
                             dz * 0.5 + 0.5,
                             1.0);
}
