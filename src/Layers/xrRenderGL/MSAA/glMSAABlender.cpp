#include "stdafx.h"
#include "./glMSAABlender.h"

void CBlender_msaa::Compile(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	switch (C.iElement) 
	{
   case 0:
		C.r_Pass	("stub_notransform_2uv", "mark_msaa_edges", false,	FALSE,	FALSE, FALSE);
		C.PassSET_ZB		(FALSE,FALSE,FALSE	);

		//C.r_dx10Texture		("s_position",		r2_RT_P);
		//C.r_dx10Texture		("s_normal",		r2_RT_N);
		
		C.r_Sampler_rtf		("s_position",		r2_RT_P);
		C.r_Sampler_rtf		("s_normal",		r2_RT_N);
		
		//C.r_dx10Sampler		("smp_nofilter");

		C.r_End				();

		break;
	}

}