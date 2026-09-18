#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "Include/xrRender/animation_motion.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/simd_math.h"
#include "ExtendedBoneMetadata.h"
#include <vector>
#include <atomic>

namespace AnimationECS {
inline std::atomic<int> g_sampling_debug_frames{0};
inline void ResetSamplingDebug() { g_sampling_debug_frames.store(0); }
}

namespace XRay::Animation
{
class OzzKinematicsAnimated;
}

namespace AnimationECS {

struct AnimationState
{
    float current_time{0.0f};
    float time_ratio{0.0f};
    u16 current_partition{0};
    bool is_playing{false};
    bool is_looping{false};

    float blend_amount{1.0f};
    float blend_time{0.0f};
    float blend_duration{0.2f};
};

struct AnimationController
{
    const ozz::animation::Skeleton* skeleton{nullptr};
    const ozz::animation::Animation* animation{nullptr};

    shared_str skeleton_name;
    shared_str animation_name;

    float playback_speed{1.0f};

    void SetSkeleton(const ozz::animation::Skeleton* skel, const shared_str& name) {
        skeleton = skel;
        skeleton_name = name;
    }

    void SetAnimation(const ozz::animation::Animation* anim, const shared_str& name) {
        animation = anim;
        animation_name = name;
    }
};

struct AnimationBuffers
{
    ozz::animation::SamplingJob::Context context;

    ozz::vector<ozz::math::SoaTransform> locals;

    ozz::vector<ozz::math::Float4x4> models;

    void Initialize(const ozz::animation::Skeleton* skeleton)
    {
        if (!skeleton)
            return;

        const int num_joints = skeleton->num_joints();
        const int num_soa_joints = skeleton->num_soa_joints();

        locals.resize(num_soa_joints);
        models.resize(num_joints);
        context.Resize(num_joints);

        const auto rest = skeleton->joint_rest_poses();
        for (int i = 0; i < num_soa_joints; ++i)
            locals[i] = rest[i];
    }

    void Clear()
    {
        locals.clear();
        models.clear();
        context.Resize(0);
    }

    bool IsInitialized() const
    {
        return !locals.empty() && !models.empty();
    }
};

struct BlendState
{
    ozz::vector<ozz::animation::BlendingJob::Layer> layers;

    ozz::vector<ozz::vector<ozz::math::SoaTransform>> layer_storage;

    float transition_time{0.0f};
    float transition_duration{0.0f};

    void PrepareForLayers(size_t num_layers, size_t num_soa_joints)
    {
        layers.clear();
        layer_storage.clear();
        layer_storage.resize(num_layers);
        for (auto& storage : layer_storage)
        {
            storage.resize(num_soa_joints);
        }
    }

    ozz::span<ozz::math::SoaTransform> GetLayerStorage(size_t layer_index)
    {
        if (layer_index >= layer_storage.size())
            return {};
        return ozz::make_span(layer_storage[layer_index]);
    }

    void AddLayer(float weight, ozz::span<ozz::math::SoaTransform> transforms)
    {
        ozz::animation::BlendingJob::Layer layer;
        layer.transform = transforms;
        layer.weight = weight;
        layers.push_back(layer);
    }

    void AddLayer(const ozz::animation::BlendingJob::Layer& layer)
    {
        layers.push_back(layer);
    }

    void ClearLayers()
    {
        layers.clear();
        layer_storage.clear();
    }

    bool HasLayers() const
    {
        return !layers.empty();
    }
};

struct PartitionMasks
{
    xr_map<u16, ozz::vector<ozz::math::SimdFloat4>> masks;

    void SetMask(u16 partition_id, const ozz::vector<ozz::math::SimdFloat4>& mask)
    {
        masks[partition_id] = mask;
    }

    const ozz::vector<ozz::math::SimdFloat4>* GetMask(u16 partition_id) const
    {
        auto it = masks.find(partition_id);
        return (it != masks.end()) ? &it->second : nullptr;
    }

    void Clear()
    {
        masks.clear();
    }
};

struct AnimationCallbacks
{
    PlayCallback on_play{nullptr};
    PlayCallback on_end{nullptr};
    PlayCallback on_stop{nullptr};
    CBlend* callback_param{nullptr};

    void InvokeOnPlay()
    {
        if (on_play && callback_param)
            on_play(callback_param);
    }

    void InvokeOnEnd()
    {
        if (on_end && callback_param)
            on_end(callback_param);
    }

    void InvokeOnStop()
    {
        if (on_stop && callback_param)
            on_stop(callback_param);
    }
};

struct XRayCompatibility
{
    CBlend* blend_handle{nullptr};

    MotionID motion_id;

    u8 channel{0};
    u8 priority{0};

    bool accrue{false};
    bool falloff{true};
};

struct SkeletonMetadata
{
    std::vector<int> joint_parents;

    std::vector<std::vector<int>> joint_children;

    XRay::Animation::ExtendedBoneMetadataCollection metadata;

    const ozz::animation::Skeleton* skeleton{nullptr};

    bool IsValid() const
    {
        return !joint_parents.empty();
    }

    void Clear()
    {
        joint_parents.clear();
        joint_children.clear();
        metadata.clear();
        skeleton = nullptr;
    }

    void BuildChildrenMap()
    {
        joint_children.clear();
        joint_children.resize(joint_parents.size());

        for (size_t i = 0; i < joint_parents.size(); ++i)
        {
            int parent = joint_parents[i];
            if (parent >= 0 && static_cast<size_t>(parent) < joint_children.size())
            {
                joint_children[parent].push_back(static_cast<int>(i));
            }
        }
    }
};

struct AnimationGPUComputeState
{
    XRay::Animation::OzzKinematicsAnimated* runtime{nullptr};
    bool dirty{false};
    u32 gpu_character_idx{u32(-1)};
};

struct SpatialComponent
{
    Fmatrix world_transform;
    Fvector world_position;
    float bounding_radius{1.0f};
    bool is_visible_this_frame{true};
    float distance_to_camera{0.0f};
    float screen_space_area{0.0f};

    void UpdateFromTransform(const Fmatrix& transform)
    {
        world_transform = transform;
        world_position = transform.c;
    }

    void SetVisibility(bool visible, float distance = 0.0f, float ssa = 0.0f)
    {
        is_visible_this_frame = visible;
        distance_to_camera = distance;
        screen_space_area = ssa;
    }
};

}
