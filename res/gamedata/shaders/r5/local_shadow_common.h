#ifndef LOCAL_SHADOW_COMMON_H
#define LOCAL_SHADOW_COMMON_H

#define LOCAL_SHADOW_ATLAS 4096.0
#define LOCAL_SHADOW_PULL_VERTICES 384u

struct LocalShadowTile
{
    float4x4 viewProj;
    float4 rect;
    float4 zparams;
    float4 lightPos;
    float4 planes[6];
};

#endif
