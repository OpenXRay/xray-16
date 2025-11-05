#pragma once

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"
#include "../AnimationECS_IK.h"
#include <vector>

namespace IKGizmo {

// IK Gizmo representing a draggable sphere for IK targets
struct Gizmo {
    enum class ChainType { LeftLeg, RightLeg, LeftArm, RightArm };

    ChainType type;
    ozz::math::Float3 position;
    float radius = 0.08f;  // Visual radius in world units
    bool is_hovered = false;
    bool is_dragging = false;

    AnimationECS::LimbIKChain* GetChain(AnimationECS::IKConfiguration& config) {
        switch (type) {
            case ChainType::LeftLeg: return &config.left_leg;
            case ChainType::RightLeg: return &config.right_leg;
            case ChainType::LeftArm: return &config.left_arm;
            case ChainType::RightArm: return &config.right_arm;
        }
        return nullptr;
    }

    const char* GetName() const {
        switch (type) {
            case ChainType::LeftLeg: return "Left Leg";
            case ChainType::RightLeg: return "Right Leg";
            case ChainType::LeftArm: return "Left Arm";
            case ChainType::RightArm: return "Right Arm";
        }
        return "Unknown";
    }
};

// Helper: Ray-sphere intersection test
// Returns true if ray intersects sphere, and outputs the distance along the ray
inline bool RaySphereIntersection(
    const ozz::math::Float3& ray_origin,
    const ozz::math::Float3& ray_direction,  // Must be normalized
    const ozz::math::Float3& sphere_center,
    float sphere_radius,
    float* out_distance = nullptr)
{
    // Vector from ray origin to sphere center
    ozz::math::Float3 oc;
    oc.x = ray_origin.x - sphere_center.x;
    oc.y = ray_origin.y - sphere_center.y;
    oc.z = ray_origin.z - sphere_center.z;

    // Quadratic equation: a*t^2 + b*t + c = 0
    const float a = ray_direction.x * ray_direction.x +
                   ray_direction.y * ray_direction.y +
                   ray_direction.z * ray_direction.z;
    const float b = 2.0f * (oc.x * ray_direction.x +
                           oc.y * ray_direction.y +
                           oc.z * ray_direction.z);
    const float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - sphere_radius * sphere_radius;

    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;  // No intersection
    }

    // Calculate nearest intersection point
    const float sqrt_disc = std::sqrt(discriminant);
    float t = (-b - sqrt_disc) / (2.0f * a);
    if (t < 0.0f) {
        t = (-b + sqrt_disc) / (2.0f * a);
    }

    if (t < 0.0f) {
        return false;  // Behind camera
    }

    if (out_distance) {
        *out_distance = t;
    }
    return true;
}

// Helper: Create a ray from screen coordinates
// Returns ray origin and direction in world space
inline void ScreenToWorldRay(
    float screen_x, float screen_y,
    int viewport_width, int viewport_height,
    const ozz::math::Float4x4& view_matrix,
    const ozz::math::Float4x4& proj_matrix,
    ozz::math::Float3* out_ray_origin,
    ozz::math::Float3* out_ray_direction)
{
    // Convert screen coordinates to NDC (-1 to 1)
    // GLFW screen coordinates: (0,0) at top-left, Y increases downward
    // NDC: X[-1,+1] left to right, Y[-1,+1] bottom to top
    // NOTE: The projection matrix has a Y-flip (negative Y scaling) for Vulkan,
    // so we need to invert our NDC Y calculation to compensate
    const float ndc_x = (2.0f * screen_x) / viewport_width - 1.0f;
    const float ndc_y = (2.0f * screen_y) / viewport_height - 1.0f;  // Don't flip - projection matrix already has Y-flip

    // Compute inverse view-projection matrix
    // Note: For unprojection, we need to invert (Proj * View)
    const ozz::math::Float4x4 view_proj = proj_matrix * view_matrix;
    const ozz::math::Float4x4 inv_view_proj = ozz::math::Invert(view_proj);

    // Ray start (near plane) and end (far plane) in NDC
    // For Vulkan: Z range is [0, 1] where 0 = near plane, 1 = far plane
    ozz::math::SimdFloat4 near_ndc = ozz::math::simd_float4::Load(ndc_x, ndc_y, 0.0f, 1.0f);
    ozz::math::SimdFloat4 far_ndc = ozz::math::simd_float4::Load(ndc_x, ndc_y, 1.0f, 1.0f);

    // Transform to world space using matrix-vector multiplication
    // ozz uses column-major matrices: result = M * v
    // We do manual multiplication to get homogeneous coordinates (before perspective divide)
    ozz::math::SimdFloat4 near_world_h = inv_view_proj * near_ndc;
    ozz::math::SimdFloat4 far_world_h = inv_view_proj * far_ndc;

    // Perspective divide to get actual world coordinates
    const float near_w = ozz::math::GetW(near_world_h);
    const float far_w = ozz::math::GetW(far_world_h);

    // Avoid division by zero
    const float safe_near_w = (std::abs(near_w) < 1e-6f) ? 1.0f : near_w;
    const float safe_far_w = (std::abs(far_w) < 1e-6f) ? 1.0f : far_w;

    ozz::math::Float3 near_world{
        ozz::math::GetX(near_world_h) / safe_near_w,
        ozz::math::GetY(near_world_h) / safe_near_w,
        ozz::math::GetZ(near_world_h) / safe_near_w
    };

    ozz::math::Float3 far_world{
        ozz::math::GetX(far_world_h) / safe_far_w,
        ozz::math::GetY(far_world_h) / safe_far_w,
        ozz::math::GetZ(far_world_h) / safe_far_w
    };

    // Ray origin and direction
    *out_ray_origin = near_world;
    out_ray_direction->x = far_world.x - near_world.x;
    out_ray_direction->y = far_world.y - near_world.y;
    out_ray_direction->z = far_world.z - near_world.z;

    // Normalize direction
    const float len = std::sqrt(out_ray_direction->x * out_ray_direction->x +
                               out_ray_direction->y * out_ray_direction->y +
                               out_ray_direction->z * out_ray_direction->z);

    // Avoid division by zero
    if (len > 1e-6f) {
        out_ray_direction->x /= len;
        out_ray_direction->y /= len;
        out_ray_direction->z /= len;
    } else {
        // Fallback: ray pointing forward
        out_ray_direction->x = 0.0f;
        out_ray_direction->y = 0.0f;
        out_ray_direction->z = -1.0f;
    }
}

// Helper: Find closest gizmo to ray
inline int FindClosestGizmo(
    const std::vector<Gizmo>& gizmos,
    const ozz::math::Float3& ray_origin,
    const ozz::math::Float3& ray_direction)
{
    int closest_index = -1;
    float closest_distance = std::numeric_limits<float>::max();

    for (size_t i = 0; i < gizmos.size(); ++i) {
        float distance;
        if (RaySphereIntersection(ray_origin, ray_direction, gizmos[i].position, gizmos[i].radius, &distance)) {
            if (distance < closest_distance) {
                closest_distance = distance;
                closest_index = static_cast<int>(i);
            }
        }
    }

    return closest_index;
}

// Helper: Find closest point on ray to a given point (for smooth dragging)
inline ozz::math::Float3 ClosestPointOnRay(
    const ozz::math::Float3& ray_origin,
    const ozz::math::Float3& ray_direction,
    const ozz::math::Float3& point)
{
    // Project point onto ray: closest_point = origin + t * direction
    // where t = dot(point - origin, direction) / dot(direction, direction)

    ozz::math::Float3 to_point;
    to_point.x = point.x - ray_origin.x;
    to_point.y = point.y - ray_origin.y;
    to_point.z = point.z - ray_origin.z;

    const float dot_prod = to_point.x * ray_direction.x +
                          to_point.y * ray_direction.y +
                          to_point.z * ray_direction.z;

    const float dir_len_sq = ray_direction.x * ray_direction.x +
                            ray_direction.y * ray_direction.y +
                            ray_direction.z * ray_direction.z;

    const float t = std::max(0.0f, dot_prod / dir_len_sq);  // Clamp to positive

    ozz::math::Float3 closest;
    closest.x = ray_origin.x + t * ray_direction.x;
    closest.y = ray_origin.y + t * ray_direction.y;
    closest.z = ray_origin.z + t * ray_direction.z;

    return closest;
}

// Helper: Project point onto plane and compute intersection
inline bool RayPlaneIntersection(
    const ozz::math::Float3& ray_origin,
    const ozz::math::Float3& ray_direction,
    const ozz::math::Float3& plane_normal,
    float plane_distance,
    ozz::math::Float3* out_intersection)
{
    // Plane equation: dot(N, P) + D = 0
    // Ray equation: P = O + t * D
    // Solve for t: dot(N, O + t * D) + D = 0

    const float denom = plane_normal.x * ray_direction.x +
                       plane_normal.y * ray_direction.y +
                       plane_normal.z * ray_direction.z;

    if (std::abs(denom) < 1e-6f) {
        return false;  // Ray is parallel to plane
    }

    const float numer = -(plane_normal.x * ray_origin.x +
                         plane_normal.y * ray_origin.y +
                         plane_normal.z * ray_origin.z + plane_distance);

    const float t = numer / denom;
    if (t < 0.0f) {
        return false;  // Intersection behind ray origin
    }

    out_intersection->x = ray_origin.x + t * ray_direction.x;
    out_intersection->y = ray_origin.y + t * ray_direction.y;
    out_intersection->z = ray_origin.z + t * ray_direction.z;

    return true;
}

} // namespace IKGizmo
