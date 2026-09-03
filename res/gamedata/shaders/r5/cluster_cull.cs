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

RWByteAddressBuffer g_OutCount : register(u0);
RWStructuredBuffer<uint> g_OutEntryIndices : register(u1);
RWStructuredBuffer<uint> g_OutFades : register(u2);
RWStructuredBuffer<uint> g_OutTerrainEntryIndices : register(u3);
RWStructuredBuffer<uint> g_OutTerrainFades : register(u4);
RWStructuredBuffer<uint> g_OutCandidates : register(u5);

float ProjErr(float4 s, float e)
{
    float d = dot(g_ViewDir.xyz, s.xyz - g_CameraPos.xyz) - s.w;
    return e * g_LodParams.x / max(g_LodParams.y, d);
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint idx = dtID.x;
    if (idx >= g_EntryCount)
        return;

    ClusterEntry e = g_Entries[idx];
    if ((e.flags & 8u) != 0u)
        return;

    if (!FrustumTestSphere(e.sphere.xyz, e.sphere.w, g_FrustumPlanes))
        return;

    if (g_SsaCull > 0.0 && (e.flags & 2u) != 0)
    {
        float d = max(0.01, dot(g_ViewDir.xyz, e.sphere.xyz - g_CameraPos.xyz) - e.sphere.w);
        if (e.sphere.w * g_SsaCull < d)
            return;
    }

    uint fA = 63u;
    uint fB = 0u;

    if ((e.flags & 2u) == 0)
    {
        if (g_LodParams.w > 0.5)
        {
            if (e.selfError != 0.0)
                return;
        }
        else
        {
            float band = g_LodParams.z;
            float pp = ProjErr(e.lodParent, e.parentError);
            float sp = ProjErr(e.lodSelf, e.selfError);

            if (band <= 0.0 || (e.flags & 1u) != 0)
            {
                if (!(pp > 1.0 && sp <= 1.0))
                    return;
            }
            else
            {
                if (pp <= 1.0 || sp > 1.0 + band)
                    return;
                fA = (uint)round(clamp((pp - 1.0) / band, 0.0, 1.0) * 63.0);
                fB = (uint)round(clamp((sp - 1.0) / band, 0.0, 1.0) * 63.0);
                if (fA == 0u)
                    return;
            }
        }
    }

    if (g_UseHiZ != 0 && fA == 63u && fB == 0u)
    {
        if (!HiZTestSphere(e.sphere.xyz, e.sphere.w, g_CameraPos.xyz, g_HiZViewProj,
                           g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels))
        {
            uint cslot;
            g_OutCount.InterlockedAdd(16, 1u, cslot);
            g_OutCandidates[cslot] = idx;
            return;
        }
    }

    uint fade = (63u - fA) | (fB << 6) | (((e.flags >> 8) & 0x3Fu) << 12) | ((idx & 0x3FFFu) << 18);

    uint slot;
    uint tris;
    if ((e.flags & 4u) != 0)
    {
        g_OutCount.InterlockedAdd(4, 1u, slot);
        g_OutCount.InterlockedAdd(12, e.indexCount / 3u, tris);
        g_OutTerrainEntryIndices[slot] = idx;
        g_OutTerrainFades[slot] = fade;
    }
    else
    {
        g_OutCount.InterlockedAdd(0, 1u, slot);
        g_OutCount.InterlockedAdd(8, e.indexCount / 3u, tris);
        g_OutEntryIndices[slot] = idx;
        g_OutFades[slot] = fade;
    }
}
