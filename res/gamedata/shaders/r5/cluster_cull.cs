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

struct IndirectDrawArgs
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
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

RWStructuredBuffer<IndirectDrawArgs> g_OutCmds : register(u0);
RWByteAddressBuffer g_OutCount : register(u1);
RWStructuredBuffer<uint> g_OutBatchIndices : register(u2);
RWStructuredBuffer<uint> g_OutMaterialIDs : register(u3);
RWStructuredBuffer<uint> g_OutFades : register(u4);

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
            return;
    }

    uint slot;
    g_OutCount.InterlockedAdd(0, 1u, slot);

    IndirectDrawArgs cmd;
    cmd.indexCountPerInstance = e.indexCount;
    cmd.instanceCount = 1;
    cmd.startIndexLocation = e.ibFirst;
    cmd.baseVertexLocation = (int)e.firstVertex;
    cmd.startInstanceLocation = slot;
    g_OutCmds[slot] = cmd;
    g_OutBatchIndices[slot] = e.batchIndex;
    g_OutMaterialIDs[slot] = e.materialID;
    g_OutFades[slot] = fA | (fB << 6);
}
