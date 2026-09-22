#ifndef CLUSTER_FADE_H
#define CLUSTER_FADE_H

#include "common_dither.h"

StructuredBuffer<uint> g_DrawFades : register(t17);


void ClusterFadeDiscard(uint fadeWord, float4 svPosition)
{
    uint fA = 63u - (fadeWord & 63u);
    uint fB = (fadeWord >> 6) & 63u;
    if (fA == 63u && fB == 0u)
        return;
    float d = Bayer4x4Threshold(uint2(svPosition.xy));
    if (!(d >= float(fB) / 63.0 && d < float(fA) / 63.0))
        discard;
}

#endif
