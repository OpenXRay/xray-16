#ifndef LOCAL_SHADOW_COMMON_H
#define LOCAL_SHADOW_COMMON_H

#define LOCAL_SHADOW_ATLAS 4096.0
#define LOCAL_SHADOW_HUD_ATLAS 4096.0
#define LOCAL_SHADOW_HUD_CASTERS 1u
#define LOCAL_SHADOW_HUD_TO_WORLD 2u
#define LOCAL_SHADOW_PULL_VERTICES 384u

struct LocalShadowView
{
    float4x4 viewProj;
    float4 rect;
    float4 zparams;
    float4 lightPos;
    float4 planes[6];
    float4 shape;
    uint4 meta;
    float4 hud;
    float4 hudZ;
    float4x4 hudViewProj;
};

#endif
