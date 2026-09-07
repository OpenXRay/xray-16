#ifndef LOCAL_SHADOW_BVH_TYPES_H
#define LOCAL_SHADOW_BVH_TYPES_H

struct LocalViewQuery
{
    float4 planes[6];
    float3 lightPos;
    float  range;
    float  nearZ;
    float  texelPerMetre;
    float  errK;
    uint   slot;
    uint   includeAT;
    uint3  pad;
};

float localBoxNear(float3 c, float3 h, float3 p) { return length(max(abs(p - c) - h, 0.0)); }
float localBoxFar(float3 c, float3 h, float3 p)  { return length(abs(p - c) + h); }

bool localBoxOutside(float3 c, float3 h, LocalViewQuery q)
{
    if (localBoxNear(c, h, q.lightPos) > q.range)
        return true;
    // The projection near plane is for rasterization, not caster rejection.
    [unroll] for (uint i = 0u; i < 5u; ++i)
    {
        float3 n = q.planes[i].xyz;
        if (dot(n, c) + q.planes[i].w > dot(abs(n), h))
            return true;
    }
    return false;
}

bool bvhNodeTest(ClusterBvhNode nd, LocalViewQuery q)
{
    float3 c = (nd.bmin + nd.bmax) * 0.5;
    float3 h = (nd.bmax - nd.bmin) * 0.5;
    if (localBoxOutside(c, h, q))
        return false;
    // One threshold for the entire hierarchy gives a complete cut. Per-entry
    // AABB distances let both a near parent and its far child reject themselves.
    float errB = q.nearZ * q.texelPerMetre * q.errK;
    return nd.minSelfError <= errB && nd.maxParentError > errB;
}

bool localEntryTouchesView(ClusterEntry e, LocalViewQuery q)
{
    if ((e.flags & CLUSTER_ENTRY_FLAG_NO_SHADOW) != 0u)
        return false;
    if (localBoxOutside(e.sphere.xyz, e.extent, q))
        return false;
    if ((e.flags & 2u) != 0u)
        return true;
    float errB = q.nearZ * q.texelPerMetre * q.errK;
    return e.selfError <= errB && e.parentError > errB;
}

LocalViewQuery localQueryFromView(LocalShadowView v, uint slot, uint includeAT, float errK)
{
    LocalViewQuery q;
    q.planes = v.planes;
    q.lightPos = v.lightPos.xyz;
    q.range = v.lightPos.w;
    q.nearZ = v.zparams.x;
    q.texelPerMetre = v.zparams.z;
    q.errK = errK;
    q.slot = slot;
    q.includeAT = includeAT;
    q.pad = 0u;
    return q;
}

#endif
