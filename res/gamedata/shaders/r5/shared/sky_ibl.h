#ifndef SHARED_SKY_IBL_H
#define SHARED_SKY_IBL_H

#include "shared/sky_visibility.h"

StructuredBuffer<float4> g_SkyIrradiance : register(t46);
TextureCube<float4> g_SkySpecular : register(t47);
Texture2D<float4> g_SkyDFG : register(t48);
StructuredBuffer<SkyProbeRecord> g_SkyProbes : register(t49);

struct SkyProbeVisibility
{
    float c0;
    float3 c1;
    float confidence;
    bool valid;
};

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

struct SkyProbeCorner
{
    uint index;
    float weight;
    float visibility;
    float c0;
    float3 c1;
    float3 position;
    bool valid;
};

float3 SkyProbeViewOffset(float3 worldPos, float3 V)
{
    return V * min(0.24 * sky_probe_origin.w, 0.5 * length(eye_position - worldPos));
}

float3 SkyProbeBiasedPosition(float3 worldPos, float3 N, float3 V)
{
    return worldPos + N * (0.06 * sky_probe_origin.w) + SkyProbeViewOffset(worldPos, V);
}

SkyProbeCorner SkyProbeEvaluateCorner(float3 worldPos, float3 N, float3 biased, uint corner)
{
    SkyProbeCorner result = (SkyProbeCorner)0;
    float spacing = sky_probe_origin.w;
    int3 dims = int3(sky_probe_dims.xyz);
    float3 local = clamp((biased - sky_probe_origin.xyz) / spacing, 0.0, sky_probe_dims.xyz - 1.0);
    int3 base = min(int3(floor(local)), max(dims - 2, 0));
    float3 f = local - float3(base);
    int3 offset = int3(corner & 1u, (corner >> 1) & 1u, (corner >> 2) & 1u);
    int3 cell = min(base + offset, dims - 1);
    float3 trilinear = lerp(1.0 - f, f, float3(offset));
    result.index = uint(cell.x) + uint(dims.x) * (uint(cell.y) + uint(dims.y) * uint(cell.z));
    float trilinearWeight = trilinear.x * trilinear.y * trilinear.z;
    if (trilinearWeight <= 0.0)
        return result;
    SkyProbeRecord record = g_SkyProbes[result.index];
    SkyVisibilityUnpack(record.visibility.xy, result.c0, result.c1);
    if (!SkyVisibilityValid(result.c0))
        return result;
    result.position = sky_probe_origin.xyz + float3(cell) * spacing + SkyProbeUnpackOffset(record.visibility.z, spacing);
    float3 toProbe = result.position - worldPos;
    float facing = dot(toProbe, N) / max(length(toProbe), 1e-4) * 0.5 + 0.5;
    result.visibility = SkyProbeChebyshev(record, result.position, biased, spacing);
    float weight = max((facing * facing + 0.05) * max(result.visibility, SKY_PROBE_VISIBILITY_FLOOR), 1e-6);
    if (weight < 0.2)
        weight *= weight * weight * 25.0;
    result.weight = weight * trilinearWeight;
    result.valid = true;
    return result;
}

SkyProbeVisibility SkyProbeSample(float3 worldPos, float3 N, float3 V)
{
    SkyProbeVisibility result;
    result.c0 = 0.0;
    result.c1 = 0.0;
    result.confidence = 0.0;
    result.valid = false;
    if (sky_probe_dims.w < 0.5)
        return result;
    result.valid = true;

    float3 biased = SkyProbeBiasedPosition(worldPos, N, V);
    float weightSum = 0.0;
    float bestVisibility = 0.0;
    for (uint i = 0u; i < 8u; ++i)
    {
        SkyProbeCorner corner = SkyProbeEvaluateCorner(worldPos, N, biased, i);
        if (!corner.valid)
            continue;
        result.c0 += corner.c0 * corner.weight;
        result.c1 += corner.c1 * corner.weight;
        weightSum += corner.weight;
        bestVisibility = max(bestVisibility, corner.visibility);
    }
    if (weightSum > 1e-12)
    {
        result.c0 /= weightSum;
        result.c1 /= weightSum;
        result.confidence = saturate(bestVisibility * 4.0);
    }
    result.c0 *= result.confidence;
    result.c1 *= result.confidence;
    return result;
}

SkyProbeVisibility SkyVisibilityUniform(float visibility)
{
    SkyProbeVisibility result;
    result.c0 = saturate(visibility) / SKY_VISIBILITY_SH_Y0;
    result.c1 = 0.0;
    result.confidence = 1.0;
    result.valid = true;
    return result;
}

float SkyProbeVisibilityToward(SkyProbeVisibility visibility, float3 direction)
{
    return visibility.valid ? SkyVisibilityCosine(visibility.c0, visibility.c1, direction) : 1.0;
}

float SkySpecularOcclusion(float NdotV, float ao, float roughness)
{
    return saturate(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
}

float3 SkyAmbient(float3 albedo, float3 N, float3 V, float metallic, float roughness, float ao,
    SkyProbeVisibility visibility)
{
    float NdotV = max(dot(N, V), 1e-4);
    float3 F0 = CalculateF0(albedo, metallic);
    float2 dfg = g_SkyDFG.SampleLevel(smp_rtlinear, float2(NdotV, roughness), 0).rg;
    float3 specularWeight = F0 * dfg.x + dfg.y;
    specularWeight *= 1.0 + F0 * (1.0 / max(dfg.x + dfg.y, 1e-4) - 1.0);
    float3 R = normalize(lerp(reflect(-V, N), N, roughness * roughness));
    float3 specular = g_SkySpecular.SampleLevel(smp_rtlinear, R, roughness * sky_ibl.y).rgb
        * specularWeight * SkySpecularOcclusion(NdotV, ao, roughness) * SkyProbeVisibilityToward(visibility, R);
    float3 diffuse = saturate(1.0 - specularWeight) * (1.0 - metallic) * albedo * SkyIrradiance(N) * ao
        * SkyProbeVisibilityToward(visibility, N);
    return diffuse + specular;
}

#endif
