#include "stdafx.h"
#pragma hdrstop

#include "Blender_Editor_Wire.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Editor_Wire::CBlender_Editor_Wire()
{
	description.CLS		= B_EDITOR_WIRE;
	xr_strcpy				(oT_Factor,"$null");
}

CBlender_Editor_Wire::~CBlender_Editor_Wire()
{

}

void	CBlender_Editor_Wire::Save	( IWriter& fs	)
{
	IBlender::Save	(fs);
	xrPWRITE_PROP	(fs,"TFactor",	xrPID_CONSTANT, oT_Factor);
}

void	CBlender_Editor_Wire::Load	( IReader& fs, u16 version	)
{
	IBlender::Load	(fs,version);
	xrPREAD_PROP	(fs,xrPID_CONSTANT,	oT_Factor);
}

void CBlender_Editor_Wire::Compile	(CBlender_Compile& C)
{
	IBlender::Compile		(C);
#if !defined(USE_DX10) && !defined(USE_DX11)
	if (C.bEditor)	
	{
		C.PassBegin		();
		{
			C.PassSET_ZB		(TRUE,TRUE);
			C.PassSET_Blend		(FALSE,D3DBLEND_ONE,D3DBLEND_ZERO,	FALSE,0);
			C.PassSET_LightFog	(FALSE,FALSE);

			// Stage0 - Base texture
			C.StageBegin		();
			C.StageSET_Color	(D3DTA_DIFFUSE,	  D3DTOP_MODULATE,		D3DTA_TFACTOR);
			C.StageSET_Alpha	(D3DTA_DIFFUSE,	  D3DTOP_MODULATE,		D3DTA_TFACTOR);
			C.Stage_Texture		("$null");
			C.Stage_Matrix		("$null",	0);
			C.Stage_Constant	("$null");
			//		C.Stage_Constant	("$base0",	"$user$wire");
			C.StageEnd			();
		}
		C.PassEnd			();
	} 
	else 
#endif	//	USE_DX10
	{
		C.r_Pass	("editor","simple_color",FALSE,TRUE,TRUE);
		C.r_End		();
	}
}
