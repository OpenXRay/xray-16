#include "stdafx.h"

#include "IK.hpp"
#include "Components.hpp"
#include "Systems.hpp"

#include "xrECS/World.hpp"

#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/math_ex.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/simd_quaternion.h>
#include <ozz/base/maths/soa_float4x4.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

#include <algorithm>
#include <cstring>
#include <initializer_list>

namespace xray::animation {

namespace {

void MultiplySoATransformQuaternion(int index,
                                    const ozz::math::SimdQuaternion& quat,
                                    const ozz::span<ozz::math::SoaTransform>& transforms)
{
    ozz::math::SoaTransform& soa = transforms[index / 4];
    ozz::math::SimdQuaternion aos[4];
    ozz::math::Transpose4x4(&soa.rotation.x, &aos->xyzw);

    ozz::math::SimdQuaternion& slot = aos[index & 3];
    slot = slot * quat;

    ozz::math::Transpose4x4(&aos->xyzw, &soa.rotation.x);
}

ozz::math::Float3 ExtractTranslation(const ozz::math::Float4x4& mat)
{
    return ozz::math::Float3{
        ozz::math::GetX(mat.cols[3]),
        ozz::math::GetY(mat.cols[3]),
        ozz::math::GetZ(mat.cols[3])
    };
}

int FindJointIndexByNames(ozz::span<const char* const> joint_names,
                          std::initializer_list<const char*> candidates)
{
    for (const char* candidate : candidates)
    {
        for (int i = 0; i < static_cast<int>(joint_names.size()); ++i)
        {
            if (std::strcmp(joint_names[i], candidate) == 0)
                return i;
        }
    }
    return -1;
}

void ResolveChainAxes(xray::ecs::LimbIKChain& chain,
                      ozz::span<const ozz::math::Float4x4> bind_pose_models)
{
    chain.mid_axis    = ozz::math::simd_float4::z_axis();
    chain.pole_vector = ozz::math::simd_float4::y_axis();

    if (chain.start < 0 || chain.mid < 0 || chain.end < 0)
        return;
    if (static_cast<size_t>(chain.mid) >= bind_pose_models.size())
        return;

    const ozz::math::SimdFloat4 min_len = ozz::math::simd_float4::Load1(1e-6f);

    ozz::math::SimdFloat4 axis_candidate = bind_pose_models[chain.mid].cols[2];
    if (ozz::math::AreAllTrue1(ozz::math::CmpGt(ozz::math::Length3Sqr(axis_candidate), min_len)))
    {
        chain.mid_axis = ozz::math::Normalize3(axis_candidate);
        if (chain.role == xray::ecs::LimbIKChain::Role::Leg)
            chain.mid_axis = -chain.mid_axis;
    }

    if (static_cast<size_t>(chain.start) < bind_pose_models.size() &&
        static_cast<size_t>(chain.end)   < bind_pose_models.size())
    {
        const ozz::math::SimdFloat4 start_pos = bind_pose_models[chain.start].cols[3];
        const ozz::math::SimdFloat4 mid_pos   = bind_pose_models[chain.mid].cols[3];
        const ozz::math::SimdFloat4 end_pos   = bind_pose_models[chain.end].cols[3];

        const ozz::math::SimdFloat4 start_to_mid = mid_pos - start_pos;
        const ozz::math::SimdFloat4 start_to_end = end_pos - start_pos;

        ozz::math::SimdFloat4 pole_candidate = (chain.role == xray::ecs::LimbIKChain::Role::Arm)
            ? ozz::math::Cross3(start_to_end, start_to_mid)
            : ozz::math::Cross3(start_to_mid, start_to_end);

        if (ozz::math::AreAllTrue1(ozz::math::CmpGt(ozz::math::Length3Sqr(pole_candidate), min_len)))
        {
            chain.pole_vector = ozz::math::Normalize3(pole_candidate);
        }
        else
        {
            ozz::math::SimdFloat4 y_candidate = bind_pose_models[chain.mid].cols[1];
            if (ozz::math::AreAllTrue1(ozz::math::CmpGt(ozz::math::Length3Sqr(y_candidate), min_len)))
                chain.pole_vector = ozz::math::Normalize3(y_candidate);
        }
    }
}

void PopulateChain(xray::ecs::LimbIKChain& chain,
                   xray::ecs::LimbIKChain::Role role,
                   ozz::span<const char* const> joint_names,
                   ozz::span<const ozz::math::Float4x4> bind_pose_models,
                   std::initializer_list<const char*> start_candidates,
                   std::initializer_list<const char*> mid_candidates,
                   std::initializer_list<const char*> end_candidates)
{
    chain.role  = role;
    chain.start = static_cast<s16>(FindJointIndexByNames(joint_names, start_candidates));
    chain.mid   = static_cast<s16>(FindJointIndexByNames(joint_names, mid_candidates));
    chain.end   = static_cast<s16>(FindJointIndexByNames(joint_names, end_candidates));
    chain.target_offset = { 0.f, 0.f, 0.f };
    ResolveChainAxes(chain, bind_pose_models);
}

bool ChainValid(const xray::ecs::LimbIKChain& chain)
{
    return chain.start >= 0 && chain.mid >= 0 && chain.end >= 0;
}

ozz::math::Float3 ComputeChainTarget(const xray::ecs::LimbIKChain& chain,
                                     ozz::span<const ozz::math::Float4x4> models,
                                     float foot_ground_height,
                                     float crouch_offset,
                                     bool crouch_affects_arms)
{
    ozz::math::Float3 target = ExtractTranslation(models[chain.end]);
    if (chain.role == xray::ecs::LimbIKChain::Role::Leg)
    {
        target.x += chain.target_offset.x;
        target.z += chain.target_offset.z;
        target.y  = foot_ground_height + chain.target_offset.y;
    }
    else
    {
        target.x += chain.target_offset.x;
        target.y += chain.target_offset.y;
        target.z += chain.target_offset.z;
        if (crouch_affects_arms)
            target.y -= crouch_offset;
    }
    return target;
}

bool SolveLimb(const xray::ecs::LimbIKChain& chain,
               const ozz::math::Float3& target_model,
               float weight, float soften, float twist_angle,
               ozz::span<const ozz::math::Float4x4> models,
               ozz::span<ozz::math::SoaTransform> locals,
               int* min_dirty_joint)
{
    if (weight < 1e-4f)
        return false;

    ozz::animation::IKTwoBoneJob job;
    job.target      = ozz::math::simd_float4::Load3PtrU(&target_model.x);
    job.pole_vector = chain.pole_vector;
    job.mid_axis    = chain.mid_axis;
    job.weight      = std::clamp(weight, 0.f, 1.f);
    job.soften      = std::clamp(soften, 0.f, 0.999f);
    job.twist_angle = twist_angle;
    job.start_joint = &models[chain.start];
    job.mid_joint   = &models[chain.mid];
    job.end_joint   = &models[chain.end];

    ozz::math::SimdQuaternion start_correction;
    ozz::math::SimdQuaternion mid_correction;
    job.start_joint_correction = &start_correction;
    job.mid_joint_correction   = &mid_correction;

    bool reached = false;
    job.reached = &reached;

    if (!job.Run())
        return false;

    MultiplySoATransformQuaternion(chain.start, start_correction, locals);
    MultiplySoATransformQuaternion(chain.mid,   mid_correction,   locals);

    if (min_dirty_joint)
        *min_dirty_joint = std::min<int>(*min_dirty_joint, static_cast<int>(chain.start));

    return true;
}

void SolveAndRebuild(const xray::ecs::LimbIKChain& chain,
                     float weight, float soften, float twist_angle,
                     float foot_ground_height,
                     float crouch_offset,
                     bool  crouch_affects_arms,
                     const ozz::animation::Skeleton* skeleton,
                     xray::ecs::AnimationBuffers& bufs)
{
    if (!ChainValid(chain))
        return;
    if (static_cast<size_t>(chain.end) >= bufs.models.size())
        return;

    const ozz::math::Float3 target = ComputeChainTarget(
        chain, ozz::make_span(bufs.models),
        foot_ground_height, crouch_offset, crouch_affects_arms);

    int chain_dirty = static_cast<int>(bufs.models.size());
    if (!SolveLimb(chain, target, weight, soften, twist_angle,
                   ozz::make_span(bufs.models),
                   ozz::make_span(bufs.locals),
                   &chain_dirty))
        return;

    if (chain_dirty < static_cast<int>(bufs.models.size()))
    {
        ozz::animation::LocalToModelJob ltm;
        ltm.skeleton = skeleton;
        ltm.from     = chain_dirty;
        ltm.input    = ozz::make_span(bufs.locals);
        ltm.output   = ozz::make_span(bufs.models);
        R_ASSERT2(ltm.Run(), "ozz LocalToModelJob (IK rebuild) failed");
    }
}

}

void InitializeIK(xray::ecs::World& world,
                  entt::entity entity,
                  ozz::span<const char* const> joint_names,
                  ozz::span<const ozz::math::Float4x4> bind_pose_models)
{
    auto& reg = world.reg();
    auto& cfg = reg.emplace_or_replace<xray::ecs::IKConfiguration>(entity);

    cfg = xray::ecs::IKConfiguration{};

    PopulateChain(cfg.left_leg, xray::ecs::LimbIKChain::Role::Leg, joint_names, bind_pose_models,
        { "bip01_l_thigh", "l_thigh", "left_thigh" },
        { "bip01_l_calf",  "l_calf",  "left_calf",  "bip01_l_knee"  },
        { "bip01_l_foot",  "l_foot",  "left_foot",  "bip01_l_ankle" });

    PopulateChain(cfg.right_leg, xray::ecs::LimbIKChain::Role::Leg, joint_names, bind_pose_models,
        { "bip01_r_thigh", "r_thigh", "right_thigh" },
        { "bip01_r_calf",  "r_calf",  "right_calf",  "bip01_r_knee"  },
        { "bip01_r_foot",  "r_foot",  "right_foot",  "bip01_r_ankle" });

    PopulateChain(cfg.left_arm, xray::ecs::LimbIKChain::Role::Arm, joint_names, bind_pose_models,
        { "bip01_l_upperarm", "bip01_l_shoulder", "l_upperarm", "left_shoulder" },
        { "bip01_l_forearm",  "l_forearm",        "left_forearm", "bip01_l_elbow" },
        { "bip01_l_hand",     "l_hand",           "left_hand",    "bip01_l_wrist" });

    PopulateChain(cfg.right_arm, xray::ecs::LimbIKChain::Role::Arm, joint_names, bind_pose_models,
        { "bip01_r_upperarm", "bip01_r_shoulder", "r_upperarm", "right_shoulder" },
        { "bip01_r_forearm",  "r_forearm",        "right_forearm", "bip01_r_elbow" },
        { "bip01_r_hand",     "r_hand",           "right_hand",    "bip01_r_wrist" });

    if (ChainValid(cfg.left_leg) && ChainValid(cfg.right_leg) && !bind_pose_models.empty())
    {
        const float lx = ozz::math::GetX(bind_pose_models[cfg.left_leg.end].cols[3]);
        const float rx = ozz::math::GetX(bind_pose_models[cfg.right_leg.end].cols[3]);
        if (lx > rx)
            std::swap(cfg.left_leg, cfg.right_leg);
    }

    if (ChainValid(cfg.left_arm) && ChainValid(cfg.right_arm) && !bind_pose_models.empty())
    {
        const float lx = ozz::math::GetX(bind_pose_models[cfg.left_arm.end].cols[3]);
        const float rx = ozz::math::GetX(bind_pose_models[cfg.right_arm.end].cols[3]);
        if (lx > rx)
            std::swap(cfg.left_arm, cfg.right_arm);
    }
}

namespace systems {

void IK(xray::ecs::World& world)
{
    auto& reg = world.reg();
    auto view = reg.view<xray::ecs::IKConfiguration,
                         xray::ecs::AnimationController,
                         xray::ecs::AnimationBuffers,
                         xray::ecs::IKEnabled>();

    for (auto entity : view)
    {
        auto& cfg  = view.get<xray::ecs::IKConfiguration>(entity);
        auto& ctl  = view.get<xray::ecs::AnimationController>(entity);
        auto& bufs = view.get<xray::ecs::AnimationBuffers>(entity);

        SolveAndRebuild(cfg.left_leg,  cfg.leg_weight, cfg.leg_soften, cfg.leg_twist_angle,
                        cfg.foot_ground_height, cfg.crouch_offset, cfg.crouch_affects_arms,
                        ctl.skeleton, bufs);
        SolveAndRebuild(cfg.right_leg, cfg.leg_weight, cfg.leg_soften, cfg.leg_twist_angle,
                        cfg.foot_ground_height, cfg.crouch_offset, cfg.crouch_affects_arms,
                        ctl.skeleton, bufs);
        SolveAndRebuild(cfg.left_arm,  cfg.arm_weight, cfg.arm_soften, cfg.arm_twist_angle,
                        cfg.foot_ground_height, cfg.crouch_offset, cfg.crouch_affects_arms,
                        ctl.skeleton, bufs);
        SolveAndRebuild(cfg.right_arm, cfg.arm_weight, cfg.arm_soften, cfg.arm_twist_angle,
                        cfg.foot_ground_height, cfg.crouch_offset, cfg.crouch_affects_arms,
                        ctl.skeleton, bufs);
    }
}

}

}
