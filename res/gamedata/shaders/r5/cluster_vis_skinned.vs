#define SM_6_0
#include "shared/common.h"
#include "visbuffer_common.h"

StructuredBuffer<ClusterEntry> g_SkinnedEntries : register(t42);
ByteAddressBuffer g_SkinnedVB : register(t43);
ByteAddressBuffer g_SkinnedIB : register(t44);

cbuffer SkinnedVisParams : register(b5)
{
    uint g_SkinnedEntryBase;
    uint3 g_SkinnedVisPad;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
    nointerpolation uint drawID : TEXCOORD2;
    nointerpolation uint visID : TEXCOORD3;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VS_OUTPUT output;

    ClusterEntry e = g_SkinnedEntries[iid];
    uint local = min(vid, e.indexCount - 1u);
    uint index = g_SkinnedIB.Load((e.ibFirst + local) * 4u);
    uint vertexByte = (e.firstVertex + index) * 48u;

    uint3 w0 = g_SkinnedVB.Load3(vertexByte);
    uint2 uvw = g_SkinnedVB.Load2(vertexByte + 24u);

    float3 position = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
    output.position = mul(m_VP, float4(position, 1.0));
    output.texcoord = float2(asfloat(uvw.x), asfloat(uvw.y));
    output.materialID = e.materialID;
    output.drawID = 0u;
    output.visID = PackVisID(g_SkinnedEntryBase + iid, local / 3u);
    return output;
}
