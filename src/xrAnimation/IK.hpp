#pragma once

#include "xrCore/xrCore.h"

#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_float4x4.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/span.h>

#include <entt/entt.hpp>

namespace xray::ecs { class World; }

namespace xray::ecs {

struct LimbIKChain
{
    enum class Role : u8 { Leg = 0, Arm = 1 };

    Role role = Role::Leg;
    s16 start = -1;
    s16 mid   = -1;
    s16 end   = -1;

    ozz::math::SimdFloat4 mid_axis;
    ozz::math::SimdFloat4 pole_vector;

    ozz::math::Float3 target_offset{0.f, 0.f, 0.f};
};

struct IKConfiguration
{
    LimbIKChain left_leg;
    LimbIKChain right_leg;
    LimbIKChain left_arm;
    LimbIKChain right_arm;

    float leg_weight       = 1.0f;
    float leg_soften       = 0.97f;
    float leg_twist_angle  = 0.0f;

    float arm_weight       = 1.0f;
    float arm_soften       = 0.85f;
    float arm_twist_angle  = 0.0f;

    float foot_ground_height  = 0.0f;
    float crouch_offset       = 0.0f;
    bool  crouch_affects_arms = false;
};

struct IKEnabled {};

}

namespace xray::animation {

void InitializeIK(xray::ecs::World& world,
                  entt::entity entity,
                  ozz::span<const char* const> joint_names,
                  ozz::span<const ozz::math::Float4x4> bind_pose_models);

}
