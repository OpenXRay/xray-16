#ifndef CLUSTER_GEO_COMMON_H
#define CLUSTER_GEO_COMMON_H

#include "visbuffer_common.h"

#ifdef CLUSTER_GEO_AUTO_BIND
#define CLUSTER_GEO_BIND(reg)
#else
#define CLUSTER_GEO_BIND(reg) : register(reg)
#endif

struct ClusterMeta
{
    float4 sphere;
    float4 lodSelf;
    float4 lodParent;
    float3 extent;
    float selfError;
    float parentError;
    uint page;
    uint payloadOffset;
    uint flags;
};

struct ClusterPage
{
    uint vertexBase;
    uint payloadBase;
    uint vertexCount;
    uint vertexBytes;
    uint payloadBytes;
    uint attributeMask;
    uint vertexFormat;
    uint assetMember;
};

struct GeoInstance
{
    float4x4 world;
    float4x4 prevWorld;
    uint assetMember;
    uint materialID;
    uint flags;
    float scaleBound;
    uint firstRef;
    uint refCount;
    uint firstPage;
    uint historyValid;
    float prevScaleBound;
    float hemiScale;
    float hemiBias;
    uint lightmapTexture;
};

struct ClusterAssetMember
{
    float4 sphere;
    float3 extent;
    uint firstCluster;
    uint clusterCount;
    uint firstNode;
    uint nodeCount;
    uint firstPage;
};

struct ClusterAssetNode
{
    float3 bmin;
    float maxParentError;
    float3 bmax;
    float minParentError;
    float3 lodCenter;
    float lodRadius;
    float maxSelfError;
    float minSelfError;
    uint child;
    uint nodeFlags;
};

#define CLUSTER_ASSET_NODE_LEAF 1u
#define CLUSTER_ASSET_NODE_STRICT 2u
#define CLUSTER_ASSET_NODE_COUNT_SHIFT 8u
#define CLUSTER_ASSET_NODE_COUNT_MASK 0xFu

#define CLUSTER_COUNT_VISIBLE 0u
#define CLUSTER_COUNT_TERRAIN 4u
#define CLUSTER_COUNT_TRIS 8u
#define CLUSTER_COUNT_TERRAIN_TRIS 12u
#define CLUSTER_COUNT_CANDIDATES 16u
#define CLUSTER_COUNT_VISIBLE2 20u
#define CLUSTER_COUNT_TERRAIN2 24u
#define CLUSTER_COUNT_TRIS2 28u
#define CLUSTER_COUNT_TERRAIN_TRIS2 32u
#define CLUSTER_COUNT_OVERFLOW 36u
#define CLUSTER_COUNT_SW 40u
#define CLUSTER_COUNT_SW2 44u
#define CLUSTER_COUNT_QUEUE_A 48u
#define CLUSTER_COUNT_QUEUE_B 52u
#define CLUSTER_COUNT_DEFER_NODES 56u
#define CLUSTER_COUNT_DEFER_INST 60u
#define CLUSTER_COUNT_LEAF 64u
#define CLUSTER_COUNT_LEAF2 68u
#define CLUSTER_COUNT_INST_VISITS 72u
#define CLUSTER_COUNT_NODE_VISITS 76u

#define CLUSTER_ERROR_INF 1e30

#define CLUSTER_META_TRIANGLE_SHIFT 16u
#define CLUSTER_META_VERTEX_SHIFT 23u
#define CLUSTER_META_COUNT_MASK 0x7Fu

uint ClusterMetaTriangles(uint flags)
{
    return ((flags >> CLUSTER_META_TRIANGLE_SHIFT) & CLUSTER_META_COUNT_MASK) + 1u;
}

uint ClusterMetaVertices(uint flags)
{
    return ((flags >> CLUSTER_META_VERTEX_SHIFT) & CLUSTER_META_COUNT_MASK) + 1u;
}

float3 ClusterTransformExtent(float4x4 world, float3 extent)
{
    return float3(
        abs(world[0][0]) * extent.x + abs(world[0][1]) * extent.y + abs(world[0][2]) * extent.z,
        abs(world[1][0]) * extent.x + abs(world[1][1]) * extent.y + abs(world[1][2]) * extent.z,
        abs(world[2][0]) * extent.x + abs(world[2][1]) * extent.y + abs(world[2][2]) * extent.z);
}

ClusterEntry ClusterDecode(GeoInstance inst, ClusterMeta cm, uint instanceIndex)
{
    ClusterEntry e;

    float3 center = mul(inst.world, float4(cm.sphere.xyz, 1.0)).xyz;
    e.sphere = float4(center, cm.sphere.w * inst.scaleBound);
    e.extent = ClusterTransformExtent(inst.world, cm.extent);
    e.extentPad = 0.0;
    e.lodSelf = float4(mul(inst.world, float4(cm.lodSelf.xyz, 1.0)).xyz, cm.lodSelf.w * inst.scaleBound);
    e.lodParent = float4(mul(inst.world, float4(cm.lodParent.xyz, 1.0)).xyz, cm.lodParent.w * inst.scaleBound);

    e.selfError = cm.selfError * inst.scaleBound;
    float pe = cm.parentError * inst.scaleBound;
    e.parentError = (pe < CLUSTER_ERROR_INF) ? pe : CLUSTER_ERROR_INF;

    e.indexCount = ClusterMetaTriangles(cm.flags) * 3u;
    e.ibFirst = 0u;
    e.firstVertex = 0u;
    e.page = cm.page;
    e.payloadOffset = cm.payloadOffset;
    e.vertexCount = ClusterMetaVertices(cm.flags);
    e.geoPad = 0u;
    e.batchIndex = instanceIndex;
    e.materialID = inst.materialID;

    uint flags = (cm.flags & (CLUSTER_ENTRY_FLAG_AT | CLUSTER_ENTRY_FLAG_TERRAIN)) | inst.flags;
    if ((flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u)
        flags &= ~CLUSTER_ENTRY_FLAG_NO_SHADOW;

    uint errClass = 3u;
    if (e.parentError < 1.0)
        errClass = 0u;
    else if (e.parentError < 10.0)
        errClass = 1u;
    else if (e.parentError < CLUSTER_ERROR_INF)
        errClass = 2u;

    e.flags = flags | (cm.flags & 0xF00u) | (errClass << 12u);
    return e;
}

bool ClusterInstanceHistoryValid(GeoInstance inst, uint staticHistoryValid)
{
    if ((inst.flags & CLUSTER_ENTRY_FLAG_DYNAMIC) != 0u)
        return inst.historyValid != 0u;
    return staticHistoryValid != 0u;
}

float ClusterProjectedError(float4 sphere, float error, float3 cameraPos, float3 viewDir, float4 lodParams)
{
    float d = dot(viewDir, sphere.xyz - cameraPos) - sphere.w;
    return error * lodParams.x / max(lodParams.y, d);
}

bool ClusterSelectLod(ClusterEntry e, float3 cameraPos, float3 viewDir, float4 lodParams,
    out uint fA, out uint fB)
{
    fA = 63u;
    fB = 0u;

    if ((e.flags & CLUSTER_ENTRY_FLAG_PLAIN) != 0u)
        return true;

    if (lodParams.w > 0.5)
        return e.selfError == 0.0;

    float band = lodParams.z;
    float pp = ClusterProjectedError(e.lodParent, e.parentError, cameraPos, viewDir, lodParams);
    float sp = ClusterProjectedError(e.lodSelf, e.selfError, cameraPos, viewDir, lodParams);

    if (band <= 0.0 || (e.flags & CLUSTER_ENTRY_FLAG_AT) != 0u)
        return pp > 1.0 && sp <= 1.0;

    if (pp <= 1.0 || sp > 1.0 + band)
        return false;
    fA = (uint)round(clamp((pp - 1.0) / band, 0.0, 1.0) * 63.0);
    fB = (uint)round(clamp((sp - 1.0) / band, 0.0, 1.0) * 63.0);
    return fA != 0u;
}

uint ClusterPackFade(uint fA, uint fB, uint flags, uint refIdx)
{
    return (63u - fA) | (fB << 6) | (((flags >> 8) & 0x3Fu) << 12) | ((refIdx & 0x3FFFu) << 18);
}

#endif
