#ifndef RT_MATERIAL_ALPHA_H
#define RT_MATERIAL_ALPHA_H

#include "bindless_common.h"
#include "rt_common.h"

float MaterialAlphaCut(MaterialData mat)
{
    if ((mat.flags & MAT_FLAG_ALPHA_TEST) != 0)
        return max(mat.alphaRef, 1.0 / 255.0);
    return 0.5;
}

float4 SampleDiffuseAlpha(MaterialData mat, float2 uv)
{
    if (mat.diffuseIndex == INVALID_TEXTURE_INDEX)
        return float4(1, 0, 1, 1);
    return GetBindlessTexture(mat.diffuseIndex).SampleLevel(smp_nofilter, uv, 0);
}

bool MaterialDiffuseOpaque(MaterialData mat, float4 diffuse)
{
    if ((mat.flags & MAT_FLAG_EMISSIVE) != 0)
        return false;
    if ((mat.flags & MAT_FLAG_WATER) != 0)
        return false;
    if ((mat.flags & MAT_FLAG_WMARK) != 0)
        return false;
    return diffuse.a >= MaterialAlphaCut(mat);
}

bool EmissiveTexelLit(MaterialData mat, float4 diffuse)
{
    if ((mat.flags & MAT_FLAG_EMISSIVE) == 0)
        return false;
    return GlowTexelMask(diffuse) > (0.01 / 255.0);
}

bool MegaMaterialOpaque(
    ByteAddressBuffer megaVB,
    ByteAddressBuffer megaIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary)
{
    RTBatchInfo info = batchInfo[batchIdx];
    MaterialData mat = g_Materials[info.materialID];
    float2 uv = GetHitUV(megaVB, megaIB, info, primIdx, bary);
    float4 diffuse = SampleDiffuseAlpha(mat, uv);
    return MaterialDiffuseOpaque(mat, diffuse);
}

bool MegaEmissiveHit(
    ByteAddressBuffer megaVB,
    ByteAddressBuffer megaIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary)
{
    RTBatchInfo info = batchInfo[batchIdx];
    MaterialData mat = g_Materials[info.materialID];
    float2 uv = GetHitUV(megaVB, megaIB, info, primIdx, bary);
    return EmissiveTexelLit(mat, SampleDiffuseLevel(mat, uv));
}

bool SkinnedMaterialOpaque(
    ByteAddressBuffer skinnedVB,
    ByteAddressBuffer skinnedIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary)
{
    RTBatchInfo info = batchInfo[batchIdx];
    MaterialData mat = g_Materials[info.materialID];
    float2 uv = GetSkinnedHitUV(skinnedVB, skinnedIB, info, primIdx, bary);
    float4 diffuse = SampleDiffuseAlpha(mat, uv);
    return MaterialDiffuseOpaque(mat, diffuse);
}

#endif
