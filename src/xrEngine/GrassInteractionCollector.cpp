#include "stdafx.h"
#include "GrassInteractionCollector.h"
#include "xr_object.h"
#include "xrCommon/xr_map.h"

GrassInteractionCollector g_GrassInteractionCollector;

static xr_map<u16, Fvector> s_last_positions;

void GrassInteractionCollector::AddEntity(IGameObject* obj)
{
    if (!obj || entities.size() >= MAX_ENTITIES)
        return;

    u16 id = obj->ID();
    Fvector pos = obj->Position();

    Fvector vel;
    vel.set(0, 0, 0);

    auto it = s_last_positions.find(id);
    if (it != s_last_positions.end() && Device.fTimeDelta > 0.0001f)
    {
        vel.sub(pos, it->second);
        vel.mul(1.0f / Device.fTimeDelta);
    }
    s_last_positions[id] = pos;

    const Fbox& bbox = obj->BoundingBox();
    Fvector bbox_size;
    bbox.getsize(bbox_size);

    float radius = _max(bbox_size.x, bbox_size.z) * 0.5f;

    radius = std::clamp(radius, 0.2f, 3.0f);

    float weight = 1.0f;

    CEntity* entity = obj->cast_entity();
    if (entity)
    {
        float mass = obj->GetPhysicsMass();

        if (mass > 0.0f)
        {
            weight = std::clamp(mass / 100.0f, 0.3f, 2.0f);
        }
    }

    Fvector dir = obj->Direction();
    dir.normalize_safe();

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
    out_entities = entities;
}

void GrassInteractionCollector::BeginFrame()
{
    entities.clear();
}
