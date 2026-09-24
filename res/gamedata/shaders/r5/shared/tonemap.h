#ifndef SHARED_TONEMAP_H
#define SHARED_TONEMAP_H

#define TONEMAP_NONE 0
#define TONEMAP_PBR_NEUTRAL 1
#define TONEMAP_AGX 2
#define TONEMAP_ACES 3

float3 TonemapPbrNeutral(float3 color)
{
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression)
        return color;
    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;
    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return lerp(color, newPeak.xxx, g);
}

float3 AgxContrast(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
    return 15.5 * x4 * x2 - 40.14 * x4 * x + 31.96 * x4 - 6.868 * x2 * x + 0.4298 * x2 + 0.1191 * x - 0.00232;
}

float3 TonemapAgx(float3 color)
{
    const float3x3 inset = float3x3(
        0.842479062253094, 0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772, 0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104);
    const float3x3 outset = float3x3(
        1.19687900512017, -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    const float minEv = -12.47393;
    const float maxEv = 4.026069;
    float3 encoded = mul(color, inset);
    encoded = clamp(log2(max(encoded, 1e-10)), minEv, maxEv);
    encoded = AgxContrast((encoded - minEv) / (maxEv - minEv));
    return pow(max(mul(encoded, outset), 0.0), 2.2);
}

float3 TonemapAcesFitted(float3 color)
{
    const float3x3 acesInput = float3x3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777);
    const float3x3 acesOutput = float3x3(
        1.60475, -0.53108, -0.07367,
        -0.10208, 1.10813, -0.00605,
        -0.00327, -0.07276, 1.07602);
    color = mul(acesInput, color);
    float3 a = color * (color + 0.0245786) - 0.000090537;
    float3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return saturate(mul(acesOutput, a / b));
}

float3 Tonemap(float3 color, uint tonemapper)
{
    if (tonemapper == TONEMAP_PBR_NEUTRAL)
        return saturate(TonemapPbrNeutral(color));
    if (tonemapper == TONEMAP_AGX)
        return saturate(TonemapAgx(color));
    if (tonemapper == TONEMAP_ACES)
        return TonemapAcesFitted(color);
    return saturate(color);
}

#endif
