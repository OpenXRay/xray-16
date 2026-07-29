#ifndef SKIN_SSS_H
#define SKIN_SSS_H

float3 SkinSSSTint()
{
    return float3(1.0, 0.42, 0.32);
}

float3 LeafSSSTint()
{
    return float3(0.5, 0.82, 0.4);
}

float3 PetalSSSTint()
{
    return float3(0.92, 0.5, 0.58);
}

float3 WaxSSSTint()
{
    return float3(1.0, 0.85, 0.62);
}

void EvalSkinSSSParams(
    float3 albedo,
    float metallic,
    float3 N,
    float3 V,
    float gloss,
    out float sssStrength,
    out float3 sssTint,
    out float sssThickness)
{
    sssStrength = 0.0;
    sssTint = SkinSSSTint();
    sssThickness = 1.0;
    if (metallic >= 0.35)
        return;

    float flesh = saturate((albedo.r - albedo.b) * 1.4 + 0.08);
    flesh *= saturate(1.08 - albedo.g * 0.22);
    if (flesh < 0.12)
        return;

    float ndv = saturate(dot(normalize(N), normalize(V)));
    sssStrength = 0.28 * flesh;
    sssThickness = saturate(0.4 + ndv * 0.35 + gloss * 0.08);
}

void ApplySSSMapSample(
    float4 sssSample,
    inout float sssStrength,
    inout float3 sssTint,
    inout float sssThickness,
    out float sssMask,
    out float sssProfile)
{
    sssThickness = saturate(lerp(sssThickness, 1.0 - sssSample.r, 0.65));
    float mapStr = saturate(sssSample.g) * 0.72;
    sssProfile = sssSample.b * 5.01;
    sssStrength = max(sssStrength, mapStr);
    sssMask = 0.0;
    if (sssProfile < 0.5)
    {
        sssTint = lerp(sssTint, SkinSSSTint(), 0.7);
        sssMask = saturate(mapStr * 0.85);
        sssStrength = max(sssStrength, mapStr * 0.4);
    }
    else if (sssProfile < 1.5)
    {
        sssTint = lerp(LeafSSSTint(), sssTint, 0.45);
        sssMask = saturate(mapStr * 0.9);
        sssStrength = max(sssStrength, mapStr * 0.48);
    }
    else if (sssProfile < 2.5)
    {
        sssTint = lerp(PetalSSSTint(), sssTint, 0.4);
        sssMask = saturate(mapStr * 0.85);
        sssStrength = max(sssStrength, mapStr * 0.45);
    }
    else if (sssProfile < 3.5)
    {
        sssTint = lerp(WaxSSSTint(), sssTint, 0.35);
        sssMask = saturate(mapStr * 0.75);
        sssStrength = max(sssStrength, mapStr * 0.4);
    }
    else
    {
        sssTint = float3(0.85, 0.92, 1.0);
        sssMask = 0.0;
    }
}

#endif
