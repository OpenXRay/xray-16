#ifndef VSM_BVH_TYPES_H
#define VSM_BVH_TYPES_H

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

struct VsmPageQuery
{
    float2 pmin;
    float2 pmax;
    float errB;
    uint slot;
    uint includeAT;
    uint pad;
};

float2 vsmLightHalfExtent(float3 h)
{
    return float2(
        abs(vsm_view[0][0]) * h.x + abs(vsm_view[0][1]) * h.y + abs(vsm_view[0][2]) * h.z,
        abs(vsm_view[1][0]) * h.x + abs(vsm_view[1][1]) * h.y + abs(vsm_view[1][2]) * h.z);
}

bool vsmBoxTouchesPage(float3 center, float3 half3, VsmPageQuery q)
{
    float2 lc = mul(vsm_view, float4(center, 1.0)).xy;
    float2 lh = vsmLightHalfExtent(half3);
    return (lc.x + lh.x >= q.pmin.x) && (lc.x - lh.x < q.pmax.x)
        && (lc.y + lh.y >= q.pmin.y) && (lc.y - lh.y < q.pmax.y);
}

bool vsmNodeTouchesPage(ClusterBvhNode nd, VsmPageQuery q)
{
    if (nd.minSelfError > q.errB || nd.maxParentError <= q.errB)
        return false;
    float3 c = (nd.bmin + nd.bmax) * 0.5;
    float3 h = (nd.bmax - nd.bmin) * 0.5;
    return vsmBoxTouchesPage(c, h, q);
}

bool vsmEntryTouchesPage(ClusterEntry e, VsmPageQuery q)
{
    if ((e.flags & 2u) == 0u)
    {
        if (e.selfError > q.errB || e.parentError <= q.errB)
            return false;
    }
    return vsmBoxTouchesPage(e.sphere.xyz, e.extent, q);
}

#endif
