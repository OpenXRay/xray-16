#define SM_5_0
#include "common.h"
#include "vsm_common.h"

StructuredBuffer<uint> g_Needed : register(t0);
RWStructuredBuffer<uint> g_DynPageTable : register(u0);
RWStructuredBuffer<uint4> g_DynPageList : register(u1);
RWStructuredBuffer<uint> g_DynAllocInfo : register(u2);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint t = dtID.x;
    if (t >= uint(VSM_PAGE_COUNT))
        return;
    if (g_DynPageTable[t] != VSM_TOUCHED)
        return;
    if (g_Needed[t] == 0u)
    {
        g_DynPageTable[t] = VSM_UNMAPPED;
        return;
    }

    uint slot;
    InterlockedAdd(g_DynAllocInfo[0], 1u, slot);
    if (slot >= uint(VSM_MAX_PHYS))
    {
        g_DynPageTable[t] = VSM_UNMAPPED;
        return;
    }

    int level = int(t) / VSM_PAGES_PER_LVL;
    int within = int(t) % VSM_PAGES_PER_LVL;
    uint2 page = uint2(uint(within % VSM_PAGES_AXIS), uint(within / VSM_PAGES_AXIS));

    g_DynPageTable[t] = slot;
    g_DynPageList[slot] = uint4(uint(level), page.x, page.y, 0u);
    uint d;
    InterlockedAdd(g_DynAllocInfo[1 + level], 1u, d);
}
