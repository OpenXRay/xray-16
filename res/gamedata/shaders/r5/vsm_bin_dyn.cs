#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"

struct ClusterEntry
{
    float4 sphere;
    float4 lodSelf;
    float4 lodParent;
    uint indexCount;
    uint ibFirst;
    uint firstVertex;
    uint batchIndex;
    uint materialID;
    uint flags;
    float selfError;
    float parentError;
    float3 extent;
    float extentPad;
};

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
StructuredBuffer<uint> g_DynPageTable : register(t1);
RWStructuredBuffer<uint> g_Stats : register(u0);
RWStructuredBuffer<uint2> g_PairsOpaque : register(u1);
RWStructuredBuffer<uint2> g_PairsAT : register(u2);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_EntryCount)
        return;

    uint entryIdx = g_EntryBase + idx;
    ClusterEntry e = g_Entries[entryIdx];
    if ((e.flags & 32u) != 0u)
        return;
    bool at = (e.flags & 1u) != 0u;
    if (at && g_IncludeAT == 0u)
        return;

    float2 lp = mul(vsm_view, float4(e.sphere.xyz, 1.0)).xy;
    float R = e.sphere.w;

    uint cnt = 0u;
    for (int Lc = 0; Lc < VSM_LEVELS; ++Lc)
    {
        VSM_PAGE_RANGE(Lc, lp, R, loC, hiC, p0C, p1C);
        for (int py = p0C.y; py <= p1C.y; ++py)
        for (int px = p0C.x; px <= p1C.x; ++px)
        {
            uint slot = g_DynPageTable[vsmPageIndex(Lc, int2(px, py))];
            if (slot == VSM_UNMAPPED)
                continue;
            ++cnt;
        }
    }

    uint prevMax;
    InterlockedMax(g_Stats[g_StatsBase + 2u], cnt, prevMax);
    if (cnt == 0u)
        return;

    uint cursor = g_StatsBase + (at ? 5u : 4u);
    uint cap = at ? g_CapAT : g_CapOpaque;
    uint base;
    InterlockedAdd(g_Stats[cursor], cnt, base);
    if (base >= cap)
    {
        uint d;
        InterlockedAdd(g_Stats[g_StatsBase + 3u], 1u, d);
        return;
    }
    uint inst = min(cnt, cap - base);

    uint w = 0u;
    for (int Lw = 0; Lw < VSM_LEVELS; ++Lw)
    {
        VSM_PAGE_RANGE(Lw, lp, R, loW, hiW, p0W, p1W);
        for (int py = p0W.y; py <= p1W.y; ++py)
        for (int px = p0W.x; px <= p1W.x; ++px)
        {
            uint slot = g_DynPageTable[vsmPageIndex(Lw, int2(px, py))];
            if (slot == VSM_UNMAPPED)
                continue;
            if (w < inst)
            {
                uint2 pair = uint2(entryIdx, slot);
                if (at)
                    g_PairsAT[base + w] = pair;
                else
                    g_PairsOpaque[base + w] = pair;
            }
            ++w;
        }
    }

    uint d1;
    InterlockedAdd(g_Stats[g_StatsBase + 1u], inst, d1);
    uint d0;
    InterlockedAdd(g_Stats[g_StatsBase + 0u], 1u, d0);
}
