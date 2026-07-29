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

    float4 clipPos = mul(pyramidViewProj, float4(center, 1.0));
    if (clipPos.w <= 0.001)
        return result;

    float3 ndc = clipPos.xyz / clipPos.w;

    float projScale = max(abs(pyramidViewProj[0][0]), abs(pyramidViewProj[1][1]));
    float2 ndcSize = float2(radius, radius) * projScale / clipPos.w;

    float2 minNDC = ndc.xy - ndcSize;
    float2 maxNDC = ndc.xy + ndcSize;

    if (any(minNDC < -1.0) || any(maxNDC > 1.0))
        return result;

    float2 minUV = minNDC * 0.5 + 0.5;
    float2 maxUV = maxNDC * 0.5 + 0.5;

    minUV.y = 1.0 - minUV.y;
    maxUV.y = 1.0 - maxUV.y;

    float4 boxUV = float4(min(minUV, maxUV), max(minUV, maxUV));

    float boxWidth = (boxUV.z - boxUV.x) * float(hiZWidth);
    float boxHeight = (boxUV.w - boxUV.y) * float(hiZHeight);

    float mipLevel = ceil(log2(max(1.0, max(boxWidth, boxHeight))));
    mipLevel = clamp(mipLevel, 0.0, float(hiZMipLevels - 1));

    float d1 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.x, boxUV.y), mipLevel);
    float d2 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.z, boxUV.y), mipLevel);
    float d3 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.x, boxUV.w), mipLevel);
    float d4 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.z, boxUV.w), mipLevel);

    result.hiZDepth = min(min(d1, d2), min(d3, d4));

    float3 viewDir = normalize(center - cameraPos);
    float3 frontPoint = center - viewDir * radius;
    float4 frontClip = mul(pyramidViewProj, float4(frontPoint, 1.0));
    if (frontClip.w <= 0.001)
        return result;

    result.frontDepth = frontClip.z / frontClip.w;
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
