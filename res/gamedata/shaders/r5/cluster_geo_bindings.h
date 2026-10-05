#ifndef CLUSTER_GEO_BINDINGS_H
#define CLUSTER_GEO_BINDINGS_H

#include "cluster_geo_common.h"

#ifndef CLUSTER_GEO_T_REFS
#define CLUSTER_GEO_T_REFS t14
#endif
#ifndef CLUSTER_GEO_T_META
#define CLUSTER_GEO_T_META t16
#endif
#ifndef CLUSTER_GEO_T_INSTANCES
#define CLUSTER_GEO_T_INSTANCES t20
#endif

StructuredBuffer<uint2> g_ClusterRefs CLUSTER_GEO_BIND(CLUSTER_GEO_T_REFS);
StructuredBuffer<ClusterMeta> g_ClusterMeta CLUSTER_GEO_BIND(CLUSTER_GEO_T_META);
StructuredBuffer<GeoInstance> g_GeoInstances CLUSTER_GEO_BIND(CLUSTER_GEO_T_INSTANCES);

#define CLUSTER_GROUP_NONE 0xFFFFFFFFu
#define CLUSTER_GROUP_RESIDENT 1u
#define CLUSTER_GROUP_COARSE_READY 2u

#ifdef CLUSTER_GEO_RESIDENCY
#ifndef CLUSTER_GEO_T_GROUPS
#define CLUSTER_GEO_T_GROUPS t26
#endif
#ifndef CLUSTER_GEO_T_GROUP_STATE
#define CLUSTER_GEO_T_GROUP_STATE t27
#endif

StructuredBuffer<uint2> g_ClusterGroups CLUSTER_GEO_BIND(CLUSTER_GEO_T_GROUPS);
ByteAddressBuffer g_ClusterGroupState CLUSTER_GEO_BIND(CLUSTER_GEO_T_GROUP_STATE);

uint ClusterGroupStateOf(uint group)
{
    return group == CLUSTER_GROUP_NONE ? 0u : g_ClusterGroupState.Load(group * 4u);
}

bool ClusterResidencyAdjust(uint clusterIndex, inout ClusterEntry e)
{
    uint2 groups = g_ClusterGroups[clusterIndex];
    uint owning = ClusterGroupStateOf(groups.x);
    if ((owning & CLUSTER_GROUP_RESIDENT) == 0u)
        return false;
    if ((owning & CLUSTER_GROUP_COARSE_READY) == 0u)
        e.parentError = CLUSTER_ERROR_INF;
    if ((ClusterGroupStateOf(groups.y) & CLUSTER_GROUP_RESIDENT) == 0u)
        e.selfError = 0.0;
    return true;
}

bool ClusterResidencyAdjustRef(uint refIdx, inout ClusterEntry e)
{
    return ClusterResidencyAdjust(g_ClusterRefs[refIdx].y, e);
}
#else
bool ClusterResidencyAdjust(uint clusterIndex, inout ClusterEntry e)
{
    return true;
}

bool ClusterResidencyAdjustRef(uint refIdx, inout ClusterEntry e)
{
    return true;
}
#endif

struct ClusterRefView
{
    ClusterEntry entry;
    float4x4 world;
    float4x4 prevWorld;
    uint historyValid;
    float hemiScale;
    float hemiBias;
    uint lightmapTexture;
};

ClusterRefView LoadClusterRefView(uint refIdx)
{
    uint2 ref = g_ClusterRefs[refIdx];
    GeoInstance inst = g_GeoInstances[ref.x];
    ClusterRefView view;
    view.entry = ClusterDecode(inst, g_ClusterMeta[ref.y], ref.x);
    view.world = inst.world;
    view.prevWorld = inst.historyValid != 0u ? inst.prevWorld : inst.world;
    view.historyValid = inst.historyValid;
    view.hemiScale = inst.hemiScale;
    view.hemiBias = inst.hemiBias;
    view.lightmapTexture = inst.lightmapTexture;
    return view;
}

ClusterEntry LoadClusterEntry(uint refIdx)
{
    uint2 ref = g_ClusterRefs[refIdx];
    ClusterEntry e = ClusterDecode(g_GeoInstances[ref.x], g_ClusterMeta[ref.y], ref.x);
#ifdef CLUSTER_GEO_RESIDENCY
    if (!ClusterResidencyAdjust(ref.y, e))
    {
        e.flags |= CLUSTER_ENTRY_FLAG_NO_SHADOW;
        e.indexCount = 0u;
    }
#endif
    return e;
}

float4x4 LoadClusterWorld(uint refIdx)
{
    return g_GeoInstances[g_ClusterRefs[refIdx].x].world;
}

float4x4 LoadClusterPrevWorld(uint refIdx)
{
    GeoInstance inst = g_GeoInstances[g_ClusterRefs[refIdx].x];
    return inst.historyValid != 0u ? inst.prevWorld : inst.world;
}

#endif
