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
StructuredBuffer<uint2> g_SrcNodes;

RWByteAddressBuffer g_OutCount;
RWStructuredBuffer<uint2> g_OutNodes;
RWStructuredBuffer<uint2> g_OutDeferredNodes;
RWStructuredBuffer<uint> g_OutLeaves;

void EmitLeaf(uint refIdx)
{
    uint slot;
    g_OutCount.InterlockedAdd(g_Phase == 0u ? CLUSTER_COUNT_LEAF : CLUSTER_COUNT_LEAF2, 1u, slot);
    if (slot < g_LeafQueueCapacity)
        g_OutLeaves[slot] = refIdx;
    else
        g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, slot);
}

void EmitNode(uint instanceIndex, uint nodeIndex)
{
    uint slot;
    g_OutCount.InterlockedAdd(g_DstCountOffset, 1u, slot);
    if (slot < g_NodeQueueCapacity)
        g_OutNodes[slot] = uint2(instanceIndex, nodeIndex);
    else
        g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, slot);
}

[numthreads(64, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint lane : SV_GroupIndex)
{
    uint slot = SwDispatchLinearGroup(groupID) * 64u + lane;
    if (slot >= g_OutCount.Load(g_SrcCountOffset))
        return;

    uint2 work = g_SrcNodes[slot];
    uint instanceIndex = work.x;
    uint nodeIndex = work.y;
    if (instanceIndex >= g_InstanceCount)
        return;

    GeoInstance inst = g_GeoInstances[instanceIndex];
    ClusterAssetMember member = g_AssetMembers[inst.assetMember];
    ClusterAssetNode node = g_AssetNodes[nodeIndex];

    uint visits;
    g_OutCount.InterlockedAdd(CLUSTER_COUNT_NODE_VISITS, 1u, visits);

    float3 localCenter = (node.bmin + node.bmax) * 0.5;
    float3 localHalf = (node.bmax - node.bmin) * 0.5;
    float3 center = mul(inst.world, float4(localCenter, 1.0)).xyz;
    float3 half3 = ClusterTransformExtent(inst.world, localHalf);
    float radius = length(half3);

    if (!FrustumTestSphere(center, radius, g_FrustumPlanes))
        return;

    float3 lodCenter = mul(inst.world, float4(node.lodCenter, 1.0)).xyz;
    float lodRadius = node.lodRadius * inst.scaleBound;
    float maxParentError = min(node.maxParentError * inst.scaleBound, CLUSTER_ERROR_INF);
    float lodDepth = dot(g_ViewDir.xyz, lodCenter - g_CameraPos.xyz);
    float dNear = max(g_LodParams.y, lodDepth - lodRadius);
    float dFar = max(g_LodParams.y, lodDepth + lodRadius);
    bool plain = (inst.flags & CLUSTER_ENTRY_FLAG_PLAIN) != 0u;

    if (!plain)
    {
        if (g_LodParams.w > 0.5)
        {
            if (g_ResidencyStreaming == 0u && node.minSelfError * inst.scaleBound > 0.0)
                return;
        }
        else
        {
            if (maxParentError < CLUSTER_ERROR_INF && maxParentError * g_LodParams.x / dNear <= 1.0)
                return;
            if (g_ResidencyStreaming == 0u)
            {
                float selfLimit = (node.nodeFlags & CLUSTER_ASSET_NODE_STRICT) != 0u
                    ? 1.0 : 1.0 + max(0.0, g_LodParams.z);
                float minSelfError = min(node.minSelfError * inst.scaleBound, CLUSTER_ERROR_INF);
                if (minSelfError * g_LodParams.x / dFar > selfLimit)
                    return;
            }
        }
    }

    bool strict = (node.nodeFlags & CLUSTER_ASSET_NODE_STRICT) != 0u
        || plain
        || g_LodParams.z <= 0.0
        || g_LodParams.w > 0.5;
    if (!strict && maxParentError < CLUSTER_ERROR_INF)
    {
        float band = g_LodParams.z;
        float minParentProjected = node.minParentError * inst.scaleBound * g_LodParams.x / dFar;
        float maxSelfProjected = node.maxSelfError * inst.scaleBound * g_LodParams.x / dNear;
        strict = minParentProjected >= 1.0 + band && maxSelfProjected <= 1.0;
    }

    bool historyOk = ClusterInstanceHistoryValid(inst, g_StaticHistoryValid);
    if (g_UseHiZ != 0u && g_CoarseHiZ != 0u && strict && (g_Phase != 0u || historyOk))
    {
        float4x4 historyWorld = (g_Phase == 0u) ? inst.prevWorld : inst.world;
        float3 historyCenter = mul(historyWorld, float4(localCenter, 1.0)).xyz;
        float historyRadius = length(ClusterTransformExtent(historyWorld, localHalf));
        if (!HiZTestSphere(historyCenter, historyRadius, g_CameraPos.xyz, g_HiZViewProj,
                           g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels))
        {
            if (g_Phase != 0u)
                return;
            uint deferSlot;
            g_OutCount.InterlockedAdd(CLUSTER_COUNT_DEFER_NODES, 1u, deferSlot);
            if (deferSlot < g_DeferredNodeCapacity)
                g_OutDeferredNodes[deferSlot] = uint2(instanceIndex, nodeIndex);
            else
                g_OutCount.InterlockedAdd(CLUSTER_COUNT_OVERFLOW, 1u, deferSlot);
            return;
        }
    }

    uint childCount = (node.nodeFlags >> CLUSTER_ASSET_NODE_COUNT_SHIFT) & CLUSTER_ASSET_NODE_COUNT_MASK;
    if ((node.nodeFlags & CLUSTER_ASSET_NODE_LEAF) != 0u)
    {
        for (uint c = 0u; c < childCount; ++c)
        {
            uint clusterIndex = node.child + c;
            if (clusterIndex < member.firstCluster || clusterIndex >= member.firstCluster + member.clusterCount)
                continue;
            EmitLeaf(inst.firstRef + (clusterIndex - member.firstCluster));
        }
        return;
    }

    for (uint k = 0u; k < childCount; ++k)
        EmitNode(instanceIndex, node.child + k);
}
