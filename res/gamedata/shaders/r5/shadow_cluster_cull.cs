#define SM_5_0
#include "common.h"
#include "cull_utils.h"

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

cbuffer ShadowCullParams : register(b5)
{
    float4 g_FrustumPlanes[6];
    float g_ErrBudget;
    uint g_EntryCount;
    uint g_IncludeAT;
    uint g_ShadowCullPad;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t0);

RWByteAddressBuffer g_OutCount : register(u0);
RWStructuredBuffer<uint> g_OutOpaque : register(u1);
RWStructuredBuffer<uint> g_OutTerrain : register(u2);
RWStructuredBuffer<uint> g_OutAT : register(u3);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_EntryCount)
        return;

    ClusterEntry e = g_Entries[idx];

    bool at = (e.flags & 1u) != 0;
    if (at && g_IncludeAT == 0)
        return;

    if (!FrustumTestSphere6(e.sphere.xyz, e.sphere.w, g_FrustumPlanes))
        return;

    if ((e.flags & 2u) == 0)
    {
        if (e.selfError > g_ErrBudget || e.parentError <= g_ErrBudget)
            return;
    }

    uint slot;
    if (at)
    {
        g_OutCount.InterlockedAdd(8, 1u, slot);
        g_OutAT[slot] = idx;
    }
    else if ((e.flags & 4u) != 0)
    {
        g_OutCount.InterlockedAdd(4, 1u, slot);
        g_OutTerrain[slot] = idx;
    }
    else
    {
        g_OutCount.InterlockedAdd(0, 1u, slot);
        g_OutOpaque[slot] = idx;
    }
}
