#ifndef PBR_BRDF_H
#define PBR_BRDF_H

static const float PI = 3.14159265359f;
static const float MIN_ROUGHNESS = 0.04f;
static const float DIELECTRIC_F0 = 0.04f;

float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001f);
}

float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    return NdotV / (NdotV * (1.0f - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = G_SchlickGGX(NdotV, roughness);
    float ggx2 = G_SchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 oneMinusRoughness = 1.0f - roughness;
    return F0 + (max(oneMinusRoughness, F0) - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

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

float3 CalculateF0(float3 albedo, float metallic)
{
    return lerp(DIELECTRIC_F0, albedo, metallic);
}

float3 PBRDirectLighting(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float metallic,
    float roughness,
    float ao)
{
    roughness = max(roughness, MIN_ROUGHNESS);

    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float HdotV = max(dot(H, V), 0.0f);

    if (NdotL <= 0.0f)
        return float3(0.0f, 0.0f, 0.0f);

    float3 F0 = CalculateF0(albedo, metallic);

    float3 F = F_Schlick(HdotV, F0);

    float3 specular = CookTorranceSpecular(NdotH, NdotV, NdotL, HdotV, roughness, F0);

    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;

    float3 Lo = (diffuse + specular) * lightColor * NdotL;

    return Lo * ao;
}

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

    float3 diffuseAmbient = kD * albedo * ambientColor;

    float3 specularAmbient = F * ambientColor * 0.3f;

    return (diffuseAmbient + specularAmbient) * ao;
}

#endif
