#include "stdafx.h"
#include "GrassInteractionCollector.h"

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
    out_entities = entities;
}

void GrassInteractionCollector::BeginFrame()
{
    entities.clear();
}
