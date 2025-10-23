// GrassInteractionCollector.h - Phase 1.5: Online A-Life Character Tracking
// Simple bridge between game logic (where characters update) and renderer (where grass interaction happens)
#pragma once

#include "xrCommon/xr_vector.h"
#include "xrCore/Threading/Lock.hpp"

// Forward declaration
class IGameObject;
class CEntity;

// Simple structure matching renderer's InteractiveEntity format
struct GrassInteractionEntity
{
    Fvector position;
    float radius;
    Fvector velocity;
    float weight; // 0-1, affects displacement
    Fvector direction; // Entity facing direction (normalized)
    u16 object_id; // For debugging
    u16 padding;
};

// Thread-safe collector for game logic → renderer handoff
class ENGINE_API GrassInteractionCollector
{
public:
    static const u32 MAX_ENTITIES = 256;

    // Called by game logic thread during object updates
    // Computes interaction parameters from entity properties (bounding box, mass, etc.)
    void AddEntity(IGameObject* obj);

    // Called by render thread to get snapshot
    void GetEntitiesForFrame(xr_vector<GrassInteractionEntity>& out_entities);

    // Called at frame start to clear old data
    void BeginFrame();

private:
    xr_vector<GrassInteractionEntity> entities;
    Lock lock; // Protect concurrent access
};

// Global instance
ENGINE_API extern GrassInteractionCollector g_GrassInteractionCollector;
