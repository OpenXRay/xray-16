// GrassInteractionCollector.cpp - Phase 1.5: Online A-Life Character Tracking Implementation
#include "stdafx.h"
#include "GrassInteractionCollector.h"
#include "xr_object.h"
#include "xrCommon/xr_map.h"

// Global instance
GrassInteractionCollector g_GrassInteractionCollector;

// Velocity tracking (position delta between frames)
static xr_map<u16, Fvector> s_last_positions;

void GrassInteractionCollector::AddEntity(IGameObject* obj)
{
    if (!obj || entities.size() >= MAX_ENTITIES)
        return;

    // Get basic properties
    u16 id = obj->ID();
    Fvector pos = obj->Position();

    // Calculate velocity from position delta
    Fvector vel;
    vel.set(0, 0, 0);

    auto it = s_last_positions.find(id);
    if (it != s_last_positions.end() && Device.fTimeDelta > 0.0001f)
    {
        vel.sub(pos, it->second);
        vel.mul(1.0f / Device.fTimeDelta);
    }
    s_last_positions[id] = pos;

    // Compute radius from bounding box
    const Fbox& bbox = obj->BoundingBox();
    Fvector bbox_size;
    bbox.getsize(bbox_size);

    // Use XZ plane for grass interaction (horizontal footprint)
    float radius = _max(bbox_size.x, bbox_size.z) * 0.5f;

    // Clamp to reasonable range
    radius = std::clamp(radius, 0.2f, 3.0f);

    // Compute weight from entity mass (if available)
    float weight = 1.0f;  // Default weight

    CEntity* entity = obj->cast_entity();
    if (entity)
    {
        float mass = obj->GetPhysicsMass();

        // Map mass to weight (0-1 range)
        // Typical masses: human ~70kg, mutants 50-200kg
        // Weight affects displacement strength
        if (mass > 0.0f)
        {
            weight = std::clamp(mass / 100.0f, 0.3f, 2.0f);
        }
    }

    // Get entity facing direction
    Fvector dir = obj->Direction();
    dir.normalize_safe();

    // Add to collection
    GrassInteractionEntity grass_entity;
    grass_entity.position = pos;
    grass_entity.radius = radius;
    grass_entity.velocity = vel;
    grass_entity.weight = weight;
    grass_entity.direction = dir;
    grass_entity.object_id = id;
    grass_entity.padding = 0;

    entities.push_back(grass_entity);
}

void GrassInteractionCollector::GetEntitiesForFrame(xr_vector<GrassInteractionEntity>& out_entities)
{
    out_entities = entities; // Copy for renderer
}

void GrassInteractionCollector::BeginFrame()
{
    entities.clear();
}
