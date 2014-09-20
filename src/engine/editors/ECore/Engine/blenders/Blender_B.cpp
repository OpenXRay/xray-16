// BlenderDefault.cpp: implementation of the CBlender_B class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "blender_B.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_B::CBlender_B	()
{
	description.CLS			= B_B;
	description.version		= 2;
}

CBlender_B::~CBlender_B	()
{
	
}

void	CBlender_B::Save		(IWriter& fs )
{
	IBlender::Save	(fs);
}

void	CBlender_B::Load		(IReader& fs, u16 version )
{
	IBlender::Load	(fs,version);

	string64		skip;
	switch(version) {
	case 0:
	case 1:
		xrPREAD_MARKER	(fs);
		xrPREAD_PROP	(fs,xrPID_TEXTURE,	skip);
		xrPREAD_PROP	(fs,xrPID_MATRIX,	skip);
		break;
	case 2:
	default:
		break;
	}
}

void	CBlender_B::Compile	(CBlender_Compile& C)
{
	IBlender::Compile		(C);
	if (C.bEditor)	{
		C.PassBegin		();
		{
			C.PassSET_ZB		(TRUE,TRUE);
			C.PassSET_Blend_SET	();
			C.PassSET_LightFog	(TRUE,TRUE);
			
			// Stage1 - Base texture
			C.StageBegin		();
			C.StageSET_Color	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,		D3DTA_DIFFUSE);
			C.StageSET_Alpha	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,		D3DTA_DIFFUSE);
			C.StageSET_TMC		(oT_Name,oT_xform,"$null",0);
			C.StageEnd			();
		}
		C.PassEnd			();
	} else {
		if (2==C.iElement)	
		{
			C.PassBegin		();
			{
				C.PassSET_ZB		(TRUE,TRUE);
				C.PassSET_Blend_SET	();
				C.PassSET_LightFog	(FALSE,TRUE);
				
				// Stage1 - Base texture
				C.StageBegin		();
				C.StageSET_Color	(D3DTA_TEXTURE,	  D3DTOP_SELECTARG1,	D3DTA_DIFFUSE);
				C.StageSET_Alpha	(D3DTA_TEXTURE,	  D3DTOP_SELECTARG1,	D3DTA_DIFFUSE);
				C.StageSET_TMC		(oT_Name,oT_xform,"$null",0);
				C.StageEnd			();
			}
			C.PassEnd			();
		} else {
			C.PassBegin		();
			{
				C.PassSET_ZB		(TRUE,TRUE);
				C.PassSET_Blend_SET	();
				C.PassSET_LightFog	(FALSE,TRUE);
				
				// Stage1 - Base texture
				C.StageBegin		();
				C.StageSET_Color	(D3DTA_TEXTURE,	  D3DTOP_SELECTARG1,	D3DTA_DIFFUSE);
				C.StageSET_Alpha	(D3DTA_TEXTURE,	  D3DTOP_SELECTARG1,	D3DTA_DIFFUSE);
				C.StageSET_TMC		(oT_Name,oT_xform,"$null",0);
				C.StageEnd			();
			}
			C.PassEnd			();
		}
	}
}
