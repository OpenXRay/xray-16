#ifndef NRD_HELPERS_H
#define NRD_HELPERS_H

#ifndef NRD_MATERIAL_FACTOR_MIN_SCALE
#define NRD_MATERIAL_FACTOR_MIN_SCALE 0.02
#endif
#ifndef NRD_ROUGHNESS_FACTOR_MIN_SCALE
#define NRD_ROUGHNESS_FACTOR_MIN_SCALE 0.1
#endif
#ifndef NRD_EPS
#define NRD_EPS 1e-6
#endif
#ifndef NRD_FP16_MAX
#define NRD_FP16_MAX 65504.0
#endif

float3 NRD_EnvironmentTerm_Rtg(float3 Rf0, float NoV, float roughness)
{
    float m = saturate(roughness * roughness);

    float4 X;
    X.x = 1.0;
    X.y = NoV;
    X.z = NoV * NoV;
    X.w = NoV * X.z;

    float4 Y;
    Y.x = 1.0;
    Y.y = m;
    Y.z = m * m;
    Y.w = m * Y.z;

    const float2x2 M1 = float2x2(0.99044, -1.28514, 1.29678, -0.755907);
    const float3x3 M2 = float3x3(1.0, 2.92338, 59.4188, 20.3225, -27.0302, 222.592, 121.563, 626.13, 316.627);
    const float2x2 M3 = float2x2(0.0365463, 3.32707, 9.0632, -9.04756);
    const float3x3 M4 = float3x3(1.0, 3.59685, -1.36772, 9.04401, -16.3174, 9.22949, 5.56589, 19.7886, -20.2123);

    float bias = dot(mul(M1, X.xy), Y.xy) * rcp(max(dot(mul(M2, X.xyw), Y.xyw), NRD_EPS));
    float scale = dot(mul(M3, X.xy), Y.xy) * rcp(max(dot(mul(M4, X.xzw), Y.xyw), NRD_EPS));

    return saturate(Rf0 * scale + bias);
}

void NRD_MaterialFactors(float3 N, float3 V, float3 albedo, float3 Rf0, float roughness, out float3 diffFactor, out float3 specFactor)
{
    float NoV = abs(dot(N, V));
    float3 Fenv = NRD_EnvironmentTerm_Rtg(Rf0, NoV, roughness);

    diffFactor = (1.0 - Fenv) * albedo;
    diffFactor = lerp(NRD_MATERIAL_FACTOR_MIN_SCALE.xxx, float3(1.0, 1.0, 1.0), diffFactor);

    specFactor = Fenv;
    specFactor *= lerp(NRD_ROUGHNESS_FACTOR_MIN_SCALE.xxx, float3(1.0, 1.0, 1.0), roughness);
    specFactor = lerp(NRD_MATERIAL_FACTOR_MIN_SCALE.xxx, float3(1.0, 1.0, 1.0), specFactor);
}

float3 NRD_LinearToYCoCg(float3 color)
{
    float Y = dot(color, float3(0.25, 0.5, 0.25));
    float Co = dot(color, float3(0.5, 0.0, -0.5));
    float Cg = dot(color, float3(-0.25, 0.5, -0.25));
    return float3(Y, Co, Cg);
}

float3 NRD_YCoCgToLinear(float3 color)
{
    float t = color.x - color.z;
    float3 r;
    r.y = color.x + color.z;
    r.x = t + color.y;
    r.z = t - color.y;
    return max(r, 0.0);
}

float NRD_GetSpecMagicCurve(float roughness)
{
    float f = 1.0 - exp2(-200.0 * roughness * roughness);
    f *= pow(saturate(roughness), 0.5);
    return f;
}

float REBLUR_GetNormHitDist(float hitDist, float viewZ, float3 hitDistParams, float roughness)
{
    float smc = NRD_GetSpecMagicCurve(roughness);
    float f = (hitDistParams.x + abs(viewZ) * hitDistParams.y) * lerp(hitDistParams.z, 1.0, smc);
    return saturate(hitDist / max(f, 1e-4));
}

float NRD_TrimHitDistance(float hitDist, float threshold)
{
    return hitDist < threshold ? 0.0 : hitDist;
}

float3 NRD_SanitizeRadiance(float3 radiance)
{
    if (any(isnan(radiance)) || any(isinf(radiance)))
        return 0;
    return clamp(radiance, 0.0, NRD_FP16_MAX);
}

#endif
