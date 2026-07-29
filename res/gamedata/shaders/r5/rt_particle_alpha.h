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

bool ParticleTexelOpaque(
    ByteAddressBuffer particleVB,
    ByteAddressBuffer particleIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary)
{
    RTBatchInfo info = batchInfo[batchIdx];
    MaterialData mat = g_Materials[info.materialID];
    if (mat.flags & MAT_FLAG_EMISSIVE)
        return false;
    float2 uv = GetParticleHitUV(particleVB, particleIB, info, primIdx, bary);
    float4 diffuse = SampleDiffuseLevel(mat, uv);
    if (mat.flags & MAT_FLAG_ALPHA_TEST)
        return diffuse.a >= mat.alphaRef;
    if (mat.flags & MAT_FLAG_ALPHA_BLEND)
        return diffuse.a >= 0.5;
    return diffuse.a >= 0.5;
}

#endif
