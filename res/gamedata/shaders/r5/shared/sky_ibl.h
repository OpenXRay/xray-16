#ifndef SHARED_SKY_IBL_H
#define SHARED_SKY_IBL_H

StructuredBuffer<float4> g_SkyIrradiance : register(t46);
TextureCube<float4> g_SkySpecular : register(t47);
Texture2D<float4> g_SkyDFG : register(t48);

bool SkyLightingEnabled()
{
    return sky_ibl.x > 0.5;
}

float3 SkyIrradiance(float3 n)
{
    float3 e = g_SkyIrradiance[0].rgb * 0.282095
        + g_SkyIrradiance[1].rgb * (0.488603 * n.y)
        + g_SkyIrradiance[2].rgb * (0.488603 * n.z)
        + g_SkyIrradiance[3].rgb * (0.488603 * n.x)
        + g_SkyIrradiance[4].rgb * (1.092548 * n.x * n.y)
        + g_SkyIrradiance[5].rgb * (1.092548 * n.y * n.z)
        + g_SkyIrradiance[6].rgb * (0.315392 * (3.0 * n.z * n.z - 1.0))
        + g_SkyIrradiance[7].rgb * (1.092548 * n.x * n.z)
        + g_SkyIrradiance[8].rgb * (0.546274 * (n.x * n.x - n.y * n.y));
    return max(e, 0.0);
}

float SkySpecularOcclusion(float NdotV, float ao, float roughness)
{
    return saturate(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
}

float3 SkyAmbient(float3 albedo, float3 N, float3 V, float metallic, float roughness, float ao)
{
    float NdotV = max(dot(N, V), 1e-4);
    float3 F0 = CalculateF0(albedo, metallic);
    float2 dfg = g_SkyDFG.SampleLevel(smp_rtlinear, float2(NdotV, roughness), 0).rg;
    float3 specularWeight = F0 * dfg.x + dfg.y;
    specularWeight *= 1.0 + F0 * (1.0 / max(dfg.x + dfg.y, 1e-4) - 1.0);
    float3 R = normalize(lerp(reflect(-V, N), N, roughness * roughness));
    float3 specular = g_SkySpecular.SampleLevel(smp_rtlinear, R, roughness * sky_ibl.y).rgb
        * specularWeight * SkySpecularOcclusion(NdotV, ao, roughness);
    float3 diffuse = saturate(1.0 - specularWeight) * (1.0 - metallic) * albedo * SkyIrradiance(N) * ao;
    return diffuse + specular;
}

#endif
