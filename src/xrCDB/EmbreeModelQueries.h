#pragma once

#include "xrCDB.h"

namespace CDB
{
bool ShouldUseEmbreeRayQueries(u32 rayMode);
void ReleaseEmbreeModel(const MODEL* model);
void QueryEmbreeModel(const MODEL* model, COLLIDER& collider, u32 rayMode,
    const Fvector& start, const Fvector& direction, float range);
}
