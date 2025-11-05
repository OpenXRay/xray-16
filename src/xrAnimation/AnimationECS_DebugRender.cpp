#include "stdafx.h"
#include "AnimationECS_DebugRender.h"
#include "AnimationECS_Components.h"
#include "AnimationECS_IK.h"
#include <algorithm>
#include <cmath>

namespace AnimationECS {

namespace {

// Constants
constexpr float kSkeletonDebugEpsilon = 1e-5f;
constexpr float kSkeletonDefaultRadius = 0.05f;

} // anonymous namespace

//=============================================================================
// SkeletonDebugRenderSystem Implementation
//=============================================================================

void SkeletonDebugRenderSystem::Render(entt::registry& registry, IDebugDrawContext& ctx)
{
    // Iterate entities with all required components
    auto view = registry.view<SkeletonDebugState, AnimationBuffers,
                              InstanceTransform, SkeletonMetadata>();

    for (auto entity : view)
    {
        const auto& debug_state = view.get<SkeletonDebugState>(entity);
        const auto& buffers = view.get<AnimationBuffers>(entity);
        const auto& inst_transform = view.get<InstanceTransform>(entity);
        const auto& metadata = view.get<SkeletonMetadata>(entity);

        // Skip if disabled or empty
        if (!debug_state.enabled || !debug_state.show_skeleton_lines)
            continue;

        if (!buffers.IsInitialized() || buffers.models.empty())
            continue;

        if (!metadata.IsValid())
            continue;

        // Convert ozz::vector to std::vector for rendering
        std::vector<ozz::math::Float4x4> pose_models(
            buffers.models.begin(), buffers.models.end());

        // Render this skeleton instance
        RenderSkeleton(ctx, pose_models, metadata,
                      inst_transform.world_transform, debug_state);
    }
}

void SkeletonDebugRenderSystem::RenderSkeleton(
    IDebugDrawContext& ctx,
    const std::vector<ozz::math::Float4x4>& pose_models,
    const SkeletonMetadata& skeleton_metadata,
    const ozz::math::Float4x4& instance_transform,
    const SkeletonDebugState& debug_state)
{
    if (pose_models.empty())
        return;

    // Define colors
    const ozz::math::Float4 bone_color{0.95f, 0.85f, 0.25f, 0.45f};  // Yellow-gold
    const ozz::math::Float4 root_color{0.55f, 0.75f, 1.0f, 0.55f};   // Light blue
    const ozz::math::Float4 joint_color{0.92f, 0.35f, 0.35f, 0.65f}; // Red-orange
    const ozz::math::Float4 link_color{0.65f, 0.65f, 0.95f, 0.55f};  // Purple-blue

    // Get references for easier access
    const auto& skeleton_parents = skeleton_metadata.joint_parents;
    const auto& joint_children = skeleton_metadata.joint_children;
    const auto& bone_metadata = skeleton_metadata.metadata;

    // Lambda to compute rest length as fallback
    auto compute_rest_length = [&](int bone_index) -> float
    {
        float result = 0.0f;
        if (bone_index < 0 || static_cast<size_t>(bone_index) >= pose_models.size())
            return result;

        // Try parent distance
        const int parent = (bone_index < static_cast<int>(skeleton_parents.size()))
            ? skeleton_parents[bone_index]
            : -1;

        if (parent >= 0 && static_cast<size_t>(parent) < pose_models.size())
        {
            result = DistanceBetween(pose_models[bone_index], pose_models[parent]);
        }

        if (result > kSkeletonDebugEpsilon)
            return result;

        // Try child distance using pre-computed children (O(1) instead of O(N))
        if (bone_index < static_cast<int>(joint_children.size()))
        {
            for (int child : joint_children[bone_index])
            {
                result = std::max(result, DistanceBetween(
                    pose_models[bone_index], pose_models[child]));
            }
        }

        return result;
    };

    // Collect all bone instances for batched rendering
    std::vector<IDebugDrawContext::BoneInstance> bone_instances;
    bone_instances.reserve(pose_models.size());

    std::vector<IDebugDrawContext::SphereInstance> sphere_instances;
    if (debug_state.show_joint_positions)
    {
        sphere_instances.reserve(pose_models.size() * 2);  // head + tail per bone
    }

    // First pass: collect all instances and draw lines
    for (size_t bone = 0; bone < pose_models.size(); ++bone)
    {
        const ozz::math::Float4x4& pose_transform = pose_models[bone];
        const ozz::math::Float4x4 world_transform = instance_transform * pose_transform;

        // Get metadata for this bone
        XRay::Animation::ExtendedBoneMetadata metadata_entry;
        if (bone < bone_metadata.size())
        {
            metadata_entry = bone_metadata[bone];
        }

        // Compute rest length if not in metadata
        if (metadata_entry.rest_length <= kSkeletonDebugEpsilon)
        {
            metadata_entry.rest_length = compute_rest_length(static_cast<int>(bone));
        }

        if (metadata_entry.rest_length <= kSkeletonDebugEpsilon)
        {
            metadata_entry.rest_length = kSkeletonDefaultRadius;
        }

        const ozz::math::Float3 bone_position = ExtractTranslation(world_transform);

        // Draw line to parent (if exists)
        const int parent_index = (bone < skeleton_parents.size())
            ? skeleton_parents[bone]
            : -1;

        if (parent_index >= 0 && static_cast<size_t>(parent_index) < pose_models.size())
        {
            const ozz::math::Float4x4 parent_world =
                instance_transform * pose_models[parent_index];
            const ozz::math::Float3 parent_position = ExtractTranslation(parent_world);

            ctx.DrawLine(parent_position, bone_position, link_color);
        }

        // Compute tail position for bone shape
        ozz::math::Float3 tail_position = bone_position;
        bool has_child = false;

        // Find first child using pre-computed children map (O(1) instead of O(N))
        if (bone < joint_children.size())
        {
            const auto& children = joint_children[bone];
            if (!children.empty() && static_cast<size_t>(children[0]) < pose_models.size())
            {
                const ozz::math::Float4x4 child_world =
                    instance_transform * pose_models[children[0]];
                tail_position = ExtractTranslation(child_world);
                has_child = true;
            }
        }

        // If no child, use X-axis direction
        if (!has_child)
        {
            ozz::math::Float3 axis;
            ozz::math::Store3PtrU(world_transform.cols[0], &axis.x);

            float axis_len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
            if (axis_len <= kSkeletonDebugEpsilon)
            {
                axis = ozz::math::Float3::x_axis();
                axis_len = 1.0f;
            }

            const float target_length = (metadata_entry.rest_length > kSkeletonDebugEpsilon)
                ? metadata_entry.rest_length
                : kSkeletonDefaultRadius;

            axis = ozz::math::Float3{
                axis.x / axis_len,
                axis.y / axis_len,
                axis.z / axis_len
            };

            tail_position = ozz::math::Float3{
                bone_position.x + axis.x * target_length,
                bone_position.y + axis.y * target_length,
                bone_position.z + axis.z * target_length
            };
        }

        // Compute bone length
        const float dx = tail_position.x - bone_position.x;
        const float dy = tail_position.y - bone_position.y;
        const float dz = tail_position.z - bone_position.z;
        float bone_length = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (bone_length <= kSkeletonDebugEpsilon)
        {
            bone_length = std::max(metadata_entry.rest_length, kSkeletonDefaultRadius);
        }

        // Compute joint radius
        const float joint_radius = std::clamp(
            bone_length * 0.18f,
            kSkeletonDefaultRadius * 0.4f,
            bone_length * 0.45f);

        // Collect bone instance for batched rendering
        IDebugDrawContext::BoneInstance bone_inst;
        bone_inst.head = bone_position;
        bone_inst.tail = tail_position;
        bone_inst.radius = joint_radius;
        bone_inst.color = (bone == 0) ? root_color : bone_color;
        bone_instances.push_back(bone_inst);

        // Collect joint sphere instances if enabled
        if (debug_state.show_joint_positions)
        {
            IDebugDrawContext::SphereInstance sphere_inst;
            sphere_inst.center = bone_position;
            sphere_inst.radius = joint_radius;
            sphere_inst.color = joint_color;
            sphere_instances.push_back(sphere_inst);

            sphere_inst.center = tail_position;
            sphere_inst.radius = joint_radius * 0.6f;
            sphere_instances.push_back(sphere_inst);
        }

        // Draw axes at root bone
        if (bone == 0 && debug_state.show_bone_axes)
        {
            const float axes_scale = metadata_entry.rest_length * 0.2f;
            ctx.DrawAxes(
                world_transform,
                axes_scale,
                ozz::math::Float4{1.0f, 0.3f, 0.3f, 1.0f},  // Red X
                ozz::math::Float4{0.3f, 1.0f, 0.3f, 1.0f},  // Green Y
                ozz::math::Float4{0.3f, 0.6f, 1.0f, 1.0f}); // Blue Z
        }
    }

    // Second pass: render all collected instances in batches
    if (!bone_instances.empty())
    {
        ctx.DrawBoneShapesInstanced(bone_instances.data(), bone_instances.size());
    }

    if (!sphere_instances.empty())
    {
        ctx.DrawSpheresInstanced(sphere_instances.data(), sphere_instances.size());
    }
}

float SkeletonDebugRenderSystem::ComputeRestLength(
    int bone_index,
    const std::vector<ozz::math::Float4x4>& rest_models,
    const std::vector<int>& skeleton_parents)
{
    float result = 0.0f;

    if (bone_index < 0 || static_cast<size_t>(bone_index) >= rest_models.size())
        return result;

    // Try parent distance
    const int parent = (bone_index < static_cast<int>(skeleton_parents.size()))
        ? skeleton_parents[bone_index]
        : -1;

    if (parent >= 0 && static_cast<size_t>(parent) < rest_models.size())
    {
        result = DistanceBetween(rest_models[bone_index], rest_models[parent]);
    }

    if (result > kSkeletonDebugEpsilon)
        return result;

    // Try child distance
    for (size_t child = 0; child < skeleton_parents.size(); ++child)
    {
        if (skeleton_parents[child] == bone_index)
        {
            result = std::max(result, DistanceBetween(
                rest_models[bone_index], rest_models[child]));
        }
    }

    return result;
}

ozz::math::Float3 SkeletonDebugRenderSystem::ExtractTranslation(
    const ozz::math::Float4x4& mat)
{
    ozz::math::Float3 result;
    ozz::math::Store3PtrU(mat.cols[3], &result.x);
    return result;
}

float SkeletonDebugRenderSystem::DistanceBetween(
    const ozz::math::Float4x4& a,
    const ozz::math::Float4x4& b)
{
    const ozz::math::Float3 pa = ExtractTranslation(a);
    const ozz::math::Float3 pb = ExtractTranslation(b);
    const float dx = pa.x - pb.x;
    const float dy = pa.y - pb.y;
    const float dz = pa.z - pb.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void SkeletonDebugRenderSystem::UpdateGlobalSettings(entt::registry& registry, bool show_skeleton_lines)
{
    // Update all entities with SkeletonDebugState
    auto view = registry.view<SkeletonDebugState>();

    for (auto entity : view)
    {
        auto& debug_state = view.get<SkeletonDebugState>(entity);
        debug_state.show_skeleton_lines = show_skeleton_lines;
    }
}

//=============================================================================
// IKGizmoRenderSystem Implementation
//=============================================================================

void IKGizmoRenderSystem::Render(entt::registry& registry, IDebugDrawContext& ctx)
{
    // Iterate entities with IK gizmo components
    auto view = registry.view<IKGizmoState, IKConfiguration, InstanceTransform>();

    for (auto entity : view)
    {
        const auto& gizmo_state = view.get<IKGizmoState>(entity);
        const auto& ik_config = view.get<IKConfiguration>(entity);
        const auto& inst_transform = view.get<InstanceTransform>(entity);

        if (!gizmo_state.enabled || !ik_config.IsInitialized())
            continue;

        // Render each gizmo
        RenderGizmo(ctx, gizmo_state.left_leg_gizmo, ik_config.left_leg,
                   inst_transform.world_transform, true);
        RenderGizmo(ctx, gizmo_state.right_leg_gizmo, ik_config.right_leg,
                   inst_transform.world_transform, true);
        RenderGizmo(ctx, gizmo_state.left_arm_gizmo, ik_config.left_arm,
                   inst_transform.world_transform, false);
        RenderGizmo(ctx, gizmo_state.right_arm_gizmo, ik_config.right_arm,
                   inst_transform.world_transform, false);
    }
}

void IKGizmoRenderSystem::RenderGizmo(
    IDebugDrawContext& ctx,
    const IKGizmoState::ChainGizmo& gizmo,
    const LimbIKChain& chain,
    const ozz::math::Float4x4& instance_transform,
    bool is_leg)
{
    if (!chain.Valid() || !chain.debug_target_valid)
        return;

    // Determine color based on state
    ozz::math::Float4 color;

    if (gizmo.is_dragging)
    {
        // Yellow when dragging
        color = ozz::math::Float4{1.0f, 1.0f, 0.0f, 0.8f};
    }
    else if (gizmo.is_hovered)
    {
        // Cyan when hovered
        color = ozz::math::Float4{0.0f, 1.0f, 1.0f, 0.7f};
    }
    else if (!chain.enabled)
    {
        // Gray/faded when disabled
        color = ozz::math::Float4{0.5f, 0.5f, 0.5f, 0.3f};
    }
    else
    {
        // Default color by type
        color = is_leg
            ? ozz::math::Float4{0.0f, 0.8f, 0.0f, 0.6f}  // Green for legs
            : ozz::math::Float4{0.8f, 0.0f, 0.0f, 0.6f}; // Red for arms
    }

    // Transform gizmo position from model space to world space
    const ozz::math::Float3 world_position =
        TransformPoint(instance_transform, gizmo.position);

    // Draw gizmo sphere
    ctx.DrawSphere(world_position, gizmo.radius, color);
}

ozz::math::Float3 IKGizmoRenderSystem::TransformPoint(
    const ozz::math::Float4x4& transform,
    const ozz::math::Float3& point)
{
    ozz::math::SimdFloat4 p = ozz::math::simd_float4::Load(
        point.x, point.y, point.z, 1.0f);
    ozz::math::SimdFloat4 result = transform * p;

    return ozz::math::Float3{
        ozz::math::GetX(result),
        ozz::math::GetY(result),
        ozz::math::GetZ(result)
    };
}

} // namespace AnimationECS
