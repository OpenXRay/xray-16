#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"

struct GPUObjectData
{
    float3 position;
    float radius;
    uint batchIndex;
    uint flags;
    float pad0;
    float pad1;
};

cbuffer VsmSkinBinParams : register(b5)
{
    uint g_BucketCount;
    uint g_Cap;
    uint g_MaxCasters;
    float g_NpcDist;
    float4 g_CamPos;
};

ByteAddressBuffer g_CompactCount : register(t0);
StructuredBuffer<uint> g_CompactIndices : register(t1);
StructuredBuffer<GPUObjectData> g_Objects : register(t2);
ByteAddressBuffer g_InputArgs : register(t14);
StructuredBuffer<uint> g_DynPageTable : register(t15);
RWStructuredBuffer<uint> g_CasterPages : register(u0);
RWByteAddressBuffer g_Args : register(u1);
RWStructuredBuffer<uint> g_Stats : register(u2);
RWStructuredBuffer<uint> g_DynUsed : register(u3);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint c = dtID.x;
    if (c >= g_MaxCasters)
        return;
    uint visible = min(g_CompactCount.Load(0), g_BucketCount);
    if (c >= visible)
        return;

    uint idx = g_CompactIndices[c];
    GPUObjectData o = g_Objects[idx];
    uint inBase = idx * 20u;
    uint indexCount = g_InputArgs.Load(inBase + 0u);
    uint firstIndex = g_InputArgs.Load(inBase + 8u);
    uint baseVertex = g_InputArgs.Load(inBase + 12u);

    float2 lp = mul(vsm_view, float4(o.position, 1.0)).xy;
    float R = o.radius;
    bool inRange = g_NpcDist <= 0.0 || distance(o.position, g_CamPos.xyz) <= g_NpcDist + R;

    uint cnt = 0u;
    if (inRange)
    {
        for (int L = 0; L < VSM_LEVELS; ++L)
        {
            VSM_PAGE_RANGE(L, lp, R, lo, hi, p0, p1);
            for (int py = p0.y; py <= p1.y; ++py)
            for (int px = p0.x; px <= p1.x; ++px)
            {
                uint slot = g_DynPageTable[vsmPageIndex(L, int2(px, py))];
                if (slot == VSM_UNMAPPED)
                    continue;
                if (cnt < g_Cap)
                {
                    g_CasterPages[c * g_Cap + cnt] = slot;
                    uint prev;
                    InterlockedOr(g_DynUsed[slot], 1u, prev);
                }
                ++cnt;
            }
        }
    }
    uint prevMax;
    InterlockedMax(g_Stats[2], cnt, prevMax);

    uint inst = min(cnt, g_Cap);
    uint outBase = c * 20u;
    g_Args.Store(outBase + 0u, indexCount);
    g_Args.Store(outBase + 4u, inst);
    g_Args.Store(outBase + 8u, firstIndex);
    g_Args.Store(outBase + 12u, baseVertex);
    g_Args.Store(outBase + 16u, c * g_Cap);
    if (inst > 0u)
    {
        uint d0;
        InterlockedAdd(g_Stats[0], 1u, d0);
        uint d1;
        InterlockedAdd(g_Stats[1], inst, d1);
    }
}
