#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#include "visbuffer_common.h"

cbuffer VsmDynBinParams : register(b5)
{
    uint g_EntryBase;
    uint g_EntryCount;
    uint g_IncludeAT;
    uint g_CapOpaque;
    uint g_CapAT;
    uint g_StatsBase;
    uint2 g_DynBinPad;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t0);
RWStructuredBuffer<uint> g_DynPageTable : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_EntryCount)
        return;
    ClusterEntry e = g_Entries[g_EntryBase + idx];
    if ((e.flags & 32u) != 0u)
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
