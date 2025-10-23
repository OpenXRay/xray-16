// GrassInteractionCollector.h - Phase 1.5: Online A-Life Character Tracking
// Simple bridge between game logic (where characters update) and renderer (where grass interaction happens)
#pragma once

#include "xrCommon/xr_vector.h"
#include "xrCore/Threading/Lock.hpp"

// Simple structure matching renderer's InteractiveEntity format
struct GrassInteractionEntity
{
    Fvector position;
    Fvector velocity;
    float radius;
    float weight; // 0-1, affects displacement
    u16 object_id; // For debugging
    u16 padding;
};

// Thread-safe collector for game logic → renderer handoff
class ENGINE_API GrassInteractionCollector
{
public:
    static const u32 MAX_ENTITIES = 256;

    // Called by game logic thread during object updates
    void AddEntity(u16 id, const Fvector& pos, const Fvector& vel, float radius, float weight);

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
