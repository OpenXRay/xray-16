#ifndef RT_GRASS_ALPHA_H
#define RT_GRASS_ALPHA_H

#include "bindless_common.h"
#include "rt_common.h"

static const float GRASS_ALPHA_CLIP = 96.0 / 255.0;

float GrassTexelAlpha(
    ByteAddressBuffer grassVB,
    ByteAddressBuffer grassIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary,
    uint detailAtlasIndex)
{
    if (detailAtlasIndex == 0)
        return 1.0;
    RTBatchInfo info = batchInfo[batchIdx];
    float2 uv = GetSkinnedHitUV(grassVB, grassIB, info, primIdx, bary);
    return GetBindlessTexture(detailAtlasIndex).SampleLevel(smp_linear, uv, 0).a;
}

bool GrassTexelOpaque(
    ByteAddressBuffer grassVB,
    ByteAddressBuffer grassIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary,
    uint detailAtlasIndex)
{
    return GrassTexelAlpha(grassVB, grassIB, batchInfo, batchIdx, primIdx, bary, detailAtlasIndex) >= GRASS_ALPHA_CLIP;
}

#endif
