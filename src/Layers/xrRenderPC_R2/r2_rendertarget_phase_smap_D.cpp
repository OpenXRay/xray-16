#include "stdafx.h"

void CRenderTarget::phase_smap_direct(CBackend& cmd_list, light* L, u32 sub_phase)
{
    cmd_list.set_pass_targets(
        rt_smap_surf,
        nullptr,
        nullptr,
        rt_smap_depth
    );

    // Clear
    if (SE_SUN_NEAR == sub_phase)
    {
        // optimized clear
        const Irect rect =
        {
            L->X.D[sub_phase].minX, L->X.D[sub_phase].minY,
            L->X.D[sub_phase].maxX, L->X.D[sub_phase].maxY
        };
        cmd_list.ClearZBRect(rt_smap_depth, 1.0f, 1, &rect);
    }
    else
    {
        // full-clear
        cmd_list.ClearZB(rt_smap_depth, 1.0f);
    }

    // Stencil	- disable
    cmd_list.set_Stencil(FALSE);

    // Misc		- draw only front/back-faces
    /*
    if (SE_SUN_NEAR==sub_phase)			RCache.set_CullMode			( CULL_CCW	);	// near
    else								{
        if (RImplementation.o.HW_smap)	RCache.set_CullMode			( CULL_CW	);	// far, reversed
        else							RCache.set_CullMode			( CULL_CCW	);	// far, front-faces
    }
    */
    //	Cull always CCW. If you want to revert to previouse solution, please, revert bias setup/
    cmd_list.set_CullMode(CULL_CCW); // near
    if (RImplementation.o.HW_smap)
        cmd_list.set_ColorWriteEnable(FALSE);
    else
        cmd_list.set_ColorWriteEnable();
}

void CRenderTarget::phase_smap_direct_tsh(CBackend& cmd_list, light* /*L*/, u32 /*sub_phase*/)
{
    VERIFY(RImplementation.o.Tshadows);
    cmd_list.set_ColorWriteEnable();
    cmd_list.ClearRT(cmd_list.get_RT(), { 1.0f, 1.0f, 1.0f, 1.0f }); // color_rgba(127, 127, 12, 12);
}
