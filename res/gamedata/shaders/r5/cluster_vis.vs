#define SM_6_0
#include "shared/common.h"
#include "visbuffer_common.h"

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_VisibleEntries : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);

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

    uint slot = iid;
    uint entryIdx = g_VisibleEntries[slot];
    ClusterEntry e = g_Entries[entryIdx];

    uint local = min(vid, e.indexCount - 1u);
    uint index = g_MegaIB.Load((e.ibFirst + local) * 4u);
    uint vertexByte = (e.firstVertex + index) * 48u;

    uint3 w0 = g_MegaVB.Load3(vertexByte);
    uint2 uvw = g_MegaVB.Load2(vertexByte + 24u);

    float3 position = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
    float4 worldPos = mul(g_InstanceData[e.batchIndex].world, float4(position, 1.0));
    output.position = mul(m_VP, float4(worldPos.xyz, 1.0));
    output.texcoord = float2(asfloat(uvw.x), asfloat(uvw.y));
    output.materialID = e.materialID;
    output.drawID = slot;
    output.visID = PackVisID(entryIdx, local / 3u);
    return output;
}
