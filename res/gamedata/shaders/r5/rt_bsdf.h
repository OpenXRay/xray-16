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

MaterialSurface GBufferMaterialSurface(float4 normalData, float4 baseColorData, float2 materialData)
{
    MaterialSurface surface;
    surface.albedo = baseColorData.rgb;
    surface.N = normalize(normalData.xyz);
    surface.roughness = abs(normalData.w);
    surface.metallic = baseColorData.a;
    surface.ao = 1.0;
    surface.emissive = 0.0;
    surface.shadingClass = GBufferShadingClass(materialData);
    surface.transmission = materialData.y;
    return surface;
}

void RTTangentFrame(float3 N, out float3 T, out float3 B)
{
    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

float3 CosineWeightedHemisphere(float2 u, float3 N)
{
    float r = sqrt(u.x);
    float phi = 6.28318530718 * u.y;
    float3 dir = float3(r * cos(phi), r * sin(phi), sqrt(max(0.0, 1.0 - u.x)));

    float3 T, B;
    RTTangentFrame(N, T, B);

    return normalize(T * dir.x + B * dir.y + N * dir.z);
}

float3 UniformSphereDirection(float2 u, float3 N)
{
    float cosTheta = 1.0 - 2.0 * u.y;
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = 6.28318530718 * u.x;
    float3 dir = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 T, B;
    RTTangentFrame(N, T, B);

    return normalize(T * dir.x + B * dir.y + N * dir.z);
}

float3 RTGGXVNDFReflect(float2 u, float3 V, float3 N, float roughness)
{
    float3 T, B;
    RTTangentFrame(N, T, B);

    float3 i = float3(dot(V, T), dot(V, B), dot(V, N));

    float clampedRoughness = clamp(roughness, MIN_ROUGHNESS, 1.0);
    float alpha = clampedRoughness * clampedRoughness;
    float3 Vh = RTSafeNormalize(float3(alpha * i.x, alpha * i.y, i.z), float3(0.0, 0.0, 1.0));

    float lensq = dot(Vh.xy, Vh.xy);
    float3 T1 = lensq > 0.0 ? float3(-Vh.y, Vh.x, 0.0) * rsqrt(lensq) : float3(1.0, 0.0, 0.0);
    float3 T2 = cross(Vh, T1);

    float r = sqrt(u.x);
    float phi = 6.28318530718 * u.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(saturate(1.0 - t1 * t1)) + s * t2;
    float t3 = sqrt(saturate(1.0 - t1 * t1 - t2 * t2));

    float3 Nh = T1 * t1 + T2 * t2 + Vh * t3;
    float3 Ne = RTSafeNormalize(float3(alpha * Nh.x, alpha * Nh.y, max(0.0, Nh.z)), float3(0.0, 0.0, 1.0));

    float3 L = 2.0 * dot(i, Ne) * Ne - i;
    return RTSafeNormalize(T * L.x + B * L.y + N * L.z, N);
}

float RTGGXVNDFPdf(float3 V, float3 L, float3 N, float roughness)
{
    float3 T, B;
    RTTangentFrame(N, T, B);

    float3 i = float3(dot(V, T), dot(V, B), dot(V, N));
    float3 o = float3(dot(L, T), dot(L, B), dot(L, N));

    float3 halfSum = i + o;
    float halfLenSq = dot(halfSum, halfSum);
    if (halfLenSq <= 1e-12)
        return 0.0;

    float3 m = halfSum * rsqrt(halfLenSq);
    if (i.z <= 0.0 || m.z <= 0.0)
        return 0.0;

    float clampedRoughness = clamp(roughness, MIN_ROUGHNESS, 1.0);
    float pdf = G1_SmithGGX(i.z, clampedRoughness) * D_GGX(m.z, clampedRoughness) / (4.0 * i.z);

    return isfinite(pdf) ? pdf : 0.0;
}

float RTGGXSpecularConeSpread(float roughness)
{
    float alpha = clamp(roughness, MIN_ROUGHNESS, 1.0);
    alpha *= alpha;

    float a2 = alpha * alpha;
    float peakRatio = 0.01;
    float cosSq = saturate((1.0 - a2 / sqrt(peakRatio)) / max(1.0 - a2, 1e-6));

    return min(2.0 * acos(sqrt(cosSq)), PI * 0.5);
}

struct RTBSDFTerms
{
    float3 bsdf;
    float3 transmission;
    float3 diffuse;
    float3 specular;
};

RTBSDFTerms RTEvaluateBSDFTerms(MaterialSurface surface, float3 V, float3 L, float3 lightColor, uint diffuseMode)
{
    RTBSDFTerms terms;
    if (surface.shadingClass == SHADING_CLASS_FOLIAGE)
    {
        PBRDirectTerms direct = PBRDirectLightingTerms(surface.albedo, surface.N, V, L, lightColor, 0.0, surface.roughness, 1u);
        float3 sssColor = surface.albedo * foliage_sss.rgb * surface.transmission
            * (1.0 - F_Schlick(saturate(dot(surface.N, V)), DIELECTRIC_F0));
        terms.diffuse = direct.diffuse;
        terms.specular = direct.specular;
        terms.bsdf = direct.diffuse + direct.specular;
        terms.transmission = FoliageTransmission(surface.N, V, L, foliage_params2.x) * sssColor * lightColor;
        return terms;
    }

    PBRDirectTerms direct = PBRDirectLightingTerms(surface.albedo, surface.N, V, L, lightColor, surface.metallic, surface.roughness, diffuseMode);
    terms.diffuse = direct.diffuse;
    terms.specular = direct.specular;
    terms.bsdf = direct.diffuse + direct.specular;
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

struct RTBSDFMixture
{
    float specular;
    float diffuse;
    float transmission;
};

RTBSDFMixture RTBSDFMixtureWeights(MaterialSurface surface, float3 V)
{
    RTBSDFMixture mixture;
    float NdotV = dot(surface.N, V);
    float3 F0 = CalculateF0(surface.albedo, surface.metallic);
    float transmissionProbability = surface.shadingClass == SHADING_CLASS_FOLIAGE ? 0.5 : 0.0;
    float specularShare = NdotV > 0.0 ? clamp(Luminance(F_Schlick(saturate(NdotV), F0)), 0.05, 0.95) : 0.0;

    mixture.specular = (1.0 - transmissionProbability) * specularShare;
    mixture.diffuse = 1.0 - transmissionProbability - mixture.specular;
    mixture.transmission = transmissionProbability;
    return mixture;
}

float RTBSDFPdf(MaterialSurface surface, float3 V, float3 L)
{
    RTBSDFMixture mixture = RTBSDFMixtureWeights(surface, V);
    float roughness = clamp(surface.roughness, MIN_ROUGHNESS, 1.0);

    float specularPdf = RTGGXVNDFPdf(V, L, surface.N, roughness);
    float diffusePdf = max(dot(surface.N, L), 0.0) / PI;
    float uniformPdf = 1.0 / (4.0 * PI);

    float pdf = mixture.specular * specularPdf + mixture.diffuse * diffusePdf + mixture.transmission * uniformPdf;
    return isfinite(pdf) ? pdf : 0.0;
}

struct RTBSDFSample
{
    float3 direction;
    float3 weight;
    float pdf;
    bool valid;
    float3 diffuseWeight;
    float3 specularWeight;
    bool delta;
    float coneSpread;
};

RTBSDFSample RTSampleBSDF(MaterialSurface surface, float3 V, uint diffuseMode, inout uint rng)
{
    RTBSDFSample result;
    result.direction = surface.N;
    result.weight = 0.0;
    result.pdf = 0.0;
    result.valid = false;
    result.diffuseWeight = 0.0;
    result.specularWeight = 0.0;
    result.delta = false;

    float roughness = clamp(surface.roughness, MIN_ROUGHNESS, 1.0);
    result.coneSpread = RTGGXSpecularConeSpread(roughness);

    RTBSDFMixture mixture = RTBSDFMixtureWeights(surface, V);
    float2 u = float2(rand_float(rng), rand_float(rng));
    float lobeSample = rand_float(rng);

    if (lobeSample < mixture.specular)
    {
        result.direction = RTGGXVNDFReflect(u, V, surface.N, roughness);
    }
    else if (lobeSample < mixture.specular + mixture.diffuse)
    {
        result.direction = CosineWeightedHemisphere(u, surface.N);
    }
    else
    {
        result.direction = UniformSphereDirection(u, surface.N);
    }

    if (!all(isfinite(result.direction)))
        return result;

    if (dot(surface.N, result.direction) <= 0.0 && mixture.transmission <= 0.0)
        return result;

    float3 halfSum = V + result.direction;
    if (dot(halfSum, halfSum) <= 1e-8)
        return result;

    float pdf = RTBSDFPdf(surface, V, result.direction);
    if (!(pdf > 0.0))
        return result;

    RTBSDFTerms terms = RTEvaluateBSDFTerms(surface, V, result.direction, float3(1.0, 1.0, 1.0), diffuseMode);
    result.pdf = pdf;
    result.diffuseWeight = (terms.diffuse + terms.transmission) / pdf;
    result.specularWeight = terms.specular / pdf;
    result.weight = result.diffuseWeight + result.specularWeight;
    result.valid = all(isfinite(result.weight)) && all(isfinite(result.diffuseWeight)) && all(isfinite(result.specularWeight));
    return result;
}

#endif
