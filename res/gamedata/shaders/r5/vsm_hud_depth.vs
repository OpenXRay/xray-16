#define SM_6_0
#include "common.h"

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
};

cbuffer VsmHudParams : register(b5)
{
    float4x4 g_HudViewProj;
};

StructuredBuffer<uint> g_HudEntries : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_SkinnedVB : register(t43);
ByteAddressBuffer g_SkinnedIB : register(t44);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VS_OUTPUT output;

    ClusterEntry e = g_Entries[g_HudEntries[iid]];
    uint local = min(vid, e.indexCount - 1u);
    uint index = g_SkinnedIB.Load((e.ibFirst + local) * 4u);
    uint vertexByte = (e.firstVertex + index) * 48u;
    uint4 w0 = g_SkinnedVB.Load4(vertexByte);
    uint4 w1 = g_SkinnedVB.Load4(vertexByte + 16u);
    float3 worldPos = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));

    output.position = mul(g_HudViewProj, float4(worldPos, 1.0));
    output.texcoord = float2(asfloat(w1.z), asfloat(w1.w));
    output.materialID = e.materialID;
    return output;
}
