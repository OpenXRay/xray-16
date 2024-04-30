#include "stdafx.h"

void CRenderTarget::phase_accumulator(CBackend& cmd_list)
{
    // Targets
    if (dwAccumulatorClearMark == Device.dwFrame)
    {
        // normal operation - setup
        if (RImplementation.o.fp16_blend)
            u_setrt(cmd_list, rt_Accumulator, nullptr, nullptr, rt_MSAADepth);
        else
            u_setrt(cmd_list, rt_Accumulator_temp, nullptr, nullptr, rt_MSAADepth);
    }
    else
    {
        // initial setup
        dwAccumulatorClearMark = Device.dwFrame;

        // clear
        u_setrt(cmd_list, rt_Accumulator, nullptr, nullptr, rt_MSAADepth);
        // dwLightMarkerID						= 5;					// start from 5, increment in 2 units
        reset_light_marker(cmd_list);

#ifdef USE_DX11
        //	Igor: AMD bug workaround. Should be fixed in 8.7 catalyst
        //	Need for MSAA to work correctly.
        if (RImplementation.o.msaa)
        {
            HW.get_context(CHW::IMM_CTX_ID)->OMSetRenderTargets(1, &(rt_Accumulator->pRT), 0); // TODO: id
        }
#endif // USE_DX11
        cmd_list.ClearRT(rt_Accumulator, {}); // black

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
        cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
        cmd_list.set_CullMode(CULL_NONE);
        cmd_list.set_ColorWriteEnable();
    }

    //	Restore viewport after shadow map rendering
    RImplementation.rmNormal(cmd_list);
}

void CRenderTarget::phase_vol_accumulator(CBackend& cmd_list)
{
    u_setrt(cmd_list, rt_Generic_2, nullptr, nullptr, rt_MSAADepth);

    if (!m_bHasActiveVolumetric)
    {
        m_bHasActiveVolumetric = true;
        cmd_list.ClearRT(rt_Generic_2, {}); // black
    }

    cmd_list.set_Stencil(FALSE);
    cmd_list.set_CullMode(CULL_NONE);
    cmd_list.set_ColorWriteEnable();
}
