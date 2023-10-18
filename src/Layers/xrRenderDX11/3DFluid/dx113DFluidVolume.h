#ifndef dx113DFluidVolume_included
#define dx113DFluidVolume_included
#pragma once

#include "dx113DFluidData.h"
#include "Layers/xrRender/FBasicVisual.h"

class dx113DFluidVolume : public dxRender_Visual
{
public:
    dx113DFluidVolume();
    virtual ~dx113DFluidVolume();

    virtual void Load(LPCSTR N, IReader* data, u32 dwFlags);
    virtual void Render(CBackend& cmd_list, float LOD, bool use_fast_geo) override; // LOD - Level Of Detail  [0.0f - min, 1.0f - max], Ignored ?
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();

private:
    //	For debug purpose only
    ref_geom m_Geom;

    dx113DFluidData m_FluidData;
};

#endif //	dx113DFluidVolume_included
