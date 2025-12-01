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

#endif // CULL_UTILS_H
