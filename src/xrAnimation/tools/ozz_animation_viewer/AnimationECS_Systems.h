#pragma once

#include "AnimationECS_Components.h"
#include "AnimationECS_IK.h"
#include "AnimationGPUComputeBridge.h"
#include "entt/entt.hpp"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/base/span.h"

namespace AnimationECS {

class AnimationSamplingSystem
{
public:
    static void Update(entt::registry& registry, float dt)
    {
        const bool debug_this_frame = (g_sampling_debug_frames.load() < 3);

        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();

        int total = 0, skipped_not_playing = 0, skipped_no_anim = 0, skipped_no_buf = 0, sampled = 0, failed = 0;

        for (auto entity : view)
        {
            auto& state = view.get<AnimationState>(entity);
            auto& controller = view.get<AnimationController>(entity);
            auto& buffers = view.get<AnimationBuffers>(entity);
            ++total;

            if (!state.is_playing) { ++skipped_not_playing; continue; }
            if (!controller.animation) { ++skipped_no_anim; continue; }
            if (!buffers.IsInitialized()) { ++skipped_no_buf; continue; }

            state.current_time += dt * controller.playback_speed;

            const float duration = controller.animation->duration();
            if (duration > 0.0f)
            {
                state.time_ratio = state.current_time / duration;

                if (state.time_ratio > 1.0f)
                {
                    if (state.is_looping)
                    {
                        state.current_time = fmodf(state.current_time, duration);
                        state.time_ratio = fmodf(state.time_ratio, 1.0f);
                    }
                    else
                    {
                        state.current_time = duration;
                        state.time_ratio = 1.0f;
                        state.is_playing = false;
                    }
                }
            }

            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = controller.animation;
            sampling_job.context = &buffers.context;
            sampling_job.ratio = state.time_ratio;
            sampling_job.output = ozz::make_span(buffers.locals);

            if (!sampling_job.Run())
            {
                ++failed;
                if (debug_this_frame) {
                    Msg("! [AnimationECS] SamplingJob FAILED entity %u: anim_tracks=%d context_max_tracks=%d locals=%zu ratio=%.3f duration=%.3f speed=%.3f",
                        static_cast<u32>(entity),
                        controller.animation->num_tracks(),
                        buffers.context.max_tracks(),
                        buffers.locals.size(),
                        state.time_ratio, duration, controller.playback_speed);
                }
            }
            else
            {
                ++sampled;
            }
        }

        if (debug_this_frame) {
            Msg("[DEBUG] Sampling: total=%d sampled=%d failed=%d skip(not_playing=%d no_anim=%d no_buf=%d) dt=%.4f",
                total, sampled, failed, skipped_not_playing, skipped_no_anim, skipped_no_buf, dt);
            g_sampling_debug_frames.fetch_add(1);
        }
    }
};

class AnimationBlendingSystem
{
public:
    static void Update(entt::registry& registry, float dt)
    {
        auto view = registry.view<AnimationBuffers, BlendState, AnimationController>();

        for (auto entity : view)
        {
            auto& buffers = view.get<AnimationBuffers>(entity);
            auto& blend_state = view.get<BlendState>(entity);
            auto& controller = view.get<AnimationController>(entity);

            if (!blend_state.HasLayers() || !controller.skeleton)
                continue;

            ozz::animation::BlendingJob blend_job;
            blend_job.layers = ozz::make_span(blend_state.layers);
            blend_job.rest_pose = controller.skeleton->joint_rest_poses();
            blend_job.output = ozz::make_span(buffers.locals);

            if (!blend_job.Run())
            {
                Msg("! [AnimationECS] BlendingJob failed for entity %u", static_cast<u32>(entity));
            }
        }
    }
};

class LocalToModelSystem
{
public:
    static void Update(entt::registry& registry)
    {
        auto view = registry.view<AnimationController, AnimationBuffers>();

        for (auto entity : view)
        {
            auto& controller = view.get<AnimationController>(entity);
            auto& buffers = view.get<AnimationBuffers>(entity);

            if (!controller.skeleton || !buffers.IsInitialized())
                continue;

            ozz::animation::LocalToModelJob ltm_job;
            ltm_job.skeleton = controller.skeleton;
            ltm_job.input = ozz::make_span(buffers.locals);
            ltm_job.output = ozz::make_span(buffers.models);

            if (!ltm_job.Run())
            {
                Msg("! [AnimationECS] LocalToModelJob failed for entity %u", static_cast<u32>(entity));
            }
        }
    }
};

class AnimationGPUComputeSystem
{
public:
    static void Update(entt::registry& registry, float dt)
    {
        XRay::Animation::GPUComputeBridge::BeginFrame(dt);

        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers, AnimationGPUComputeState>();

        for (auto entity : view)
        {
            auto& state = view.get<AnimationState>(entity);
            auto& controller = view.get<AnimationController>(entity);
            auto& buffers = view.get<AnimationBuffers>(entity);
            auto& gpu_state = view.get<AnimationGPUComputeState>(entity);

            gpu_state.dirty = true;
            XRay::Animation::GPUComputeBridge::SubmitCharacter(state, controller, buffers, gpu_state);
        }
    }
};

class AnimationCallbackSystem
{
public:
    static void Update(entt::registry& registry)
    {
        auto view = registry.view<AnimationState, AnimationCallbacks>();

        for (auto entity : view)
        {
            auto& state = view.get<AnimationState>(entity);
            auto& callbacks = view.get<AnimationCallbacks>(entity);

            if (!state.is_playing && state.time_ratio >= 1.0f)
            {
                callbacks.InvokeOnEnd();
            }

        }
    }
};

class AnimationUpdateOrchestrator
{
public:
    static void Update(entt::registry& registry, float dt)
    {
        AnimationSamplingSystem::Update(registry, dt);

        AnimationBlendingSystem::Update(registry, dt);

        LocalToModelSystem::Update(registry);

        AnimationGPUComputeSystem::Update(registry, dt);

        IKSolverSystem::Update(registry);

        AnimationCallbackSystem::Update(registry);
    }
};

}
