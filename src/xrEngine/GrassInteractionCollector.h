#pragma once

#include "xrCommon/xr_vector.h"
#include "xrCore/Threading/Lock.hpp"

struct GrassInteractionEntity
{
    Fvector position;
    Fvector velocity;
    float radius;
    float weight;
    u16 object_id;
    u16 padding;
};

class ENGINE_API GrassInteractionCollector
{
public:
    static const u32 MAX_ENTITIES = 256;

    void AddEntity(u16 id, const Fvector& pos, const Fvector& vel, float radius, float weight);

    void GetEntitiesForFrame(xr_vector<GrassInteractionEntity>& out_entities);

    void BeginFrame();

private:
    xr_vector<GrassInteractionEntity> entities;
    Lock lock;
};

ENGINE_API extern GrassInteractionCollector g_GrassInteractionCollector;
