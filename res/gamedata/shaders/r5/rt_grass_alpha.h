#ifndef RT_GRASS_ALPHA_H
#define RT_GRASS_ALPHA_H

#include "bindless_common.h"
#include "rt_common.h"

bool GrassTexelOpaque(
    ByteAddressBuffer grassVB,
    ByteAddressBuffer grassIB,
    StructuredBuffer<RTBatchInfo> batchInfo,
    uint batchIdx,
    uint primIdx,
    float2 bary,
    uint detailAtlasIndex)
{
    if (detailAtlasIndex == 0)
        return true;
    RTBatchInfo info = batchInfo[batchIdx];
    float2 uv = GetSkinnedHitUV(grassVB, grassIB, info, primIdx, bary);
    return GetBindlessTexture(detailAtlasIndex).SampleLevel(smp_linear, uv, 0).a >= 0.3;
}

#endif
