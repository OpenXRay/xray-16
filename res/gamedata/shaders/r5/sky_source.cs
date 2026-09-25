#include "shared/sky_source.h"

cbuffer SkySourceParams
{
    float3 g_SkyTint;
    uint g_FaceSize;
    float g_SkyBlend;
    float g_SkyRotation;
    float g_SkyEnergy;
    float g_GroundAlbedo;
};

TextureCube<float4> g_Sky0;
TextureCube<float4> g_Sky1;
SamplerState smp_rtlinear;
RWTexture2DArray<float4> g_SkyCube;

float3 SampleAuthoredSky(float3 direction)
{
    float3 lookup = SkyHalfBoxWarp(direction);
    float3 sky0 = g_Sky0.SampleLevel(smp_rtlinear, lookup, 0).rgb;
    float3 sky1 = g_Sky1.SampleLevel(smp_rtlinear, lookup, 0).rgb;
    return lerp(sky0, sky1, g_SkyBlend);
}

float3 AuthoredSkyRadiance(float3 direction)
{
    float3 local = SkyRotateY(direction, -g_SkyRotation);
    if (local.y >= 0.0)
        return SampleAuthoredSky(local);
    return SampleAuthoredSky(SkyHorizonDirection(local)) * g_GroundAlbedo;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_FaceSize || id.y >= g_FaceSize || id.z >= 6u)
        return;

    float3 radiance = 0.0;
    [unroll]
    for (uint tap = 0u; tap < 4u; ++tap)
    {
        float2 offset = float2(float(tap & 1u), float(tap >> 1u)) * 0.5 + 0.25;
        float2 uv = (float2(id.xy) + offset) / float(g_FaceSize) * 2.0 - 1.0;
        radiance += AuthoredSkyRadiance(SkyCubeFaceDirection(id.z, uv));
    }
    g_SkyCube[id] = float4(radiance * g_SkyTint * (0.25 * g_SkyEnergy), 1.0);
}
