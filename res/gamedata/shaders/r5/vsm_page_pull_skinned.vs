#define SM_6_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"

struct ClusterEntry
{
    float4 sphere;
    float4 lodSelf;
    float4 lodParent;
    uint indexCount;
    uint ibFirst;
    uint firstVertex;
    uint batchIndex;
    uint materialID;
    uint flags;
    float selfError;
    float parentError;
    float3 extent;
    float extentPad;
};

StructuredBuffer<uint2> g_Pairs : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
StructuredBuffer<uint4> g_PageList : register(t17);
ByteAddressBuffer g_SkinnedVB : register(t43);
ByteAddressBuffer g_SkinnedIB : register(t44);

#define VSM_ROUTE_ATLAS_W VSM_ATLAS_W
#define VSM_ROUTE_ATLAS_H VSM_ATLAS_H
#include "vsm_page_route.h"

struct VS_OUTPUT
{
    precise float4 position : SV_Position;
    float clip[4] : SV_ClipDistance;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VS_OUTPUT output;

    uint2 pair = g_Pairs[iid];
    ClusterEntry e = g_Entries[pair.x];
    uint slot = pair.y;

    uint local = min(vid, e.indexCount - 1u);
    uint index = g_SkinnedIB.Load((e.ibFirst + local) * 4u);
    uint vertexByte = (e.firstVertex + index) * 48u;
    uint4 w0 = g_SkinnedVB.Load4(vertexByte);
    uint4 w1 = g_SkinnedVB.Load4(vertexByte + 16u);
    float3 worldPos = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
    float2 texcoord = float2(asfloat(w1.z), asfloat(w1.w));

    VsmRoute r = VsmRoutePage(slot, worldPos);
    output.position = r.position;
    output.clip[0] = r.clip.x;
    output.clip[1] = r.clip.y;
    output.clip[2] = r.clip.z;
    output.clip[3] = r.clip.w;
    output.texcoord = texcoord;
    output.materialID = e.materialID;
    return output;
}
