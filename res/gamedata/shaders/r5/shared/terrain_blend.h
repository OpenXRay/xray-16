#ifndef TERRAIN_BLEND_H
#define TERRAIN_BLEND_H

float4 TerrainNormalizeMask(float4 mask)
{
    float s = dot(mask, float4(1, 1, 1, 1));
    return s > 0.001 ? mask / s : float4(0.25, 0.25, 0.25, 0.25);
}

float3 TerrainBlendRGB(float3 r, float3 g, float3 b, float3 a, float4 mask)
{
    return r * mask.r + g * mask.g + b * mask.b + a * mask.a;
}

#endif
