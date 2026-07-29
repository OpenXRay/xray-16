#include "stdafx.h"
#include "ParticleEmitterSource.h"

namespace xray::render::fg
{

void ParticleEmitterSource::PrepareGPUData() {}

void ParticleEmitterSource::GetWorldBounds(Fvector& outMin, Fvector& outMax) const
{
    outMin.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    outMax.set(FLT_MAX, FLT_MAX, FLT_MAX);
}

} // namespace xray::render::fg
