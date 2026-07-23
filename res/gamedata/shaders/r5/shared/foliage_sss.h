// foliage_sss.h — basic subsurface / transmission for grass, leaves, bushes
// Thickness defaults to 1; pass a texture sample later (MAT_FLAG_HAS_SSS_MAP).
#ifndef FOLIAGE_SSS_H
#define FOLIAGE_SSS_H

// Wrap + backlit transmission (Jimenez-style cheap foliage SSS)
// Extra view↔light term so CoP density billboards still glow with sun behind them
// (flat cards often face the camera → classic -N·L transmission dies).
float3 EvaluateFoliageSSS(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,           // direction toward sun
    float3 lightColor,
    float shadow,       // 0..1 (can be softened for transmission)
    float3 sssTint,
    float thickness,    // 1 = opaque leaf, lower = thinner
    float intensity)
{
    if (intensity <= 1e-5)
        return float3(0, 0, 0);

    N = normalize(N);
    L = normalize(L);
    V = normalize(V);

    const float wrap = 0.45;
    float NdotL = dot(N, L);
    float wrapDiffuse = saturate((NdotL + wrap) / (1.0 + wrap));

    // Transmission through the blade (either face)
    float backLit = saturate(-NdotL);
    float3 H = normalize(L + N * 0.35);
    float VdotH = saturate(dot(V, -H));
    float scatter = pow(VdotH, 1.6) * backLit;

    // Looking into the sun through foliage (billboard-friendly)
    float through = pow(saturate(dot(V, -L)), 3.0);
    through *= lerp(0.35, 1.0, saturate(1.0 - abs(NdotL)));

    float thin = saturate(thickness);
    float term = (wrapDiffuse * 0.35 + scatter * 1.1 + through * 0.85) * thin * intensity;

    float sssShadow = lerp(shadow, 1.0, 0.65);

    return albedo * sssTint * lightColor * term * sssShadow;
}

#if defined(SKY_IBL)
// Soft sky bleed through foliage (cheap multi-scatter ambient)
float3 EvaluateFoliageSkySSS(
    float3 albedo,
    float3 N,
    float3 sssTint,
    float thickness,
    float intensity)
{
    if (intensity <= 1e-5)
        return float3(0, 0, 0);
    float3 sky = SampleSkyRGB(-normalize(N));
    float weather = length(L_ambient.rgb + L_hemi_color.rgb * L_hemi_color.w);
    return albedo * sssTint * sky * weather * thickness * intensity * 0.35;
}
#endif

#endif // FOLIAGE_SSS_H
