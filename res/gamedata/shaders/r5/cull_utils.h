// cull_utils.h - Shared GPU culling utilities
// Used by: detail_cull.cs, object_cull.cs
//
// X-Ray plane convention:
//   dot(normal, point) + d > 0  =>  OUTSIDE frustum (cull)
//   dot(normal, point) + d <= 0 =>  INSIDE frustum (visible)
//
// Plane order (from GPUCullingManager::ExtractFrustumPlanes):
//   0: Left, 1: Right, 2: Top, 3: Bottom, 4: Far, 5: Near
//
// We test planes 0-4 and SKIP plane 5 (near) to avoid culling
// objects that intersect the near plane.

#ifndef CULL_UTILS_H
#define CULL_UTILS_H

// ═══════════════════════════════════════════════════════
//  FRUSTUM CULLING
// ═══════════════════════════════════════════════════════

// Test sphere against frustum planes (skip near plane)
// Returns: true = visible, false = culled
bool FrustumTestSphere(float3 center, float radius, float4 planes[6])
{
    // Test against 5 planes, skip near plane (index 5)
    for (uint i = 0; i < 5; ++i)
    {
        float dist = dot(planes[i].xyz, center) + planes[i].w;
        if (dist > radius)
            return false;
    }
    return true;
}

// Test AABB against frustum planes (skip near plane)
// Returns: true = visible, false = culled
bool FrustumTestAABB(float3 aabb_min, float3 aabb_max, float4 planes[6])
{
    // Test against 5 planes, skip near plane (index 5)
    for (uint i = 0; i < 5; ++i)
    {
        float3 plane_normal = planes[i].xyz;
        float plane_dist = planes[i].w;

        // Find the negative vertex (corner most likely to be outside)
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

// ═══════════════════════════════════════════════════════
//  DISTANCE CULLING
// ═══════════════════════════════════════════════════════

// Test if sphere is within max distance from camera
// Returns: true = within range, false = too far (cull)
bool DistanceTestSphere(float3 center, float radius, float3 camera_pos, float max_dist_sq)
{
    float3 delta = center - camera_pos;
    float dist_sq = dot(delta, delta);

    // Account for object radius (closest point could be closer)
    float effective_dist_sq = dist_sq - (radius * radius);
    effective_dist_sq = max(0.0, effective_dist_sq);

    return effective_dist_sq <= max_dist_sq;
}

// Test if AABB is within max distance from camera
// Returns: true = within range, false = too far (cull)
bool DistanceTestAABB(float3 aabb_min, float3 aabb_max, float3 camera_pos, float max_dist_sq)
{
    // Find closest point on AABB to camera
    float3 closest = clamp(camera_pos, aabb_min, aabb_max);
    float3 delta = closest - camera_pos;
    float dist_sq = dot(delta, delta);

    return dist_sq < max_dist_sq;
}

// ═══════════════════════════════════════════════════════
//  HI-Z OCCLUSION CULLING
// ═══════════════════════════════════════════════════════
// Requires caller to define:
//   Texture2D<float> g_HiZPyramid : register(t1);
//   SamplerState g_PointSampler : register(s0);  (or g_PointClampSampler)

// Hi-Z occlusion test for sphere (4-tap conservative)
// Returns: true = visible, false = occluded (cull)
//
// TEMPORAL HI-Z: When using previous frame's depth for Hi-Z:
// - Use prevViewProj for Hi-Z UV calculation (matches depth buffer camera)
// - Use currentViewProj for depth comparison (current camera position)
// - This handles camera movement without false culling
bool HiZTestSphereTemporal(
    float3 center,
    float radius,
    float3 cameraPos,
    float4x4 currentViewProj,  // Current frame - for frustum/depth compare
    float4x4 prevViewProj,     // Previous frame - for Hi-Z UV lookup
    Texture2D<float> hiZPyramid,
    SamplerState pointSampler,
    uint hiZWidth,
    uint hiZHeight,
    uint hiZMipLevels)
{
    // Project sphere center to PREVIOUS frame's clip space (matches Hi-Z data)
    float4 prevClipPos = mul(prevViewProj, float4(center, 1.0));

    // Behind previous camera - conservatively visible
    if (prevClipPos.w <= 0.001)
        return true;

    // Perspective divide -> NDC (previous frame)
    float3 prevNdc = prevClipPos.xyz / prevClipPos.w;

    // Calculate screen-space bounding box (in previous frame's space)
    float projScale = max(abs(prevViewProj[0][0]), abs(prevViewProj[1][1]));
    float2 ndcSize = float2(radius, radius) * projScale / prevClipPos.w;

    float2 minNDC = prevNdc.xy - ndcSize;
    float2 maxNDC = prevNdc.xy + ndcSize;

    // Conservatively visible if off-screen in previous frame
    if (any(minNDC > 1.0) || any(maxNDC < -1.0))
        return true;

    // Convert NDC to UV space [0, 1]
    float2 minUV = saturate(minNDC * 0.5 + 0.5);
    float2 maxUV = saturate(maxNDC * 0.5 + 0.5);

    // Flip Y (NDC Y+ is up, UV Y+ is down)
    minUV.y = 1.0 - minUV.y;
    maxUV.y = 1.0 - maxUV.y;

    // Re-sort after Y flip
    float4 boxUV = float4(min(minUV, maxUV), max(minUV, maxUV));

    // Calculate box size in pixels
    float boxWidth = (boxUV.z - boxUV.x) * float(hiZWidth);
    float boxHeight = (boxUV.w - boxUV.y) * float(hiZHeight);

    // Select mip where box is roughly 2x2 pixels
    float mipLevel = floor(log2(max(1.0, max(boxWidth, boxHeight) * 0.5)));
    mipLevel = clamp(mipLevel, 0.0, float(hiZMipLevels - 1));

    // Sample Hi-Z at 4 corners (previous frame's depth)
    float d1 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.x, boxUV.y), mipLevel);
    float d2 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.z, boxUV.y), mipLevel);
    float d3 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.x, boxUV.w), mipLevel);
    float d4 = hiZPyramid.SampleLevel(pointSampler, float2(boxUV.z, boxUV.w), mipLevel);

    // MAX = farthest depth in region (0=near, 1=far)
    float hiZDepth = max(max(d1, d2), max(d3, d4));

    // Calculate front depth of sphere in PREVIOUS frame's space
    // (must match the Hi-Z data we're comparing against)
    float3 viewDir = normalize(center - cameraPos);
    float3 frontPoint = center - viewDir * radius;
    float4 frontClip = mul(prevViewProj, float4(frontPoint, 1.0));

    // Front point behind previous camera - straddles near plane, visible
    if (frontClip.w <= 0.001)
        return true;

    float frontDepth = frontClip.z / frontClip.w;

    // Visible if front of sphere is in front of Hi-Z depth
    return frontDepth <= hiZDepth;
}

// Legacy single-viewProj version (for non-temporal Hi-Z)
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
    // Use same viewProj for both sampling and depth comparison
    return HiZTestSphereTemporal(center, radius, cameraPos, viewProj, viewProj,
                                  hiZPyramid, pointSampler, hiZWidth, hiZHeight, hiZMipLevels);
}

#endif // CULL_UTILS_H
