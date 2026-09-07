#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"
#include "visbuffer_common.h"
#include "cluster_bvh_types.h"
#include "local_shadow_bvh_types.h"

cbuffer LocalShadowDynBinParams : register(b5)
{
    uint g_EntryBase;
    uint g_EntryCount;
    uint g_StatsBase;
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_IncludeAT;
    uint g_StaticOverflow;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t14);
StructuredBuffer<LocalShadowView> g_Tiles : register(t15);
StructuredBuffer<uint> g_Refresh : register(t16);
RWStructuredBuffer<uint> g_Stats : register(u0);
RWStructuredBuffer<uint2> g_PairsOpaque : register(u1);
RWStructuredBuffer<uint2> g_PairsTerrain : register(u2);
RWStructuredBuffer<uint2> g_PairsAT : register(u3);

void emitStream(bool emit, uint cursor, uint cap, uint2 pair, uint stream)
{
    uint cnt = WaveActiveCountBits(emit);
    if (cnt == 0u)
        return;
    uint rank = WavePrefixCountBits(emit);
    uint base = 0u;
    if (WaveIsFirstLane())
    {
        InterlockedAdd(g_Stats[cursor], cnt, base);
        uint ignored;
        InterlockedAdd(g_Stats[g_StatsBase + stream], cnt, ignored);
    }
    base = WaveReadLaneFirst(base);
    if (!emit)
        return;
    uint pos = base + rank;
    if (pos >= cap)
    {
        uint d;
        InterlockedAdd(g_Stats[g_StatsBase + 7u], 1u, d);
        return;
    }
    if (stream == 2u)
        g_PairsAT[pos] = pair;
    else if (stream == 1u)
        g_PairsTerrain[pos] = pair;
    else
        g_PairsOpaque[pos] = pair;
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    if (g_StaticOverflow != 0u && g_Stats[2] == 0u)
        return;
    uint idx = dtID.x;
    bool valid = idx < g_EntryCount;

    uint entryIdx = g_EntryBase + (valid ? idx : 0u);
    ClusterEntry e = (ClusterEntry)0;
    if (valid)
        e = g_Entries[entryIdx];
    if ((e.flags & CLUSTER_ENTRY_FLAG_HUD) != 0u)
        valid = false;
    bool at = (e.flags & CLUSTER_ENTRY_FLAG_AT) != 0u;
    if (at && g_IncludeAT == 0u)
        valid = false;
    uint stream = at ? 2u : (((e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u) ? 1u : 0u);

    uint n = g_Stats[g_StaticOverflow != 0u ? 0u : 13u];
    for (uint k = 0u; k < n; ++k)
    {
        uint slot = g_Refresh[k];
        LocalShadowView v = g_Tiles[slot];
        bool hit = valid && v.zparams.w > 0.5;
        if (g_StaticOverflow != 0u && v.shape.x < 0.5)
            hit = false;
        if (hit)
        {
            LocalViewQuery q = localQueryFromView(v, slot, g_IncludeAT, 1.0);
            bool skinned = (e.flags & CLUSTER_ENTRY_FLAG_SKINNED) != 0u;
            if (skinned ? localBoxOutside(e.sphere.xyz, e.extent, q) : !localEntryTouchesView(e, q))
                hit = false;
        }
        uint2 pair = uint2(entryIdx, slot);
        emitStream(hit && stream == 0u, g_StatsBase + 4u, g_CapOpaque, pair, 0u);
        emitStream(hit && stream == 1u, g_StatsBase + 5u, g_CapTerrain, pair, 1u);
        emitStream(hit && stream == 2u, g_StatsBase + 6u, g_CapAT, pair, 2u);
    }
}
