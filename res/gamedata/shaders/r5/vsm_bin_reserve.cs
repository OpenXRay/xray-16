#define SM_5_0
#include "common.h"
#include "vsm_common.h"

#define RESERVE_THREADS 256

cbuffer VsmReserveParams : register(b5)
{
    int4 g_PageBase[3];
    uint g_Frame;
    uint g_MaxPages;
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_ReservePad0;
    uint g_ReservePad1;
    uint g_ReservePad2;
    uint4 g_Interval[2];
    float4 g_Pivot;
    float4 g_Sun;
    float4 g_LevelOrigin[VSM_LEVELS];
};

StructuredBuffer<uint4> g_PageCount : register(t0);
StructuredBuffer<uint4> g_CandList : register(t1);
RWByteAddressBuffer g_Counters : register(u0);
RWStructuredBuffer<uint> g_DirtyList : register(u1);
RWStructuredBuffer<uint4> g_PairBase : register(u2);
RWStructuredBuffer<uint> g_PageTable : register(u3);
RWStructuredBuffer<uint2> g_PhysTile : register(u4);
RWStructuredBuffer<uint> g_SlotFrame : register(u5);
RWStructuredBuffer<float4> g_SlotPivot : register(u6);
RWStructuredBuffer<float4> g_SlotSun : register(u7);
RWStructuredBuffer<uint> g_SlotDirty : register(u8);
RWStructuredBuffer<uint> g_Stats : register(u9);
RWByteAddressBuffer g_EmitArgs : register(u10);

groupshared uint gs_scan[3][RESERVE_THREADS];
groupshared uint gs_carry[3];
groupshared uint gs_total[3];
groupshared uint gs_accepted;
groupshared uint gs_stop;

int2 pageBaseOf(int L)
{
    int4 v = g_PageBase[L >> 1];
    return ((L & 1) == 0) ? v.xy : v.zw;
}

uint intervalOf(int L)
{
    uint4 v = g_Interval[L >> 2];
    int c = L & 3;
    return (c == 0) ? v.x : ((c == 1) ? v.y : ((c == 2) ? v.z : v.w));
}

uint3 groupScanInclusive(uint t, uint3 v)
{
    gs_scan[0][t] = v.x;
    gs_scan[1][t] = v.y;
    gs_scan[2][t] = v.z;
    GroupMemoryBarrierWithGroupSync();
    for (uint off = 1u; off < uint(RESERVE_THREADS); off <<= 1u)
    {
        uint3 add = uint3(0u, 0u, 0u);
        if (t >= off)
            add = uint3(gs_scan[0][t - off], gs_scan[1][t - off], gs_scan[2][t - off]);
        GroupMemoryBarrierWithGroupSync();
        gs_scan[0][t] += add.x;
        gs_scan[1][t] += add.y;
        gs_scan[2][t] += add.z;
        GroupMemoryBarrierWithGroupSync();
    }
    return uint3(gs_scan[0][t], gs_scan[1][t], gs_scan[2][t]);
}

void publishPage(uint index, uint4 cand, uint3 base)
{
    uint slot = cand.x;
    uint vp = min(cand.y, uint(VSM_PAGE_COUNT - 1));
    uint kind = cand.z;

    g_DirtyList[index] = slot;
    g_PairBase[index] = uint4(base, 0u);

    int level = int(vp) / VSM_PAGES_PER_LVL;
    int within = int(vp) % VSM_PAGES_PER_LVL;
    int2 wpage = int2(within % VSM_PAGES_AXIS, within / VSM_PAGES_AXIS);
    int2 absPage = pageBaseOf(level) + wpage;

    g_PageTable[vp] = slot;
    g_PhysTile[slot] = uint2(uint(absPage.x), uint(absPage.y));

    uint phase = 0u;
    if (kind != 2u)
    {
        uint h = slot * 2654435761u;
        h ^= h >> 15;
        phase = h % max(intervalOf(level), 1u);
    }
    g_SlotFrame[slot] = g_Frame - phase;

    float2 org = g_LevelOrigin[level].xy + float2(wpage) * g_LevelOrigin[level].z;
    g_SlotPivot[slot] = float4(g_Pivot.xyz, org.x);
    g_SlotSun[slot] = float4(g_Sun.xyz, org.y);
    g_SlotDirty[slot] = kind;
}

[numthreads(RESERVE_THREADS, 1, 1)]
void main(uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    uint n = g_Counters.Load(8);

    if (t == 0u)
    {
        for (uint s = 0u; s < 3u; ++s)
        {
            gs_carry[s] = 0u;
            gs_total[s] = 0u;
        }
        gs_accepted = 0u;
        gs_stop = 0u;
    }
    GroupMemoryBarrierWithGroupSync();

    for (uint base = 0u; base < n; base += uint(RESERVE_THREADS))
    {
        if (gs_stop != 0u)
            break;

        uint g = base + t;
        bool have = g < n;
        uint3 cnt = have ? g_PageCount[g].xyz : uint3(0u, 0u, 0u);
        uint3 incl = groupScanInclusive(t, cnt);
        uint3 pre = uint3(gs_carry[0], gs_carry[1], gs_carry[2]) + incl;

        bool fits = have && g < g_MaxPages
            && pre.x <= g_CapOpaque && pre.y <= g_CapTerrain && pre.z <= g_CapAT;
        if (have && !fits)
        {
            uint d;
            InterlockedMax(gs_stop, 1u, d);
        }
        if (fits)
        {
            uint4 cand = g_CandList[g];
            publishPage(g, cand, pre - cnt);
            uint d;
            InterlockedAdd(gs_accepted, 1u, d);
            uint m;
            InterlockedMax(gs_total[0], pre.x, m);
            InterlockedMax(gs_total[1], pre.y, m);
            InterlockedMax(gs_total[2], pre.z, m);
        }
        GroupMemoryBarrierWithGroupSync();
        if (t == uint(RESERVE_THREADS - 1))
        {
            gs_carry[0] += incl.x;
            gs_carry[1] += incl.y;
            gs_carry[2] += incl.z;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    GroupMemoryBarrierWithGroupSync();
    if (t == 0u)
    {
        uint k = gs_accepted;
        g_Counters.Store4(0, uint4(6u, k, 0u, 0u));
        g_Counters.Store(28, n - k);
        g_Stats[4] = gs_total[0];
        g_Stats[5] = gs_total[1];
        g_Stats[6] = gs_total[2];
        g_EmitArgs.Store3(0, uint3(k, 1u, 1u));
    }
}
