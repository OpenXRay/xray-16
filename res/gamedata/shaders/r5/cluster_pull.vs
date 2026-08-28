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

struct InstanceData
{
    float4x4 world;
    uint materialID;
    uint flags;
    float pad0, pad1;
};

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_VisibleEntries : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);

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

float4 UnpackD3DColor(uint v)
{
    return float4(
        float((v >> 16) & 0xFFu),
        float((v >> 8) & 0xFFu),
        float(v & 0xFFu),
        float((v >> 24) & 0xFFu)) / 255.0;
}

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VS_OUTPUT output;

    uint slot = iid;
    uint entryIdx = g_VisibleEntries[slot];
    ClusterEntry e = g_Entries[entryIdx];

    uint local = min(vid, e.indexCount - 1u);
    uint index = g_MegaIB.Load((e.ibFirst + local) * 4u);
    uint vertexByte = (e.firstVertex + index) * 48u;

    uint4 w0 = g_MegaVB.Load4(vertexByte);
    uint4 w1 = g_MegaVB.Load4(vertexByte + 16u);

    float3 position = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
    float3 normalUnpacked = UnpackD3DColor(w0.w).rgb * 2.0 - 1.0;
    float3 tangentUnpacked = UnpackD3DColor(w1.x).rgb * 2.0 - 1.0;
    float3 binormalUnpacked = UnpackD3DColor(w1.y).rgb * 2.0 - 1.0;
    float2 texcoord = float2(asfloat(w1.z), asfloat(w1.w));

    InstanceData instanceData = g_InstanceData[e.batchIndex];
    float4x4 worldMatrix = instanceData.world;

    float4 worldPos = mul(worldMatrix, float4(position, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(m_VP, float4(worldPos.xyz, 1.0));

    float3x3 worldMatrix3x3 = (float3x3)worldMatrix;
    output.normal = normalize(mul(worldMatrix3x3, normalUnpacked));
    output.tangent = normalize(mul(worldMatrix3x3, tangentUnpacked));
    output.bitangent = normalize(mul(worldMatrix3x3, binormalUnpacked));

    output.texcoord = texcoord;
    output.materialID = e.materialID;
    output.drawID = slot;

    return output;
}
