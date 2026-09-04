#define SM_5_0
#include "common.h"
#include "vsm_common.h"

cbuffer VsmResidParams : register(b5)
{
    int4 g_PageBase[3];
    uint g_Frame;
    uint g_RefreshBudget;
    uint g_WrongBudget;
    uint g_ForceDirty;
    uint4 g_Interval[2];
    float4 g_Pivot;
    float4 g_Sun;
    float4 g_LevelOrigin[VSM_LEVELS];
};

StructuredBuffer<uint> g_Needed : register(t0);
RWStructuredBuffer<uint> g_PageTable : register(u0);
RWStructuredBuffer<uint4> g_PageList : register(u1);
RWStructuredBuffer<uint2> g_PhysTile : register(u2);
RWStructuredBuffer<uint> g_SlotDirty : register(u3);
RWStructuredBuffer<uint> g_DirtyList : register(u4);
RWByteAddressBuffer g_Counters : register(u5);
RWStructuredBuffer<uint> g_SlotFrame : register(u6);
RWStructuredBuffer<float4> g_SlotPivot : register(u7);
RWStructuredBuffer<float4> g_SlotSun : register(u8);

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
    if (vp == 0u)
        g_Counters.Store(0, 6u);
    if (!inRange)
        return;

    int within = int(vp) % VSM_PAGES_PER_LVL;
    int2 wpage = int2(within % VSM_PAGES_AXIS, within / VSM_PAGES_AXIS);
    int2 absPage = pageBaseOf(level) + wpage;
    int slot = vsmToroidalSlot(level, absPage);
    if (!needed)
    {
        g_PageTable[vp] = VSM_UNMAPPED;
        g_SlotDirty[slot] = 0u;
        return;
    }
    g_PageTable[vp] = uint(slot);
    g_PageList[slot] = uint4(uint(level), uint(wpage.x), uint(wpage.y), 0u);

    uint2 tile = uint2(uint(absPage.x), uint(absPage.y));
    bool force = g_ForceDirty != 0u;
    bool wrong = any(g_PhysTile[slot] != tile);
    bool refresh = false;

    if (wrong && !force)
    {
        uint n;
        g_Counters.InterlockedAdd(16u, 1u, n);
        if (g_WrongBudget != 0u && n >= g_WrongBudget)
        {
            g_PageTable[vp] = VSM_UNMAPPED;
            g_SlotDirty[slot] = 0u;
            return;
        }
    }
    else if (!force)
    {
        uint age = g_Frame - g_SlotFrame[slot];
        if (age >= intervalOf(level))
        {
            uint n;
            g_Counters.InterlockedAdd(20u, 1u, n);
            if (n < g_RefreshBudget)
            {
                refresh = true;
            }
            else
            {
                uint m;
                g_Counters.InterlockedAdd(24u, 1u, m);
            }
        }
    }

    if (wrong || refresh || force)
    {
        g_PhysTile[slot] = tile;
        uint phase = 0u;
        if (!refresh)
        {
            uint h = uint(slot) * 2654435761u;
            h ^= h >> 15;
            phase = h % max(intervalOf(level), 1u);
        }
        g_SlotFrame[slot] = g_Frame - phase;
        float2 org = g_LevelOrigin[level].xy + float2(wpage) * g_LevelOrigin[level].z;
        g_SlotPivot[slot] = float4(g_Pivot.xyz, org.x);
        g_SlotSun[slot] = float4(g_Sun.xyz, org.y);
        g_SlotDirty[slot] = wrong ? 1u : (refresh ? 2u : 4u);
        uint d;
        g_Counters.InterlockedAdd(4u, 1u, d);
        if (d < uint(VSM_MAX_PHYS_S))
            g_DirtyList[d] = uint(slot);
    }
    else
    {
        g_SlotDirty[slot] = 0u;
    }
}
