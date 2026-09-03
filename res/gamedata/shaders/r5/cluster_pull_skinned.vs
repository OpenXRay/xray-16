#define SM_6_0
#include "common.h"
#include "visbuffer_common.h"

StructuredBuffer<uint> g_VisibleEntries : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_SkinnedVB : register(t43);
ByteAddressBuffer g_SkinnedIB : register(t44);

struct VS_OUTPUT
{
    precise float4 position : SV_Position;
    float3 worldPos : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;
    float3 tangent  : TEXCOORD3;
    float3 bitangent: TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;
    nointerpolation uint drawID : TEXCOORD6;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VS_OUTPUT output;

    uint entryIdx = g_VisibleEntries[iid];
    ClusterEntry e = g_Entries[entryIdx];

    uint local = min(vid, e.indexCount - 1u);
    uint index = g_SkinnedIB.Load((e.ibFirst + local) * 4u);
    MegaVertex v = LoadMegaVertex(g_SkinnedVB, e.firstVertex + index);

    output.worldPos = v.position;
    output.position = mul(m_VP, float4(v.position, 1.0));
    output.normal = v.normal;
    output.tangent = v.tangent;
    output.bitangent = v.binormal;
    output.texcoord = v.uv;
    output.materialID = e.materialID;
    output.drawID = iid;
    return output;
}
