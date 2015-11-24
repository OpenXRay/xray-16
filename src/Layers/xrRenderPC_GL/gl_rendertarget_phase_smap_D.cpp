#include "stdafx.h"

void	CRenderTarget::phase_smap_direct		(light* L, u32 sub_phase)
{
	//	TODO: DX10: Check thst we will never need old SMap implementation
	// Targets
	if (RImplementation.o.HW_smap)		u_setrt	(rt_smap_surf, NULL, NULL, rt_smap_depth->pZRT);
	//else								u_setrt	(rt_smap_surf, NULL, NULL, rt_smap_ZB);
	else								VERIFY(!"Use HW SMap only for DX10!");


	//	Don't have rect clear for DX10
	//	TODO: DX9:	Full clear must be faster for the near phase for SLI
	//	inobody clears this buffer _this_ frame.
	// Clear
	//if (SE_SUN_NEAR==sub_phase)			{
		// optimized clear
	//	D3DRECT		R;
	//	R.x1		= L->X.D.minX;
	//	R.x2		= L->X.D.maxX;
	//	R.y1		= L->X.D.minY;
	//	R.y2		= L->X.D.maxY;
	//	CHK_DX							(HW.pDevice->Clear( 1L, &R,	  D3DCLEAR_ZBUFFER,	0xFFFFFFFF, 1.0f, 0L));
	//} else {
		// full-clear
	//	CHK_DX							(HW.pDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER,	0xFFFFFFFF, 1.0f, 0L));
	//}

	HW.pDevice->ClearDepthStencilView(rt_smap_depth->pZRT, D3D10_CLEAR_DEPTH, 1.0f, 0L);

	//	Prepare viewport for shadow map rendering
	if (sub_phase!=SE_SUN_RAIN_SMAP	)
		RImplementation.rmNormal();
	else
	{
		D3D_VIEWPORT VP					=	{L->X.D.minX,L->X.D.minY,
			(L->X.D.maxX - L->X.D.minX) , 
			(L->X.D.maxY - L->X.D.minY) , 
			0,1 };
		//CHK_DX								(HW.pDevice->SetViewport(&VP));
		HW.pDevice->RSSetViewports(1, &VP);
	}

	// Stencil	- disable
	RCache.set_Stencil					( FALSE );

	//	TODO: DX10:	Implement culling reverse for DX10
	// Misc		- draw only front/back-faces
	/*
	if (SE_SUN_NEAR==sub_phase)			RCache.set_CullMode			( CULL_CCW	);	// near
	else								{
		if (RImplementation.o.HW_smap)	RCache.set_CullMode			( CULL_CW	);	// far, reversed
		else							RCache.set_CullMode			( CULL_CCW	);	// far, front-faces
	}
	if (RImplementation.o.HW_smap)		RCache.set_ColorWriteEnable	( FALSE		);
	else								RCache.set_ColorWriteEnable	( );
	*/
}

void	CRenderTarget::phase_smap_direct_tsh	(light* L, u32 sub_phase)
{
	VERIFY								(RImplementation.o.Tshadows);
	//u32		_clr						= 0xffffffff;	//color_rgba(127,127,12,12);
	FLOAT ColorRGBA[4] = { 1.0f, 1.0f, 1.0f, 1.0f};
	RCache.set_ColorWriteEnable			();
	//	Prepare viewport for shadow map rendering
	RImplementation.rmNormal();
	HW.pDevice->ClearRenderTargetView( RCache.get_RT(0), ColorRGBA);
	//CHK_DX								(HW.pDevice->Clear( 0L, NULL, D3DCLEAR_TARGET,	_clr,	1.0f, 0L));
}
