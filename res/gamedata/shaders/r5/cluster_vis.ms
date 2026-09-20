#define SM_6_0
#include "shared/common.h"
#define CLUSTER_GEO_T_REFS t14
#define CLUSTER_GEO_T_META t16
#define CLUSTER_GEO_T_INSTANCES t20
#include "cluster_geo_bindings.h"
#include "cluster_geo_payload.h"

StructuredBuffer<uint> g_VisibleEntries : register(t15);
ByteAddressBuffer g_ClusterArgs : register(t21);

struct MS_VERTEX
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
    nointerpolation uint drawID : TEXCOORD2;
};

struct MS_PRIMITIVE
{
#ifdef TARGET_SPIRV
    uint visID : SV_PrimitiveID;
#else
    uint visID : TEXCOORD3;
#endif
};

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(CLUSTER_MESH_LANES, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint threadID : SV_GroupIndex,
    OutputVertices<MS_VERTEX, CLUSTER_MAX_VERTICES> vertices,
    OutputIndices<uint3, CLUSTER_MAX_TRIANGLES> triangles,
    OutputPrimitives<MS_PRIMITIVE, CLUSTER_MAX_TRIANGLES> primitives)
{
    uint visibleCount = g_ClusterArgs.Load(4u);
    uint meshGroupsX = g_ClusterArgs.Load(16u);
    uint slot = groupID.x + groupID.y * meshGroupsX;

    uint refIdx = 0u;
    ClusterRefView view = (ClusterRefView)0;
    ClusterGeoView geo = (ClusterGeoView)0;
    uint vertexCount = 0u;
    uint triangleCount = 0u;
    if (slot < visibleCount)
    {
        refIdx = g_VisibleEntries[slot];
        view = LoadClusterRefView(refIdx);
        geo = ClusterGeoResolve(view.entry);
        vertexCount = geo.vertexCount;
        triangleCount = geo.triangleCount;
    }

    SetMeshOutputCounts(vertexCount, triangleCount);

    if (threadID < vertexCount)
    {
        uint slotIndex = ClusterPayloadSlot(geo, threadID);
        float3 position = ClusterLoadPositionSlot(geo, slotIndex);
        float4 worldPos = mul(view.world, float4(position, 1.0));

        MS_VERTEX output;
        output.position = mul(m_VP, float4(worldPos.xyz, 1.0));
        output.texcoord = ClusterLoadUVSlot(geo, slotIndex);
        output.materialID = view.entry.materialID;
        output.drawID = slot;
        vertices[threadID] = output;
    }

    if (threadID < triangleCount)
    {
        triangles[threadID] = ClusterPayloadTriangle(geo, threadID);

        MS_PRIMITIVE prim;
        prim.visID = PackVisID(refIdx, threadID);
        primitives[threadID] = prim;
    }
}
