#pragma once

#include "xrCore/xrCore.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "entt/entt.hpp"
#include "IDebugDrawContext.h"
#include "ExtendedBoneMetadata.h"
#include "AnimationECS_Components.h"
#include "AnimationECS_IK.h"
#include <vector>

namespace AnimationECS {

//=============================================================================
// SKELETON DEBUG RENDERING SYSTEM
//=============================================================================

/// <summary>
/// Renders skeleton bones as debug primitives using ECS component data.
///
/// This system iterates over all entities with skeleton debug components
/// and issues draw calls to a provided IDebugDrawContext implementation.
/// </summary>
///
/// <remarks>
/// Required Components per Entity:
/// - SkeletonDebugState: Visualization settings (colors, line width, etc.)
/// - AnimationBuffers: Current pose matrices (models vector)
/// - InstanceTransform: World transform for this entity
/// - SkeletonMetadata: Bone hierarchy (parent indices) and extended metadata
///
/// Rendering Approach:
/// 1. Iterate entities with all required components
/// 2. Extract pose, hierarchy, and instance transform
/// 3. For each bone in skeleton:
///    - Draw line from parent to child (if parent exists)
///    - Draw sphere at joint position
///    - Draw bone shape (octahedron/capsule)
///    - Draw axes at root bone
///
/// Design Notes:
/// - Decoupled from rendering backend via IDebugDrawContext
/// - All rendering logic in one place (not split across application)
/// - Supports multi-instance rendering (grid layout)
/// - Uses extended metadata for bone lengths, physics shapes, etc.
/// </remarks>
class SkeletonDebugRenderSystem
{
public:
    /// <summary>
    /// Render skeleton debug visualization for all entities.
    /// </summary>
    /// <param name="registry">ECS registry containing entities</param>
    /// <param name="ctx">Debug draw context (application-provided renderer)</param>
    /// <remarks>
    /// Call this once per frame after animation update but before scene rendering.
    /// The ctx implementation is responsible for batching/queuing commands.
    /// </remarks>
    static void Render(entt::registry& registry, IDebugDrawContext& ctx);

    /// <summary>
    /// Update skeleton debug state from global settings.
    /// </summary>
    /// <param name="registry">ECS registry</param>
    /// <param name="show_skeleton_lines">Global setting for skeleton lines</param>
    static void UpdateGlobalSettings(entt::registry& registry, bool show_skeleton_lines);

private:
    /// <summary>
    /// Render a single skeleton instance.
    /// </summary>
    /// <param name="ctx">Debug draw context</param>
    /// <param name="pose_models">Model-space transforms for each joint (current pose)</param>
    /// <param name="skeleton_metadata">Skeleton hierarchy, children map, and extended metadata</param>
    /// <param name="instance_transform">World transform for this instance</param>
    /// <param name="debug_state">Visualization settings</param>
    static void RenderSkeleton(
        IDebugDrawContext& ctx,
        const std::vector<ozz::math::Float4x4>& pose_models,
        const SkeletonMetadata& skeleton_metadata,
        const ozz::math::Float4x4& instance_transform,
        const SkeletonDebugState& debug_state);

    /// <summary>
    /// Compute rest length for a bone (distance to parent or child).
    /// Used as fallback when metadata doesn't provide rest_length.
    /// </summary>
    /// <param name="bone_index">Index of bone to compute length for</param>
    /// <param name="rest_models">Rest pose model-space transforms</param>
    /// <param name="skeleton_parents">Parent indices</param>
    /// <returns>Bone rest length in world units</returns>
    static float ComputeRestLength(
        int bone_index,
        const std::vector<ozz::math::Float4x4>& rest_models,
        const std::vector<int>& skeleton_parents);

    /// <summary>
    /// Extract translation component from transform matrix.
    /// </summary>
    static ozz::math::Float3 ExtractTranslation(const ozz::math::Float4x4& mat);

    /// <summary>
    /// Compute distance between two transforms.
    /// </summary>
    static float DistanceBetween(const ozz::math::Float4x4& a, const ozz::math::Float4x4& b);
};

//=============================================================================
// IK GIZMO RENDERING SYSTEM
//=============================================================================

/// <summary>
/// Renders IK target gizmos as debug primitives using ECS component data.
///
/// This system iterates over all entities with IK gizmo components
/// and draws interactive spheres at IK target positions.
/// </summary>
///
/// <remarks>
/// Required Components per Entity:
/// - IKGizmoState: Gizmo positions, hover/drag states, visual settings
/// - IKConfiguration: IK chain configuration (left/right leg/arm)
/// - InstanceTransform: World transform for this entity
///
/// Gizmo States:
/// - Normal: Default color (green for legs, red for arms)
/// - Hovered: Cyan (mouse over gizmo)
/// - Dragging: Yellow (actively dragging)
/// - Disabled: Gray/faded (IK chain disabled)
///
/// Design Notes:
/// - Renders all 4 limb gizmos per entity (left/right leg/arm)
/// - Transforms gizmo positions from model space to world space
/// - Color indicates both type (leg/arm) and state (hover/drag)
/// - Only renders gizmos for valid IK chains
/// </remarks>
class IKGizmoRenderSystem
{
public:
    /// <summary>
    /// Render IK gizmos for all entities.
    /// </summary>
    /// <param name="registry">ECS registry containing entities</param>
    /// <param name="ctx">Debug draw context (application-provided renderer)</param>
    /// <remarks>
    /// Call this once per frame before HandleMouseInteraction() to ensure
    /// gizmos are visible before processing interactions.
    /// </remarks>
    static void Render(entt::registry& registry, IDebugDrawContext& ctx);

private:
    /// <summary>
    /// Render a single gizmo for an IK chain.
    /// </summary>
    /// <param name="ctx">Debug draw context</param>
    /// <param name="gizmo">Gizmo visual state (position, radius, hover/drag)</param>
    /// <param name="chain">IK chain data (validity, enabled state)</param>
    /// <param name="instance_transform">World transform for this entity</param>
    /// <param name="is_leg">True for leg chains, false for arm chains</param>
    static void RenderGizmo(
        IDebugDrawContext& ctx,
        const IKGizmoState::ChainGizmo& gizmo,
        const LimbIKChain& chain,
        const ozz::math::Float4x4& instance_transform,
        bool is_leg);

    /// <summary>
    /// Transform a point from model space to world space.
    /// </summary>
    static ozz::math::Float3 TransformPoint(
        const ozz::math::Float4x4& transform,
        const ozz::math::Float3& point);
};

} // namespace AnimationECS
