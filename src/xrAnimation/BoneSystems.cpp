#include "stdafx.h"

#include "BoneSystems.hpp"
#include "Components.hpp"
#include "OzzConversion.h"

#include "xrECS/Components.hpp"
#include "xrECS/World.hpp"

#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

namespace xray::animation::bone_systems {

namespace {

ozz::math::SimdFloat4 PatchLane(ozz::math::SimdFloat4 v, int lane, float value)
{
    float f[4] = {
        ozz::math::GetX(v), ozz::math::GetY(v),
        ozz::math::GetZ(v), ozz::math::GetW(v)
    };
    f[lane] = value;
    return ozz::math::simd_float4::Load(f[0], f[1], f[2], f[3]);
}

void SetSoATransformIdentity(int joint_index, ozz::span<ozz::math::SoaTransform> transforms)
{
    const int soa_idx = joint_index / 4;
    const int lane    = joint_index & 3;
    if (soa_idx >= static_cast<int>(transforms.size()))
        return;

    ozz::math::SoaTransform& soa = transforms[soa_idx];
    soa.translation.x = PatchLane(soa.translation.x, lane, 0.f);
    soa.translation.y = PatchLane(soa.translation.y, lane, 0.f);
    soa.translation.z = PatchLane(soa.translation.z, lane, 0.f);
    soa.rotation.x    = PatchLane(soa.rotation.x,    lane, 0.f);
    soa.rotation.y    = PatchLane(soa.rotation.y,    lane, 0.f);
    soa.rotation.z    = PatchLane(soa.rotation.z,    lane, 0.f);
    soa.rotation.w    = PatchLane(soa.rotation.w,    lane, 1.f);
    soa.scale.x       = PatchLane(soa.scale.x,       lane, 1.f);
    soa.scale.y       = PatchLane(soa.scale.y,       lane, 1.f);
    soa.scale.z       = PatchLane(soa.scale.z,       lane, 1.f);
}

}

void ApplyLocalOverrides(xray::ecs::World& world)
{
    auto view = world.reg().view<xray::ecs::AnimationBuffers,
                                 xray::ecs::CForceIdentityJoints>();

    for (auto entity : view)
    {
        auto& bufs      = view.get<xray::ecs::AnimationBuffers>(entity);
        const auto& ids = view.get<xray::ecs::CForceIdentityJoints>(entity);

        if (bufs.locals.empty())
            continue;

        for (int joint : ids.joints)
            SetSoATransformIdentity(joint, ozz::make_span(bufs.locals));
    }
}

void ApplyBoneVisibility(xray::ecs::World& world)
{
    auto view = world.reg().view<xray::ecs::AnimationBuffers, xray::ecs::CBoneVisibility>();

    for (auto entity : view)
    {
        auto& bufs = view.get<xray::ecs::AnimationBuffers>(entity);
        const auto& vis = view.get<xray::ecs::CBoneVisibility>(entity);

        if (bufs.models.empty())
            continue;

        const ozz::math::Float4x4 zero = {{
            ozz::math::simd_float4::zero(),
            ozz::math::simd_float4::zero(),
            ozz::math::simd_float4::zero(),
            ozz::math::simd_float4::zero()
        }};
        const int count = static_cast<int>(bufs.models.size());
        for (int i = 0; i < count && i < 64; ++i)
        {
            if (!vis.get(static_cast<u16>(i)))
                bufs.models[i] = zero;
        }
    }
}

void UpdateSocketLocalTransforms(xray::ecs::World& world)
{
    auto& reg = world.reg();
    auto view = reg.view<xray::ecs::CHudSocketAttachment, xray::ecs::CLocalTransform>();

    for (auto entity : view)
    {
        const auto& attachment = view.get<xray::ecs::CHudSocketAttachment>(entity);
        if (attachment.parent_skeleton == entt::null || attachment.anchor_bone < 0)
            continue;
        if (!reg.valid(attachment.parent_skeleton))
            continue;

        const auto* parent_bufs = reg.try_get<xray::ecs::AnimationBuffers>(attachment.parent_skeleton);
        if (!parent_bufs || parent_bufs->models.empty())
            continue;
        if (attachment.anchor_bone >= static_cast<int>(parent_bufs->models.size()))
            continue;

        const Fmatrix bone_model = XRay::Animation::ConvertOzzMatrixToXRay(
            parent_bufs->models[attachment.anchor_bone]);

        auto& local = view.get<xray::ecs::CLocalTransform>(entity);
        local.local.mul(bone_model, attachment.local_offset);
    }
}

}
