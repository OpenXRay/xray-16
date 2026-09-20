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

bool FrustumTestSphere6(float3 center, float radius, float4 planes[6])
{
    for (uint i = 0; i < 6; ++i)
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

    float rx = radius * length(pyramidViewProj[0].xyz);
    float ry = radius * length(pyramidViewProj[1].xyz);
    float rz = radius * length(pyramidViewProj[2].xyz);
    float rw = radius * length(pyramidViewProj[3].xyz);

    float wMin = clipC.w - rw;
    float wMax = clipC.w + rw;
    if (wMin <= 0.001)
        return result;

    float xMin = clipC.x - rx;
    float xMax = clipC.x + rx;
    float yMin = clipC.y - ry;
    float yMax = clipC.y + ry;
    float zMin = clipC.z - rz;
    float zMax = clipC.z + rz;

    float2 ndcX = float2(min(min(xMin / wMin, xMin / wMax), min(xMax / wMin, xMax / wMax)),
                         max(max(xMin / wMin, xMin / wMax), max(xMax / wMin, xMax / wMax)));
    float2 ndcY = float2(min(min(yMin / wMin, yMin / wMax), min(yMax / wMin, yMax / wMax)),
                         max(max(yMin / wMin, yMin / wMax), max(yMax / wMin, yMax / wMax)));

    if (ndcX.x < -1.0 || ndcX.y > 1.0 || ndcY.x < -1.0 || ndcY.y > 1.0)
        return result;

    result.frontDepth = max(max(zMax / wMin, zMax / wMax), max(zMin / wMin, zMin / wMax));

    float2 uvMin = float2(ndcX.x * 0.5 + 0.5, 0.5 - ndcY.y * 0.5);
    float2 uvMax = float2(ndcX.y * 0.5 + 0.5, 0.5 - ndcY.x * 0.5);

    float2 pMin = uvMin * float2(hiZWidth, hiZHeight) - 1.0;
    float2 pMax = uvMax * float2(hiZWidth, hiZHeight) + 1.0;
    pMin = max(pMin, float2(0.0, 0.0));
    pMax = min(pMax, float2(float(hiZWidth) - 1.0, float(hiZHeight) - 1.0));
    float spanTexels = max(pMax.x - pMin.x, pMax.y - pMin.y) + 1.0;
    uint mi = (uint)ceil(log2(max(1.0, spanTexels)));
    if (mi > hiZMipLevels - 1u)
        return result;

    int2 tmax = int2(max(1u, hiZWidth >> mi), max(1u, hiZHeight >> mi)) - 1;
    int2 t0 = clamp(int2(floor(pMin)) >> (int)mi, int2(0, 0), tmax);
    int2 t1 = clamp(int2(floor(pMax)) >> (int)mi, int2(0, 0), tmax);

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
