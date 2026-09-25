#ifndef BLOOM_COMMON_H
#define BLOOM_COMMON_H

#define BLOOM_GROUP_SIZE 8

Texture2D<float4> t_Source : register(t0);
RWTexture2D<float4> u_Target : register(u0);
SamplerState smp_rtlinear : register(s0);

cbuffer BloomParams : register(b0)
{
    float2 g_SourceTexelSize;
    uint2 g_TargetSize;
    uint g_FirstLevel;
    uint3 g_BloomPad;
};

float2 BloomTargetUV(uint2 pixel)
{
    return (float2(pixel) + 0.5) / float2(g_TargetSize);
}

#endif
