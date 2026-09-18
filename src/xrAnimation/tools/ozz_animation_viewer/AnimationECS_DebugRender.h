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

class SkeletonDebugRenderSystem
{
public:
    static void Render(entt::registry& registry, IDebugDrawContext& ctx);

    static void UpdateGlobalSettings(entt::registry& registry, bool show_skeleton_lines);

private:
    static void RenderSkeleton(
        IDebugDrawContext& ctx,
        const std::vector<ozz::math::Float4x4>& pose_models,
        const SkeletonMetadata& skeleton_metadata,
        const ozz::math::Float4x4& instance_transform,
        const SkeletonDebugState& debug_state);

    static float ComputeRestLength(
        int bone_index,
        const std::vector<ozz::math::Float4x4>& rest_models,
        const std::vector<int>& skeleton_parents);

    static ozz::math::Float3 ExtractTranslation(const ozz::math::Float4x4& mat);

    static float DistanceBetween(const ozz::math::Float4x4& a, const ozz::math::Float4x4& b);
};

class IKGizmoRenderSystem
{
public:
    static void Render(entt::registry& registry, IDebugDrawContext& ctx);

private:
    static void RenderGizmo(
        IDebugDrawContext& ctx,
        const IKGizmoState::ChainGizmo& gizmo,
        const LimbIKChain& chain,
        const ozz::math::Float4x4& instance_transform,
        bool is_leg);

    static ozz::math::Float3 TransformPoint(
        const ozz::math::Float4x4& transform,
        const ozz::math::Float3& point);
};

}
