#include "shared/sky_source.h"
#include "shared/sky_filter.h"

cbuffer SkySpecularParams
{
    float g_Roughness;
    uint g_FaceSize;
    float g_MirrorLod;
    float g_SourceTexelSolidAngle;
    uint g_SampleCount;
    uint3 g_SkySpecularPad;
};

TextureCube<float4> g_Sky;
SamplerState smp_rtlinear;
RWTexture2DArray<float4> g_Specular;

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_FaceSize || id.y >= g_FaceSize || id.z >= 6u)
        return;

    float2 uv = (float2(id.xy) + 0.5) / float(g_FaceSize) * 2.0 - 1.0;
    float3 N = SkyCubeFaceDirection(id.z, uv);
    if (g_Roughness <= 0.0)
    {
        g_Specular[id] = float4(g_Sky.SampleLevel(smp_rtlinear, N, g_MirrorLod).rgb, 1.0);
        return;
    }

    float alpha = g_Roughness * g_Roughness;
    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    float3 radiance = 0.0;
    float weight = 0.0;
    for (uint i = 0u; i < g_SampleCount; ++i)
    {
        float3 h = SkyImportanceSampleGGX(SkyHammersley(i, g_SampleCount), alpha);
        float3 H = T * h.x + B * h.y + N * h.z;
        float3 L = 2.0 * h.z * H - N;
        float NdotL = dot(N, L);
        if (NdotL <= 0.0)
            continue;
        float pdf = SkyDistributionGGX(h.z, alpha) * 0.25;
        float sampleSolidAngle = 1.0 / (float(g_SampleCount) * pdf + 1e-6);
        float lod = max(0.5 * log2(sampleSolidAngle / g_SourceTexelSolidAngle) + 1.0, 0.0);
        radiance += g_Sky.SampleLevel(smp_rtlinear, L, lod).rgb * NdotL;
        weight += NdotL;
    }
    g_Specular[id] = float4(radiance / max(weight, 1e-6), 1.0);
}
