#include "stdafx.h"

#include "Systems.hpp"
#include "Components.hpp"

#include "xrECS/Components.hpp"
#include "xrECS/Time.hpp"
#include "xrECS/World.hpp"

#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/KinematicsAnimated.h"

#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

#include <cmath>

namespace xray::animation::systems {

void Sample(xray::ecs::World& world)
{
    const float dt = world.Resource<xray::ecs::Time>().delta_seconds;
    auto& reg = world.reg();

    auto view = reg.view<xray::ecs::AnimationState,
                         xray::ecs::AnimationController,
                         xray::ecs::AnimationBuffers,
                         xray::ecs::Playing>(entt::exclude<xray::ecs::MultiChannelAnimator>);

    for (auto entity : view)
    {
        auto& state = view.get<xray::ecs::AnimationState>(entity);
        auto& ctl   = view.get<xray::ecs::AnimationController>(entity);
        auto& bufs  = view.get<xray::ecs::AnimationBuffers>(entity);

        if (!ctl.animation || !ctl.skeleton || bufs.locals.empty())
        {
            reg.remove<xray::ecs::Playing>(entity);
            continue;
        }

        state.current_time += dt * ctl.playback_speed;

        const float duration = ctl.animation->duration();
        if (duration <= 0.f)
        {
            reg.remove<xray::ecs::Playing>(entity);
            continue;
        }
        state.time_ratio = state.current_time / duration;

        if (state.time_ratio > 1.0f)
        {
            if (reg.all_of<xray::ecs::Looping>(entity))
            {
                state.current_time = std::fmod(state.current_time, duration);
                state.time_ratio   = std::fmod(state.time_ratio,   1.0f);
            }
            else
            {
                state.current_time = duration;
                state.time_ratio   = 1.0f;
                reg.remove<xray::ecs::Playing>(entity);
            }
        }

        if (ctl.animation->num_tracks() != ctl.skeleton->num_joints())
        {
            Msg("! [Sample] mismatch entity=%d anim_tracks=%d skel_joints=%d locals=%zu models=%zu",
                (int)entity,
                ctl.animation->num_tracks(),
                ctl.skeleton->num_joints(),
                bufs.locals.size(),
                bufs.models.size());
            reg.remove<xray::ecs::Playing>(entity);
            continue;
        }

        ozz::animation::SamplingJob job;
        job.animation = ctl.animation;
        job.context   = &bufs.context;
        job.ratio     = state.time_ratio;
        job.output    = ozz::make_span(bufs.locals);
        if (!job.Run())
            Msg("! [Sample] job.Run failed entity=%d", (int)entity);
    }
}

void Blend(xray::ecs::World& world)
{
    auto view = world.reg().view<xray::ecs::AnimationBuffers,
                                 xray::ecs::AnimationController,
                                 xray::ecs::BlendState>();

    for (auto entity : view)
    {
        auto& bufs  = view.get<xray::ecs::AnimationBuffers>(entity);
        auto& ctl   = view.get<xray::ecs::AnimationController>(entity);
        auto& blend = view.get<xray::ecs::BlendState>(entity);

        ozz::animation::BlendingJob job;
        job.layers    = ozz::make_span(blend.layers);
        job.rest_pose = ctl.skeleton->joint_rest_poses();
        job.output    = ozz::make_span(bufs.locals);
        R_ASSERT2(job.Run(), "ozz BlendingJob::Run failed");
    }
}

void LocalToModel(xray::ecs::World& world)
{
    auto view = world.reg().view<xray::ecs::AnimationController,
                                 xray::ecs::AnimationBuffers>();

    for (auto entity : view)
    {
        auto& ctl  = view.get<xray::ecs::AnimationController>(entity);
        auto& bufs = view.get<xray::ecs::AnimationBuffers>(entity);

        if (!ctl.skeleton || bufs.locals.empty() || bufs.models.empty())
            continue;

        const std::size_t expected_joints = static_cast<std::size_t>(ctl.skeleton->num_joints());
        if (bufs.models.size() != expected_joints)
            continue;

        ozz::animation::LocalToModelJob job;
        job.skeleton = ctl.skeleton;
        job.input    = ozz::make_span(bufs.locals);
        job.output   = ozz::make_span(bufs.models);
        if (!job.Run())
            Msg("! [LocalToModel] failed for entity=%d joints=%zu", (int)entity, expected_joints);
    }
}

void MultiChannelSample(xray::ecs::World& world)
{
    const float dt = world.Resource<xray::ecs::Time>().delta_seconds;
    auto& reg = world.reg();

    auto view = reg.view<xray::ecs::MultiChannelAnimator,
                         xray::ecs::AnimationBuffers,
                         xray::ecs::AnimationController>();

    for (auto entity : view)
    {
        auto& mc   = view.get<xray::ecs::MultiChannelAnimator>(entity);
        auto& bufs = view.get<xray::ecs::AnimationBuffers>(entity);
        auto& ctl  = view.get<xray::ecs::AnimationController>(entity);

        if (!mc.initialized || !ctl.skeleton || bufs.locals.empty())
            continue;

        for (int ch = 0; ch < mc.kChannelCount; ++ch) {
            mc.channels[ch].AdvanceTime(dt);
            mc.channels[ch].Sample();
        }

        auto* crossfade = reg.try_get<xray::ecs::CrossfadeState>(entity);
        if (crossfade) {
            for (int ch = 0; ch < mc.kChannelCount; ++ch) {
                if (!crossfade->IsFading(ch))
                    continue;

                crossfade->fadeTime[ch] += dt;
                const float w = crossfade->NewWeight(ch);

                if (w >= 1.0f) {
                    crossfade->active[ch] = false;
                    continue;
                }

                ozz::animation::BlendingJob::Layer layers[2];
                layers[0].transform = ozz::make_span(crossfade->frozenLocals[ch]);
                layers[0].weight = 1.0f - w;
                layers[1].transform = ozz::make_span(mc.channels[ch].locals);
                layers[1].weight = w;

                ozz::animation::BlendingJob fadeJob;
                fadeJob.layers = ozz::span<ozz::animation::BlendingJob::Layer>(layers, 2);
                fadeJob.rest_pose = ctl.skeleton->joint_rest_poses();
                fadeJob.output = ozz::make_span(crossfade->blendTemp);
                fadeJob.Run();

                std::copy(crossfade->blendTemp.begin(), crossfade->blendTemp.end(),
                          mc.channels[ch].locals.begin());
            }
        }

        ozz::animation::BlendingJob::Layer blendLayers[xray::ecs::MultiChannelAnimator::kChannelCount];
        int layerCount = 0;

        for (int ch = 0; ch < mc.kChannelCount; ++ch) {
            if (!mc.channels[ch].IsInitialized())
                continue;
            blendLayers[layerCount].transform = ozz::make_span(mc.channels[ch].locals);
            blendLayers[layerCount].weight = 1.0f;
            blendLayers[layerCount].joint_weights = ozz::make_span(mc.jointWeights[ch]);
            ++layerCount;
        }

        ozz::animation::BlendingJob blendJob;
        blendJob.layers = ozz::span<ozz::animation::BlendingJob::Layer>(blendLayers, layerCount);
        blendJob.rest_pose = ctl.skeleton->joint_rest_poses();
        blendJob.output = ozz::make_span(bufs.locals);
        blendJob.Run();
    }
}

void ProcessRequests(xray::ecs::World& world)
{
    auto& reg = world.reg();
    auto view = reg.view<xray::ecs::CAnimationRequest, xray::ecs::CMeshSkinned>();

    for (auto entity : view)
    {
        const auto& req  = view.get<xray::ecs::CAnimationRequest>(entity);
        const auto& mesh = view.get<xray::ecs::CMeshSkinned>(entity);

        if (!req.id.valid() || !mesh.visual)
            continue;

        IKinematicsAnimated* kin = mesh.visual->dcast_PKinematicsAnimated();
        if (!kin)
            continue;

        kin->PlayCycle(req.id, req.mix ? TRUE : FALSE, nullptr, nullptr, req.channel);
    }

    reg.clear<xray::ecs::CAnimationRequest>();
}

}
