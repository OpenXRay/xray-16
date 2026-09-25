#include "shared/sky_filter.h"

RWTexture2D<float4> g_DFG;

#define SKY_DFG_SAMPLES 1024u

float SkyVisibilitySchlick(float NdotX, float k)
{
    return NdotX / (NdotX * (1.0 - k) + k);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    g_DFG.GetDimensions(width, height);
    if (id.x >= width || id.y >= height)
        return;

    float NdotV = (float(id.x) + 0.5) / float(width);
    float roughness = (float(id.y) + 0.5) / float(height);
    float alpha = roughness * roughness;
    float k = alpha * 0.5;
    float3 V = float3(sqrt(max(1.0 - NdotV * NdotV, 0.0)), 0.0, NdotV);

    float scale = 0.0;
    float bias = 0.0;
    for (uint i = 0u; i < SKY_DFG_SAMPLES; ++i)
    {
        float3 H = SkyImportanceSampleGGX(SkyHammersley(i, SKY_DFG_SAMPLES), alpha);
        float VdotH = dot(V, H);
        float3 L = 2.0 * VdotH * H - V;
        float NdotL = saturate(L.z);
        if (NdotL <= 0.0)
            continue;
        float NdotH = saturate(H.z);
        VdotH = saturate(VdotH);
        float G = SkyVisibilitySchlick(NdotV, k) * SkyVisibilitySchlick(NdotL, k);
        float visibility = G * VdotH / max(NdotH * NdotV, 1e-6);
        float fresnel = pow(1.0 - VdotH, 5.0);
        scale += (1.0 - fresnel) * visibility;
        bias += fresnel * visibility;
    }
    g_DFG[id.xy] = float4(scale / float(SKY_DFG_SAMPLES), bias / float(SKY_DFG_SAMPLES), 0.0, 1.0);
}
