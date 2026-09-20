#define SM_5_0
#include "common.h"
#include "cull_utils.h"

#define CLUSTER_GEO_T_REFS t0
#define CLUSTER_GEO_T_META t1
#define CLUSTER_GEO_T_INSTANCES t2
#define CLUSTER_GEO_RESIDENCY
#define CLUSTER_GEO_T_GROUPS t5
#define CLUSTER_GEO_T_GROUP_STATE t6
#include "cluster_geo_bindings.h"
#include "cluster_cull_params.h"
#include "sw_dispatch_common.h"

Texture2D<float> g_HiZPyramid : register(t3);
StructuredBuffer<uint> g_LeafQueue : register(t4);

RWByteAddressBuffer g_OutCount : register(u0);
RWStructuredBuffer<uint> g_OutEntryIndices : register(u1);
RWStructuredBuffer<uint> g_OutFades : register(u2);
RWStructuredBuffer<uint> g_OutTerrainEntryIndices : register(u3);
RWStructuredBuffer<uint> g_OutTerrainFades : register(u4);
RWStructuredBuffer<uint> g_OutCandidates : register(u5);
RWStructuredBuffer<uint> g_OutSwEntries : register(u6);

[numthreads(64, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint lane : SV_GroupIndex)
{
    uint slot = SwDispatchLinearGroup(groupID) * 64u + lane;
    if (slot >= g_OutCount.Load(g_SrcCountOffset))
        return;

    uint refIdx = g_LeafQueue[slot];
    if (refIdx >= g_RefCount)
        return;

    uint2 clusterRef = g_ClusterRefs[refIdx];
    GeoInstance inst = g_GeoInstances[clusterRef.x];
    ClusterMeta cm = g_ClusterMeta[clusterRef.y];
    ClusterEntry e = ClusterDecode(inst, cm, clusterRef.x);
    if (!ClusterResidencyAdjust(clusterRef.y, e))
        return;
    if ((e.flags & CLUSTER_ENTRY_FLAG_SHADOW_ONLY) != 0u)
        return;

    if (!FrustumTestSphere(e.sphere.xyz, e.sphere.w, g_FrustumPlanes))
        return;

    if (g_SsaCull > 0.0 && (e.flags & CLUSTER_ENTRY_FLAG_PLAIN) != 0u)
    {
        float d = max(0.01, dot(g_ViewDir.xyz, e.sphere.xyz - g_CameraPos.xyz) - e.sphere.w);
        if (e.sphere.w * g_SsaCull < d)
            return;
    }

    uint fA;
    uint fB;
    if (!ClusterSelectLod(e, g_CameraPos.xyz, g_ViewDir.xyz, g_LodParams, fA, fB))
        return;

    bool historyOk = ClusterInstanceHistoryValid(inst, g_StaticHistoryValid);
    if (g_UseHiZ != 0u && fA == 63u && fB == 0u && (g_Phase != 0u || historyOk))
    {
        float3 testCenter = e.sphere.xyz;
        float testRadius = e.sphere.w;
        if (g_Phase == 0u)
        {
            testCenter = mul(inst.prevWorld, float4(cm.sphere.xyz, 1.0)).xyz;
            testRadius = cm.sphere.w * inst.prevScaleBound;
        }
        if (!HiZTestSphere(testCenter, testRadius, g_CameraPos.xyz, g_HiZViewProj,
                           g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels))
        {
            if (g_Phase != 0u)
                return;
            uint cslot;
            g_OutCount.InterlockedAdd(CLUSTER_COUNT_CANDIDATES, 1u, cslot);
            if (cslot < g_LeafQueueCapacity)
                g_OutCandidates[cslot] = refIdx;
            else
                g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, cslot);
            return;
        }
    }

    if (fA == 63u && fB == 0u && (e.flags & CLUSTER_ENTRY_FLAG_AT) == 0u && g_SwCull > 0.0)
    {
        float d = dot(g_ViewDir.xyz, e.sphere.xyz - g_CameraPos.xyz) - e.sphere.w;
        if (d > g_SwNearZ && e.sphere.w <= g_SwCull * d)
        {
            uint swSlot;
            g_OutCount.InterlockedAdd(g_Phase == 0u ? CLUSTER_COUNT_SW : CLUSTER_COUNT_SW2, 1u, swSlot);
            if (swSlot < g_LeafQueueCapacity)
                g_OutSwEntries[swSlot] = refIdx;
            else
                g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, swSlot);
            return;
        }
    }

    uint fade = ClusterPackFade(fA, fB, e.flags, refIdx);

    uint outSlot;
    uint tris;
    if ((e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u)
    {
        g_OutCount.InterlockedAdd(g_Phase == 0u ? CLUSTER_COUNT_TERRAIN : CLUSTER_COUNT_TERRAIN2, 1u, outSlot);
        g_OutCount.InterlockedAdd(g_Phase == 0u ? CLUSTER_COUNT_TERRAIN_TRIS : CLUSTER_COUNT_TERRAIN_TRIS2,
            e.indexCount / 3u, tris);
        if (outSlot < g_LeafQueueCapacity)
        {
            g_OutTerrainEntryIndices[outSlot] = refIdx;
            g_OutTerrainFades[outSlot] = fade;
        }
        else
        {
            g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, outSlot);
        }
    }
    else
    {
        g_OutCount.InterlockedAdd(g_Phase == 0u ? CLUSTER_COUNT_VISIBLE : CLUSTER_COUNT_VISIBLE2, 1u, outSlot);
        g_OutCount.InterlockedAdd(g_Phase == 0u ? CLUSTER_COUNT_TRIS : CLUSTER_COUNT_TRIS2,
            e.indexCount / 3u, tris);
        if (outSlot < g_LeafQueueCapacity)
        {
            g_OutEntryIndices[outSlot] = refIdx;
            g_OutFades[outSlot] = fade;
        }
        else
        {
            g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, outSlot);
        }
    }
}
