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
};

cbuffer VsmBinParams : register(b5)
{
    uint g_EntryCount;
    uint g_IncludeAT;
    uint g_PairCapOpaque;
    uint g_PairCapTerrain;
    uint g_PairCapAT;
    float g_ErrK;
    uint2 g_BinPad;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t0);
StructuredBuffer<uint> g_PageTable : register(t1);
StructuredBuffer<uint> g_SlotDirty : register(t2);
RWStructuredBuffer<uint> g_Stats : register(u0);
RWStructuredBuffer<uint2> g_PairsOpaque : register(u1);
RWStructuredBuffer<uint2> g_PairsTerrain : register(u2);
RWStructuredBuffer<uint2> g_PairsAT : register(u3);

bool LodPass(ClusterEntry e, int L)
{
    if ((e.flags & 2u) != 0u)
        return true;
    float errB = vsm_level[L].z / float(VSM_VIRTUAL_RES) * g_ErrK;
    return !(e.selfError > errB || e.parentError <= errB);
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_EntryCount)
        return;

    ClusterEntry e = g_Entries[idx];
    bool at = (e.flags & 1u) != 0u;
    if (at && g_IncludeAT == 0u)
        return;
    bool terrain = (e.flags & 4u) != 0u;

    float2 lp = mul(vsm_view, float4(e.sphere.xyz, 1.0)).xy;
    float R = e.sphere.w;

    uint cnt = 0u;
    bool lodDropped = false;
    for (int Lc = 0; Lc < VSM_LEVELS; ++Lc)
    {
        if (!LodPass(e, Lc))
        {
            lodDropped = true;
            continue;
        }
        VSM_PAGE_RANGE(Lc, lp, R, loC, hiC, p0C, p1C);
        for (int py = p0C.y; py <= p1C.y; ++py)
        for (int px = p0C.x; px <= p1C.x; ++px)
        {
            uint slot = g_PageTable[vsmPageIndex(Lc, int2(px, py))];
            if (slot == VSM_UNMAPPED)
                continue;
            if (g_SlotDirty[slot] == 0u)
                continue;
            ++cnt;
        }
    }

    uint prevMax;
    InterlockedMax(g_Stats[2], cnt, prevMax);
    if (cnt == 0u)
    {
        if (lodDropped)
        {
            uint d;
            InterlockedAdd(g_Stats[3], 1u, d);
        }
        return;
    }

    uint cursor = at ? 6u : (terrain ? 5u : 4u);
    uint cap = at ? g_PairCapAT : (terrain ? g_PairCapTerrain : g_PairCapOpaque);
    uint base;
    InterlockedAdd(g_Stats[cursor], cnt, base);
    if (base >= cap)
    {
        uint d;
        InterlockedAdd(g_Stats[7], 1u, d);
        return;
    }
    uint inst = min(cnt, cap - base);

    uint w = 0u;
    for (int Lw = 0; Lw < VSM_LEVELS; ++Lw)
    {
        if (!LodPass(e, Lw))
            continue;
        VSM_PAGE_RANGE(Lw, lp, R, loW, hiW, p0W, p1W);
        for (int py = p0W.y; py <= p1W.y; ++py)
        for (int px = p0W.x; px <= p1W.x; ++px)
        {
            uint slot = g_PageTable[vsmPageIndex(Lw, int2(px, py))];
            if (slot == VSM_UNMAPPED)
                continue;
            if (g_SlotDirty[slot] == 0u)
                continue;
            if (w < inst)
            {
                uint2 pair = uint2(idx, slot);
                if (at)
                    g_PairsAT[base + w] = pair;
                else if (terrain)
                    g_PairsTerrain[base + w] = pair;
                else
                    g_PairsOpaque[base + w] = pair;
            }
            ++w;
        }
    }

    uint d1;
    InterlockedAdd(g_Stats[1], inst, d1);
    uint d0;
    InterlockedAdd(g_Stats[0], 1u, d0);
}
