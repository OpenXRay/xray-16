#include "stdafx.h"
#pragma hdrstop

#include "Blender_Blur.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Blur::CBlender_Blur()
{
	description.CLS		= B_BLUR;
}

CBlender_Blur::~CBlender_Blur()
{

}

void	CBlender_Blur::Save	( IWriter& fs	)
{
	IBlender::Save	(fs);
}

void	CBlender_Blur::Load	( IReader& fs, u16 version	)
{
	IBlender::Load	(fs,version);
}

void CBlender_Blur::Compile	(CBlender_Compile& C)
{
	IBlender::Compile		(C);
	C.PassBegin		();
	{
		C.PassSET_ZB		(FALSE, FALSE);
		C.PassSET_Blend_SET	();
		C.PassSET_LightFog	(FALSE, FALSE);

		// Stage0 - B0*F
		C.StageBegin		();
		C.StageSET_Color	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,		D3DTA_TFACTOR);
		C.StageSET_Alpha	(D3DTA_TEXTURE,	  D3DTOP_SELECTARG1,	D3DTA_TFACTOR);
		C.Stage_Texture		("$base0");
		C.Stage_Matrix		("$null",0);
		C.Stage_Constant	("$null");
		C.StageEnd			();

		// Stage1 - B1*F + current
		C.StageBegin		();
		C.StageSET_Color3	(D3DTA_TEXTURE,	  D3DTOP_MULTIPLYADD,	D3DTA_TFACTOR, D3DTA_CURRENT);
		C.StageSET_Alpha	(D3DTA_CURRENT,	  D3DTOP_SELECTARG1,	D3DTA_TFACTOR);
		C.Stage_Texture		("$base1");
		C.Stage_Matrix		("$null",1);
		C.Stage_Constant	("$null");
		C.StageEnd			();
		
		// 
		C.R().SetRS			(D3DRS_TEXTUREFACTOR,color_rgba(127,127,127,127));
	}
	C.PassEnd			();
}
