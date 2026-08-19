#ifndef FOLIAGE_SSS_H
#define FOLIAGE_SSS_H

float3 EvaluateFoliageSSS(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float shadow,
    float3 sssTint,
    float thickness,
    float intensity)
{
    if (intensity <= 1e-5)
        return float3(0, 0, 0);

    N = normalize(N);
    L = normalize(L);
    V = normalize(V);

    float thin = saturate(thickness);
    float thick = saturate(1.0 - thin);
    const float wrap = lerp(0.55, 0.35, thick);
    float NdotL = dot(N, L);
    float wrapDiffuse = saturate((NdotL + wrap) / (1.0 + wrap));

    float backLit = saturate(-NdotL);
    float3 H = normalize(L + N * 0.35);
    float VdotH = saturate(dot(V, -H));
    float scatter = pow(VdotH, lerp(1.35, 2.1, thick)) * backLit;

    float through = pow(saturate(dot(V, -L)), lerp(2.4, 3.6, thick));
    through *= lerp(0.45, 1.0, saturate(1.0 - abs(NdotL)));

    float term = (wrapDiffuse * 0.4 + scatter * 1.15 + through * 0.95) * thin * intensity;
    float sssShadow = lerp(shadow, 1.0, lerp(0.75, 0.45, thick));

    return albedo * sssTint * lightColor * term * sssShadow;
}

#if defined(SKY_IBL)
float3 EvaluateFoliageSkySSS(
    float3 albedo,
    float3 N,
    float3 sssTint,
    float thickness,
    float intensity)
{
    if (intensity <= 1e-5)
        return float3(0, 0, 0);
    float thin = saturate(thickness);
    float3 sky = SampleSkyRGB(-normalize(N));
    float weather = length(L_ambient.rgb + L_hemi_color.rgb * L_hemi_color.w);
    return albedo * sssTint * sky * weather * thin * intensity * 0.38;
}
#endif

#endif
