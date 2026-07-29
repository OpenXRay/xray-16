#pragma once

#include "IVolumetricSource.h"

namespace xray::render::fg
{

struct WorldFogGPUParams
{
    float density = 0.02f;
    float heightFalloff = 0.05f;
    float baseHeight = 0.f;
    Fvector albedo{0.7f, 0.75f, 0.85f};
};

class WorldFogSource : public IVolumetricSource
{
public:
    void PrepareGPUData() override;
    const char* GetShaderName() const override { return "volumetric\\world_fog"; }
    void GetWorldBounds(Fvector& outMin, Fvector& outMax) const override;
    const char* GetTypeName() const override { return "WorldFog"; }

    const WorldFogGPUParams& GetParams() const { return m_params; }

private:
    WorldFogGPUParams m_params;
};

} // namespace xray::render::fg
