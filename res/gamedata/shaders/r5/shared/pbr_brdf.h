// PBR BRDF Library - Cook-Torrance/GGX Implementation
// Forward+ Rendering Pipeline Phase 3.1

#ifndef PBR_BRDF_H
#define PBR_BRDF_H

static const float PI = 3.14159265359f;
static const float MIN_ROUGHNESS = 0.04f;
static const float DIELECTRIC_F0 = 0.04f;

// GGX/Trowbridge-Reitz Normal Distribution Function
float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001f);
}

// Schlick-GGX Geometry Function (single direction)
float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;  // Direct lighting

    return NdotV / (NdotV * (1.0f - k) + k);
}

// Smith's method for geometry (combines both view and light directions)
float G_Smith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = G_SchlickGGX(NdotV, roughness);
    float ggx2 = G_SchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// Fresnel-Schlick with roughness (for IBL)
float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 oneMinusRoughness = 1.0f - roughness;
    return F0 + (max(oneMinusRoughness, F0) - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// Cook-Torrance specular BRDF
float3 CookTorranceSpecular(
    float NdotH,
    float NdotV,
    float NdotL,
    float HdotV,
    float roughness,
    float3 F0)
{
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    float3 F = F_Schlick(HdotV, F0);

    float3 numerator = D * G * F;
    float denominator = 4.0f * max(NdotV, 0.001f) * max(NdotL, 0.001f);

    return numerator / denominator;
}

// Calculate F0 (base reflectivity) from metallic and albedo
float3 CalculateF0(float3 albedo, float metallic)
{
    return lerp(DIELECTRIC_F0, albedo, metallic);
}

// Lambertian diffuse (simplest, uniform)
float LambertianDiffuse()
{
    return 1.0f / PI;
}

// Disney/Burley diffuse BRDF
// Roughness-dependent with retroreflection at grazing angles
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness)
{
    float fd90 = 0.5f + 2.0f * LdotH * LdotH * roughness;
    float lightScatter = 1.0f + (fd90 - 1.0f) * pow(saturate(1.0f - NdotL), 5.0f);
    float viewScatter = 1.0f + (fd90 - 1.0f) * pow(saturate(1.0f - NdotV), 5.0f);
    return lightScatter * viewScatter / PI;
}

// Directional albedo approximation for multiscattering (Lazarov 2013 / Kulla-Conty)
// Returns approximate integral of single-scatter BRDF over hemisphere
float DirectionalAlbedo(float NdotV, float roughness)
{
    float a = roughness;
    float r = 1.0f - a;
    return 1.0f - (1.0f - NdotV) * pow(1.0f - r, 5.0f)
         - a * (0.1f + 0.9f * pow(1.0f - NdotV, 2.0f));
}

// Multiscatter energy compensation (Fdez-Aguera / Kulla-Conty approximation)
// Adds energy lost to inter-microfacet bounces at high roughness
float3 MultiscatterCompensation(float3 F0, float NdotV, float NdotL, float roughness)
{
    float Eo = DirectionalAlbedo(NdotV, roughness);
    float Ei = DirectionalAlbedo(NdotL, roughness);
    float Eavg = F0.r * 0.2126f + F0.g * 0.7152f + F0.b * 0.0722f;
    float denom = 1.0f - Eavg * (1.0f - Eo);
    return (1.0f - Eo) * (1.0f - Ei) / max(denom, 0.001f) * F0;
}

// Full PBR direct lighting calculation
// diffuseMode: 0=Disney/Burley, 1=Lambertian, 2=Oren-Nayar
float3 PBRDirectLighting(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float metallic,
    float roughness,
    uint diffuseMode)
{
    roughness = max(roughness, MIN_ROUGHNESS);

    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float HdotV = max(dot(H, V), 0.0f);
    float LdotH = max(dot(L, H), 0.0f);

    float3 F0 = CalculateF0(albedo, metallic);
    float3 F = F_Schlick(HdotV, F0);

    float3 specular = CookTorranceSpecular(NdotH, NdotV, NdotL, HdotV, roughness, F0);
    specular += MultiscatterCompensation(F0, NdotV, NdotL, roughness);

    float fd = (diffuseMode == 1)
        ? LambertianDiffuse()
        : DisneyDiffuse(NdotV, NdotL, LdotH, roughness);

    float3 kD = (1.0f - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo * fd;

    return (diffuse + specular) * lightColor * NdotL;
}

// Simplified ambient term (placeholder for future IBL)
float3 PBRAmbient(
    float3 albedo,
    float3 N,
    float3 V,
    float metallic,
    float roughness,
    float ao,
    float3 ambientColor)
{
    float3 F0 = CalculateF0(albedo, metallic);
    float NdotV = max(dot(N, V), 0.0f);
    float3 F = F_SchlickRoughness(NdotV, F0, roughness);

    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    // Diffuse ambient
    float3 diffuseAmbient = kD * albedo * ambientColor;

    // Approximate specular ambient (will be replaced by IBL)
    float3 specularAmbient = F * ambientColor * 0.3f;

    return (diffuseAmbient + specularAmbient) * ao;
}

#endif // PBR_BRDF_H
