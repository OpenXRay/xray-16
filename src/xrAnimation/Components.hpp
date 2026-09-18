#pragma once

#include "xrCore/xrCore.h"
#include "AnimationRequest.hpp"

#include <array>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

#include <algorithm>
#include <cmath>

namespace xray::ecs {

struct AnimationController
{
    const ozz::animation::Skeleton*  skeleton  = nullptr;
    const ozz::animation::Animation* animation = nullptr;
    float playback_speed = 1.0f;
};

struct AnimationState
{
    float current_time = 0.0f;
    float time_ratio   = 0.0f;
};

struct AnimationBuffers
{
    ozz::animation::SamplingJob::Context           context;
    ozz::vector<ozz::math::SoaTransform>           locals;
    ozz::vector<ozz::math::Float4x4>               models;
};

struct BlendState
{
    ozz::vector<ozz::animation::BlendingJob::Layer>      layers;
    ozz::vector<ozz::vector<ozz::math::SoaTransform>>    layer_storage;
};

struct Playing {};
struct Looping {};

enum class BodyChannel : u8 { Legs = 0, Torso = 1, Head = 2, Count = 3 };
constexpr int kBodyChannelCount = static_cast<int>(BodyChannel::Count);

struct ChannelState
{
    const ozz::animation::Animation* animation{nullptr};
    std::unique_ptr<ozz::animation::SamplingJob::Context> context;
    ozz::vector<ozz::math::SoaTransform> locals;
    float currentTime{0.0f};
    float timeRatio{0.0f};
    float playbackSpeed{1.0f};
    bool isPlaying{false};
    bool isLooping{true};

    void Initialize(int numJoints, int numSoaJoints)
    {
        locals.resize(numSoaJoints);
        context = std::make_unique<ozz::animation::SamplingJob::Context>(numJoints);
    }

    bool IsInitialized() const { return !locals.empty() && context; }

    void SetAnimation(const ozz::animation::Animation* anim, float speed = 1.0f, bool loop = true)
    {
        if (animation == anim)
            return;
        animation = anim;
        playbackSpeed = speed;
        currentTime = 0.0f;
        timeRatio = 0.0f;
        isPlaying = (anim != nullptr);
        isLooping = loop;
    }

    void AdvanceTime(float dt)
    {
        if (!isPlaying || !animation)
            return;
        currentTime += dt * playbackSpeed;
        const float duration = animation->duration();
        if (duration <= 0.0f)
            return;
        timeRatio = currentTime / duration;
        if (timeRatio > 1.0f) {
            if (isLooping) {
                currentTime = fmodf(currentTime, duration);
                timeRatio = fmodf(timeRatio, 1.0f);
            } else {
                currentTime = duration;
                timeRatio = 1.0f;
                isPlaying = false;
            }
        }
    }

    bool Sample()
    {
        if (!isPlaying || !animation || locals.empty())
            return false;
        ozz::animation::SamplingJob job;
        job.animation = animation;
        job.context = context.get();
        job.ratio = timeRatio;
        job.output = ozz::make_span(locals);
        return job.Run();
    }

    bool Finished() const { return !isPlaying && timeRatio >= 1.0f; }
};

struct MultiChannelAnimator
{
    static constexpr int kChannelCount = kBodyChannelCount;

    std::array<ChannelState, kChannelCount> channels;
    std::array<ozz::vector<ozz::math::SimdFloat4>, kChannelCount> jointWeights;

    int torsoRootJoint{-1};
    int headRootJoint{-1};
    bool initialized{false};

    void Initialize(const ozz::animation::Skeleton* skeleton)
    {
        const int nj = skeleton->num_joints();
        const int nsj = skeleton->num_soa_joints();

        for (int ch = 0; ch < kChannelCount; ++ch) {
            channels[ch].Initialize(nj, nsj);
            jointWeights[ch].resize(nsj);
        }

        const auto restPose = skeleton->joint_rest_poses();
        for (int ch = 0; ch < kChannelCount; ++ch)
            for (int i = 0; i < nsj; ++i)
                channels[ch].locals[i] = restPose[i];

        static const char* spineNames[] = {"bip01_spine", "bip01_spine1", "spine", "Spine1", nullptr};
        static const char* headNames[] = {"bip01_head", "bip01_neck", "head", "Head", nullptr};

        for (const char** n = spineNames; *n && torsoRootJoint < 0; ++n)
            torsoRootJoint = ozz::animation::FindJoint(*skeleton, *n);
        for (const char** n = headNames; *n && headRootJoint < 0; ++n)
            headRootJoint = ozz::animation::FindJoint(*skeleton, *n);

        BuildPartitionMasks(*skeleton);
        initialized = true;
    }

    void BuildPartitionMasks(const ozz::animation::Skeleton& skeleton)
    {
        const int nsj = skeleton.num_soa_joints();

        for (int i = 0; i < nsj; ++i)
            jointWeights[static_cast<int>(BodyChannel::Legs)][i] = ozz::math::simd_float4::one();
        for (int i = 0; i < nsj; ++i)
            jointWeights[static_cast<int>(BodyChannel::Torso)][i] = ozz::math::simd_float4::zero();
        for (int i = 0; i < nsj; ++i)
            jointWeights[static_cast<int>(BodyChannel::Head)][i] = ozz::math::simd_float4::zero();

        auto setWeight = [](ozz::vector<ozz::math::SimdFloat4>* w, float val) {
            return [w, val](int joint, int) {
                auto& soa = w->at(joint / 4);
                soa = ozz::math::SetI(soa, ozz::math::simd_float4::Load1(val), joint % 4);
            };
        };

        if (torsoRootJoint >= 0) {
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Legs)], 0.0f), torsoRootJoint);
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Torso)], 1.0f), torsoRootJoint);
        }

        if (headRootJoint >= 0) {
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Torso)], 0.0f), headRootJoint);
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Head)], 1.0f), headRootJoint);
        }
    }
};

struct CrossfadeState
{
    static constexpr int kChannelCount = kBodyChannelCount;
    static constexpr float kDefaultDuration = 0.15f;

    ozz::vector<ozz::math::SoaTransform> frozenLocals[kChannelCount];
    ozz::vector<ozz::math::SoaTransform> blendTemp;
    float fadeTime[kChannelCount]{};
    float fadeDuration{kDefaultDuration};
    bool active[kChannelCount]{};

    void Initialize(int numSoaJoints)
    {
        for (int ch = 0; ch < kChannelCount; ++ch)
            frozenLocals[ch].resize(numSoaJoints);
        blendTemp.resize(numSoaJoints);
    }

    void Snapshot(int channel, const ozz::vector<ozz::math::SoaTransform>& src)
    {
        const size_t count = std::min(frozenLocals[channel].size(), src.size());
        std::copy(src.begin(), src.begin() + count, frozenLocals[channel].begin());
        fadeTime[channel] = 0.0f;
        active[channel] = true;
    }

    bool IsFading(int ch) const { return active[ch] && fadeTime[ch] < fadeDuration; }
    float NewWeight(int ch) const { return active[ch] ? std::min(fadeTime[ch] / fadeDuration, 1.0f) : 1.0f; }
};

}
