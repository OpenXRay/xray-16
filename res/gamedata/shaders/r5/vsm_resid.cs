#define SM_5_0
#include "common.h"
#include "vsm_common.h"

#include "vsm_resid_params.h"

StructuredBuffer<uint> g_Needed : register(t0);
RWStructuredBuffer<uint> g_PageTable : register(u0);
RWStructuredBuffer<uint4> g_PageList : register(u1);
RWStructuredBuffer<uint2> g_PhysTile : register(u2);
RWStructuredBuffer<uint> g_SlotDirty : register(u3);
RWStructuredBuffer<uint4> g_CandList : register(u4);
RWByteAddressBuffer g_Counters : register(u5);
RWStructuredBuffer<uint> g_SlotFrame : register(u6);

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

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint vp = dtID.x;
    bool inRange = vp < uint(VSM_PAGE_COUNT);
    bool needed = inRange && g_Needed[vp] != 0u;
    int level = int(min(vp, uint(VSM_PAGE_COUNT - 1))) / VSM_PAGES_PER_LVL;
    uint nWave = WaveActiveCountBits(needed);
    if (WaveIsFirstLane() && nWave != 0u)
    {
        uint d;
        g_Counters.InterlockedAdd(32u + 4u * uint(level), nWave, d);
    }
    if (!inRange)
        return;

    g_CandList[uint(VSM_MAX_PHYS_S) + vp] = uint4(0u, 0u, 0u, 0u);
    int within = int(vp) % VSM_PAGES_PER_LVL;
    int2 wpage = int2(within % VSM_PAGES_AXIS, within / VSM_PAGES_AXIS);
    int2 absPage = pageBaseOf(level) + wpage;
    int slot = vsmToroidalSlot(level, absPage);
    g_SlotDirty[slot] = 0u;
    if (!needed)
    {
        g_PageTable[vp] = VSM_UNMAPPED;
        return;
    }
    g_PageList[slot] = uint4(uint(level), uint(wpage.x), uint(wpage.y), 0u);

    uint2 tile = uint2(uint(absPage.x), uint(absPage.y));
    bool force = g_ForceDirty != 0u;
    bool wrong = any(g_PhysTile[slot] != tile);
    uint kind = 0u;

    if (wrong)
    {
        uint n;
        g_Counters.InterlockedAdd(16u, 1u, n);
        kind = 1u;
    }
    else if (force)
    {
        kind = 4u;
    }
    else
    {
        uint age = g_Frame - g_SlotFrame[slot];
        if (age >= intervalOf(level))
        {
            uint n;
            g_Counters.InterlockedAdd(20u, 1u, n);
            kind = 2u;
        }
    }

    g_PageTable[vp] = (kind == 1u) ? VSM_UNMAPPED : uint(slot);
    if (kind == 0u)
        return;

    g_CandList[uint(VSM_MAX_PHYS_S) + vp] = uint4(uint(slot), vp, kind, 0u);
}
