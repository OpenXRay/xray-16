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
StructuredBuffer<uint4> g_CandList : register(t1);
RWStructuredBuffer<uint4> g_PageCount : register(u0);

groupshared uint gs_count[3];

void VsmVisit(bool active, uint entryIdx, VsmPageQuery q)
{
    if (!active)
        return;
    ClusterEntry e = g_Entries[entryIdx];
    bool at = (e.flags & 1u) != 0u;
    bool terrain = (e.flags & 4u) != 0u;
    if (at && q.includeAT == 0u)
        return;
    if (!vsmEntryTouchesPage(e, q))
        return;
    uint stream = at ? 2u : (terrain ? 1u : 0u);
    uint d;
    InterlockedAdd(gs_count[stream], 1u, d);
}

#include "vsm_bvh.h"

[numthreads(VSM_BVH_GROUP, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    if (t < 3u)
        gs_count[t] = 0u;
    GroupMemoryBarrierWithGroupSync();

    uint g = gID.x;
    uint4 cand = g_CandList[g];

    uint vp = min(cand.y, uint(VSM_PAGE_COUNT - 1));
    int L = int(vp) / VSM_PAGES_PER_LVL;
    int within = int(vp) % VSM_PAGES_PER_LVL;
    int2 wpage = int2(within % VSM_PAGES_AXIS, within / VSM_PAGES_AXIS);
    float pw = vsm_level[L].z / float(VSM_PAGES_AXIS);

    VsmPageQuery q;
    q.pmin = vsm_level[L].xy + float2(wpage) * pw;
    q.pmax = q.pmin + float2(pw, pw);
    q.errB = vsm_level[L].z / float(VSM_VIRTUAL_RES) * g_ErrK;
    q.slot = cand.x;
    q.includeAT = g_IncludeAT;
    q.pad = 0u;

    vsmBvhTraverse(t, g_NodeCount, q);

    GroupMemoryBarrierWithGroupSync();
    if (t == 0u)
        g_PageCount[g] = uint4(gs_count[0], gs_count[1], gs_count[2], 0u);
}
