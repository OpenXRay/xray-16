#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"
#include "visbuffer_common.h"

cbuffer LocalShadowBinParams : register(b5)
{
    uint g_EntryBase;
    uint g_EntryCount;
    uint g_RefreshCount;
    uint g_StatsBase;
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_IncludeAT;
    float g_ErrK;
    uint3 g_BinPad;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t14);
StructuredBuffer<LocalShadowTile> g_Tiles : register(t15);
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
        InterlockedAdd(g_Stats[cursor], cnt, base);
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
    uint idx = dtID.x;
    bool valid = idx < g_EntryCount;

    uint entryIdx = g_EntryBase + (valid ? idx : 0u);
    ClusterEntry e = g_Entries[entryIdx];
    if ((e.flags & CLUSTER_ENTRY_FLAG_HUD) != 0u)
        valid = false;
    bool at = (e.flags & CLUSTER_ENTRY_FLAG_AT) != 0u;
    if (at && g_IncludeAT == 0u)
        valid = false;
    bool plain = (e.flags & CLUSTER_ENTRY_FLAG_PLAIN) != 0u;
    uint stream = at ? 2u : (((e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u) ? 1u : 0u);

    float3 c = e.sphere.xyz;
    float R = e.sphere.w;

    for (uint k = 0u; k < g_RefreshCount; ++k)
    {
        uint slot = g_Refresh[k];
        LocalShadowTile t = g_Tiles[slot];
        bool hit = valid;
        if (hit && distance(c, t.lightPos.xyz) > t.lightPos.w + R)
            hit = false;
        if (hit)
        {
            for (uint p = 0u; p < 6u; ++p)
            {
                if (dot(t.planes[p].xyz, c) + t.planes[p].w > R)
                {
                    hit = false;
                    break;
                }
            }
        }
        if (hit && !plain)
        {
            float d = max(distance(c, t.lightPos.xyz) - R, t.zparams.x);
            float errB = d * t.zparams.z * g_ErrK;
            if (e.selfError > errB || e.parentError <= errB)
                hit = false;
        }
        uint2 pair = uint2(entryIdx, slot);
        emitStream(hit && stream == 0u, g_StatsBase + 4u, g_CapOpaque, pair, 0u);
        emitStream(hit && stream == 1u, g_StatsBase + 5u, g_CapTerrain, pair, 1u);
        emitStream(hit && stream == 2u, g_StatsBase + 6u, g_CapAT, pair, 2u);
    }
}
