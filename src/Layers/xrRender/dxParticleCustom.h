#pragma once

#include "Include/xrRender/ParticleCustom.h"
#include "FBasicVisual.h"

namespace xray::render::RENDER_NAMESPACE
{
class dxParticleCustom : public dxRender_Visual, public IParticleCustom
{
public:
    // geometry-format
    ref_geom geom;

public:
    virtual ~dxParticleCustom() { ; }
    virtual IParticleCustom* dcast_ParticleCustom() { return this; }
};
} // namespace xray::render::RENDER_NAMESPACE
