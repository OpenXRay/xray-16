#ifndef VSM_SKINNED_ROUTE_H
#define VSM_SKINNED_ROUTE_H

#define VSM_ROUTE_ATLAS_W VSM_ATLAS_W
#define VSM_ROUTE_ATLAS_H VSM_ATLAS_H

StructuredBuffer<uint4> g_PageList : register(t17);
StructuredBuffer<uint> g_CasterPages : register(t18);

#include "vsm_page_route.h"

struct VS_OUTPUT
{
    precise float4 position : SV_Position;
    float clip[4] : SV_ClipDistance;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
};

VS_OUTPUT VsmSkinnedOut(uint drawIndex, float3 worldPos, float2 tc)
{
    VS_OUTPUT o;
    o.texcoord = tc;
    o.materialID = 0u;
    uint slot = g_CasterPages[drawIndex];
    if (slot >= uint(VSM_MAX_PHYS))
    {
        o.position = float4(2.0, 2.0, 2.0, 1.0);
        o.clip[0] = -1.0;
        o.clip[1] = -1.0;
        o.clip[2] = -1.0;
        o.clip[3] = -1.0;
        return o;
    }
    VsmRoute r = VsmRoutePage(slot, worldPos);
    o.position = r.position;
    o.clip[0] = r.clip.x;
    o.clip[1] = r.clip.y;
    o.clip[2] = r.clip.z;
    o.clip[3] = r.clip.w;
    return o;
}

#endif
