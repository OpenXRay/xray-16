// Blender_Vertex.cpp: implementation of the CBlender_LIGHT class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Blender_light.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_LIGHT::CBlender_LIGHT()
{
	description.CLS		= B_LIGHT;
}

CBlender_LIGHT::~CBlender_LIGHT()
{

}

void	CBlender_LIGHT::Save	( IWriter& fs	)
{
	IBlender::Save	(fs);
}

void	CBlender_LIGHT::Load	( IReader& fs, u16 version	)
{
	IBlender::Load	(fs,version);
}

void CBlender_LIGHT::Compile	(CBlender_Compile& C)
{
	IBlender::Compile		(C);
	C.PassBegin		();
	{
		C.PassSET_ZB		(TRUE, FALSE);
		C.PassSET_Blend		(TRUE, D3DBLEND_ONE,D3DBLEND_ONE,	TRUE,0);
		C.PassSET_LightFog	(FALSE,FALSE);
		
		// Stage0 - 2D map
		C.StageBegin		();
		C.StageSET_Address	(D3DTADDRESS_CLAMP);
		C.StageSET_Color	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,		D3DTA_TFACTOR);
		C.StageSET_Alpha	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,		D3DTA_TFACTOR);
		C.Stage_Texture		("$base0"	);
		C.Stage_Matrix		("$null",	0);
		C.Stage_Constant	("$null"	);
		C.StageEnd			();

		// Stage1 - 1D map
		C.StageBegin		();
		C.StageSET_Address	(D3DTADDRESS_CLAMP);
		C.StageSET_Color	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,		D3DTA_CURRENT);
		C.StageSET_Alpha	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,		D3DTA_CURRENT);
		C.Stage_Texture		("$base1"	);
		C.Stage_Matrix		("$null",	1);
		C.Stage_Constant	("$null"	);
		C.StageEnd			();
	}
	C.PassEnd			();
}
