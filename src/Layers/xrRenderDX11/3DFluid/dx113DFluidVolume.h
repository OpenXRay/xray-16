#pragma once

#include "dx113DFluidData.h"
#include "Layers/xrRender/FBasicVisual.h"

namespace xray::render::fg
{
class dx113DFluidVolume : public dxRender_Visual
{
public:
    dx113DFluidVolume();
    virtual ~dx113DFluidVolume();

    virtual void Load(LPCSTR N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();

private:
    //	For debug purpose only
    ref_geom m_Geom;

    dx113DFluidData m_FluidData;
};
} // namespace xray::render::fg
