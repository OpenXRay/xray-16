#include "stdafx.h"

void CRenderTarget::phase_smap_direct(light* L, u32 sub_phase)
{
    // Targets
    u_setrt(rt_smap_surf, NULL, NULL, rt_smap_depth->pRT);

    // Clear
    if (SE_SUN_NEAR == sub_phase)
    {
        // optimized clear
        const Irect rect =
        {
            L->X.D.minX, L->X.D.minY,
            L->X.D.maxX, L->X.D.maxY
        };
        RCache.ClearZBRect(rt_smap_depth, 1.0f, 1, &rect);
    }
    else
    {
        // full-clear
        RCache.ClearZB(rt_smap_depth, 1.0f);
    }

    // Stencil	- disable
    RCache.set_Stencil(FALSE);

    // Misc		- draw only front/back-faces
    /*
    if (SE_SUN_NEAR==sub_phase)			RCache.set_CullMode			( CULL_CCW	);	// near
    else								{
        if (RImplementation.o.HW_smap)	RCache.set_CullMode			( CULL_CW	);	// far, reversed
        else							RCache.set_CullMode			( CULL_CCW	);	// far, front-faces
    }
    */
    //	Cull always CCW. If you want to revert to previouse solution, please, revert bias setup/
    RCache.set_CullMode(CULL_CCW); // near
    if (RImplementation.o.HW_smap)
        RCache.set_ColorWriteEnable(FALSE);
    else
        RCache.set_ColorWriteEnable();
}

void CRenderTarget::phase_smap_direct_tsh(light* /*L*/, u32 /*sub_phase*/)
{
    VERIFY(RImplementation.o.Tshadows);
    RCache.set_ColorWriteEnable();
    RCache.ClearRT(RCache.get_RT(), { 1.0f, 1.0f, 1.0f, 1.0f }); // color_rgba(127, 127, 12, 12);
}
