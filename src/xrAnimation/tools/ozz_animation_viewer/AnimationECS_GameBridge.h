#pragma once

#include "AnimationECS_Registry.h"
#include "AnimationECS_Components.h"
#include "xrCore/xrCore.h"

class IKinematicsAnimated;

namespace AnimationECS {

class AnimationEntityFactory
{
public:
    static entt::entity CreateForGameObject(
        IKinematicsAnimated* kinematics,
        const shared_str& visual_name)
    {
        if (!kinematics)
        {
            Msg("! [AnimationECS] CreateForGameObject: null kinematics pointer");
            return entt::null;
        }

        auto& registry = GetAnimationRegistry();
        if (!registry.IsInitialized())
        {
            Msg("! [AnimationECS] CreateForGameObject: registry not initialized");
            return entt::null;
        }

        entt::entity entity = registry.CreateAnimatedEntity();

        auto& controller = registry.GetRegistry().get<AnimationController>(entity);
        controller.skeleton_name = visual_name;

        auto& state = registry.GetRegistry().get<AnimationState>(entity);
        state.is_playing = false;

        Msg("* [AnimationECS] Created entity %u for visual '%s'",
            static_cast<u32>(entity), visual_name.c_str());

        return entity;
    }

    static void DestroyForGameObject(entt::entity entity)
    {
        if (entity == entt::null)
            return;

        auto& registry = GetAnimationRegistry();
        if (!registry.IsInitialized())
            return;

        if (registry.IsValidEntity(entity))
        {
            Msg("* [AnimationECS] Destroying entity %u", static_cast<u32>(entity));
            registry.DestroyAnimatedEntity(entity);
        }
    }

    static bool IsValidEntity(entt::entity entity)
    {
        if (entity == entt::null)
            return false;

        auto& registry = GetAnimationRegistry();
        if (!registry.IsInitialized())
            return false;

        return registry.IsValidEntity(entity);
    }
};

}
