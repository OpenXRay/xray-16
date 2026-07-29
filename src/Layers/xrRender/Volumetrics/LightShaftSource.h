#pragma once

#include "IVolumetricSource.h"

namespace xray::render::fg
{

class LightShaftSource : public IVolumetricSource
{
public:
    void PrepareGPUData() override;
    const char* GetShaderName() const override { return "volumetric\\light_shafts"; }
    void GetWorldBounds(Fvector& outMin, Fvector& outMax) const override;
    const char* GetTypeName() const override { return "LightShaft"; }

    float GetIntensity() const { return m_intensity; }

private:
    float m_intensity = 0.f;
};

} // namespace xray::render::fg
