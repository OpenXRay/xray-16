#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#define CLUSTER_GEO_T_REFS t2
#define CLUSTER_GEO_T_META t3
#define CLUSTER_GEO_T_INSTANCES t4
#define CLUSTER_GEO_RESIDENCY
#define CLUSTER_GEO_T_GROUPS t26
#define CLUSTER_GEO_T_GROUP_STATE t27
#include "cluster_geo_bindings.h"

cbuffer VsmDynBinParams : register(b5)
{
    uint g_EntryBase;
    uint g_EntryCount;
    uint g_IncludeAT;
    uint g_CapOpaque;
    uint g_CapAT;
    uint g_StatsBase;
    uint g_UseSkinned;
    uint g_DynBinPad;
};

StructuredBuffer<ClusterEntry> g_SkinnedEntries : register(t0);
RWStructuredBuffer<uint> g_DynPageTable : register(u0);

ClusterEntry LoadSourceEntry(uint idx)
{
    if (g_UseSkinned != 0u)
        return g_SkinnedEntries[idx];
    return LoadClusterEntry(idx);
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_EntryCount)
        return;
    ClusterEntry e = LoadSourceEntry(g_EntryBase + idx);
    if ((e.flags & (CLUSTER_ENTRY_FLAG_HUD | CLUSTER_ENTRY_FLAG_NO_SHADOW)) != 0u)
        return;
    bool at = (e.flags & 1u) != 0u;
    if (at && g_IncludeAT == 0u)
        return;
    float2 lp = mul(vsm_view, float4(e.sphere.xyz, 1.0)).xy;
    float R = e.sphere.w;
    for (int L = 0; L < VSM_LEVELS; ++L)
    {
        VSM_PAGE_RANGE(L, lp, R, lo, hi, p0, p1);
        for (int py = p0.y; py <= p1.y; ++py)
        for (int px = p0.x; px <= p1.x; ++px)
            g_DynPageTable[vsmPageIndex(L, int2(px, py))] = VSM_TOUCHED;
    }
}
