#include "stdafx.h"
#pragma hdrstop

#include "Blender_light_occq.h"

CBlender_light_occq::CBlender_light_occq	()	{	description.CLS		= 0;	}
CBlender_light_occq::~CBlender_light_occq	()	{	}
//	TODO: DX10: if nesessary for NV stencil optimisation implement pass 1
void	CBlender_light_occq::Compile(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	switch (C.iElement) 
	{
	case 0:			// occlusion testing
		C.r_Pass	("dumb", "dumb",false,TRUE,FALSE,FALSE);
		C.r_End		();
		//	Color write as well as culling and stencil are set up manually in code.
		break;
	case 1:			// NV40 optimization :)
		C.r_Pass	("stub_notransform_t", "dumb",false,FALSE,FALSE,FALSE);
		C.r_ColorWriteEnable	(false,false,false,false);
		C.r_CullMode			(D3DCULL_NONE);
		C.r_Stencil				(TRUE,D3DCMP_LESSEQUAL,0xff,0x00);	// keep/keep/keep
		C.r_End		();
		break;
	case 2:			// Stencil clear in case we've ran out of markers.
		C.r_Pass	("stub_notransform_t", "dumb",false,FALSE,FALSE,FALSE);
		C.r_ColorWriteEnable	(false,false,false,false);
		C.r_CullMode			(D3DCULL_NONE);
		if( RImplementation.o.dx10_msaa )
			C.r_Stencil				(TRUE,D3DCMP_ALWAYS,0x00,0x7E, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO);
		else
		{
			//	Clear all bits except the last one
			C.r_Stencil				(TRUE,D3DCMP_ALWAYS,0x00,0xFE, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO);
		}
		//C.r_Stencil				(TRUE,D3DCMP_ALWAYS,0x00,0xFF, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO);	// keep/keep/keep
		C.r_End		();
		break;
	}
}
