#pragma once

#include "IVolumetricSource.h"

namespace xray::render::fg
{

class ParticleEmitterSource : public IVolumetricSource
{
public:
    void PrepareGPUData() override;
    const char* GetShaderName() const override { return "volumetric\\particle_inject"; }
    void GetWorldBounds(Fvector& outMin, Fvector& outMax) const override;
    const char* GetTypeName() const override { return "ParticleEmitter"; }
};

} // namespace xray::render::fg
