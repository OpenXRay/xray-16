#ifndef CLUSTER_FADE_H
#define CLUSTER_FADE_H

StructuredBuffer<uint> g_DrawFades : register(t17);

static const float kClusterBayer4x4[16] = {
    0.0, 8.0, 2.0, 10.0,
    12.0, 4.0, 14.0, 6.0,
    3.0, 11.0, 1.0, 9.0,
    15.0, 7.0, 13.0, 5.0
};

void ClusterFadeDiscard(uint fadeWord, float4 svPosition)
{
    uint fA = 63u - (fadeWord & 63u);
    uint fB = (fadeWord >> 6) & 63u;
    if (fA == 63u && fB == 0u)
        return;
    uint2 p = uint2(svPosition.xy);
    float d = (kClusterBayer4x4[(p.y & 3u) * 4u + (p.x & 3u)] + 0.5) / 16.0;
    if (!(d >= float(fB) / 63.0 && d < float(fA) / 63.0))
        discard;
}

#endif
