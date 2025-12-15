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

bool HiZTestSphereTemporal(
    float3 center,
    float radius,
    float3 cameraPos,
    float4x4 currentViewProj,
    float4x4 prevViewProj,
    Texture2D<float> hiZPyramid,
    SamplerState pointSampler,
    uint hiZWidth,
    uint hiZHeight,
    uint hiZMipLevels)
{
    float4 prevClipPos = mul(prevViewProj, float4(center, 1.0));

    if (prevClipPos.w <= 0.001)
        return true;

    float3 prevNdc = prevClipPos.xyz / prevClipPos.w;

    float projScale = max(abs(prevViewProj[0][0]), abs(prevViewProj[1][1]));
    float2 ndcSize = float2(radius, radius) * projScale / prevClipPos.w;

    float2 minNDC = prevNdc.xy - ndcSize;
    float2 maxNDC = prevNdc.xy + ndcSize;

    if (any(minNDC > 1.0) || any(maxNDC < -1.0))
        return true;

    float2 minUV = saturate(minNDC * 0.5 + 0.5);
    float2 maxUV = saturate(maxNDC * 0.5 + 0.5);

    minUV.y = 1.0 - minUV.y;
    maxUV.y = 1.0 - maxUV.y;

    float4 boxUV = float4(min(minUV, maxUV), max(minUV, maxUV));

    float boxWidth = (boxUV.z - boxUV.x) * float(hiZWidth);
    float boxHeight = (boxUV.w - boxUV.y) * float(hiZHeight);

    float mipLevel = floor(log2(max(1.0, max(boxWidth, boxHeight) * 0.5)));
    mipLevel = clamp(mipLevel, 0.0, float(hiZMipLevels - 1));

    float d1 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.x, boxUV.y), mipLevel);
    float d2 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.z, boxUV.y), mipLevel);
    float d3 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.x, boxUV.w), mipLevel);
    float d4 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.z, boxUV.w), mipLevel);

    float hiZDepth = max(max(d1, d2), max(d3, d4));

    float3 viewDir = normalize(center - cameraPos);
    float3 frontPoint = center - viewDir * radius;
    float4 frontClip = mul(prevViewProj, float4(frontPoint, 1.0));

    if (frontClip.w <= 0.001)
        return true;

    float frontDepth = frontClip.z / frontClip.w;

    return frontDepth <= hiZDepth;
}

bool HiZTestSphere(
    float3 center,
    float radius,
    float3 cameraPos,
    float4x4 viewProj,
    Texture2D<float> hiZPyramid,
    SamplerState pointSampler,
    uint hiZWidth,
    uint hiZHeight,
    uint hiZMipLevels)
{
    return HiZTestSphereTemporal(center, radius, cameraPos, viewProj, viewProj,
                                  hiZPyramid, pointSampler, hiZWidth, hiZHeight, hiZMipLevels);
}

#endif
