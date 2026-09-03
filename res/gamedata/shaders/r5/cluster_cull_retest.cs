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

cbuffer ClusterCullParams : register(b5)
{
    float4x4 g_HiZViewProj;
    float4 g_FrustumPlanes[6];
    float4 g_CameraPos;
    float4 g_ViewDir;
    float4 g_LodParams;
    uint g_EntryCount;
    uint g_UseHiZ;
    uint g_HiZWidth;
    uint g_HiZHeight;
    uint g_HiZMipLevels;
    float g_SsaCull;
    uint2 g_ClusterPad;
};

StructuredBuffer<ClusterEntry> g_Entries : register(t0);
Texture2D<float> g_HiZPyramid : register(t1);
StructuredBuffer<uint> g_Candidates : register(t2);

RWByteAddressBuffer g_OutCount : register(u0);
RWStructuredBuffer<uint> g_OutEntryIndices : register(u1);
RWStructuredBuffer<uint> g_OutFades : register(u2);
RWStructuredBuffer<uint> g_OutTerrainEntryIndices : register(u3);
RWStructuredBuffer<uint> g_OutTerrainFades : register(u4);

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint i = dtID.x;
    if (i >= g_OutCount.Load(16))
        return;

    uint idx = g_Candidates[i];
    ClusterEntry e = g_Entries[idx];

    if (!HiZTestSphere(e.sphere.xyz, e.sphere.w, g_CameraPos.xyz, g_HiZViewProj,
                       g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels))
        return;

    uint fade = (((e.flags >> 8) & 0x3Fu) << 12) | ((idx & 0x3FFFu) << 18);

    uint slot;
    uint tris;
    if ((e.flags & 4u) != 0)
    {
        g_OutCount.InterlockedAdd(24, 1u, slot);
        g_OutCount.InterlockedAdd(32, e.indexCount / 3u, tris);
        g_OutTerrainEntryIndices[slot] = idx;
        g_OutTerrainFades[slot] = fade;
    }
    else
    {
        g_OutCount.InterlockedAdd(20, 1u, slot);
        g_OutCount.InterlockedAdd(28, e.indexCount / 3u, tris);
        g_OutEntryIndices[slot] = idx;
        g_OutFades[slot] = fade;
    }
}
