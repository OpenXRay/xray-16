#include "stdafx.h"

void CRenderTarget::phase_accumulator()
{
    // Targets
    if (dwAccumulatorClearMark == Device.dwFrame)
    {
        // normal operation - setup
        if (RImplementation.o.fp16_blend)
            u_setrt(rt_Accumulator, nullptr, nullptr, rt_MSAADepth);
        else
            u_setrt(rt_Accumulator_temp, nullptr, nullptr, rt_MSAADepth);
    }
    else
    {
        // initial setup
        dwAccumulatorClearMark = Device.dwFrame;

        // clear
        u_setrt(rt_Accumulator, nullptr, nullptr, rt_MSAADepth);
        // dwLightMarkerID						= 5;					// start from 5, increment in 2 units
        reset_light_marker();

#ifdef USE_DX11
        //	Igor: AMD bug workaround. Should be fixed in 8.7 catalyst
        //	Need for MSAA to work correctly.
        if (RImplementation.o.dx10_msaa)
        {
            HW.pContext->OMSetRenderTargets(1, &(rt_Accumulator->pRT), 0);
        }
#endif // USE_DX11
        RCache.ClearRT(rt_Accumulator, {}); // black

        //	render this after sun to avoid troubles with sun
        /*
        // Render emissive geometry, stencil - write 0x0 at pixel pos
        RCache.set_xform_project					(Device.mProject);
        RCache.set_xform_view						(Device.mView);
        // Stencil - write 0x1 at pixel pos -
        RCache.set_Stencil							( TRUE,D3DCMP_ALWAYS,0x01,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
        //RCache.set_Stencil						(TRUE,D3DCMP_ALWAYS,0x00,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
        RCache.set_CullMode							(CULL_CCW);
        RCache.set_ColorWriteEnable					();
        RImplementation.r_dsgraph_render_emissive	();
        */
        // Stencil	- draw only where stencil >= 0x1
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
        RCache.set_CullMode(CULL_NONE);
        RCache.set_ColorWriteEnable();
    }

#ifndef USE_DX9
    //	Restore viewport after shadow map rendering
    RImplementation.rmNormal();
#endif
}

void CRenderTarget::phase_vol_accumulator()
{
    u_setrt(rt_Generic_2, nullptr, nullptr, rt_MSAADepth);

    if (!m_bHasActiveVolumetric)
    {
        m_bHasActiveVolumetric = true;
        RCache.ClearRT(rt_Generic_2, {}); // black
    }

    RCache.set_Stencil(FALSE);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_ColorWriteEnable();
}
