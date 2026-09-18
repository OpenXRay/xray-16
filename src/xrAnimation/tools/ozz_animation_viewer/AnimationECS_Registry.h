#pragma once

#include "AnimationECS_Components.h"
#include "AnimationECS_Systems.h"
#include "AnimationECS_ParallelSystems.h"
#include "entt/entt.hpp"
#include "xrCore/xrCore.h"

namespace AnimationECS {

class AnimationRegistry
{
private:
    entt::registry m_registry;
    bool m_initialized{false};

    AnimationRegistry() = default;
    ~AnimationRegistry() = default;

    AnimationRegistry(const AnimationRegistry&) = delete;
    AnimationRegistry& operator=(const AnimationRegistry&) = delete;

public:
    static AnimationRegistry& Instance()
    {
        static AnimationRegistry instance;
        return instance;
    }

    void Initialize()
    {
        if (m_initialized)
            return;

        Msg("[AnimationECS] Initializing Animation ECS Registry");
        m_initialized = true;
    }

    void Shutdown()
    {
        if (!m_initialized)
            return;

        Msg("[AnimationECS] Shutting down Animation ECS Registry");
        m_registry.clear();
        m_initialized = false;
    }

    bool IsInitialized() const { return m_initialized; }

    entt::registry& GetRegistry() { return m_registry; }
    const entt::registry& GetRegistry() const { return m_registry; }

    entt::entity CreateAnimatedEntity()
    {
        entt::entity entity = m_registry.create();

        m_registry.emplace<AnimationState>(entity);
        m_registry.emplace<AnimationController>(entity);
        m_registry.emplace<AnimationBuffers>(entity);
        m_registry.emplace<AnimationCallbacks>(entity);
        m_registry.emplace<AnimationGPUComputeState>(entity);
        m_registry.emplace<SpatialComponent>(entity);

        Msg("* [AnimationRegistry] Created entity %u with all 6 components", static_cast<u32>(entity));

        return entity;
    }

    void DestroyAnimatedEntity(entt::entity entity)
    {
        if (m_registry.valid(entity))
        {
            m_registry.destroy(entity);
        }
    }

    bool IsValidEntity(entt::entity entity) const
    {
        return m_registry.valid(entity);
    }

    template<typename Component>
    Component* GetComponent(entt::entity entity)
    {
        return m_registry.try_get<Component>(entity);
    }

    template<typename Component>
    const Component* GetComponent(entt::entity entity) const
    {
        return m_registry.try_get<Component>(entity);
    }

    template<typename Component>
    bool HasComponent(entt::entity entity) const
    {
        return m_registry.all_of<Component>(entity);
    }

    template<typename Component, typename... Args>
    Component& AddComponent(entt::entity entity, Args&&... args)
    {
        return m_registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    void RemoveComponent(entt::entity entity)
    {
        m_registry.remove<Component>(entity);
    }

    void Update(float dt)
    {
        static bool once = false;
        if (!once)
        {
            Msg("* [AnimationRegistry::Update] FIRST CALL - dt=%.4f", dt);
            once = true;
        }

        if (!m_initialized)
        {
            Msg("! [AnimationRegistry::Update] Called but NOT initialized!");
            return;
        }

        ParallelAnimationOrchestrator::Update(m_registry, dt);
    }

    void UpdateBatched(float dt)
    {
        if (!m_initialized)
            return;

        ParallelAnimationOrchestrator::Update(m_registry, dt);
    }

    size_t GetEntityCount() const
    {
        auto* storage = m_registry.storage<AnimationState>();
        return storage ? storage->size() : 0;
    }

    void PrintStatistics() const
    {
        Msg("[AnimationECS] Statistics:");
        Msg("  Total entities: %zu", GetEntityCount());

        auto view = m_registry.view<AnimationState>();
        size_t playing_count = 0;
        for (auto entity : view)
        {
            const auto& state = view.get<AnimationState>(entity);
            if (state.is_playing)
                ++playing_count;
        }

        Msg("  Playing animations: %zu", playing_count);
    }
};

inline AnimationRegistry& GetAnimationRegistry()
{
    return AnimationRegistry::Instance();
}

inline entt::registry& GetAnimationECSRegistry()
{
    return AnimationRegistry::Instance().GetRegistry();
}

}
