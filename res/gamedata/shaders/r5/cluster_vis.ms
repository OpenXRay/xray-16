#define SM_6_0
#include "shared/common.h"
#include "visbuffer_common.h"

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_VisibleEntries : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
StructuredBuffer<InstanceData> g_DynamicInstanceData : register(t20);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);
ByteAddressBuffer g_ClusterArgs : register(t21);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
    nointerpolation uint drawID : TEXCOORD2;
    nointerpolation uint visID : TEXCOORD3;
};

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(64, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint threadID : SV_GroupIndex,
    OutputVertices<VS_OUTPUT, 192> vertices, OutputIndices<uint3, 64> triangles)
{
    uint visibleCount = g_ClusterArgs.Load(4u);
    uint meshGroupsX = g_ClusterArgs.Load(16u);
    uint linearGroup = groupID.x + groupID.y * meshGroupsX;
    uint slot = linearGroup / 2u;
    uint firstTriangle = (linearGroup % 2u) * 64u;
    uint entryIdx = 0u;
    ClusterEntry e = (ClusterEntry)0;
    uint triangleCount = 0u;
    if (slot < visibleCount)
    {
        entryIdx = g_VisibleEntries[slot];
        e = g_Entries[entryIdx];
        uint entryTriangleCount = e.indexCount / 3u;
        if (firstTriangle < entryTriangleCount)
            triangleCount = min(entryTriangleCount - firstTriangle, 64u);
    }

    SetMeshOutputCounts(triangleCount * 3u, triangleCount);

    if (threadID < triangleCount)
    {
        uint originalTriangle = firstTriangle + threadID;
        uint firstOutputVertex = threadID * 3u;
        float4x4 world = (e.flags & CLUSTER_ENTRY_FLAG_DYNAMIC) != 0u ? g_DynamicInstanceData[e.batchIndex].world : g_InstanceData[e.batchIndex].world;
        [unroll]
        for (uint corner = 0u; corner < 3u; ++corner)
        {
            uint local = originalTriangle * 3u + corner;
            uint index = g_MegaIB.Load((e.ibFirst + local) * 4u);
            uint vertexByte = (e.firstVertex + index) * 48u;
            uint3 w0 = g_MegaVB.Load3(vertexByte);
            uint2 uvw = g_MegaVB.Load2(vertexByte + 24u);
            float3 position = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
            float4 worldPos = mul(world, float4(position, 1.0));
            VS_OUTPUT output;
            output.position = mul(m_VP, float4(worldPos.xyz, 1.0));
            output.texcoord = float2(asfloat(uvw.x), asfloat(uvw.y));
            output.materialID = e.materialID;
            output.drawID = slot;
            output.visID = PackVisID(entryIdx, originalTriangle);
            vertices[firstOutputVertex + corner] = output;
        }
        triangles[threadID] = uint3(firstOutputVertex, firstOutputVertex + 1u, firstOutputVertex + 2u);
    }
}
