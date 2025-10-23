// GrassInteractionCollector.cpp - Phase 1.5: Online A-Life Character Tracking Implementation
#include "stdafx.h"
#include "GrassInteractionCollector.h"

// Global instance
GrassInteractionCollector g_GrassInteractionCollector;

void GrassInteractionCollector::AddEntity(u16 id, const Fvector& pos, const Fvector& vel, float radius, float weight)
{
    if (entities.size() >= MAX_ENTITIES)
        return;

    GrassInteractionEntity entity;
    entity.position = pos;
    entity.velocity = vel;
    entity.radius = radius;
    entity.weight = weight;
    entity.object_id = id;
    entity.padding = 0;

    entities.push_back(entity);
}

void GrassInteractionCollector::GetEntitiesForFrame(xr_vector<GrassInteractionEntity>& out_entities)
{
    out_entities = entities; // Copy for renderer
}

void GrassInteractionCollector::BeginFrame()
{
    entities.clear();
}
