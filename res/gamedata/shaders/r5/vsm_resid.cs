#define SM_5_0
#include "common.h"
#include "vsm_common.h"

cbuffer VsmResidParams : register(b5)
{
    int4 g_PageBase[3];
    uint g_Frame;
    uint g_RefreshN;
    uint g_SunMoving;
    uint g_ForceDirty;
    float4 g_Inval[4];
};

StructuredBuffer<uint> g_Needed : register(t0);
RWStructuredBuffer<uint> g_PageTable : register(u0);
RWStructuredBuffer<uint4> g_PageList : register(u1);
RWStructuredBuffer<uint2> g_PhysTile : register(u2);
RWStructuredBuffer<uint> g_SlotDirty : register(u3);
RWStructuredBuffer<uint> g_DirtyList : register(u4);
RWByteAddressBuffer g_DrawClear : register(u5);

int2 pageBaseOf(int L)
{
    int4 v = g_PageBase[L >> 1];
    return ((L & 1) == 0) ? v.xy : v.zw;
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint vp = dtID.x;
    if (vp >= uint(VSM_PAGE_COUNT))
        return;
    if (g_Needed[vp] == 0u)
    {
        g_PageTable[vp] = VSM_UNMAPPED;
        return;
    }

    int level = int(vp) / VSM_PAGES_PER_LVL;
    int within = int(vp) % VSM_PAGES_PER_LVL;
    int2 wpage = int2(within % VSM_PAGES_AXIS, within / VSM_PAGES_AXIS);
    int2 absPage = pageBaseOf(level) + wpage;
    int slot = vsmToroidalSlot(level, absPage);
    g_PageTable[vp] = uint(slot);
    g_PageList[slot] = uint4(uint(level), uint(wpage.x), uint(wpage.y), 0u);

    uint2 tile = uint2(uint(absPage.x), uint(absPage.y));
    bool wrong = any(g_PhysTile[slot] != tile);
    bool refresh = (g_SunMoving != 0u) && (g_RefreshN != 0u) && ((uint(slot) % g_RefreshN) == (g_Frame % g_RefreshN));

    bool inval = false;
    {
        float pw = g_Inval[0].w * float(1 << level);
        if (pw > 0.0)
        {
            float2 pmin = float2(absPage) * pw;
            float2 pmax = pmin + float2(pw, pw);
            for (int i = 0; i < 4; ++i)
            {
                float r = g_Inval[i].z;
                if (r <= 0.0)
                    continue;
                float2 d = clamp(g_Inval[i].xy, pmin, pmax) - g_Inval[i].xy;
                if (dot(d, d) <= r * r)
                {
                    inval = true;
                    break;
                }
            }
        }
    }

    if (wrong && !inval && g_ForceDirty == 0u)
    {
        uint budget = uint(g_Inval[1].w + 0.5);
        uint n;
        InterlockedAdd(g_DirtyList[uint(VSM_MAX_PHYS_S)], 1u, n);
        if (budget != 0u && n >= budget)
        {
            g_PageTable[vp] = VSM_UNMAPPED;
            return;
        }
    }

    if (wrong || refresh || inval || g_ForceDirty != 0u)
    {
        g_PhysTile[slot] = tile;
        g_SlotDirty[slot] = wrong ? 1u : (refresh ? 2u : (inval ? 3u : 4u));
        uint d;
        g_DrawClear.InterlockedAdd(4, 1u, d);
        if (d < uint(VSM_MAX_PHYS_S))
            g_DirtyList[d] = uint(slot);
    }
}
