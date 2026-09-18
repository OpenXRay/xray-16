#pragma once

#include "xrCore/xrCore.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/span.h"
#include "entt/entt.hpp"
#include <string>

namespace AnimationECS {

struct LimbIKChain
{
    enum class Role : u8
    {
        Leg = 0,
        Arm = 1,
    };

    Role role{Role::Leg};

    s16 start{-1};
    s16 mid{-1};
    s16 end{-1};

    ozz::math::SimdFloat4 mid_axis;
    ozz::math::SimdFloat4 pole_vector;

    ozz::math::Float3 target_offset{0.f, 0.f, 0.f};
    bool enabled{true};
    bool reached{false};

    ozz::math::Float3 debug_target{0.f, 0.f, 0.f};
    bool debug_target_valid{false};

    std::string start_name;
    std::string mid_name;
    std::string end_name;
    std::string label;

    bool Valid() const { return start >= 0 && mid >= 0 && end >= 0; }
};

struct IKConfiguration
{
    LimbIKChain left_leg;
    LimbIKChain right_leg;
    LimbIKChain left_arm;
    LimbIKChain right_arm;

    bool leg_ik_available{false};
    bool arm_ik_available{false};
    bool initialized{false};

    struct IKParams
    {
        float weight{1.0f};
        float soften{0.97f};
        float twist_angle{0.f};
    };

    IKParams leg_params;
    IKParams arm_params;

    float foot_ground_height{0.f};
    float crouch_offset{0.f};
    bool crouch_affects_arms{false};

    bool IsInitialized() const { return initialized; }
    bool HasLegIK() const { return leg_ik_available; }
    bool HasArmIK() const { return arm_ik_available; }
};

struct IKGizmoState
{
    struct ChainGizmo
    {
        ozz::math::Float3 position{0.f, 0.f, 0.f};
        float radius{0.08f};
        bool is_hovered{false};
        bool is_dragging{false};
    };

    ChainGizmo left_leg_gizmo;
    ChainGizmo right_leg_gizmo;
    ChainGizmo left_arm_gizmo;
    ChainGizmo right_arm_gizmo;

    ozz::math::Float4x4 instance_transform{ozz::math::Float4x4::identity()};

    int dragged_chain_index{-1};
    ozz::math::Float3 drag_start_offset{0.f, 0.f, 0.f};
    float drag_distance_from_camera{0.f};

    bool enabled{true};

    ChainGizmo* GetChainGizmo(int index)
    {
        switch (index) {
            case 0: return &left_leg_gizmo;
            case 1: return &right_leg_gizmo;
            case 2: return &left_arm_gizmo;
            case 3: return &right_arm_gizmo;
            default: return nullptr;
        }
    }
};

struct InstanceTransform
{
    ozz::math::Float4x4 world_transform{ozz::math::Float4x4::identity()};
};

struct SkeletonDebugState
{
    bool show_skeleton_lines{true};
    bool show_joint_positions{false};
    bool show_bone_names{false};
    bool show_bone_axes{false};

    float line_width{2.0f};
    float joint_radius{0.02f};
    ozz::math::Float4 line_color{1.0f, 1.0f, 1.0f, 1.0f};
    ozz::math::Float4 joint_color{1.0f, 0.5f, 0.0f, 1.0f};

    bool enabled{true};
};

class IKInitializationSystem
{
public:
    static void Initialize(
        entt::registry& registry,
        entt::entity entity,
        ozz::span<const char* const> joint_names,
        ozz::span<const ozz::math::Float4x4> bind_pose_models);

private:
    static int FindJointIndexByNames(
        ozz::span<const char* const> joint_names,
        std::initializer_list<const char*> candidates);

    static void ResolveChainAxes(
        LimbIKChain& chain,
        ozz::span<const ozz::math::Float4x4> bind_pose_models);

    static void PopulateChain(
        LimbIKChain& chain,
        LimbIKChain::Role role,
        ozz::span<const char* const> joint_names,
        ozz::span<const ozz::math::Float4x4> bind_pose_models,
        std::initializer_list<const char*> start_candidates,
        std::initializer_list<const char*> mid_candidates,
        std::initializer_list<const char*> end_candidates);
};

class IKSolverSystem
{
public:
    static void Update(entt::registry& registry);

    static bool SolveLimbIK(
        LimbIKChain& chain,
        const ozz::math::Float3& target_model,
        float weight,
        float soften,
        float twist_angle,
        ozz::span<const ozz::math::Float4x4> models,
        ozz::span<ozz::math::SoaTransform> locals,
        int* min_dirty_joint = nullptr);

    static ozz::math::Float3 ComputeChainTarget(
        const LimbIKChain& chain,
        ozz::span<const ozz::math::Float4x4> models,
        float foot_ground_height = 0.f,
        float crouch_offset = 0.f,
        bool crouch_affects_arms = false);

    static void ApplyDraggedTarget(
        LimbIKChain& chain,
        const ozz::math::Float3& target_world,
        ozz::span<const ozz::math::Float4x4> models);

private:
    static ozz::math::Float3 ExtractTranslation(const ozz::math::Float4x4& mat);
};

class IKDebugSystem
{
public:
    static void UpdateDebugVisualization(entt::registry& registry);

    static void RenderDebugInfo(const IKConfiguration& ik_config,
                                ozz::span<const ozz::math::Float4x4> models);
};

class IKGizmoSystem
{
public:
    static void UpdateGizmoPositions(entt::registry& registry);

    static bool HandleMouseInteraction(
        entt::registry& registry,
        float mouse_x, float mouse_y,
        int viewport_width, int viewport_height,
        const ozz::math::Float4x4& view_matrix,
        const ozz::math::Float4x4& proj_matrix,
        bool mouse_button_down,
        bool imgui_wants_mouse);

    static void RenderGizmos(entt::registry& registry, void* debug_renderer);
};

}
