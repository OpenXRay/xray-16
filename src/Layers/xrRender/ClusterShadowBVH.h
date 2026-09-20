#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::fg
{

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

class GPUClusterAssetNode
{
public:
    float bmin[3];
    float maxParentError;
    float bmax[3];
    float minParentError;
    float lodCenter[3];
    float lodRadius;
    float maxSelfError;
    float minSelfError;
    u32 child;
    u32 flags;
};
#pragma pack(pop)

static_assert(sizeof(ClusterBvhNode) == 48, "ClusterBvhNode is shader-visible");
static_assert(sizeof(GPUClusterAssetNode) == 64, "GPUClusterAssetNode is shader-visible");

constexpr u32 kClusterBvhLeafSize = 4;
constexpr u32 kClusterAssetBvhWidth = 8;

constexpr u32 CLUSTER_ASSET_NODE_LEAF = 1u << 0;
constexpr u32 CLUSTER_ASSET_NODE_STRICT = 1u << 1;
constexpr u32 CLUSTER_ASSET_NODE_COUNT_SHIFT = 8;
constexpr u32 CLUSTER_ASSET_NODE_COUNT_MASK = 0xFu;

class ClusterBvhPrim
{
public:
    Fvector lo;
    Fvector hi;
    Fvector centroid;
    Fvector lodCenter;
    float lodRadius;
    float selfError;
    float parentError;
    u32 key;
    u32 payload;
};

struct ClusterBvh {
    xr_vector<ClusterBvhNode> nodes;
    xr_vector<u32> indices;
    u32 maxDepth = 0;
    u32 leafCount = 0;
};

class ClusterAssetBvh
{
public:
    xr_vector<GPUClusterAssetNode> nodes;
    xr_vector<u32> order;
    u32 maxDepth = 0;
    u32 leafCount = 0;
};

struct ClusterShadowCaster {
    float radius;
    float selfError;
    float parentError;
    u32 stream;
};

namespace cluster_bvh_detail
{
class Box
{
public:
    Fvector lo;
    Fvector hi;
    Fvector lodCenter;
    float lodRadius;
    float minSelf;
    float maxSelf;
    float minParent;
    float maxParent;
    bool hasLod;

    void reset();
    void addSphere(const Fvector& center, float radius);
    void add(const ClusterBvhPrim& prim);
    void merge(const Box& other);
    float area() const;
};

class EscapeBuilder
{
public:
    xr_vector<ClusterBvhPrim>* prims;
    ClusterBvh* out;

    u32 Build(u32 first, u32 count, u32 depth);
};

class TempNode
{
public:
    Box bounds;
    u32 left;
    u32 right;
    u32 first;
    u32 count;
    bool leaf;
};

class WideBuilder
{
public:
    xr_vector<ClusterBvhPrim>* prims;
    xr_vector<TempNode> temps;
    ClusterAssetBvh* out;
    bool strict;
    bool overflow;

    u32 BuildTemp(u32 first, u32 count);
    void Store(u32 slot, const Box& bounds, u32 child, u32 flags);
    void BuildWide(u32 slot, u32 temp, u32 depth);
};

u32 MedianSplit(xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count, u32 axis);
u32 SpatialSplit(xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count, const Fvector& lo, const Fvector& hi);
void CentroidRange(const xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count, Fvector& lo, Fvector& hi);
u32 KeySplit(xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count);
}

void ClusterMergeSphere(Fvector& center, float& radius, const Fvector& other, float otherRadius);
void BuildClusterShadowBVH(ClusterBvhPrim* prims, u32 count, ClusterBvh& out);
bool BuildClusterAssetBVH8(ClusterBvhPrim* prims, u32 count, bool strict, ClusterAssetBvh& out);
bool ClusterShadowPairCapacity(const xr_vector<ClusterShadowCaster>& casters,
    float pageWidth, float errorThreshold, u32 pagesAxis, u32* capacity);

}
