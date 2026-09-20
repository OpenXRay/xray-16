#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"
#define CLUSTER_GEO_T_REFS t14
#define CLUSTER_GEO_T_META t20
#define CLUSTER_GEO_T_INSTANCES t21
#define CLUSTER_GEO_RESIDENCY
#define CLUSTER_GEO_T_GROUPS t26
#define CLUSTER_GEO_T_GROUP_STATE t27
#include "cluster_geo_bindings.h"
#include "cluster_bvh_types.h"
#include "local_shadow_bvh_types.h"
#define GEOMETRY_CUTS_REGISTER t28
#include "geometry_cut_common.h"

cbuffer LocalShadowBinParams : register(b5)
{
    uint g_CandCount;
    uint g_NodeCount;
    uint g_IncludeAT;
    float g_ErrK;
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_Budget;
    uint g_Frame;
    uint g_BinRefCount;
    uint g_ResidencyStreaming;
    uint g_BinPad2;
};

StructuredBuffer<LocalShadowView> g_Request : register(t15);
StructuredBuffer<uint4> g_CandList : register(t16);
StructuredBuffer<LocalShadowView> g_TileState : register(t17);
RWStructuredBuffer<uint4> g_TileCount : register(u0);
RWStructuredBuffer<uint2> g_GeometryDirty : register(u1);

groupshared uint gs_count[3];
groupshared uint gs_geometryDirty;

typedef LocalViewQuery BvhQuery;

void BvhVisit(bool active, uint entryIdx, LocalViewQuery q)
{
    if (!active)
        return;
    ClusterEntry e = LoadClusterEntry(entryIdx);
    bool at = (e.flags & CLUSTER_ENTRY_FLAG_AT) != 0u;
    bool terrain = (e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u;
    if (at && q.includeAT == 0u)
        return;
    if (!localEntryTouchesView(e, q))
        return;
    uint stream = at ? 2u : (terrain ? 1u : 0u);
    uint d;
    InterlockedAdd(gs_count[stream], 1u, d);
}

#define CLUSTER_BVH_T_NODES t18
#define CLUSTER_BVH_T_INDEX t19
#include "cluster_bvh.h"

[numthreads(VSM_BVH_GROUP, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    if (t < 3u)
        gs_count[t] = 0u;

    uint g = gID.x;
    uint4 cand = g_CandList[g];
    LocalShadowView st = g_TileState[cand.x];
    if (t == 0u)
    {
        uint2 geometry = g_GeometryDirty[cand.x];
        uint2 header = g_GeometryCuts.Load2(0u);
        if (geometry.x < header.y && geometry.y == 0u && st.zparams.w > 0.5)
        {
            for (uint i = geometryFirstCutAfter(geometry.x, header.x); i < header.x; ++i)
            {
                if (geometryCutTouchesPlanes(i, st.planes))
                {
                    geometry.y = 1u;
                    break;
                }
            }
        }
        geometry.x = header.y;
        g_GeometryDirty[cand.x] = geometry;
        gs_geometryDirty = geometry.y;
    }
    GroupMemoryBarrierWithGroupSync();
    bool release = cand.y == 0u;
    bool upToDate = st.zparams.w > 0.5 && st.meta.x == cand.y && st.meta.y == cand.z && gs_geometryDirty == 0u;
    uint flags = release ? 2u : (upToDate ? 1u : 0u);
    if (flags != 0u)
    {
        if (t == 0u)
            g_TileCount[g] = uint4(0u, 0u, 0u, flags);
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    LocalViewQuery q = localQueryFromView(g_Request[cand.x], cand.x, g_IncludeAT, g_ErrK, g_ResidencyStreaming);
    bvhTraverse(t, g_NodeCount, q);

    GroupMemoryBarrierWithGroupSync();
    if (t == 0u)
        g_TileCount[g] = uint4(gs_count[0], gs_count[1], gs_count[2], 0u);
}
