#ifndef CULL_UTILS_H
#define CULL_UTILS_H

bool FrustumTestSphere(float3 center, float radius, float4 planes[6])
{
    for (uint i = 0; i < 5; ++i)
    {
        float dist = dot(planes[i].xyz, center) + planes[i].w;
        if (dist > radius)
            return false;
    }
    return true;
}

bool FrustumTestAABB(float3 aabb_min, float3 aabb_max, float4 planes[6])
{
    for (uint i = 0; i < 5; ++i)
    {
        float3 plane_normal = planes[i].xyz;
        float plane_dist = planes[i].w;

        float3 negative_vertex = float3(
            plane_normal.x < 0.0 ? aabb_max.x : aabb_min.x,
            plane_normal.y < 0.0 ? aabb_max.y : aabb_min.y,
            plane_normal.z < 0.0 ? aabb_max.z : aabb_min.z
        );

        float dist = dot(plane_normal, negative_vertex) + plane_dist;
        if (dist > 0.0)
            return false;
    }
    return true;
}

bool DistanceTestSphere(float3 center, float radius, float3 camera_pos, float max_dist_sq)
{
    float3 delta = center - camera_pos;
    float dist_sq = dot(delta, delta);

    float effective_dist_sq = dist_sq - (radius * radius);
    effective_dist_sq = max(0.0, effective_dist_sq);

    return effective_dist_sq <= max_dist_sq;
}

bool DistanceTestAABB(float3 aabb_min, float3 aabb_max, float3 camera_pos, float max_dist_sq)
{
    float3 closest = clamp(camera_pos, aabb_min, aabb_max);
    float3 delta = closest - camera_pos;
    float dist_sq = dot(delta, delta);

    return dist_sq < max_dist_sq;
}

struct HiZTestResult
{
    bool visible;
    float frontDepth;
    float hiZDepth;
};

HiZTestResult HiZTestSphereEx(
    float3 center,
    float radius,
    float3 cameraPos,
    float4x4 pyramidViewProj,
    Texture2D<float> hiZPyramid,
    SamplerState pointSampler,
    uint hiZWidth,
    uint hiZHeight,
    uint hiZMipLevels)
{
    HiZTestResult result;
    result.visible = true;
    result.frontDepth = 0.0;
    result.hiZDepth = 0.0;

    float4 clipC = mul(pyramidViewProj, float4(center, 1.0));
    if (clipC.w <= 0.001)
        return result;

    float2 ndc = clipC.xy / clipC.w;
    float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0))
        return result;

    float3 viewForward = normalize(pyramidViewProj[3].xyz);
    float4 clipN = mul(pyramidViewProj, float4(center - viewForward * radius, 1.0));
    if (clipN.w <= 0.001)
        return result;
    result.frontDepth = clipN.z / clipN.w;

    float focal = length(pyramidViewProj[1].xyz);
    float screenTexels = radius * focal * float(hiZHeight) / clipC.w;
    uint mi = (uint)ceil(log2(max(1.0, screenTexels)));
    if (mi > hiZMipLevels - 1)
        return result;

    float2 pc0 = uv * float2(hiZWidth, hiZHeight);
    float halfT = 0.5 * screenTexels;
    int2 tmax = int2(max(1u, hiZWidth >> mi), max(1u, hiZHeight >> mi)) - 1;
    int2 t0 = clamp(int2(floor(pc0 - halfT)) >> (int)mi, int2(0, 0), tmax);
    int2 t1 = clamp(int2(floor(pc0 + halfT)) >> (int)mi, int2(0, 0), tmax);

    float d0 = hiZPyramid.Load(int3(t0.x, t0.y, mi));
    float d1 = hiZPyramid.Load(int3(t1.x, t0.y, mi));
    float d2 = hiZPyramid.Load(int3(t0.x, t1.y, mi));
    float d3 = hiZPyramid.Load(int3(t1.x, t1.y, mi));

    result.hiZDepth = min(min(d0, d1), min(d2, d3));
    result.visible = result.frontDepth >= result.hiZDepth;
    return result;
}

bool HiZTestSphere(
    float3 center,
    float radius,
    float3 cameraPos,
    float4x4 pyramidViewProj,
    Texture2D<float> hiZPyramid,
    SamplerState pointSampler,
    uint hiZWidth,
    uint hiZHeight,
    uint hiZMipLevels)
{
    return HiZTestSphereEx(center, radius, cameraPos, pyramidViewProj,
                           hiZPyramid, pointSampler, hiZWidth, hiZHeight, hiZMipLevels).visible;
}

#endif
