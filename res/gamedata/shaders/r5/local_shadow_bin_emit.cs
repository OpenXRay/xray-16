#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"
#include "visbuffer_common.h"
#include "cluster_bvh_types.h"
#include "local_shadow_bvh_types.h"

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
    uint g_BinPad0;
    uint g_BinPad1;
    uint g_BinPad2;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t14);
StructuredBuffer<LocalShadowView> g_TileState : register(t17);
StructuredBuffer<uint4> g_PairBase : register(t20);
StructuredBuffer<uint> g_DirtyList : register(t21);
RWStructuredBuffer<uint> g_Stats : register(u0);
RWStructuredBuffer<uint2> g_PairsOpaque : register(u1);
RWStructuredBuffer<uint2> g_PairsTerrain : register(u2);
RWStructuredBuffer<uint2> g_PairsAT : register(u3);

groupshared uint gs_cursor[3];
groupshared uint gs_visited;

void writePair(uint stream, uint pos, uint2 pair)
{
    uint cap = (stream == 2u) ? g_CapAT : ((stream == 1u) ? g_CapTerrain : g_CapOpaque);
    if (pos >= cap)
    {
        uint d;
        InterlockedAdd(g_Stats[7], 1u, d);
        return;
    }
    if (stream == 2u)
        g_PairsAT[pos] = pair;
    else if (stream == 1u)
        g_PairsTerrain[pos] = pair;
    else
        g_PairsOpaque[pos] = pair;
}

typedef LocalViewQuery BvhQuery;

void BvhVisit(bool active, uint entryIdx, LocalViewQuery q)
{
    if (!active)
        return;
    uint v;
    InterlockedAdd(gs_visited, 1u, v);
    ClusterEntry e = g_Entries[entryIdx];
    bool at = (e.flags & CLUSTER_ENTRY_FLAG_AT) != 0u;
    bool terrain = (e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u;
    if (at && q.includeAT == 0u)
        return;
    if (!localEntryTouchesView(e, q))
        return;
    uint stream = at ? 2u : (terrain ? 1u : 0u);
    uint pos;
    InterlockedAdd(gs_cursor[stream], 1u, pos);
    writePair(stream, pos, uint2(entryIdx, q.slot));
}

#define CLUSTER_BVH_T_NODES t18
#define CLUSTER_BVH_T_INDEX t19
#include "cluster_bvh.h"

[numthreads(VSM_BVH_GROUP, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    uint4 pairBase = g_PairBase[gID.x];
    if (t < 3u)
        gs_cursor[t] = pairBase[t];
    if (t == 0u)
        gs_visited = 0u;
    GroupMemoryBarrierWithGroupSync();

    uint slot = g_DirtyList[gID.x];
    LocalViewQuery q = localQueryFromView(g_TileState[slot], slot, g_IncludeAT, g_ErrK);
    bvhTraverse(t, g_NodeCount, q);

    GroupMemoryBarrierWithGroupSync();
    if (t == 0u)
    {
        uint m;
        InterlockedMax(g_Stats[8], gs_visited, m);
    }
}
