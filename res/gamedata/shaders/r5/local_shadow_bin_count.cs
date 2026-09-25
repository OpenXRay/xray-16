#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"
#define CLUSTER_GEO_AUTO_BIND
#define CLUSTER_GEO_RESIDENCY
#include "cluster_geo_bindings.h"
#include "cluster_bvh_types.h"
#include "local_shadow_bvh_types.h"
#include "geometry_cut_common.h"

cbuffer LocalShadowBinParams
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

StructuredBuffer<LocalShadowView> g_Request;
StructuredBuffer<uint4> g_CandList;
StructuredBuffer<LocalShadowView> g_TileState;
RWStructuredBuffer<uint4> g_TileCount;
RWStructuredBuffer<uint2> g_GeometryDirty;

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
