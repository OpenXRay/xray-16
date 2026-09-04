#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#include "visbuffer_common.h"

cbuffer VsmBinParams : register(b5)
{
    uint g_IncludeAT;
    uint g_PairCapOpaque;
    uint g_PairCapTerrain;
    uint g_PairCapAT;
    int4 g_WinToBucket[3];
    uint4 g_ItemBase[2];
};

StructuredBuffer<ClusterEntry> g_Entries : register(t0);
StructuredBuffer<uint> g_DirtyList : register(t1);
StructuredBuffer<uint4> g_PageList : register(t2);
StructuredBuffer<uint> g_BucketStart : register(t3);
StructuredBuffer<uint> g_BucketEnd : register(t4);
StructuredBuffer<uint> g_Items : register(t5);
RWStructuredBuffer<uint> g_Stats : register(u0);
RWStructuredBuffer<uint2> g_PairsOpaque : register(u1);
RWStructuredBuffer<uint2> g_PairsTerrain : register(u2);
RWStructuredBuffer<uint2> g_PairsAT : register(u3);

groupshared uint gs_pairs;

int2 winToBucket(int L)
{
    int4 v = g_WinToBucket[L >> 1];
    return ((L & 1) == 0) ? v.xy : v.zw;
}

uint itemBase(int L)
{
    uint4 v = g_ItemBase[L >> 2];
    int c = L & 3;
    return (c == 0) ? v.x : ((c == 1) ? v.y : ((c == 2) ? v.z : v.w));
}

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

[numthreads(256, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    if (t == 0u)
        gs_pairs = 0u;
    GroupMemoryBarrierWithGroupSync();

    uint slot = g_DirtyList[gID.x];
    uint4 pg = g_PageList[slot];
    int L = int(min(pg.x, uint(VSM_LEVELS - 1)));
    int2 wpage = int2(pg.yz);
    int2 bc = wpage + winToBucket(L);
    uint start = 0u;
    uint end = 0u;
    if (all(bc >= 0) && all(bc < VSM_BUCKET_AXIS))
    {
        uint b = uint(L) * uint(VSM_BUCKETS_PER_LVL) + uint(bc.y * VSM_BUCKET_AXIS + bc.x);
        start = g_BucketStart[b];
        end = g_BucketEnd[b];
    }
    uint base = itemBase(L);
    float2 origin = vsm_level[L].xy;
    float pw = vsm_level[L].z / float(VSM_PAGES_AXIS);
    float2 pmin = origin + float2(wpage) * pw;
    float2 pmax = pmin + float2(pw, pw);

    uint n = end - start;
    uint rounds = (n + 255u) / 256u;
    for (uint r = 0u; r < rounds; ++r)
    {
        uint i = r * 256u + t;
        bool hit = false;
        uint stream = 0u;
        uint entryIdx = 0u;
        if (i < n)
        {
            entryIdx = g_Items[base + start + i];
            ClusterEntry e = g_Entries[entryIdx];
            bool at = (e.flags & 1u) != 0u;
            bool terrain = (e.flags & 4u) != 0u;
            if (!(at && g_IncludeAT == 0u))
            {
                float2 lp = mul(vsm_view, float4(e.sphere.xyz, 1.0)).xy;
                float R = e.sphere.w;
                hit = (lp.x + R >= pmin.x) && (lp.x - R < pmax.x) && (lp.y + R >= pmin.y) && (lp.y - R < pmax.y);
                stream = at ? 2u : (terrain ? 1u : 0u);
            }
        }
        uint2 pair = uint2(entryIdx, slot);
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
    GroupMemoryBarrierWithGroupSync();
    if (t == 0u)
    {
        uint m;
        InterlockedMax(g_Stats[2], n, m);
        if (gs_pairs > 0u)
        {
            uint d0;
            InterlockedAdd(g_Stats[0], 1u, d0);
            uint d1;
            InterlockedAdd(g_Stats[1], gs_pairs, d1);
        }
    }
}
