#include "stdafx.h"

void	CRenderTarget::phase_accumulator()
{
	// Targets
	if (dwAccumulatorClearMark==Device.dwFrame)	{
		// normal operation - setup
		if (RImplementation.o.fp16_blend)	u_setrt	(rt_Accumulator,		NULL,NULL,HW.pBaseZB);
		else								u_setrt	(rt_Accumulator_temp,	NULL,NULL,HW.pBaseZB);
	} else {
		// initial setup
		dwAccumulatorClearMark				= Device.dwFrame;

		// clear
		u_setrt								(rt_Accumulator,		NULL,NULL,HW.pBaseZB);
		//dwLightMarkerID						= 5;					// start from 5, increment in 2 units
		reset_light_marker();
		u32		clr4clear					= color_rgba(0,0,0,0);	// 0x00
		CHK_DX	(HW.pDevice->Clear			( 0L, NULL, D3DCLEAR_TARGET, clr4clear, 1.0f, 0L));

		//	Do it after the sun to preserve data.
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
		RCache.set_Stencil					(TRUE,D3DCMP_LESSEQUAL,0x01,0xff,0x00);
		RCache.set_CullMode					(CULL_NONE);
		RCache.set_ColorWriteEnable			();
	}
}

void	CRenderTarget::phase_vol_accumulator()
{
	if (!m_bHasActiveVolumetric)
	{
		m_bHasActiveVolumetric = true;
		
		u_setrt								(rt_Generic_2,		NULL,NULL,HW.pBaseZB);
		u32		clr4clearVol				= color_rgba(0,0,0,0);	// 0x00
		CHK_DX	(HW.pDevice->Clear			( 0L, NULL, D3DCLEAR_TARGET, clr4clearVol, 1.0f, 0L));
	}
	else
		u_setrt	(rt_Generic_2,NULL,NULL,HW.pBaseZB);

	RCache.set_Stencil							(FALSE);
	RCache.set_CullMode							(CULL_NONE);
	RCache.set_ColorWriteEnable					();
}