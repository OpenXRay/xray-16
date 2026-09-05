#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_resid_params.h"

#define PREP_THREADS 256

RWByteAddressBuffer g_Counters : register(u0);
RWByteAddressBuffer g_BinArgs : register(u1);
RWStructuredBuffer<uint4> g_CandList : register(u2);

groupshared uint gs_scan[PREP_THREADS];
groupshared uint gs_coarseRefresh;

uint scanInclusive(uint t, uint value)
{
    gs_scan[t] = value;
    GroupMemoryBarrierWithGroupSync();
    for (uint off = 1u; off < uint(PREP_THREADS); off <<= 1u)
    {
        uint add = (t >= off) ? gs_scan[t - off] : 0u;
        GroupMemoryBarrierWithGroupSync();
        gs_scan[t] += add;
        GroupMemoryBarrierWithGroupSync();
    }
    return gs_scan[t];
}

[numthreads(PREP_THREADS, 1, 1)]
void main(uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    if (t == 0u)
        gs_coarseRefresh = 0u;
    GroupMemoryBarrierWithGroupSync();

    uint outputBase = 0u;
    uint fineRefresh = 0u;
    for (uint phase = 0u; phase < 3u; ++phase)
    {
        uint pageCount = (phase == 0u) ? uint(VSM_PAGES_PER_LVL) : uint(VSM_PAGE_COUNT - VSM_PAGES_PER_LVL);
        uint limit = pageCount;
        if (g_ForceDirty == 0u)
        {
            if (phase == 1u && g_WrongBudget != 0u)
                limit = min(limit, g_WrongBudget);
            if (phase == 2u)
                limit = min(limit, g_RefreshBudget);
        }
        uint selected = 0u;
        for (uint base = 0u; base < pageCount && selected < limit; base += uint(PREP_THREADS))
        {
            uint order = base + t;
            uint vp;
            if (phase == 0u)
                vp = uint(VSM_PAGE_COUNT - VSM_PAGES_PER_LVL) + order;
            else if (phase == 1u)
            {
                uint level = uint(VSM_LEVELS - 2) - order / uint(VSM_PAGES_PER_LVL);
                uint within = (order + g_Frame * 257u) % uint(VSM_PAGES_PER_LVL);
                vp = level * uint(VSM_PAGES_PER_LVL) + within;
            }
            else
                vp = (order + g_Frame * 257u) % pageCount;

            uint4 cand = g_CandList[uint(VSM_MAX_PHYS_S) + vp];
            bool wanted = (phase == 0u) ? cand.z != 0u : ((phase == 1u) ? cand.z == 1u : cand.z >= 2u);
            uint incl = scanInclusive(t, wanted ? 1u : 0u);
            uint rank = selected + incl;
            if (wanted && rank <= limit)
            {
                g_CandList[outputBase + rank - 1u] = cand;
                if (phase == 0u && cand.z == 2u)
                {
                    uint d;
                    InterlockedAdd(gs_coarseRefresh, 1u, d);
                }
            }
            uint total = gs_scan[PREP_THREADS - 1];
            GroupMemoryBarrierWithGroupSync();
            selected += total;
        }
        uint accepted = min(selected, limit);
        outputBase += accepted;
        if (phase == 2u)
            fineRefresh = accepted;
    }
    GroupMemoryBarrierWithGroupSync();
    if (t == 0u)
    {
        uint due = g_Counters.Load(20);
        uint refreshed = gs_coarseRefresh + fineRefresh;
        g_Counters.Store(8, outputBase);
        g_Counters.Store(12, 0u);
        g_Counters.Store(24, (g_ForceDirty == 0u) ? due - min(due, refreshed) : 0u);
        g_BinArgs.Store3(0, uint3(outputBase, 1u, 1u));
    }
}
