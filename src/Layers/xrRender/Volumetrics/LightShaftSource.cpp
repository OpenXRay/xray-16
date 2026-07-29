#include "stdafx.h"
#include "LightShaftSource.h"
#include "Layers/xrRender/FrameGraphPasses/SunShaftsPassSetup.h"
#include "xrEngine/device.h"

namespace xray::render::fg
{

void LightShaftSource::PrepareGPUData()
{
    m_intensity = passes::ResolveSunShaftsIntensity();
}

void LightShaftSource::GetWorldBounds(Fvector& outMin, Fvector& outMax) const
{
    outMin.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    outMax.set(FLT_MAX, FLT_MAX, FLT_MAX);
}

} // namespace xray::render::fg
