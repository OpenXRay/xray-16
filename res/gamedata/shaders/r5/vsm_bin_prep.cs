#define SM_5_0
#include "common.h"
#include "vsm_common.h"

cbuffer VsmBinPrepParams : register(b5)
{
    uint g_EntryCount;
    uint3 g_PrepPad;
};

ByteAddressBuffer g_DrawClear : register(t0);
StructuredBuffer<uint> g_DirtyList : register(t1);
StructuredBuffer<uint4> g_PageList : register(t2);
RWStructuredBuffer<int4> g_DirtyRects : register(u0);
RWByteAddressBuffer g_BinArgs : register(u1);

groupshared int gs_min[VSM_LEVELS * 2];
groupshared int gs_max[VSM_LEVELS * 2];

[numthreads(256, 1, 1)]
void main(uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    if (t < uint(VSM_LEVELS * 2))
    {
        gs_min[t] = VSM_PAGES_AXIS;
        gs_max[t] = -1;
    }
    GroupMemoryBarrierWithGroupSync();

    uint dirty = min(g_DrawClear.Load(4), uint(VSM_MAX_PHYS_S));
    for (uint i = t; i < dirty; i += 256u)
    {
        uint slot = g_DirtyList[i];
        uint4 page = g_PageList[slot];
        uint L = min(page.x, uint(VSM_LEVELS - 1));
        InterlockedMin(gs_min[L * 2u], int(page.y));
        InterlockedMin(gs_min[L * 2u + 1u], int(page.z));
        InterlockedMax(gs_max[L * 2u], int(page.y));
        InterlockedMax(gs_max[L * 2u + 1u], int(page.z));
    }
    GroupMemoryBarrierWithGroupSync();

    if (t < uint(VSM_LEVELS))
        g_DirtyRects[t] = int4(gs_min[t * 2u], gs_min[t * 2u + 1u], gs_max[t * 2u], gs_max[t * 2u + 1u]);
    if (t == 0u)
        g_BinArgs.Store3(0, uint3(dirty > 0u ? (g_EntryCount + 63u) / 64u : 0u, 1u, 1u));
}
