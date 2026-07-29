#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{

struct FroxelContribution
{
    float density = 0.f;
    float extinction = 0.f;
    Fcolor albedo{};
    float anisotropy = 0.f;
};

class IVolumetricSource
{
public:
    virtual ~IVolumetricSource() = default;

    // Upload / refresh GPU-side params (MVP: may only fill CPU struct consumed by renderer CB)
    virtual void PrepareGPUData() = 0;

    virtual const char* GetShaderName() const = 0;
    virtual void GetWorldBounds(Fvector& outMin, Fvector& outMax) const = 0;
    virtual const char* GetTypeName() const = 0;
};

} // namespace xray::render::fg
