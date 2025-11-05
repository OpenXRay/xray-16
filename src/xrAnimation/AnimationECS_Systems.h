#pragma once

#include "AnimationECS_Components.h"
#include "AnimationECS_IK.h"
#include "entt/entt.hpp"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/base/span.h"

namespace AnimationECS {

//-----------------------------------------------------------------------------
// AnimationSamplingSystem
// Samples animations and produces local-space transforms
// Can run in parallel (each entity independent)
//-----------------------------------------------------------------------------
class AnimationSamplingSystem
{
public:
    static void Update(entt::registry& registry, float dt)
    {
        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();

        for (auto entity : view)
        {
            auto& state = view.get<AnimationState>(entity);
            auto& controller = view.get<AnimationController>(entity);
            auto& buffers = view.get<AnimationBuffers>(entity);

            // Skip if not playing or no animation
            if (!state.is_playing || !controller.animation || !buffers.IsInitialized())
                continue;

            // Update time
            state.current_time += dt * controller.playback_speed;

            // Calculate time ratio
            const float duration = controller.animation->duration();
            if (duration > 0.0f)
            {
                state.time_ratio = state.current_time / duration;

                // Handle looping
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

            // Sample animation
            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = controller.animation;
            sampling_job.context = &buffers.context;
            sampling_job.ratio = state.time_ratio;
            sampling_job.output = ozz::make_span(buffers.locals);

            if (!sampling_job.Run())
            {
                Msg("! [AnimationECS] SamplingJob failed for entity %u", static_cast<u32>(entity));
            }
        }
    }
};

//-----------------------------------------------------------------------------
// AnimationBlendingSystem
// Blends multiple animations together
// Can run in parallel (each entity independent)
//-----------------------------------------------------------------------------
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

            // Skip if no layers to blend
            if (!blend_state.HasLayers() || !controller.skeleton)
                continue;

            // Setup blending job
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

//-----------------------------------------------------------------------------
// LocalToModelSystem
// Converts local-space transforms to model-space matrices
// Can run in parallel (each entity independent)
//-----------------------------------------------------------------------------
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

            // Skip if no skeleton or uninitialized
            if (!controller.skeleton || !buffers.IsInitialized())
                continue;

            // Setup local-to-model job
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

//-----------------------------------------------------------------------------
// AnimationCallbackSystem
// Processes animation callbacks (must run on main thread)
// Sequential only (callbacks may have side effects)
//-----------------------------------------------------------------------------
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

            // Check for animation end
            if (!state.is_playing && state.time_ratio >= 1.0f)
            {
                callbacks.InvokeOnEnd();
            }

            // Check for loop point
            // (could add more sophisticated callback handling here)
        }
    }
};

//-----------------------------------------------------------------------------
// AnimationUpdateOrchestrator
// Coordinates all animation systems in correct order
//-----------------------------------------------------------------------------
class AnimationUpdateOrchestrator
{
public:
    static void Update(entt::registry& registry, float dt)
    {
        // Phase 1: Sample animations (parallel-friendly)
        AnimationSamplingSystem::Update(registry, dt);

        // Phase 2: Blend animations if needed (parallel-friendly)
        AnimationBlendingSystem::Update(registry, dt);

        // Phase 3: Convert to model space (parallel-friendly)
        LocalToModelSystem::Update(registry);

        // Phase 4: Apply IK (requires model space, parallel-friendly)
        IKSolverSystem::Update(registry);

        // Phase 5: Process callbacks (main thread only)
        AnimationCallbackSystem::Update(registry);
    }
};

} // namespace AnimationECS
