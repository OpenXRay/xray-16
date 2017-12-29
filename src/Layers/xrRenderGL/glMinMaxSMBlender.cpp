#include "stdafx.h"
#include "./glMinMaxSMBlender.h"

void CBlender_createminmax::Compile(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	switch (C.iElement) 
	{
   case 0:
		C.r_Pass	("stub_notransform_2uv", "create_minmax_sm", false,	FALSE,	FALSE, FALSE);
		C.PassSET_ZB		(FALSE,FALSE,FALSE	);

		C.r_Sampler_cmp		("s_smap",		r2_RT_smap_depth);

		C.r_End				();

		break;
	}

}