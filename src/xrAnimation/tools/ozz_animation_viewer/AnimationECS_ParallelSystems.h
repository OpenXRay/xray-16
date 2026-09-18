#pragma once

#include "AnimationECS_Components.h"
#include "AnimationECS_Systems.h"
#include "AnimationECS_IK.h"
#include "AnimationECS_Performance.h"
#include "AnimationECS_MovementStateMachine.h"
#include "entt/entt.hpp"
#include "xrCore/Threading/ParallelForEach.hpp"

#if defined(_MSC_VER) || (defined(__cpp_lib_execution) && __cpp_lib_execution >= 201603L)
#include <execution>
#define OZZ_HAS_STD_EXECUTION 1
#else
#define OZZ_HAS_STD_EXECUTION 0
#endif
#include <algorithm>

namespace AnimationECS {

enum class ParallelImplementation
{
    XRayTaskSystem,
    StdExecution
};

inline ParallelImplementation g_parallel_implementation = ParallelImplementation::XRayTaskSystem;

class ParallelAnimationProcessor
{
private:
    xr_vector<entt::entity> m_entity_cache;

    ParallelAnimationProcessor() = default;
    ~ParallelAnimationProcessor() = default;
    ParallelAnimationProcessor(const ParallelAnimationProcessor&) = delete;
    ParallelAnimationProcessor& operator=(const ParallelAnimationProcessor&) = delete;

public:
    static ParallelAnimationProcessor& Instance()
    {
        static ParallelAnimationProcessor instance;
        return instance;
    }

    void UpdateParallel(entt::registry& registry, float dt)
    {
        auto& profiler = GetPerformanceProfiler();
        PerformanceTimer total_timer;

        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();

        m_entity_cache.clear();
        m_entity_cache.reserve(view.size_hint());

        for (auto entity : view)
        {
            m_entity_cache.push_back(entity);
        }

        if (profiler.IsEnabled())
        {
            profiler.GetStats().total_entities += m_entity_cache.size();
            profiler.GetStats().parallel_mode = true;
        }

        if (m_entity_cache.empty())
        {
            if (profiler.IsEnabled())
            {
                profiler.GetStats().total_update_time += total_timer.ElapsedMs();
            }
            return;
        }

        PerformanceTimer combined_timer;

        auto combined_processing_lambda = [&, dt](entt::entity entity)
        {
            auto& state = view.get<AnimationState>(entity);
            auto& controller = view.get<AnimationController>(entity);
            auto& buffers = view.get<AnimationBuffers>(entity);

            if (!controller.skeleton || !buffers.IsInitialized())
                return;

            auto* multiChannel = registry.try_get<MultiChannelAnimator>(entity);
            if (multiChannel && multiChannel->initialized) {
                auto* crossfade = registry.try_get<CrossfadeState>(entity);
                MultiChannelSamplingSystem::ProcessEntity(
                    *multiChannel, crossfade, controller.skeleton, buffers, dt);
            }
            else if (state.is_playing && controller.animation)
            {
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
                if (!sampling_job.Run()) {
                    Msg("! [ParallelECS] SamplingJob FAILED entity %u: anim_tracks=%d ctx_max=%d locals=%zu ratio=%.3f dur=%.3f speed=%.3f",
                        static_cast<u32>(entity), controller.animation->num_tracks(),
                        buffers.context.max_tracks(), buffers.locals.size(),
                        state.time_ratio, duration, controller.playback_speed);
                }

                auto* blend_state = registry.try_get<BlendState>(entity);
                if (blend_state && blend_state->HasLayers())
                {
                    ozz::animation::BlendingJob blend_job;
                    blend_job.layers = ozz::make_span(blend_state->layers);
                    blend_job.rest_pose = controller.skeleton->joint_rest_poses();
                    blend_job.output = ozz::make_span(buffers.locals);
                    blend_job.Run();
                }
            }

            ozz::animation::LocalToModelJob ltm_job;
            ltm_job.skeleton = controller.skeleton;
            ltm_job.input = ozz::make_span(buffers.locals);
            ltm_job.output = ozz::make_span(buffers.models);
            ltm_job.Run();
        };

        if (g_parallel_implementation == ParallelImplementation::StdExecution)
        {
#if OZZ_HAS_STD_EXECUTION
            std::for_each(std::execution::par, m_entity_cache.begin(), m_entity_cache.end(), combined_processing_lambda);
#else
            std::for_each(m_entity_cache.begin(), m_entity_cache.end(), combined_processing_lambda);
#endif
        }
        else
        {
            xr_parallel_for_each(m_entity_cache, combined_processing_lambda);
        }

        if (profiler.IsEnabled())
        {
            profiler.GetStats().sampling_time += combined_timer.ElapsedMs();
        }

        PerformanceTimer ik_timer;

        IKSolverSystem::Update(registry);

        if (profiler.IsEnabled())
        {
            profiler.GetStats().local_to_model_time += ik_timer.ElapsedMs();
        }

        PerformanceTimer gpu_timer;

        AnimationGPUComputeSystem::Update(registry, dt);

        if (profiler.IsEnabled())
        {
            profiler.GetStats().local_to_model_time += gpu_timer.ElapsedMs();
        }

        PerformanceTimer callback_timer;

        AnimationCallbackSystem::Update(registry);

        if (profiler.IsEnabled())
        {
            profiler.GetStats().callback_time += callback_timer.ElapsedMs();
            profiler.GetStats().total_update_time += total_timer.ElapsedMs();
        }
    }
};

class ParallelAnimationOrchestrator
{
public:
    static constexpr size_t PARALLEL_THRESHOLD = 4;

    static void Update(entt::registry& registry, float dt, bool force_parallel = false)
    {
        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();
        const size_t entity_count = view.size_hint();

        bool hasMultiChannel = (registry.view<MultiChannelAnimator>().size_hint() > 0);

        if (force_parallel || hasMultiChannel || entity_count >= PARALLEL_THRESHOLD)
        {
            ParallelAnimationProcessor::Instance().UpdateParallel(registry, dt);
        }
        else
        {
            AnimationUpdateOrchestrator::Update(registry, dt);
        }
    }
};

}
