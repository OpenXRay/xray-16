#ifndef RT_BSDF_H
#define RT_BSDF_H

#include "rt_common.h"
#include "shared/common.h"
#include "shared/pbr_brdf.h"
#include "shared/material_surface.h"

float Luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float3 CosineWeightedHemisphere(float2 u, float3 N)
{
    float r = sqrt(u.x);
    float phi = 6.28318530718 * u.y;
    float3 dir = float3(r * cos(phi), r * sin(phi), sqrt(max(0.0, 1.0 - u.x)));

    float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    return normalize(T * dir.x + B * dir.y + N * dir.z);
}

float3 UniformSphereDirection(float2 u, float3 N)
{
    float cosTheta = 1.0 - 2.0 * u.y;
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = 6.28318530718 * u.x;
    float3 dir = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    return normalize(T * dir.x + B * dir.y + N * dir.z);
}

float3 SampleGGX(float2 u, float3 N, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float phi = 6.28318530718 * u.x;
    float cosTheta = sqrt((1.0 - u.y) / (1.0 + (a2 - 1.0) * u.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));

    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    return normalize(T * H.x + B * H.y + N * H.z);
}

struct RTBSDFTerms
{
    float3 bsdf;
    float3 transmission;
};

RTBSDFTerms RTEvaluateBSDFTerms(MaterialSurface surface, float3 V, float3 L, float3 lightColor, uint diffuseMode)
{
    RTBSDFTerms terms;
    if (surface.shadingClass == SHADING_CLASS_FOLIAGE)
    {
        float3 sssColor = surface.albedo * foliage_sss.rgb * surface.transmission
            * (1.0 - F_Schlick(saturate(dot(surface.N, V)), DIELECTRIC_F0));
        terms.bsdf = PBRDirectLighting(surface.albedo, surface.N, V, L, lightColor, 0.0, surface.roughness, 1u);
        terms.transmission = FoliageTransmission(surface.N, V, L, foliage_params2.x) * sssColor * lightColor;
        return terms;
    }
    terms.bsdf = PBRDirectLighting(surface.albedo, surface.N, V, L, lightColor, surface.metallic, surface.roughness, diffuseMode);
    terms.transmission = 0.0;
    return terms;
}

float3 RTCombineBSDFTerms(RTBSDFTerms terms, float visibility)
{
    return (terms.bsdf + terms.transmission) * visibility;
}

float3 RTEvaluateBSDF(MaterialSurface surface, float3 V, float3 L, float3 lightColor, uint diffuseMode)
{
    RTBSDFTerms terms = RTEvaluateBSDFTerms(surface, V, L, lightColor, diffuseMode);
    return terms.bsdf + terms.transmission;
}

struct RTBSDFSample
{
    float3 direction;
    float3 weight;
    float pdf;
    bool valid;
};

RTBSDFSample RTSampleBSDF(MaterialSurface surface, float3 V, uint diffuseMode, inout uint rng)
{
    RTBSDFSample result;
    result.direction = surface.N;
    result.weight = 0.0;
    result.pdf = 0.0;
    result.valid = false;

    float roughness = clamp(surface.roughness, MIN_ROUGHNESS, 1.0);
    float3 F0 = CalculateF0(surface.albedo, surface.metallic);
    float transmissionProbability = surface.shadingClass == SHADING_CLASS_FOLIAGE ? 0.5 : 0.0;
    float specularShare = clamp(Luminance(F_Schlick(saturate(dot(surface.N, V)), F0)), 0.05, 0.95);
    float specularProbability = (1.0 - transmissionProbability) * specularShare;
    float diffuseProbability = 1.0 - transmissionProbability - specularProbability;
    float uniformPdf = 1.0 / (4.0 * PI);

    float2 u = float2(rand_float(rng), rand_float(rng));
    float lobeSample = rand_float(rng);
    if (lobeSample < specularProbability)
    {
        float3 H = SampleGGX(u, surface.N, roughness);
        if (dot(V, H) <= 0.0)
            return result;
        result.direction = reflect(-V, H);
    }
    else if (lobeSample < specularProbability + diffuseProbability)
    {
        result.direction = CosineWeightedHemisphere(u, surface.N);
    }
    else
    {
        result.direction = UniformSphereDirection(u, surface.N);
    }

    float NdotL = dot(surface.N, result.direction);
    if (NdotL <= 0.0 && transmissionProbability <= 0.0)
        return result;

    float3 halfSum = V + result.direction;
    float halfLenSq = dot(halfSum, halfSum);
    if (halfLenSq <= 1e-8)
        return result;

    float3 H = halfSum * rsqrt(halfLenSq);
    float NdotH = saturate(dot(surface.N, H));
    float VdotH = dot(V, H);
    float specularPdf = VdotH > 0.0 ? D_GGX(NdotH, roughness) * NdotH / (4.0 * VdotH) : 0.0;
    float diffusePdf = max(NdotL, 0.0) / PI;
    result.pdf = specularProbability * specularPdf + diffuseProbability * diffusePdf + transmissionProbability * uniformPdf;
    if (!(result.pdf > 0.0))
        return result;

    result.weight = RTEvaluateBSDF(surface, V, result.direction, float3(1.0, 1.0, 1.0), diffuseMode) / result.pdf;
    result.valid = all(isfinite(result.weight));
    return result;
}

#endif
