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

//=============================================================================
// IK COMPONENTS (Data-only, ECS compatible)
//=============================================================================

//-----------------------------------------------------------------------------
// LimbIKChain Component
// Represents a two-bone IK chain (shoulder-elbow-wrist or hip-knee-ankle)
//-----------------------------------------------------------------------------
struct LimbIKChain
{
    enum class Role : u8
    {
        Leg = 0,
        Arm = 1,
    };

    // Chain configuration
    Role role{Role::Leg};                       // Chain type (determines bend direction)

    // Joint indices in skeleton
    s16 start{-1};                              // Shoulder/Hip joint index
    s16 mid{-1};                                // Elbow/Knee joint index
    s16 end{-1};                                // Wrist/Ankle joint index

    // IK solving parameters (calculated from bind pose)
    ozz::math::SimdFloat4 mid_axis;             // Bend axis (which way elbow/knee bends)
    ozz::math::SimdFloat4 pole_vector;          // Stable pole direction from bind pose

    // Target control
    ozz::math::Float3 target_offset{0.f, 0.f, 0.f};  // Offset from animated position
    bool enabled{true};                         // Whether to apply IK to this chain
    bool reached{false};                        // Whether IK reached target last frame

    // Debug visualization
    ozz::math::Float3 debug_target{0.f, 0.f, 0.f};
    bool debug_target_valid{false};

    // Cached names for debugging
    std::string start_name;
    std::string mid_name;
    std::string end_name;
    std::string label;                          // Display name (e.g., "Left arm")

    bool Valid() const { return start >= 0 && mid >= 0 && end >= 0; }
};

//-----------------------------------------------------------------------------
// IKConfiguration Component
// Container for all IK chains on an entity
//-----------------------------------------------------------------------------
struct IKConfiguration
{
    // IK chains (optional - only exist if skeleton has appropriate bones)
    LimbIKChain left_leg;
    LimbIKChain right_leg;
    LimbIKChain left_arm;
    LimbIKChain right_arm;

    // Availability flags
    bool leg_ik_available{false};
    bool arm_ik_available{false};
    bool initialized{false};

    // IK solving parameters
    struct IKParams
    {
        float weight{1.0f};                     // IK weight (0-1)
        float soften{0.97f};                    // Joint softness (0-0.999)
        float twist_angle{0.f};                 // Twist angle in radians
    };

    IKParams leg_params;
    IKParams arm_params;

    // Ground/crouch parameters
    float foot_ground_height{0.f};
    float crouch_offset{0.f};
    bool crouch_affects_arms{false};

    bool IsInitialized() const { return initialized; }
    bool HasLegIK() const { return leg_ik_available; }
    bool HasArmIK() const { return arm_ik_available; }
};

//-----------------------------------------------------------------------------
// IKGizmoState Component
// Per-entity gizmo interaction state for IK targets
//-----------------------------------------------------------------------------
struct IKGizmoState
{
    // Gizmo visual state per chain
    struct ChainGizmo
    {
        ozz::math::Float3 position{0.f, 0.f, 0.f};  // World-space position
        float radius{0.08f};                         // Visual radius
        bool is_hovered{false};                      // Mouse hovering over gizmo
        bool is_dragging{false};                     // Currently being dragged
    };

    ChainGizmo left_leg_gizmo;
    ChainGizmo right_leg_gizmo;
    ChainGizmo left_arm_gizmo;
    ChainGizmo right_arm_gizmo;

    // Instance transform (for multi-instance rendering)
    ozz::math::Float4x4 instance_transform{ozz::math::Float4x4::identity()};

    // Global dragging state for this entity
    int dragged_chain_index{-1};  // -1=none, 0=left_leg, 1=right_leg, 2=left_arm, 3=right_arm
    ozz::math::Float3 drag_start_offset{0.f, 0.f, 0.f};
    float drag_distance_from_camera{0.f};

    // Settings
    bool enabled{true};  // Master enable/disable for this entity's gizmos

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

//-----------------------------------------------------------------------------
// InstanceTransform Component
// Per-entity world transform for multi-instance rendering
//-----------------------------------------------------------------------------
struct InstanceTransform
{
    ozz::math::Float4x4 world_transform{ozz::math::Float4x4::identity()};
};

//-----------------------------------------------------------------------------
// SkeletonDebugState Component
// Per-entity skeleton visualization state
//-----------------------------------------------------------------------------
struct SkeletonDebugState
{
    // Visualization settings
    bool show_skeleton_lines{true};     // Show bone hierarchy as lines
    bool show_joint_positions{false};   // Show joint positions as spheres
    bool show_bone_names{false};        // Show bone names as text labels
    bool show_bone_axes{false};         // Show local axes at each bone

    // Visual parameters
    float line_width{2.0f};             // Width of skeleton lines
    float joint_radius{0.02f};          // Radius of joint spheres
    ozz::math::Float4 line_color{1.0f, 1.0f, 1.0f, 1.0f};   // RGBA color for skeleton lines
    ozz::math::Float4 joint_color{1.0f, 0.5f, 0.0f, 1.0f};  // RGBA color for joints

    // Master enable/disable
    bool enabled{true};                 // Master toggle for this entity's debug visualization
};

//=============================================================================
// IK SYSTEMS (Logic operating on components)
//=============================================================================

//-----------------------------------------------------------------------------
// IKInitializationSystem
// Initializes IK chains by finding appropriate bones in skeleton
//-----------------------------------------------------------------------------
class IKInitializationSystem
{
public:
    /// <summary>
    /// Initialize IK configuration for an entity
    /// </summary>
    /// <param name="registry">ECS registry</param>
    /// <param name="entity">Entity to initialize IK for</param>
    /// <param name="joint_names">Skeleton joint names</param>
    /// <param name="bind_pose_models">Bind pose model-space transforms</param>
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

//-----------------------------------------------------------------------------
// IKSolverSystem
// Solves IK for all chains on entities with IKConfiguration
//-----------------------------------------------------------------------------
class IKSolverSystem
{
public:
    /// <summary>
    /// Solve IK for all entities with IK configuration
    /// Called after animation sampling but before local-to-model conversion
    /// </summary>
    /// <param name="registry">ECS registry</param>
    static void Update(entt::registry& registry);

    /// <summary>
    /// Solve IK for a specific limb chain
    /// </summary>
    static bool SolveLimbIK(
        LimbIKChain& chain,
        const ozz::math::Float3& target_model,
        float weight,
        float soften,
        float twist_angle,
        ozz::span<const ozz::math::Float4x4> models,
        ozz::span<ozz::math::SoaTransform> locals,
        int* min_dirty_joint = nullptr);

    /// <summary>
    /// Compute target position for a chain based on its offset and role
    /// </summary>
    static ozz::math::Float3 ComputeChainTarget(
        const LimbIKChain& chain,
        ozz::span<const ozz::math::Float4x4> models,
        float foot_ground_height = 0.f,
        float crouch_offset = 0.f,
        bool crouch_affects_arms = false);

    /// <summary>
    /// Apply dragged target to a chain (for interactive control)
    /// </summary>
    static void ApplyDraggedTarget(
        LimbIKChain& chain,
        const ozz::math::Float3& target_world,
        ozz::span<const ozz::math::Float4x4> models);

private:
    static ozz::math::Float3 ExtractTranslation(const ozz::math::Float4x4& mat);
};

//-----------------------------------------------------------------------------
// IKDebugSystem
// Visualization and debugging for IK chains
//-----------------------------------------------------------------------------
class IKDebugSystem
{
public:
    /// <summary>
    /// Update debug visualization for IK chains
    /// </summary>
    static void UpdateDebugVisualization(entt::registry& registry);

    /// <summary>
    /// Render IK chain debug info (targets, poles, etc.)
    /// </summary>
    static void RenderDebugInfo(const IKConfiguration& ik_config,
                                ozz::span<const ozz::math::Float4x4> models);
};

//-----------------------------------------------------------------------------
// IKGizmoSystem
// Interactive gizmo system for IK target manipulation
//-----------------------------------------------------------------------------
class IKGizmoSystem
{
public:
    /// <summary>
    /// Update gizmo positions from IK chain targets (call after IK solving)
    /// </summary>
    /// <param name="registry">ECS registry</param>
    static void UpdateGizmoPositions(entt::registry& registry);

    /// <summary>
    /// Handle mouse interaction with gizmos (hover, click, drag)
    /// </summary>
    /// <param name="registry">ECS registry</param>
    /// <param name="mouse_x">Mouse X in screen coordinates</param>
    /// <param name="mouse_y">Mouse Y in screen coordinates</param>
    /// <param name="viewport_width">Viewport width</param>
    /// <param name="viewport_height">Viewport height</param>
    /// <param name="view_matrix">Camera view matrix</param>
    /// <param name="proj_matrix">Camera projection matrix</param>
    /// <param name="mouse_button_down">Is mouse button pressed</param>
    /// <param name="imgui_wants_mouse">Does ImGui want to capture mouse</param>
    /// <returns>True if gizmo is being interacted with</returns>
    static bool HandleMouseInteraction(
        entt::registry& registry,
        float mouse_x, float mouse_y,
        int viewport_width, int viewport_height,
        const ozz::math::Float4x4& view_matrix,
        const ozz::math::Float4x4& proj_matrix,
        bool mouse_button_down,
        bool imgui_wants_mouse);

    /// <summary>
    /// Render gizmos for all entities (call before scene rendering)
    /// </summary>
    /// <param name="registry">ECS registry</param>
    /// <param name="debug_renderer">Debug renderer for drawing</param>
    static void RenderGizmos(entt::registry& registry, void* debug_renderer);
};

} // namespace AnimationECS
