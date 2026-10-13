#pragma once

#include "xrCommon/xr_vector.h"
#include "xrCore/Threading/Lock.hpp"

class IGameObject;
class CEntity;

struct GrassInteractionEntity
{
    Fvector position;
    float radius;
    Fvector velocity;
    float weight;
    Fvector direction;
    u16 object_id;
    u16 padding;
};

class ENGINE_API GrassInteractionCollector
{
public:
    static const u32 MAX_ENTITIES = 256;

    void AddEntity(IGameObject* obj);

    void GetEntitiesForFrame(xr_vector<GrassInteractionEntity>& out_entities);

    void BeginFrame();

private:
    xr_vector<GrassInteractionEntity> entities;
    Lock lock;
};

ENGINE_API extern GrassInteractionCollector g_GrassInteractionCollector;
