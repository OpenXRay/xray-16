#define SM_6_0
#include "common.h"
#define CLUSTER_GEO_T_REFS t14
#define CLUSTER_GEO_T_META t16
#define CLUSTER_GEO_T_INSTANCES t20
#include "cluster_geo_bindings.h"
#include "cluster_geo_payload.h"
#include "sw_raster_common.h"
#include "sw_dispatch_common.h"

StructuredBuffer<uint> g_SwEntries : register(t15);
ByteAddressBuffer g_SwDispatchArgs : register(t22);

cbuffer SwRasterParams : register(b5)
{
    uint g_Width;
    uint g_Height;
    uint2 g_SwPad;
};

groupshared float3 gs_Projected[CLUSTER_SW_LANES];
groupshared uint gs_Valid[CLUSTER_SW_LANES];

[numthreads(CLUSTER_SW_LANES, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint lane : SV_GroupIndex)
{
    uint slot = SwDispatchLinearGroup(groupID);
    if (slot >= g_SwDispatchArgs.Load(16u))
        return;

    uint refIdx = g_SwEntries[slot];
    ClusterRefView view = LoadClusterRefView(refIdx);
    ClusterEntry e = view.entry;
    ClusterGeoView geo = ClusterGeoResolve(e);

    float4x4 wvp = mul(m_VP, view.world);

    float3 projected = float3(0.0, 0.0, 0.0);
    uint valid = 0u;
    if (lane < geo.vertexCount)
        valid = SwProjectVertex(wvp, ClusterLoadPosition(geo, lane), g_Width, g_Height, projected) ? 1u : 0u;
    gs_Projected[lane] = projected;
    gs_Valid[lane] = valid;

    GroupMemoryBarrierWithGroupSync();

    if (lane >= geo.triangleCount)
        return;

    uint3 corners = ClusterPayloadTriangle(geo, lane);
    if (gs_Valid[corners.x] == 0u || gs_Valid[corners.y] == 0u || gs_Valid[corners.z] == 0u)
        return;

    SwRasterizeTriangle(gs_Projected[corners.x], gs_Projected[corners.y], gs_Projected[corners.z],
        g_Width, g_Height, PackVisID(refIdx, lane));
}
