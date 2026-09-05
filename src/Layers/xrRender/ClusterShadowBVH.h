#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::fg
{

struct GPUClusterEntry;

#pragma pack(push, 4)
struct ClusterBvhNode {
    float bmin[3];
    float minSelfError;
    float bmax[3];
    float maxParentError;
    u32 first;
    u32 count;
    u32 escape;
    u32 pad;
};
#pragma pack(pop)

static_assert(sizeof(ClusterBvhNode) == 48, "ClusterBvhNode is shader-visible");

constexpr u32 kClusterBvhLeafSize = 4;

struct ClusterBvh {
    xr_vector<ClusterBvhNode> nodes;
    xr_vector<u32> indices;
    u32 maxDepth = 0;
    u32 leafCount = 0;
};

void BuildClusterShadowBVH(const GPUClusterEntry* entries, u32 count, ClusterBvh& out);

}
