#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"


cbuffer LocalShadowBinParams
{
    uint g_CandCount;
    uint g_NodeCount;
    uint g_IncludeAT;
    float g_ErrK;
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_Budget;
    uint g_Frame;
    uint g_BinRefCount;
    uint g_ResidencyStreaming;
    uint g_BinPad2;
};

StructuredBuffer<LocalShadowView> g_Request;
StructuredBuffer<uint4> g_CandList;
StructuredBuffer<uint4> g_TileCount;

RWStructuredBuffer<LocalShadowView> g_TileState;
RWStructuredBuffer<uint4> g_Schedule;
RWStructuredBuffer<uint> g_DirtyList;
RWStructuredBuffer<uint> g_RefreshDyn;
RWStructuredBuffer<uint4> g_PairBase;
RWByteAddressBuffer g_EmitArgs;
RWByteAddressBuffer g_ClearArgs;
RWStructuredBuffer<uint> g_Stats;
RWStructuredBuffer<uint2> g_GeometryDirty;

[numthreads(1, 1, 1)]
void main()
{
    uint3 sum = 0u;
    uint dirty = 0u, dynamicCount = 0u, cached = 0u, overflow = 0u;
    for (uint i = 0u; i < g_CandCount; ++i)
    {
        uint4 cand = g_CandList[i];
        uint slot = cand.x;
        LocalShadowView req = g_Request[slot];
        LocalShadowView old = g_TileState[slot];
        bool current = old.zparams.w == 1.0 && old.meta.x == cand.y && old.meta.y == cand.z
            && g_GeometryDirty[slot].y == 0u;
        req.zparams.w = current ? 1.0 : 2.0;
        req.shape.x = 0.0;
        if (!current)
        {
            uint2 geometry = g_GeometryDirty[slot];
            g_GeometryDirty[slot] = uint2(geometry.x, 1u);
            uint3 count = g_TileCount[i].xyz;
            bool fallback = any(count > uint3(g_CapOpaque, g_CapTerrain, g_CapAT) - sum)
                || (g_NodeCount == 0u && g_BinRefCount != 0u);
            // The work budget limits compact scratch usage; overflow still draws.
            fallback = fallback || (dirty > 0u && dot(float3(sum + count), 1.0) > float(g_Budget));
            req.shape.x = fallback ? 1.0 : 0.0;
            g_DirtyList[dirty] = slot;
            g_PairBase[dirty] = uint4(sum, fallback ? 1u : 0u);
            ++dirty;
            if (fallback)
                ++overflow;
            else
                sum += count;
        }
        else
            ++cached;
        g_TileState[slot] = req;
        g_Schedule[slot] = uint4(cand.z, 0u, 0u, cand.y);
        g_RefreshDyn[dynamicCount++] = slot;
    }
    g_Stats[0] = dirty;
    g_Stats[1] = 0u;
    g_Stats[2] = overflow;
    g_Stats[3] = cached;
    g_Stats[4] = sum.x;
    g_Stats[5] = sum.y;
    g_Stats[6] = sum.z;
    g_Stats[13] = dynamicCount;
    g_EmitArgs.Store4(0, uint4(dirty, 1u, 1u, 0u));
    g_ClearArgs.Store4(0, uint4(6u, dirty, 0u, 0u));
    g_ClearArgs.Store4(16, uint4(6u, dynamicCount, 0u, 0u));
}
