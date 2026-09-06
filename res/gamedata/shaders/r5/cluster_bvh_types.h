#ifndef CLUSTER_BVH_TYPES_H
#define CLUSTER_BVH_TYPES_H

#define VSM_BVH_STACK_CAP 1024
#define VSM_BVH_LEAF_CAP  1024
#define VSM_BVH_GROUP     256
#define VSM_BVH_NONE      0xFFFFFFFFu

struct ClusterBvhNode
{
    float3 bmin;
    float minSelfError;
    float3 bmax;
    float maxParentError;
    uint first;
    uint count;
    uint escape;
    uint pad;
};

#endif
