#ifndef GLASS_SSS_H
#define GLASS_SSS_H

float3 EvaluateGlassTransmission(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float shadow,
    float thickness,
    float intensity)
{
    if (intensity <= 1e-5)
        return float3(0, 0, 0);

    N = normalize(N);
    V = normalize(V);
    L = normalize(L);

    float thin = saturate(thickness);
    float NdV = saturate(dot(N, V));
    float fresnel = pow(1.0 - NdV, 4.0);
    float through = pow(saturate(dot(V, -L)), 2.2);
    float wrap = saturate((-dot(N, L) + 0.35) / 1.35);
    float term = (through * 0.85 + wrap * 0.45) * thin * intensity * (0.35 + fresnel * 0.65);
    float tShadow = lerp(shadow, 1.0, 0.8);
    float3 tint = lerp(albedo, float3(0.85, 0.92, 1.0), 0.55);
    return tint * lightColor * term * tShadow;
}

#endif
