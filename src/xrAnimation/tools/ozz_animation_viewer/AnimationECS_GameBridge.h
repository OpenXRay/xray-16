#pragma once

#include "AnimationECS_Registry.h"
#include "AnimationECS_Components.h"
#include "xrCore/xrCore.h"

class IKinematicsAnimated;

namespace AnimationECS {

//-----------------------------------------------------------------------------
// AnimationEntityFactory
// Helper to bridge game spawning with AnimationECS
// This provides the interface between legacy game code and the ECS system
//-----------------------------------------------------------------------------
class AnimationEntityFactory
{
public:
    //-------------------------------------------------------------------------
    // Create ECS entity for an animated game object
    //
    // Parameters:
    //   kinematics - Pointer to the IKinematicsAnimated interface
    //   visual_name - Name of the visual/model for skeleton/animation loading
    //
    // Returns:
    //   entt::entity - Handle to the created ECS entity, or entt::null on failure
    //-------------------------------------------------------------------------
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

        // Setup controller with visual name for future skeleton/animation loading
        auto& controller = registry.GetRegistry().get<AnimationController>(entity);
        controller.skeleton_name = visual_name;

        // Start in non-playing state (wait for first PlayCycle call)
        auto& state = registry.GetRegistry().get<AnimationState>(entity);
        state.is_playing = false;

        Msg("* [AnimationECS] Created entity %u for visual '%s'",
            static_cast<u32>(entity), visual_name.c_str());

        return entity;
    }

    //-------------------------------------------------------------------------
    // Destroy ECS entity for a game object
    //
    // Parameters:
    //   entity - Handle to the ECS entity to destroy
    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    // Check if entity is valid
    //-------------------------------------------------------------------------
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

} // namespace AnimationECS
