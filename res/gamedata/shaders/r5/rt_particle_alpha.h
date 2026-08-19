#ifndef RT_PARTICLE_ALPHA_H
#define RT_PARTICLE_ALPHA_H

#include "bindless_common.h"
#include "rt_common.h"

float2 GetParticleHitUV(ByteAddressBuffer particleVB, ByteAddressBuffer particleIB,
                        RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = particleIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = particleIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = particleIB.Load(triBase * 4 + 8) + info.baseVertex;

    float2 uv0 = asfloat(particleVB.Load2(i0 * 32 + 16));
    float2 uv1 = asfloat(particleVB.Load2(i1 * 32 + 16));
    float2 uv2 = asfloat(particleVB.Load2(i2 * 32 + 16));

    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    return uv0 * w0 + uv1 * barycentrics.x + uv2 * barycentrics.y;
}

uint GetParticleHitMaterial(ByteAddressBuffer particleVB, ByteAddressBuffer particleIB,
                            RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = particleIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint matId = particleVB.Load(i0 * 32 + 24);
    return matId;
}

float4 UnpackParticleColor(uint c)
{
    return float4(
        float((c >> 16) & 255),
        float((c >> 8) & 255),
        float(c & 255),
        float((c >> 24) & 255)) / 255.0;
}

float4 GetParticleHitColor(ByteAddressBuffer particleVB, ByteAddressBuffer particleIB,
                           RTBatchInfo info, uint primitiveIndex, float2 barycentrics)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = particleIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = particleIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = particleIB.Load(triBase * 4 + 8) + info.baseVertex;
    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    return UnpackParticleColor(particleVB.Load(i0 * 32 + 12)) * w0
        + UnpackParticleColor(particleVB.Load(i1 * 32 + 12)) * barycentrics.x
        + UnpackParticleColor(particleVB.Load(i2 * 32 + 12)) * barycentrics.y;
}

float3 GetParticleHitGeoNormal(ByteAddressBuffer particleVB, ByteAddressBuffer particleIB,
                               RTBatchInfo info, uint primitiveIndex)
{
    uint triBase = (info.startIndex + primitiveIndex * 3);
    uint i0 = particleIB.Load(triBase * 4 + 0) + info.baseVertex;
    uint i1 = particleIB.Load(triBase * 4 + 4) + info.baseVertex;
    uint i2 = particleIB.Load(triBase * 4 + 8) + info.baseVertex;
    float3 p0 = asfloat(particleVB.Load3(i0 * 32));
    float3 p1 = asfloat(particleVB.Load3(i1 * 32));
    float3 p2 = asfloat(particleVB.Load3(i2 * 32));
    return normalize(cross(p1 - p0, p2 - p0));
}

bool ParticleTexelGlowing(
    ByteAddressBuffer particleVB,
    ByteAddressBuffer particleIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary)
{
    RTBatchInfo info = batchInfo[batchIdx];
    uint matId = GetParticleHitMaterial(particleVB, particleIB, info, primIdx, bary);
    MaterialData mat = g_Materials[matId];
    if ((mat.flags & MAT_FLAG_EMISSIVE) == 0)
        return false;
    float2 uv = GetParticleHitUV(particleVB, particleIB, info, primIdx, bary);
    float4 diffuse = SampleDiffuseLevel(mat, uv);
    float4 vcol = GetParticleHitColor(particleVB, particleIB, info, primIdx, bary);
    return ParticleTexelAlpha(diffuse) * vcol.a > (0.01 / 255.0);
}

bool ParticleTexelOpaque(
    ByteAddressBuffer particleVB,
    ByteAddressBuffer particleIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary)
{
    return false;
}

#endif
