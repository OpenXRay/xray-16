#ifndef CLUSTER_BVH_H
#define CLUSTER_BVH_H

#include "cluster_bvh_types.h"

#ifndef CLUSTER_BVH_T_NODES
#define CLUSTER_BVH_T_NODES t14
#endif
#ifndef CLUSTER_BVH_T_INDEX
#define CLUSTER_BVH_T_INDEX t15
#endif

StructuredBuffer<ClusterBvhNode> g_BvhNodes : register(CLUSTER_BVH_T_NODES);
StructuredBuffer<uint> g_BvhIndex : register(CLUSTER_BVH_T_INDEX);

groupshared uint gs_bvhStack[VSM_BVH_STACK_CAP];
groupshared uint gs_bvhStackCount;
groupshared uint gs_bvhLeaf[VSM_BVH_LEAF_CAP];
groupshared uint gs_bvhLeafCount;

void bvhSubtree(uint root, BvhQuery q)
{
    uint end = g_BvhNodes[root].escape;
    uint i = root;
    [loop] while (i < end)
    {
        ClusterBvhNode nd = g_BvhNodes[i];
        if (!bvhNodeTest(nd, q))
        {
            i = nd.escape;
            continue;
        }
        if (nd.count != 0u)
        {
            for (uint k = 0u; k < nd.count; ++k)
                BvhVisit(true, g_BvhIndex[nd.first + k], q);
            i = nd.escape;
        }
        else
        {
            ++i;
        }
    }
}

void bvhTraverse(uint t, uint nodeCount, BvhQuery q)
{
    if (t == 0u)
    {
        gs_bvhStackCount = (nodeCount > 0u) ? 1u : 0u;
        gs_bvhStack[0] = 0u;
        gs_bvhLeafCount = 0u;
    }
    GroupMemoryBarrierWithGroupSync();

    [loop] while (true)
    {
        uint avail = gs_bvhStackCount;
        if (avail == 0u)
            break;
        uint n = min(avail, uint(VSM_BVH_GROUP));
        uint node = VSM_BVH_NONE;
        if (t < n)
            node = gs_bvhStack[avail - 1u - t];
        GroupMemoryBarrierWithGroupSync();
        if (t == 0u)
            gs_bvhStackCount = avail - n;
        GroupMemoryBarrierWithGroupSync();

        if (node != VSM_BVH_NONE)
        {
            ClusterBvhNode nd = g_BvhNodes[node];
            if (bvhNodeTest(nd, q))
            {
                if (nd.count != 0u)
                {
                    uint pos;
                    InterlockedAdd(gs_bvhLeafCount, nd.count, pos);
                    for (uint k = 0u; k < nd.count; ++k)
                    {
                        if (pos + k < uint(VSM_BVH_LEAF_CAP))
                            gs_bvhLeaf[pos + k] = g_BvhIndex[nd.first + k];
                    }
                }
                else
                {
                    uint left = node + 1u;
                    uint right = g_BvhNodes[left].escape;
                    uint p0;
                    InterlockedAdd(gs_bvhStackCount, 1u, p0);
                    if (p0 < uint(VSM_BVH_STACK_CAP))
                        gs_bvhStack[p0] = left;
                    else
                        bvhSubtree(left, q);
                    uint p1;
                    InterlockedAdd(gs_bvhStackCount, 1u, p1);
                    if (p1 < uint(VSM_BVH_STACK_CAP))
                        gs_bvhStack[p1] = right;
                    else
                        bvhSubtree(right, q);
                }
            }
        }
        GroupMemoryBarrierWithGroupSync();

        uint leafN = min(gs_bvhLeafCount, uint(VSM_BVH_LEAF_CAP));
        if (t == 0u)
            gs_bvhStackCount = min(gs_bvhStackCount, uint(VSM_BVH_STACK_CAP));
        GroupMemoryBarrierWithGroupSync();

        for (uint base = 0u; base < leafN; base += uint(VSM_BVH_GROUP))
        {
            uint i = base + t;
            bool active = i < leafN;
            BvhVisit(active, active ? gs_bvhLeaf[i] : 0u, q);
        }
        GroupMemoryBarrierWithGroupSync();
        if (t == 0u)
            gs_bvhLeafCount = 0u;
        GroupMemoryBarrierWithGroupSync();
    }
}

#endif
