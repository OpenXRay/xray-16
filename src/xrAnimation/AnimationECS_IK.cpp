#include "stdafx.h"
#include "AnimationECS_IK.h"
#include "AnimationECS_Components.h"
#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "../Externals/ozz-animation/samples/framework/utils.h"
#include <algorithm>
#include <cstring>
// NOTE: Debug visualization removed from this low-level library to avoid dependencies on xrGame.
// Debug drawing should be implemented at the application level (e.g., in xrGame or viewer tools)
// where DBG_Draw functions are available. See IK_CURRENT_LIMITATIONS.md for details.

namespace AnimationECS {

//=============================================================================
// IKInitializationSystem Implementation
//=============================================================================

int IKInitializationSystem::FindJointIndexByNames(
    ozz::span<const char* const> joint_names,
    std::initializer_list<const char*> candidates)
{
    for (const char* candidate : candidates)
    {
        for (int i = 0; i < static_cast<int>(joint_names.size()); ++i)
        {
            if (std::strcmp(joint_names[i], candidate) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

void IKInitializationSystem::ResolveChainAxes(
    LimbIKChain& chain,
    ozz::span<const ozz::math::Float4x4> bind_pose_models)
{
    chain.mid_axis = ozz::math::simd_float4::z_axis();
    chain.pole_vector = ozz::math::simd_float4::y_axis();  // Default pole vector

    if (chain.Valid() && chain.mid >= 0 && static_cast<size_t>(chain.mid) < bind_pose_models.size())
    {
        // Calculate mid_axis (bend axis) from bind pose Z-axis
        ozz::math::SimdFloat4 axis_candidate = bind_pose_models[chain.mid].cols[2];
        const ozz::math::SimdFloat4 axis_len_sq = ozz::math::Length3Sqr(axis_candidate);
        const ozz::math::SimdFloat4 min_len = ozz::math::simd_float4::Load1(1e-6f);
        if (ozz::math::AreAllTrue1(ozz::math::CmpGt(axis_len_sq, min_len)))
        {
            chain.mid_axis = ozz::math::Normalize3(axis_candidate);
            // Legs bend backwards (knees bend opposite to elbows)
            // Negate the axis for leg chains
            if (chain.role == LimbIKChain::Role::Leg)
            {
                chain.mid_axis = -chain.mid_axis;
            }
        }

        // Calculate stable pole vector from the geometry of the three joints in bind pose
        // This is more robust than using bind pose Y-axis, especially for skeletons with twist/pole bones
        if (chain.start >= 0 && static_cast<size_t>(chain.start) < bind_pose_models.size() &&
            chain.end >= 0 && static_cast<size_t>(chain.end) < bind_pose_models.size())
        {
            // Get joint positions
            const ozz::math::Float4x4& start_mat = bind_pose_models[chain.start];
            const ozz::math::Float4x4& mid_mat = bind_pose_models[chain.mid];
            const ozz::math::Float4x4& end_mat = bind_pose_models[chain.end];

            ozz::math::SimdFloat4 start_pos = start_mat.cols[3];
            ozz::math::SimdFloat4 mid_pos = mid_mat.cols[3];
            ozz::math::SimdFloat4 end_pos = end_mat.cols[3];

            // Calculate vectors from start to mid and start to end
            ozz::math::SimdFloat4 start_to_mid = mid_pos - start_pos;
            ozz::math::SimdFloat4 start_to_end = end_pos - start_pos;

            // Calculate cross product to get perpendicular (the direction the elbow/knee should point)
            ozz::math::SimdFloat4 pole_candidate = ozz::math::Cross3(start_to_mid, start_to_end);

            // For arms, we want the pole to point outward (left arm = negative X, right arm = positive X)
            // For legs, we want the pole to point forward (positive Z in most rigs)
            if (chain.role == LimbIKChain::Role::Arm)
            {
                // Cross product gives us a vector perpendicular to the arm plane
                // For consistency, ensure it points in a reasonable direction
                pole_candidate = ozz::math::Cross3(start_to_end, start_to_mid);
            }

            const ozz::math::SimdFloat4 pole_len_sq = ozz::math::Length3Sqr(pole_candidate);
            if (ozz::math::AreAllTrue1(ozz::math::CmpGt(pole_len_sq, min_len)))
            {
                chain.pole_vector = ozz::math::Normalize3(pole_candidate);
            }
            else
            {
                // Fallback to Y-axis if triangle is degenerate
                ozz::math::SimdFloat4 y_axis_candidate = mid_mat.cols[1];
                const ozz::math::SimdFloat4 y_len_sq = ozz::math::Length3Sqr(y_axis_candidate);
                if (ozz::math::AreAllTrue1(ozz::math::CmpGt(y_len_sq, min_len)))
                {
                    chain.pole_vector = ozz::math::Normalize3(y_axis_candidate);
                }
            }
        }
        else
        {
            // Fallback: Calculate from bind pose Y-axis
            ozz::math::SimdFloat4 pole_candidate = bind_pose_models[chain.mid].cols[1];
            const ozz::math::SimdFloat4 pole_len_sq = ozz::math::Length3Sqr(pole_candidate);
            if (ozz::math::AreAllTrue1(ozz::math::CmpGt(pole_len_sq, min_len)))
            {
                chain.pole_vector = ozz::math::Normalize3(pole_candidate);
            }
        }
    }
}

void IKInitializationSystem::PopulateChain(
    LimbIKChain& chain,
    LimbIKChain::Role role,
    ozz::span<const char* const> joint_names,
    ozz::span<const ozz::math::Float4x4> bind_pose_models,
    std::initializer_list<const char*> start_candidates,
    std::initializer_list<const char*> mid_candidates,
    std::initializer_list<const char*> end_candidates)
{
    chain.role = role;
    chain.start = static_cast<s16>(FindJointIndexByNames(joint_names, start_candidates));
    chain.mid = static_cast<s16>(FindJointIndexByNames(joint_names, mid_candidates));
    chain.end = static_cast<s16>(FindJointIndexByNames(joint_names, end_candidates));
    chain.start_name = chain.start >= 0 ? joint_names[chain.start] : std::string();
    chain.mid_name = chain.mid >= 0 ? joint_names[chain.mid] : std::string();
    chain.end_name = chain.end >= 0 ? joint_names[chain.end] : std::string();
    chain.target_offset = { 0.f, 0.f, 0.f };
    chain.reached = false;
    ResolveChainAxes(chain, bind_pose_models);
}

void IKInitializationSystem::Initialize(
    entt::registry& registry,
    entt::entity entity,
    ozz::span<const char* const> joint_names,
    ozz::span<const ozz::math::Float4x4> bind_pose_models)
{
    // Add or get IK configuration component
    auto& ik_config = registry.emplace_or_replace<IKConfiguration>(entity);

    // Reset chains
    ik_config.left_leg = LimbIKChain{};
    ik_config.right_leg = LimbIKChain{};
    ik_config.left_arm = LimbIKChain{};
    ik_config.right_arm = LimbIKChain{};

    // Populate leg chains
    PopulateChain(ik_config.left_leg, LimbIKChain::Role::Leg,
        joint_names, bind_pose_models,
        { "bip01_l_thigh", "l_thigh", "left_thigh" },
        { "bip01_l_calf", "l_calf", "left_calf", "bip01_l_knee" },
        { "bip01_l_foot", "l_foot", "left_foot", "bip01_l_ankle" });

    PopulateChain(ik_config.right_leg, LimbIKChain::Role::Leg,
        joint_names, bind_pose_models,
        { "bip01_r_thigh", "r_thigh", "right_thigh" },
        { "bip01_r_calf", "r_calf", "right_calf", "bip01_r_knee" },
        { "bip01_r_foot", "r_foot", "right_foot", "bip01_r_ankle" });

    // Populate arm chains
    PopulateChain(ik_config.left_arm, LimbIKChain::Role::Arm,
        joint_names, bind_pose_models,
        { "bip01_l_upperarm", "bip01_l_shoulder", "l_upperarm", "left_shoulder" },
        { "bip01_l_forearm", "l_forearm", "left_forearm", "bip01_l_elbow" },
        { "bip01_l_hand", "l_hand", "left_hand", "bip01_l_wrist" });

    PopulateChain(ik_config.right_arm, LimbIKChain::Role::Arm,
        joint_names, bind_pose_models,
        { "bip01_r_upperarm", "bip01_r_shoulder", "r_upperarm", "right_shoulder" },
        { "bip01_r_forearm", "r_forearm", "right_forearm", "bip01_r_elbow" },
        { "bip01_r_hand", "r_hand", "right_hand", "bip01_r_wrist" });

    // Auto-detect left/right swap based on X position in bind pose
    // For legs: left foot should have SMALLER X than right foot (left is negative X, right is positive X)
    if (ik_config.left_leg.Valid() && ik_config.right_leg.Valid() && !bind_pose_models.empty())
    {
        const float left_x = ozz::math::GetX(bind_pose_models[ik_config.left_leg.end].cols[3]);
        const float right_x = ozz::math::GetX(bind_pose_models[ik_config.right_leg.end].cols[3]);
        if (left_x > right_x)
        {
            std::swap(ik_config.left_leg, ik_config.right_leg);
            Msg("[AnimationECS IK] Swapped left/right legs (left_x=%.2f > right_x=%.2f)", left_x, right_x);
        }
    }

    // For arms: left hand should have SMALLER X than right hand
    if (ik_config.left_arm.Valid() && ik_config.right_arm.Valid() && !bind_pose_models.empty())
    {
        const float left_x = ozz::math::GetX(bind_pose_models[ik_config.left_arm.end].cols[3]);
        const float right_x = ozz::math::GetX(bind_pose_models[ik_config.right_arm.end].cols[3]);
        if (left_x > right_x)
        {
            std::swap(ik_config.left_arm, ik_config.right_arm);
            Msg("[AnimationECS IK] Swapped left/right arms (left_x=%.2f > right_x=%.2f)", left_x, right_x);
        }
    }

    // Set labels
    ik_config.left_leg.label = "Left leg";
    ik_config.right_leg.label = "Right leg";
    ik_config.left_arm.label = "Left arm";
    ik_config.right_arm.label = "Right arm";

    // Set availability
    ik_config.leg_ik_available = ik_config.left_leg.Valid() || ik_config.right_leg.Valid();
    ik_config.arm_ik_available = ik_config.left_arm.Valid() || ik_config.right_arm.Valid();

    // Disable chains by default - application must explicitly enable them
    // Enabling by default causes numerical instability when solving to trivial targets
    ik_config.left_leg.enabled = false;
    ik_config.right_leg.enabled = false;
    ik_config.left_arm.enabled = false;
    ik_config.right_arm.enabled = false;

    // Set default parameters
    ik_config.leg_params.weight = 1.0f;
    ik_config.leg_params.soften = 0.97f;
    ik_config.leg_params.twist_angle = 0.f;

    ik_config.arm_params.weight = 1.0f;
    ik_config.arm_params.soften = 0.85f;
    ik_config.arm_params.twist_angle = 0.f;

    ik_config.initialized = true;

    Msg("[AnimationECS IK] Initialized IK for entity. Legs: %d, Arms: %d",
        ik_config.leg_ik_available, ik_config.arm_ik_available);
}

//=============================================================================
// IKSolverSystem Implementation
//=============================================================================

ozz::math::Float3 IKSolverSystem::ExtractTranslation(const ozz::math::Float4x4& mat)
{
    return ozz::math::Float3{
        ozz::math::GetX(mat.cols[3]),
        ozz::math::GetY(mat.cols[3]),
        ozz::math::GetZ(mat.cols[3])
    };
}

bool IKSolverSystem::SolveLimbIK(
    LimbIKChain& chain,
    const ozz::math::Float3& target_model,
    float weight,
    float soften,
    float twist_angle,
    ozz::span<const ozz::math::Float4x4> models,
    ozz::span<ozz::math::SoaTransform> locals,
    int* min_dirty_joint)
{
    chain.reached = false;
    if (!chain.Valid() || !chain.enabled)
    {
        chain.debug_target_valid = false;
        return false;
    }

    // Skip IK if weight is effectively zero (no contribution)
    // This avoids numerical errors when IK isn't actually needed
    if (weight < 1e-4f)
    {
        chain.debug_target_valid = false;
        chain.reached = true;  // Trivially "reached" since we're not solving
        return false;
    }

    const size_t model_count = models.size();
    if (chain.start < 0 || chain.mid < 0 || chain.end < 0 ||
        static_cast<size_t>(chain.start) >= model_count ||
        static_cast<size_t>(chain.mid) >= model_count ||
        static_cast<size_t>(chain.end) >= model_count)
    {
        chain.debug_target_valid = false;
        return false;
    }

    // Store debug info for external visualization
    chain.debug_target = target_model;
    chain.debug_target_valid = true;

    const ozz::math::SimdFloat4 target_ms = ozz::math::simd_float4::Load3PtrU(&target_model.x);

    // Use the stable pole vector from bind pose instead of recalculating from current frame
    // This prevents IK flipping/snapping issues, especially for first-person arm rigs
    ozz::math::SimdFloat4 pole_vector_ms = chain.pole_vector;

    ozz::animation::IKTwoBoneJob ik_job;
    ik_job.target = target_ms;
    ik_job.pole_vector = pole_vector_ms;
    ik_job.mid_axis = chain.mid_axis;
    ik_job.weight = std::clamp(weight, 0.f, 1.f);
    ik_job.soften = std::clamp(soften, 0.f, 0.999f);
    ik_job.twist_angle = twist_angle;
    ik_job.start_joint = &models[chain.start];
    ik_job.mid_joint = &models[chain.mid];
    ik_job.end_joint = &models[chain.end];
    ozz::math::SimdQuaternion start_correction;
    ozz::math::SimdQuaternion mid_correction;
    ik_job.start_joint_correction = &start_correction;
    ik_job.mid_joint_correction = &mid_correction;
    ik_job.reached = &chain.reached;

    if (!ik_job.Run())
    {
        chain.reached = false;
        return false;
    }

    ozz::sample::MultiplySoATransformQuaternion(chain.start, start_correction, locals);
    ozz::sample::MultiplySoATransformQuaternion(chain.mid, mid_correction, locals);

    if (min_dirty_joint)
    {
        *min_dirty_joint = std::min<int>(*min_dirty_joint, static_cast<int>(chain.start));
    }

    return true;
}

ozz::math::Float3 IKSolverSystem::ComputeChainTarget(
    const LimbIKChain& chain,
    ozz::span<const ozz::math::Float4x4> models,
    float foot_ground_height,
    float crouch_offset,
    bool crouch_affects_arms)
{
    if (!chain.Valid() || static_cast<size_t>(chain.end) >= models.size())
    {
        return { 0.f, 0.f, 0.f };
    }

    ozz::math::Float3 target = ExtractTranslation(models[chain.end]);
    if (chain.role == LimbIKChain::Role::Leg)
    {
        target.x += chain.target_offset.x;
        target.z += chain.target_offset.z;
        target.y = foot_ground_height + chain.target_offset.y;
    }
    else
    {
        target.x += chain.target_offset.x;
        target.y += chain.target_offset.y;
        target.z += chain.target_offset.z;
        if (crouch_affects_arms)
        {
            target.y -= crouch_offset;
        }
    }
    return target;
}

void IKSolverSystem::ApplyDraggedTarget(
    LimbIKChain& chain,
    const ozz::math::Float3& target_world,
    ozz::span<const ozz::math::Float4x4> models)
{
    if (!chain.Valid() || static_cast<size_t>(chain.end) >= models.size())
    {
        return;
    }

    const ozz::math::Float3 current_pos = ExtractTranslation(models[chain.end]);
    // Set offset directly (don't accumulate) - target_world is the absolute position we want
    chain.target_offset.x = target_world.x - current_pos.x;
    chain.target_offset.y = target_world.y - current_pos.y;
    chain.target_offset.z = target_world.z - current_pos.z;
}

void IKSolverSystem::Update(entt::registry& registry)
{
    // NOTE: Currently this IK system does NOT calculate ground collision targets like the legacy IKLimb system!
    // The legacy system (see xrGame/ik/IKLimb.cpp):
    //   - Uses m_foot.Collide() to raycast for ground surfaces
    //   - Calculates proper foot placement on terrain/geometry
    //   - Blends between animation and IK based on foot contact state
    //
    // This new system currently just uses animated positions + user offsets as targets.
    // To get visible foot IK like the legacy system, we need to:
    //   1. Implement ground collision detection (raycasting)
    //   2. Calculate proper foot placement matrices
    //   3. Handle blending and state transitions
    //
    // For now, this serves as a foundation and can be extended with proper target calculation.

    // Process all entities with IK configuration
    auto view = registry.view<IKConfiguration, AnimationBuffers>();

    if (view.size_hint() == 0)
        return;

    for (auto entity : view)
    {
        auto& ik_config = view.get<IKConfiguration>(entity);
        auto& buffers = view.get<AnimationBuffers>(entity);

        if (!ik_config.IsInitialized() || !buffers.IsInitialized())
            continue;

        // Get controller for skeleton access (needed for LocalToModel rebuilds)
        auto* controller = registry.try_get<AnimationController>(entity);
        if (!controller || !controller->skeleton)
            continue;

        int min_dirty_joint = static_cast<int>(buffers.models.size());

        // PER-LIMB REBUILD APPROACH
        // Problem: Skeleton hierarchy dependencies!
        // - Right leg (joints 3,4,5) modifies pelvis/spine
        // - Left leg (joints 7,8,9) shares pelvis as parent
        // - When we rebuild from joint 3, joint 7's parent changes!
        // - Left leg's IK assumed ANIMATED parent, not IK-modified parent
        //
        // Solution: Rebuild after EACH limb so next limb sees updated hierarchy
        // Cost: 4 rebuilds per entity (acceptable for correctness)

        auto solve_and_rebuild_limb = [&](LimbIKChain& chain, const IKConfiguration::IKParams& params)
        {
            if (!chain.Valid())
                return;

            // Compute target position first
            const ozz::math::Float3 target = ComputeChainTarget(
                chain,
                ozz::make_span(buffers.models),
                ik_config.foot_ground_height,
                ik_config.crouch_offset,
                ik_config.crouch_affects_arms);

            // Always set debug target so gizmos work properly (even when disabled)
            chain.debug_target = target;
            chain.debug_target_valid = true;

            // If chain is disabled, don't actually solve IK
            if (!chain.enabled)
                return;

            // Check if target offset is effectively zero (avoid IK on trivial cases)
            const float offset_len_sq = chain.target_offset.x * chain.target_offset.x +
                                       chain.target_offset.y * chain.target_offset.y +
                                       chain.target_offset.z * chain.target_offset.z;
            const float epsilon = 1e-6f;

            // For legs with ground height, always run IK even if offset is zero
            const bool is_leg_with_ground = (chain.role == LimbIKChain::Role::Leg &&
                                            std::abs(ik_config.foot_ground_height) > epsilon);

            // Skip IK solver if offset is zero and no ground height (prevents numerical drift)
            // But we still set debug_target above so gizmos work
            if (!is_leg_with_ground && offset_len_sq < epsilon && params.weight > 0.999f)
            {
                chain.reached = true;  // Trivially reached (staying at animated position)
                return;
            }

            int chain_dirty = static_cast<int>(buffers.models.size());
            bool success = SolveLimbIK(
                chain,
                target,
                params.weight,
                params.soften,
                params.twist_angle,
                ozz::make_span(buffers.models),
                ozz::make_span(buffers.locals),
                &chain_dirty);

            if (success && chain_dirty < static_cast<int>(buffers.models.size()))
            {
                // CRITICAL: Rebuild immediately so next limb sees updated hierarchy
                ozz::animation::LocalToModelJob ltm_job;
                ltm_job.skeleton = controller->skeleton;
                ltm_job.from = chain_dirty;
                ltm_job.input = ozz::make_span(buffers.locals);
                ltm_job.output = ozz::make_span(buffers.models);

                if (!ltm_job.Run())
                {
                    Msg("! [IKSolverSystem] LocalToModel rebuild failed for entity %u", static_cast<u32>(entity));
                }

                // Track minimum dirty joint
                if (chain_dirty < min_dirty_joint)
                {
                    min_dirty_joint = chain_dirty;
                }
            }
        };

        // Solve each limb with immediate rebuild
        if (ik_config.leg_ik_available)
        {
            solve_and_rebuild_limb(ik_config.left_leg, ik_config.leg_params);
            solve_and_rebuild_limb(ik_config.right_leg, ik_config.leg_params);
        }

        if (ik_config.arm_ik_available)
        {
            solve_and_rebuild_limb(ik_config.left_arm, ik_config.arm_params);
            solve_and_rebuild_limb(ik_config.right_arm, ik_config.arm_params);
        }
    }
}

//=============================================================================
// IKDebugSystem Implementation
//=============================================================================

void IKDebugSystem::UpdateDebugVisualization(entt::registry& registry)
{
    // TODO: Implement debug visualization
    // This would draw IK targets, pole vectors, chain bones, etc.
}

void IKDebugSystem::RenderDebugInfo(const IKConfiguration& ik_config,
                                   ozz::span<const ozz::math::Float4x4> models)
{
    // TODO: Implement debug rendering
    // This would draw gizmos for IK targets, chains, etc.
}

//=============================================================================
// IKGizmoSystem Implementation
//=============================================================================

void IKGizmoSystem::UpdateGizmoPositions(entt::registry& registry)
{
    // Update gizmo positions from IK chain debug targets
    auto view = registry.view<IKConfiguration, IKGizmoState>();

    for (auto entity : view)
    {
        auto& ik_config = view.get<IKConfiguration>(entity);
        auto& gizmo_state = view.get<IKGizmoState>(entity);

        if (!gizmo_state.enabled || !ik_config.IsInitialized())
            continue;

        // Update each chain's gizmo position from debug target
        auto update_chain_gizmo = [](const LimbIKChain& chain, IKGizmoState::ChainGizmo& gizmo)
        {
            if (chain.Valid() && chain.debug_target_valid)
            {
                gizmo.position = chain.debug_target;
            }
        };

        update_chain_gizmo(ik_config.left_leg, gizmo_state.left_leg_gizmo);
        update_chain_gizmo(ik_config.right_leg, gizmo_state.right_leg_gizmo);
        update_chain_gizmo(ik_config.left_arm, gizmo_state.left_arm_gizmo);
        update_chain_gizmo(ik_config.right_arm, gizmo_state.right_arm_gizmo);
    }
}

bool IKGizmoSystem::HandleMouseInteraction(
    entt::registry& registry,
    float mouse_x, float mouse_y,
    int viewport_width, int viewport_height,
    const ozz::math::Float4x4& view_matrix,
    const ozz::math::Float4x4& proj_matrix,
    bool mouse_button_down,
    bool imgui_wants_mouse)
{
    // Skip if ImGui wants the mouse
    if (imgui_wants_mouse)
        return false;

    // Helper: Screen to world ray
    auto screen_to_world_ray = [&](ozz::math::Float3* out_origin, ozz::math::Float3* out_direction)
    {
        const float ndc_x = (2.0f * mouse_x) / viewport_width - 1.0f;
        const float ndc_y = (2.0f * mouse_y) / viewport_height - 1.0f;

        const ozz::math::Float4x4 view_proj = proj_matrix * view_matrix;
        const ozz::math::Float4x4 inv_view_proj = ozz::math::Invert(view_proj);

        ozz::math::SimdFloat4 near_ndc = ozz::math::simd_float4::Load(ndc_x, ndc_y, 0.0f, 1.0f);
        ozz::math::SimdFloat4 far_ndc = ozz::math::simd_float4::Load(ndc_x, ndc_y, 1.0f, 1.0f);

        ozz::math::SimdFloat4 near_world_h = inv_view_proj * near_ndc;
        ozz::math::SimdFloat4 far_world_h = inv_view_proj * far_ndc;

        const float near_w = ozz::math::GetW(near_world_h);
        const float far_w = ozz::math::GetW(far_world_h);
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

        *out_origin = near_world;
        out_direction->x = far_world.x - near_world.x;
        out_direction->y = far_world.y - near_world.y;
        out_direction->z = far_world.z - near_world.z;

        const float len = std::sqrt(out_direction->x * out_direction->x +
                                   out_direction->y * out_direction->y +
                                   out_direction->z * out_direction->z);
        if (len > 1e-6f) {
            out_direction->x /= len;
            out_direction->y /= len;
            out_direction->z /= len;
        }
    };

    // Helper: Ray-sphere intersection
    auto ray_sphere_intersection = [](
        const ozz::math::Float3& ray_origin,
        const ozz::math::Float3& ray_direction,
        const ozz::math::Float3& sphere_center,
        float sphere_radius,
        float* out_distance = nullptr) -> bool
    {
        ozz::math::Float3 oc;
        oc.x = ray_origin.x - sphere_center.x;
        oc.y = ray_origin.y - sphere_center.y;
        oc.z = ray_origin.z - sphere_center.z;

        const float a = ray_direction.x * ray_direction.x +
                       ray_direction.y * ray_direction.y +
                       ray_direction.z * ray_direction.z;
        const float b = 2.0f * (oc.x * ray_direction.x +
                               oc.y * ray_direction.y +
                               oc.z * ray_direction.z);
        const float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - sphere_radius * sphere_radius;

        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f)
            return false;

        const float sqrt_disc = std::sqrt(discriminant);
        float t = (-b - sqrt_disc) / (2.0f * a);
        if (t < 0.0f)
            t = (-b + sqrt_disc) / (2.0f * a);

        if (t < 0.0f)
            return false;

        if (out_distance)
            *out_distance = t;
        return true;
    };

    // Compute ray
    ozz::math::Float3 ray_origin, ray_direction;
    screen_to_world_ray(&ray_origin, &ray_direction);

    bool any_interaction = false;

    // Process all entities with IK gizmos
    auto view = registry.view<IKConfiguration, IKGizmoState, AnimationBuffers>();

    for (auto entity : view)
    {
        auto& ik_config = view.get<IKConfiguration>(entity);
        auto& gizmo_state = view.get<IKGizmoState>(entity);
        auto& buffers = view.get<AnimationBuffers>(entity);

        if (!gizmo_state.enabled || !ik_config.IsInitialized())
            continue;

        // Reset hover states
        gizmo_state.left_leg_gizmo.is_hovered = false;
        gizmo_state.right_leg_gizmo.is_hovered = false;
        gizmo_state.left_arm_gizmo.is_hovered = false;
        gizmo_state.right_arm_gizmo.is_hovered = false;

        // If currently dragging, handle drag update
        if (gizmo_state.dragged_chain_index >= 0 && mouse_button_down)
        {
            // Find the chain being dragged
            LimbIKChain* chain = nullptr;
            switch (gizmo_state.dragged_chain_index) {
                case 0: chain = &ik_config.left_leg; break;
                case 1: chain = &ik_config.right_leg; break;
                case 2: chain = &ik_config.left_arm; break;
                case 3: chain = &ik_config.right_arm; break;
            }

            if (chain && chain->Valid())
            {
                // Project ray to drag plane
                ozz::math::Float3 new_pos;
                new_pos.x = ray_origin.x + ray_direction.x * gizmo_state.drag_distance_from_camera;
                new_pos.y = ray_origin.y + ray_direction.y * gizmo_state.drag_distance_from_camera;
                new_pos.z = ray_origin.z + ray_direction.z * gizmo_state.drag_distance_from_camera;

                // Apply drag offset
                new_pos.x += gizmo_state.drag_start_offset.x;
                new_pos.y += gizmo_state.drag_start_offset.y;
                new_pos.z += gizmo_state.drag_start_offset.z;

                // Update target offset
                IKSolverSystem::ApplyDraggedTarget(*chain, new_pos, ozz::make_span(buffers.models));

                any_interaction = true;
            }
        }
        else if (mouse_button_down)
        {
            // Start drag - find closest gizmo
            struct GizmoHit {
                int index;
                float distance;
                IKGizmoState::ChainGizmo* gizmo;
                LimbIKChain* chain;
            };

            GizmoHit closest_hit{-1, std::numeric_limits<float>::max(), nullptr, nullptr};

            auto check_gizmo = [&](int index, IKGizmoState::ChainGizmo& gizmo, LimbIKChain& chain)
            {
                // Allow interaction with any valid chain (even if disabled) so users can enable by dragging
                if (!chain.Valid() || !chain.debug_target_valid)
                    return;

                float distance;
                if (ray_sphere_intersection(ray_origin, ray_direction, gizmo.position, gizmo.radius, &distance))
                {
                    if (distance < closest_hit.distance)
                    {
                        closest_hit.index = index;
                        closest_hit.distance = distance;
                        closest_hit.gizmo = &gizmo;
                        closest_hit.chain = &chain;
                    }
                }
            };

            check_gizmo(0, gizmo_state.left_leg_gizmo, ik_config.left_leg);
            check_gizmo(1, gizmo_state.right_leg_gizmo, ik_config.right_leg);
            check_gizmo(2, gizmo_state.left_arm_gizmo, ik_config.left_arm);
            check_gizmo(3, gizmo_state.right_arm_gizmo, ik_config.right_arm);

            if (closest_hit.index >= 0)
            {
                // Start dragging
                gizmo_state.dragged_chain_index = closest_hit.index;
                gizmo_state.drag_distance_from_camera = closest_hit.distance;
                gizmo_state.drag_start_offset = {0.f, 0.f, 0.f}; // No offset initially

                closest_hit.gizmo->is_dragging = true;

                // Enable the chain when dragging starts (allows users to enable IK by dragging)
                if (closest_hit.chain && !closest_hit.chain->enabled) {
                    closest_hit.chain->enabled = true;
                }

                any_interaction = true;
            }
        }
        else
        {
            // Released - stop dragging
            if (gizmo_state.dragged_chain_index >= 0)
            {
                auto* gizmo = gizmo_state.GetChainGizmo(gizmo_state.dragged_chain_index);
                if (gizmo)
                    gizmo->is_dragging = false;

                gizmo_state.dragged_chain_index = -1;
            }

            // Just hovering - check which gizmo is under mouse
            auto check_hover = [&](IKGizmoState::ChainGizmo& gizmo, const LimbIKChain& chain)
            {
                // Allow hovering over any valid chain (even if disabled) to show it's interactive
                if (!chain.Valid() || !chain.debug_target_valid)
                    return;

                if (ray_sphere_intersection(ray_origin, ray_direction, gizmo.position, gizmo.radius))
                {
                    gizmo.is_hovered = true;
                    any_interaction = true;
                }
            };

            check_hover(gizmo_state.left_leg_gizmo, ik_config.left_leg);
            check_hover(gizmo_state.right_leg_gizmo, ik_config.right_leg);
            check_hover(gizmo_state.left_arm_gizmo, ik_config.left_arm);
            check_hover(gizmo_state.right_arm_gizmo, ik_config.right_arm);
        }
    }

    return any_interaction;
}

void IKGizmoSystem::RenderGizmos(entt::registry& registry, void* debug_renderer_ptr)
{
    if (!debug_renderer_ptr)
        return;

    // Forward declare DebugRenderer to avoid header dependency
    // Actual rendering must be done by the application (e.g., viewer)
    // that has access to the DebugRenderer type

    // This function signature uses void* to keep the ECS system independent
    // of rendering implementation details. The calling code should cast
    // debug_renderer_ptr to the appropriate type (e.g., DebugRenderer*)
    // and call the appropriate rendering functions.

    // See ozz_animation_viewer.cpp for example usage:
    // IKGizmoSystem::RenderGizmos(registry, &renderer.GetDebugRenderer());
}

} // namespace AnimationECS
