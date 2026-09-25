#define SM_5_0
#include "common.h"
#include "cull_utils.h"
#include "cluster_geo_common.h"
#include "cluster_cull_params.h"
#include "sw_dispatch_common.h"

StructuredBuffer<GeoInstance> g_GeoInstances;
StructuredBuffer<ClusterAssetMember> g_AssetMembers;
StructuredBuffer<ClusterAssetNode> g_AssetNodes;
Texture2D<float> g_HiZPyramid;
StructuredBuffer<uint> g_DeferredInstanceList;

RWByteAddressBuffer g_OutCount;
RWStructuredBuffer<uint2> g_OutNodes;
RWStructuredBuffer<uint> g_OutDeferredInstances;

[numthreads(64, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint lane : SV_GroupIndex)
{
    uint slot = SwDispatchLinearGroup(groupID) * 64u + lane;
    uint available = (g_Phase == 0u) ? g_InstanceCount : g_OutCount.Load(CLUSTER_COUNT_DEFER_INST);
    if (slot >= available)
        return;

    uint instanceIndex = (g_Phase == 0u) ? slot : g_DeferredInstanceList[slot];
    if (instanceIndex >= g_InstanceCount)
        return;

    GeoInstance inst = g_GeoInstances[instanceIndex];
    if (inst.refCount == 0u)
        return;
    if ((inst.flags & CLUSTER_ENTRY_FLAG_SHADOW_ONLY) != 0u)
        return;

    ClusterAssetMember member = g_AssetMembers[inst.assetMember];
    if (member.nodeCount == 0u)
        return;

    float3 center = mul(inst.world, float4(member.sphere.xyz, 1.0)).xyz;
    float radius = member.sphere.w * inst.scaleBound;
    if (!FrustumTestSphere(center, radius, g_FrustumPlanes))
        return;

    uint visits;
    g_OutCount.InterlockedAdd(CLUSTER_COUNT_INST_VISITS, 1u, visits);

    ClusterAssetNode root = g_AssetNodes[member.firstNode];
    bool strict = (root.nodeFlags & CLUSTER_ASSET_NODE_STRICT) != 0u
        || (inst.flags & CLUSTER_ENTRY_FLAG_PLAIN) != 0u
        || g_LodParams.z <= 0.0
        || g_LodParams.w > 0.5;

    bool historyOk = ClusterInstanceHistoryValid(inst, g_StaticHistoryValid);
    if (g_UseHiZ != 0u && g_CoarseHiZ != 0u && strict && (g_Phase != 0u || historyOk))
    {
        float4x4 historyWorld = (g_Phase == 0u) ? inst.prevWorld : inst.world;
        float historyScale = (g_Phase == 0u) ? inst.prevScaleBound : inst.scaleBound;
        float historyRadius = member.sphere.w * historyScale;
        float3 historyCenter = mul(historyWorld, float4(member.sphere.xyz, 1.0)).xyz;
        if (!HiZTestSphere(historyCenter, historyRadius, g_CameraPos.xyz, g_HiZViewProj,
                           g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels))
        {
            if (g_Phase != 0u)
                return;
            uint deferSlot;
            g_OutCount.InterlockedAdd(CLUSTER_COUNT_DEFER_INST, 1u, deferSlot);
            if (deferSlot < g_InstanceCount)
                g_OutDeferredInstances[deferSlot] = instanceIndex;
            else
                g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, deferSlot);
            return;
        }
    }

    uint nodeSlot;
    g_OutCount.InterlockedAdd(g_DstCountOffset, 1u, nodeSlot);
    uint capacity = (g_Phase == 0u) ? g_NodeQueueCapacity : g_DeferredNodeCapacity;
    if (nodeSlot < capacity)
        g_OutNodes[nodeSlot] = uint2(instanceIndex, member.firstNode);
    else
        g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, nodeSlot);
}
