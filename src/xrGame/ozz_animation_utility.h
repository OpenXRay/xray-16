#pragma once

#include "xrEngine/editor_base.h"
#include "xrAnimation/OzzKinematicsAnimated.h"
#include "Include/xrRender/animation_motion.h"

typedef XRay::Animation::OzzKinematicsAnimated OzzKinematicsAnimated;

class COzzAnimationUtility final : public xray::editor::ide_tool
{
public:
    COzzAnimationUtility();
    void on_tool_frame() override;
    bool is_active() const override;

private:
    pcstr tool_name() const override { return "Ozz Animation Utility"; }

    // Forward declare structs first
    // Animation playback state
    struct PlaybackState
    {
        bool playing{ false };
        bool loop{ true };
        float speed{ 1.0f };
        float time_ratio{ 0.0f };
        int current_animation_index{ -1 };
    };

    // Skeleton debug state
    struct SkeletonDebugState
    {
        bool draw_skeleton{ true };
        bool draw_bone_names{ false };
        bool draw_bone_axes{ false };
        int bone_display_limit{ 64 };
        xr_vector<bool> bone_visibility;
    };

    // Blending state
    struct BlendingState
    {
        bool show_blend_panel{ true };
        float blend_weight{ 1.0f };
        int blend_animation_index{ -1 };
        float blend_transition_time{ 0.3f };
    };

    // IK state
    struct IKState
    {
        bool enable_foot_ik{ false };
        float ground_height{ 0.0f };
        float ik_weight{ 1.0f };
        float ik_soften{ 0.5f };
        float twist_angle_deg{ 0.0f };

        bool enable_arm_ik{ false };
        float arm_ik_weight{ 1.0f };
        float arm_ik_soften{ 0.5f };
        float arm_twist_angle_deg{ 0.0f };
    };

    // HUD item tracking
    struct HudItemState
    {
        OzzKinematicsAnimated* anim_object{ nullptr };
        xr_vector<shared_str> available_animations;
        PlaybackState playback;
        SkeletonDebugState skeleton_debug;
        shared_str display_name;
    };

    // Function declarations (after struct definitions)
    void DrawAnimationPanel();
    void DrawPlaybackControls();
    void DrawBlendingControls();
    void DrawIKControls();
    void DrawSkeletonDebug();

    // HUD mode drawing
    void DrawHudModeUI();
    void DrawAnimatedObjectPanel(const char* object_name, OzzKinematicsAnimated* anim_obj,
                                  xr_vector<shared_str>& animations, PlaybackState& playback,
                                  SkeletonDebugState& skeleton_debug);

    // Get current animated object
    OzzKinematicsAnimated* GetCurrentAnimatedObject();

    // Check if we're in HUD mode
    bool IsInHudMode() const;

    // UI state
    bool paused_{ false };
    bool is_hud_mode_{ false };

    // Third-person mode state
    PlaybackState playback_;
    SkeletonDebugState skeleton_debug_;
    BlendingState blending_;
    IKState ik_;
    OzzKinematicsAnimated* current_object_{ nullptr };
    xr_vector<shared_str> available_animations_;

    // HUD mode state
    HudItemState hud_hands_;           // HUD arms model
    HudItemState hud_item_slot0_;      // Item in slot 0 (usually right hand)
    HudItemState hud_item_slot1_;      // Item in slot 1 (usually left hand)

    u32 last_update_frame_{ 0 };
};
