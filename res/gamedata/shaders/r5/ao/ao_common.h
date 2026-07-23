// Shared helpers for FrameGraph ambient occlusion passes
#ifndef AO_COMMON_H
#define AO_COMMON_H

#define SM_6_0
#include "common.h"

cbuffer AOConstants : register(b3)
{
    float4 ao_screen;      // xy = size, zw = 1/size
    float4 ao_params;      // x=radius, y=bias, z=strength, w=quality (1..4)
    float4 ao_proj;        // x=gtao_parameters (focal), y=frame, zw=unused
};

Texture2D g_Depth : register(t0);
Texture2D g_Normal : register(t1);
Texture2D g_WorldPos : register(t2);

struct PS_INPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

float AoQuality()
{
    return max(ao_params.w, 1.0);
}

float AoRadius()
{
    return ao_params.x;
}

float AoBias()
{
    return ao_params.y;
}

float AoStrength()
{
    return ao_params.z;
}

float SampleDepth(float2 uv)
{
    return g_Depth.SampleLevel(smp_nofilter, uv, 0).x;
}

float3 SampleWorldNormal(float2 uv)
{
    // Forward FrameGraph writes signed world normals in RGBA16F (xyz = N, w = roughness).
    float3 n = g_Normal.SampleLevel(smp_nofilter, uv, 0).xyz;
    return normalize(n);
}

// GTAO/HBAO focal: ao_proj.x is already IX-Ray half-focal; full focal for world→UV helpers.
float AoFocalPixels()
{
    return max(ao_proj.x * 2.0, 1.0);
}

float4 SampleWorldPos(float2 uv)
{
    return g_WorldPos.SampleLevel(smp_nofilter, uv, 0);
}

float3 WorldToView(float3 worldPos)
{
    return mul(m_V, float4(worldPos, 1.0)).xyz;
}

// X-Ray view space: camera looks along +Z (depth increases with distance).
float ViewDepth(float3 viewPos)
{
    return max(abs(viewPos.z), 0.5);
}

// World radius → UV radius using full focal (pixels at z=1).
float2 WorldRadiusToUV(float worldRadius, float viewZ)
{
    float px = (worldRadius * AoFocalPixels()) / max(viewZ, 0.5);
    return px * ao_screen.zw;
}

// Interleaved gradient noise (blue-noise substitute)
float Ign(float2 pixel)
{
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}

float2 Ign2(float2 pixel)
{
    float n = Ign(pixel);
    return float2(n, Ign(pixel + 19.19));
}

bool IsSky(float depth, float4 worldPos)
{
    if (depth >= 0.9999)
        return true;
    if (worldPos.w < 0.5)
        return true;
    return false;
}

#endif
