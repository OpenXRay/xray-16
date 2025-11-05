#pragma once

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"

namespace AnimationECS {

/// <summary>
/// Abstract interface for debug primitive drawing.
///
/// This interface decouples ECS rendering systems from specific rendering implementations,
/// allowing the same ECS code to work with different backends (Vulkan, OpenGL, DirectX, etc.).
/// </summary>
///
/// <remarks>
/// Implementations:
/// - VulkanRenderer (ozz_animation_viewer): Forwards to DebugRenderer
/// - EngineDebugDrawAdapter (xrGame): Forwards to engine's DBG_Draw* functions
/// - MockDebugDrawContext (unit tests): Records calls for verification
///
/// Usage Pattern:
///   // In application code
///   VulkanRenderer renderer;  // Implements IDebugDrawContext
///   AnimationECS::SkeletonDebugRenderSystem::Render(registry, renderer);
///
/// Design Notes:
/// - Uses ozz::math types for consistency with animation data
/// - All methods are pure virtual (no implementation in interface)
/// - Implementations are responsible for:
///   * Type conversion (ozz::math::Float3 → backend-specific types)
///   * Queuing/batching draw commands
///   * Actual rendering at end-of-frame
/// </remarks>
class IDebugDrawContext
{
public:
    virtual ~IDebugDrawContext() = default;

    /// <summary>
    /// Draw a line between two points.
    /// </summary>
    /// <param name="start">Line start position (world space)</param>
    /// <param name="end">Line end position (world space)</param>
    /// <param name="color">RGBA color (components in range [0, 1])</param>
    /// <remarks>
    /// Used for:
    /// - Skeleton bone connections (parent → child)
    /// - Debug axes
    /// - Bounding shapes (boxes, capsules)
    /// </remarks>
    virtual void DrawLine(const ozz::math::Float3& start,
                         const ozz::math::Float3& end,
                         const ozz::math::Float4& color) = 0;

    /// <summary>
    /// Draw a sphere at a position.
    /// </summary>
    /// <param name="center">Sphere center position (world space)</param>
    /// <param name="radius">Sphere radius (world units)</param>
    /// <param name="color">RGBA color (components in range [0, 1])</param>
    /// <param name="segments">Number of subdivisions (higher = smoother sphere)</param>
    /// <remarks>
    /// Used for:
    /// - Skeleton joint visualization
    /// - IK target gizmos
    /// - Collision shape visualization
    ///
    /// Implementations may:
    /// - Use latitude/longitude lines for wireframe sphere
    /// - Use solid sphere with shading
    /// - Ignore segments parameter if using fixed LOD
    /// </remarks>
    virtual void DrawSphere(const ozz::math::Float3& center,
                           float radius,
                           const ozz::math::Float4& color,
                           int segments = 12) = 0;

    /// <summary>
    /// Draw a bone shape between two points.
    /// </summary>
    /// <param name="head">Bone start position (joint/parent)</param>
    /// <param name="tail">Bone end position (child)</param>
    /// <param name="radius">Bone thickness</param>
    /// <param name="color">RGBA color (components in range [0, 1])</param>
    /// <remarks>
    /// Used for skeleton bone visualization. Typically rendered as:
    /// - Octahedron (diamond shape) - preferred for clarity
    /// - Capsule (cylinder with rounded ends)
    /// - Simple line + spheres at joints (minimal implementation)
    ///
    /// The shape should taper from head to tail to show bone direction.
    /// </remarks>
    virtual void DrawBoneShape(const ozz::math::Float3& head,
                              const ozz::math::Float3& tail,
                              float radius,
                              const ozz::math::Float4& color) = 0;

    /// <summary>
    /// Draw coordinate axes at a transform.
    /// </summary>
    /// <param name="transform">Transform matrix (world space)</param>
    /// <param name="scale">Length of each axis line (world units)</param>
    /// <param name="color_x">X-axis color (typically red)</param>
    /// <param name="color_y">Y-axis color (typically green)</param>
    /// <param name="color_z">Z-axis color (typically blue)</param>
    /// <remarks>
    /// Used for:
    /// - Root bone orientation visualization
    /// - Joint local axes
    /// - Transform gizmos
    ///
    /// Standard color conventions:
    /// - X-axis: Red   (1, 0, 0, 1)
    /// - Y-axis: Green (0, 1, 0, 1)
    /// - Z-axis: Blue  (0, 0, 1, 1)
    /// </remarks>
    virtual void DrawAxes(const ozz::math::Float4x4& transform,
                         float scale,
                         const ozz::math::Float4& color_x,
                         const ozz::math::Float4& color_y,
                         const ozz::math::Float4& color_z) = 0;

    // ========================================================================
    // BATCHED/INSTANCED RENDERING API
    // ========================================================================

    /// <summary>
    /// Instance data for bone shape rendering.
    /// </summary>
    struct BoneInstance {
        ozz::math::Float3 head;     // Bone start position (joint/parent)
        ozz::math::Float3 tail;     // Bone end position (child)
        float radius;                // Bone thickness
        ozz::math::Float4 color;    // RGBA color
    };

    /// <summary>
    /// Instance data for sphere rendering.
    /// </summary>
    struct SphereInstance {
        ozz::math::Float3 center;   // Sphere center position
        float radius;                // Sphere radius
        ozz::math::Float4 color;    // RGBA color
    };

    /// <summary>
    /// Draw multiple bone shapes in a single batched call.
    /// </summary>
    /// <param name="instances">Array of bone instances to render</param>
    /// <param name="count">Number of instances in the array</param>
    /// <remarks>
    /// Performance optimization: Reduces per-bone CPU overhead and consolidates
    /// geometry generation and draw calls. Implementations should batch all bones
    /// into a single vertex buffer and issue minimal draw calls.
    /// </remarks>
    virtual void DrawBoneShapesInstanced(const BoneInstance* instances, size_t count) = 0;

    /// <summary>
    /// Draw multiple spheres in a single batched call.
    /// </summary>
    /// <param name="instances">Array of sphere instances to render</param>
    /// <param name="count">Number of instances in the array</param>
    /// <remarks>
    /// Performance optimization: Reduces per-sphere CPU overhead and consolidates
    /// geometry generation and draw calls. Implementations should batch all spheres
    /// into a single vertex buffer and issue minimal draw calls.
    /// </remarks>
    virtual void DrawSpheresInstanced(const SphereInstance* instances, size_t count) = 0;
};

} // namespace AnimationECS
