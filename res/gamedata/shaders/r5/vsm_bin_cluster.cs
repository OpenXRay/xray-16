#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#include "visbuffer_common.h"
#include "vsm_bvh_types.h"

cbuffer VsmBinParams : register(b5)
{
    uint g_IncludeAT;
    uint g_PairCapOpaque;
    uint g_PairCapTerrain;
    uint g_PairCapAT;
    uint g_NodeCount;
    float g_ErrK;
    uint2 g_BinPad;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t0);
StructuredBuffer<uint> g_DirtyList : register(t1);
StructuredBuffer<uint4> g_PageList : register(t2);
RWStructuredBuffer<uint> g_Stats : register(u0);
RWStructuredBuffer<uint2> g_PairsOpaque : register(u1);
RWStructuredBuffer<uint2> g_PairsTerrain : register(u2);
RWStructuredBuffer<uint2> g_PairsAT : register(u3);

groupshared uint gs_pairs;
groupshared uint gs_visited;

void emitStream(bool emit, uint cursor, uint cap, uint2 pair, uint stream)
{
    uint cnt = WaveActiveCountBits(emit);
    if (cnt == 0u)
        return;
    uint rank = WavePrefixCountBits(emit);
    uint base = 0u;
    if (WaveIsFirstLane())
        InterlockedAdd(g_Stats[cursor], cnt, base);
    base = WaveReadLaneFirst(base);
    if (!emit)
        return;
    uint pos = base + rank;
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

void VsmVisit(bool active, uint entryIdx, VsmPageQuery q)
{
    bool hit = false;
    uint stream = 0u;
    if (active)
    {
        uint d;
        InterlockedAdd(gs_visited, 1u, d);
        ClusterEntry e = g_Entries[entryIdx];
        bool at = (e.flags & 1u) != 0u;
        bool terrain = (e.flags & 4u) != 0u;
        if (!(at && q.includeAT == 0u))
        {
            hit = vsmEntryTouchesPage(e, q);
            stream = at ? 2u : (terrain ? 1u : 0u);
        }
    }
    uint2 pair = uint2(entryIdx, q.slot);
    emitStream(hit && stream == 0u, 4u, g_PairCapOpaque, pair, 0u);
    emitStream(hit && stream == 1u, 5u, g_PairCapTerrain, pair, 1u);
    emitStream(hit && stream == 2u, 6u, g_PairCapAT, pair, 2u);
    uint hits = WaveActiveCountBits(hit);
    if (WaveIsFirstLane() && hits > 0u)
    {
        uint d;
        InterlockedAdd(gs_pairs, hits, d);
    }
}

#include "vsm_bvh.h"

[numthreads(VSM_BVH_GROUP, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    if (t == 0u)
    {
        gs_pairs = 0u;
        gs_visited = 0u;
    }
    GroupMemoryBarrierWithGroupSync();

    uint slot = g_DirtyList[gID.x];
    uint4 pg = g_PageList[slot];
    int L = int(min(pg.x, uint(VSM_LEVELS - 1)));
    int2 wpage = int2(pg.yz);
    float2 origin = vsm_level[L].xy;
    float pw = vsm_level[L].z / float(VSM_PAGES_AXIS);

    VsmPageQuery q;
    q.pmin = origin + float2(wpage) * pw;
    q.pmax = q.pmin + float2(pw, pw);
    q.errB = vsm_level[L].z / float(VSM_VIRTUAL_RES) * g_ErrK;
    q.slot = slot;
    q.includeAT = g_IncludeAT;
    q.pad = 0u;

    vsmBvhTraverse(t, g_NodeCount, q);

    GroupMemoryBarrierWithGroupSync();
    if (t == 0u)
    {
        uint m;
        InterlockedMax(g_Stats[2], gs_visited, m);
        if (gs_pairs > 0u)
        {
            uint d0;
            InterlockedAdd(g_Stats[0], 1u, d0);
            uint d1;
            InterlockedAdd(g_Stats[1], gs_pairs, d1);
        }
    }
}
