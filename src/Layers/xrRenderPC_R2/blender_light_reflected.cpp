#include "stdafx.h"
#pragma hdrstop

#include "Blender_light_reflected.h"

CBlender_accum_reflected::CBlender_accum_reflected	()	{	description.CLS		= 0;	}
CBlender_accum_reflected::~CBlender_accum_reflected	()	{	}

void	CBlender_accum_reflected::Compile(CBlender_Compile& C)
{
	IBlender::Compile		(C);
	BOOL		blend		= RImplementation.o.fp16_blend;
	D3DBLEND	dest		= blend?D3DBLEND_ONE:D3DBLEND_ZERO;

	C.r_Pass			("accum_volume",	"accum_indirect",false,	FALSE,FALSE,blend,D3DBLEND_ONE,dest);
	C.r_Sampler_rtf		("s_position",		r2_RT_P);
	C.r_Sampler_rtf		("s_normal",		r2_RT_N);
	C.r_Sampler_clw		("s_material",		r2_material);
	C.r_Sampler_rtf		("s_accumulator",	r2_RT_accum		);
	C.r_End				();
}
