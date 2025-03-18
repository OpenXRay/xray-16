#pragma once

#include "Include/xrRender/ThunderboltRender.h"

namespace xray::render::RENDER_NAMESPACE
{
class dxThunderboltRender : public IThunderboltRender
{
public:
    dxThunderboltRender();
    virtual ~dxThunderboltRender();

    virtual void Copy(IThunderboltRender& _in);

    virtual void Render(CEffect_Thunderbolt& owner);

private:
    ref_geom hGeom_model;
    ref_geom hGeom_gradient;
};
} // namespace xray::render::RENDER_NAMESPACE
