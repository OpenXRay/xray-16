#include "stdafx.h"

void CRenderTarget::phase_smap_direct(light* L, u32 sub_phase)
{
    if (sub_phase == SE_SUN_RAIN_SMAP)
    {
        u_setrt(nullptr, nullptr, nullptr, rt_smap_rain);
        RCache.ClearZB(rt_smap_rain, 1.0f);
        RCache.SetViewport({0, 0, rt_smap_rain->dwWidth, rt_smap_rain->dwHeight, 0.0, 1.0});
    }
    else
    {
        u_setrt(nullptr, nullptr, nullptr, rt_smap_depth);
        RCache.ClearZB(rt_smap_depth, 1.0f);
        RImplementation.rmNormal();
    }

    // Stencil	- disable
    RCache.set_Stencil(FALSE);
}

void CRenderTarget::phase_smap_direct_tsh(light* L, u32 sub_phase)
{
    VERIFY(RImplementation.o.Tshadows);
    RCache.set_ColorWriteEnable();
    //	Prepare viewport for shadow map rendering
    RImplementation.rmNormal();
    RCache.ClearRT(RCache.get_RT(), { 1.0f, 1.0f, 1.0f, 1.0f }); // color_rgba(127, 127, 12, 12);
}
