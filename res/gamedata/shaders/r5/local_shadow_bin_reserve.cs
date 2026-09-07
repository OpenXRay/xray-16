#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"


cbuffer LocalShadowBinParams : register(b5)
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
    uint g_BinPad0;
    uint g_BinPad1;
    uint g_BinPad2;
};

StructuredBuffer<LocalShadowView> g_Request : register(t15);
StructuredBuffer<uint4> g_CandList : register(t16);
StructuredBuffer<uint4> g_TileCount : register(t22);

RWStructuredBuffer<LocalShadowView> g_TileState : register(u0);
RWStructuredBuffer<uint4> g_Schedule : register(u1);
RWStructuredBuffer<uint> g_DirtyList : register(u2);
RWStructuredBuffer<uint> g_RefreshDyn : register(u3);
RWStructuredBuffer<uint4> g_PairBase : register(u4);
RWByteAddressBuffer g_EmitArgs : register(u5);
RWByteAddressBuffer g_ClearArgs : register(u6);
RWStructuredBuffer<uint> g_Stats : register(u7);

// Every requested view is published only for a same-frame complete draw. Views
// that exceed the compact pair buffers use bounded caster batches in Static.
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
        bool current = old.zparams.w > 0.5 && old.meta.x == cand.y && old.meta.y == cand.z;
        req.zparams.w = 1.0;
        req.shape.x = 0.0;
        if (!current)
        {
            uint3 count = g_TileCount[i].xyz;
            bool fallback = any(count > uint3(g_CapOpaque, g_CapTerrain, g_CapAT) - sum)
                || (g_NodeCount == 0u && g_BinPad0 != 0u);
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
