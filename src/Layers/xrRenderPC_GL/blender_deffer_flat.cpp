#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/uber_deffer.h"
#include "Blender_deffer_flat.h"

CBlender_deffer_flat::CBlender_deffer_flat	()	{	description.CLS		= B_DEFAULT;	}
CBlender_deffer_flat::~CBlender_deffer_flat	()	{	}

void	CBlender_deffer_flat::Save	(	IWriter& fs )
{
	IBlender::Save	(fs);
}
void	CBlender_deffer_flat::Load	(	IReader& fs, u16 version )
{
	IBlender::Load	(fs,version);
}

void	CBlender_deffer_flat::Compile(CBlender_Compile& C)
{
	IBlender::Compile		(C);

	// codepath is the same, only the shaders differ
	switch(C.iElement) 
	{
	case SE_R2_NORMAL_HQ: 		// deffer
		uber_deffer		(C,true,"base","base",false);
		break;
	case SE_R2_NORMAL_LQ: 		// deffer
		uber_deffer		(C,false,"base","base",false);
		break;
	case SE_R2_SHADOW:			// smap-direct
		if (RImplementation.o.HW_smap)	C.r_Pass	("shadow_direct_base","dumb",	FALSE,TRUE,TRUE,FALSE);
		else							C.r_Pass	("shadow_direct_base","shadow_direct_base",FALSE);
		C.r_Sampler		("s_base",C.L_textures[0]);
		C.r_End			();
		break;
	}
}
