#pragma once

#include "AnimationECS_Components.h"
#include "AnimationECS_Systems.h"
#include "AnimationECS_IK.h"
#include "AnimationECS_Performance.h"
#include "entt/entt.hpp"
#include "xrCore/Threading/ParallelForEach.hpp"

// C++17 parallel algorithms
#include <execution>
#include <algorithm>

namespace AnimationECS {

// Global toggle for parallel implementation
enum class ParallelImplementation
{
    XRayTaskSystem,     // Use X-Ray's xr_parallel_for_each
    StdExecution        // Use std::for_each with std::execution::par
};

inline ParallelImplementation g_parallel_implementation = ParallelImplementation::XRayTaskSystem;

//-----------------------------------------------------------------------------
// ParallelAnimationProcessor
// Processes multiple animated entities in parallel using X-Ray's task system
// Singleton with reusable buffers to avoid per-frame allocations
//-----------------------------------------------------------------------------
class ParallelAnimationProcessor
{
private:
    // Reusable buffer to avoid per-frame allocations
    xr_vector<entt::entity> m_entity_cache;

    // Singleton
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

    // Process all animation systems in parallel
    void UpdateParallel(entt::registry& registry, float dt)
    {
        auto& profiler = GetPerformanceProfiler();
        PerformanceTimer total_timer;

        // Collect all entities that need animation updates
        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();

        // Reuse cached vector instead of allocating new one
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

        // OPTIMIZED: Single-pass processing - Sample + Blend + LocalToModel in ONE parallel pass
        // This eliminates 2 synchronization points and greatly improves cache utilization
        PerformanceTimer combined_timer;

        auto combined_processing_lambda = [&, dt](entt::entity entity)
        {
            auto& state = view.get<AnimationState>(entity);
            auto& controller = view.get<AnimationController>(entity);
            auto& buffers = view.get<AnimationBuffers>(entity);

            // Skip if no skeleton or uninitialized
            if (!controller.skeleton || !buffers.IsInitialized())
                return;

            // Step 1: Sample animation (if playing)
            if (state.is_playing && controller.animation)
            {
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

                // Sample animation into locals
                ozz::animation::SamplingJob sampling_job;
                sampling_job.animation = controller.animation;
                sampling_job.context = &buffers.context;
                sampling_job.ratio = state.time_ratio;
                sampling_job.output = ozz::make_span(buffers.locals);
                sampling_job.Run();
            }

            // Step 2: Blend (if BlendState component exists)
            // Check for BlendState component - only some entities have it
            auto* blend_state = registry.try_get<BlendState>(entity);
            if (blend_state && blend_state->HasLayers())
            {
                // Setup blending job
                ozz::animation::BlendingJob blend_job;
                blend_job.layers = ozz::make_span(blend_state->layers);
                blend_job.rest_pose = controller.skeleton->joint_rest_poses();
                blend_job.output = ozz::make_span(buffers.locals);
                blend_job.Run();
            }

            // Step 3: Local-to-Model transformation
            ozz::animation::LocalToModelJob ltm_job;
            ltm_job.skeleton = controller.skeleton;
            ltm_job.input = ozz::make_span(buffers.locals);
            ltm_job.output = ozz::make_span(buffers.models);
            ltm_job.Run();
        };

        // Execute single-pass processing in parallel
        if (g_parallel_implementation == ParallelImplementation::StdExecution)
        {
            std::for_each(std::execution::par, m_entity_cache.begin(), m_entity_cache.end(), combined_processing_lambda);
        }
        else
        {
            xr_parallel_for_each(m_entity_cache, combined_processing_lambda);
        }

        if (profiler.IsEnabled())
        {
            // Track combined processing time (sampling + blending + local-to-model)
            // Cannot accurately split this since they're in one pass now
            profiler.GetStats().sampling_time += combined_timer.ElapsedMs();
        }

        // Phase 4: IK Solving (after LocalToModel, before callbacks)
        // IK works in model space, so must run after local-to-model conversion
        // Can potentially be parallelized per-entity, but sequential for now
        PerformanceTimer ik_timer;

        IKSolverSystem::Update(registry);

        if (profiler.IsEnabled())
        {
            profiler.GetStats().local_to_model_time += ik_timer.ElapsedMs(); // IK time added to LTM for now
        }

        // Phase 5: Callbacks (MUST run on main thread sequentially)
        // This is intentionally NOT parallelized due to potential side effects
        PerformanceTimer callback_timer;

        AnimationCallbackSystem::Update(registry);

        if (profiler.IsEnabled())
        {
            profiler.GetStats().callback_time += callback_timer.ElapsedMs();
            profiler.GetStats().total_update_time += total_timer.ElapsedMs();
        }
    }
};

//-----------------------------------------------------------------------------
// Parallel-aware AnimationUpdateOrchestrator
// Chooses between sequential and parallel execution based on entity count
//-----------------------------------------------------------------------------
class ParallelAnimationOrchestrator
{
public:
    // Threshold for switching to parallel execution
    static constexpr size_t PARALLEL_THRESHOLD = 4;

    static void Update(entt::registry& registry, float dt, bool force_parallel = false)
    {
        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();
        const size_t entity_count = view.size_hint();

        // Use parallel processing if we have enough entities to benefit
        if (force_parallel || entity_count >= PARALLEL_THRESHOLD)
        {
            ParallelAnimationProcessor::Instance().UpdateParallel(registry, dt);
        }
        else
        {
            // Sequential path for small entity counts (less overhead)
            AnimationUpdateOrchestrator::Update(registry, dt);
        }
    }
};

} // namespace AnimationECS
