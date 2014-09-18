#include "stdafx.h"
#pragma hdrstop

#include "blender_light_direct_cascade.h"

CBlender_accum_direct_cascade::CBlender_accum_direct_cascade	()	{	description.CLS		= 0;	}
CBlender_accum_direct_cascade::~CBlender_accum_direct_cascade	()	{	}

void	CBlender_accum_direct_cascade::Compile(CBlender_Compile& C)
{
	IBlender::Compile		(C);

	BOOL	b_HW_smap		= RImplementation.o.HW_smap;
	BOOL	b_HW_PCF		= RImplementation.o.HW_smap_PCF;
	BOOL		blend		= FALSE;	//RImplementation.o.fp16_blend;
	D3DBLEND	dest		= blend?D3DBLEND_ONE:D3DBLEND_ZERO;
	if (RImplementation.o.sunfilter)	{ blend = FALSE; dest = D3DBLEND_ZERO; }

	switch (C.iElement)
	{
	case SE_SUN_NEAR:		// near pass - enable Z-test to perform depth-clipping
	case SE_SUN_MIDDLE:
		C.r_Pass			("accum_volume",	"accum_sun_cascade",	false,	TRUE,	FALSE,blend,D3DBLEND_ONE,dest);
		C.PassSET_ZB		(TRUE,FALSE,TRUE	);	// force inverted Z-Buffer
		C.r_Sampler_rtf		("s_position",		r2_RT_P			);
		C.r_Sampler_rtf		("s_normal",		r2_RT_N			);
		C.r_Sampler_clw		("s_material",		r2_material		);
		C.r_Sampler_rtf		("s_accumulator",	r2_RT_accum		);
		C.r_Sampler			("s_lmap",			r2_sunmask		);
		if (b_HW_smap)		{
			if (b_HW_PCF)	C.r_Sampler_clf		("s_smap",r2_RT_smap_depth	);
			else			{
				C.r_Sampler_rtf		("s_smap",r2_RT_smap_depth	);
			}
		}
		else				
			C.r_Sampler_rtf		("s_smap",r2_RT_smap_surf	);

		jitter				(C);
// 		{
// 			u32 s = C.i_Sampler("s_smap");
// 			C.i_Address		(s, D3DTADDRESS_BORDER);	
// 			C.i_BorderColor	(s, D3DCOLOR_ARGB(255, 255, 255, 255));
// 		}
		C.r_End				();
		break;
	case SE_SUN_FAR:		// far pass, only stencil clipping performed
		C.r_Pass			("accum_volume",			"accum_sun_cascade_far",	false,	TRUE,	FALSE,blend,D3DBLEND_ONE,dest);
		C.r_Sampler_rtf		("s_position",		r2_RT_P			);
		C.r_Sampler_rtf		("s_normal",		r2_RT_N			);
		C.r_Sampler_clw		("s_material",		r2_material		);
		C.r_Sampler_rtf		("s_accumulator",	r2_RT_accum		);
		C.r_Sampler			("s_lmap",			r2_sunmask		);
		if (b_HW_smap)		{
			if (b_HW_PCF)	C.r_Sampler_clf		("s_smap",r2_RT_smap_depth	);
			else			C.r_Sampler_rtf		("s_smap",r2_RT_smap_depth	);
		}
		else				C.r_Sampler_rtf		("s_smap",r2_RT_smap_surf	);
		jitter				(C);
		{
			u32 s = C.i_Sampler("s_smap");
			C.i_Address		(s, D3DTADDRESS_BORDER);
			C.i_BorderColor	(s, D3DCOLOR_ARGB(255, 255, 255, 255));
		}
		C.r_End				();
		break;
	}
}
