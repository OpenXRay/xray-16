#include "shared/sky_source.h"

cbuffer SkyIrradianceParams
{
    uint g_FaceSize;
    float g_SourceLod;
    uint2 g_SkyIrradiancePad;
};

TextureCube<float4> g_Sky;
SamplerState smp_rtlinear;
RWStructuredBuffer<float4> g_SkyIrradiance;

#define SKY_SH_THREADS 64
#define SKY_SH_COEFFICIENTS 9

groupshared float3 s_skySH[SKY_SH_COEFFICIENTS][SKY_SH_THREADS];

[numthreads(SKY_SH_THREADS, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID)
{
    float3 sh[SKY_SH_COEFFICIENTS];
    [unroll]
    for (uint c = 0u; c < SKY_SH_COEFFICIENTS; ++c)
        sh[c] = 0.0;

    uint faceTexels = g_FaceSize * g_FaceSize;
    uint texelCount = faceTexels * 6u;
    for (uint texel = gtid.x; texel < texelCount; texel += SKY_SH_THREADS)
    {
        uint face = texel / faceTexels;
        uint local = texel - face * faceTexels;
        float2 uv = (float2(local % g_FaceSize, local / g_FaceSize) + 0.5) / float(g_FaceSize) * 2.0 - 1.0;
        float3 d = SkyCubeFaceDirection(face, uv);
        float spread = 1.0 + dot(uv, uv);
        float solidAngle = 4.0 / (spread * sqrt(spread) * float(faceTexels));
        float3 radiance = g_Sky.SampleLevel(smp_rtlinear, d, g_SourceLod).rgb * solidAngle;
        sh[0] += radiance * 0.282095;
        sh[1] += radiance * (0.488603 * d.y);
        sh[2] += radiance * (0.488603 * d.z);
        sh[3] += radiance * (0.488603 * d.x);
        sh[4] += radiance * (1.092548 * d.x * d.y);
        sh[5] += radiance * (1.092548 * d.y * d.z);
        sh[6] += radiance * (0.315392 * (3.0 * d.z * d.z - 1.0));
        sh[7] += radiance * (1.092548 * d.x * d.z);
        sh[8] += radiance * (0.546274 * (d.x * d.x - d.y * d.y));
    }

    [unroll]
    for (uint i = 0u; i < SKY_SH_COEFFICIENTS; ++i)
        s_skySH[i][gtid.x] = sh[i];
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = SKY_SH_THREADS / 2u; stride > 0u; stride >>= 1u)
    {
        if (gtid.x < stride)
        {
            [unroll]
            for (uint k = 0u; k < SKY_SH_COEFFICIENTS; ++k)
                s_skySH[k][gtid.x] += s_skySH[k][gtid.x + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (gtid.x < SKY_SH_COEFFICIENTS)
    {
        float band = gtid.x == 0u ? 1.0 : (gtid.x < 4u ? 2.0 / 3.0 : 0.25);
        g_SkyIrradiance[gtid.x] = float4(s_skySH[gtid.x][0] * band, 0.0);
    }
}
