#pragma once

#include "Include/xrRender/RainRender.h"

namespace xray::render::RENDER_NAMESPACE
{
class dxRainRender : public IRainRender
{
public:
    dxRainRender();
    virtual ~dxRainRender();
    virtual void Copy(IRainRender& _in);

    virtual void Render(CEffect_Rain& owner);

    virtual const Fsphere& GetDropBounds() const;

private:
    // Visualization	(rain)
    // Legacy D3D11
    ref_shader SH_Rain;

    // DX12: NVRHI shader handles
    nvrhi::ShaderHandle SH_Rain_VS;
    nvrhi::ShaderHandle SH_Rain_PS;

    ref_geom hGeom_Rain;

    // Visualization	(drops)
    IRender_DetailModel* DM_Drop;
    ref_geom hGeom_Drops;
};
} // namespace xray::render::RENDER_NAMESPACE
