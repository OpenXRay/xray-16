// Blender_Vertex_aref.cpp: implementation of the CBlender_Detail_Still class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Blender_Detail_still.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Detail_Still::CBlender_Detail_Still()
{
	description.CLS		= B_DETAIL;
	description.version	= 0;
}

CBlender_Detail_Still::~CBlender_Detail_Still()
{

}

void	CBlender_Detail_Still::Save		(IWriter& fs )
{
	IBlender::Save		(fs);
	xrPWRITE_PROP		(fs,"Alpha-blend",	xrPID_BOOL,		oBlend);
}

void	CBlender_Detail_Still::Load		(IReader& fs, u16 version )
{
	IBlender::Load		(fs,version);
	xrPREAD_PROP		(fs,xrPID_BOOL,		oBlend);
}

#if RENDER==R_R1
void	CBlender_Detail_Still::Compile	(CBlender_Compile& C)
{
	IBlender::Compile	(C);
	
	if (C.bEditor)
	{
		C.PassBegin		();
		{
			C.PassSET_ZB		(TRUE,TRUE);
			if (oBlend.value)	C.PassSET_Blend_BLEND	(TRUE, 200);
			else				C.PassSET_Blend_SET		(TRUE, 200);
			C.PassSET_LightFog	(TRUE,TRUE);
			
			// Stage1 - Base texture
			C.StageBegin		();
			C.StageSET_Color	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,	D3DTA_DIFFUSE);
			C.StageSET_Alpha	(D3DTA_TEXTURE,	  D3DTOP_MODULATE,	D3DTA_DIFFUSE);
			C.StageSET_TMC		(oT_Name,"$null","$null",0);
			C.StageEnd			();
		}
		C.PassEnd			();
	} else {
		switch (C.iElement)
		{
		case SE_R1_NORMAL_HQ:
			C.r_Pass	("detail_wave",		"detail",FALSE,TRUE,TRUE,FALSE, D3DBLEND_ONE,D3DBLEND_ZERO,oBlend.value?TRUE:FALSE,oBlend.value?200:0);
			C.r_Sampler	("s_base",	C.L_textures[0]);
			C.r_End		();
			break;
		case SE_R1_NORMAL_LQ:
			C.r_Pass	("detail_still",	"detail",FALSE,TRUE,TRUE,FALSE, D3DBLEND_ONE,D3DBLEND_ZERO,oBlend.value?TRUE:FALSE,oBlend.value?200:0);
			C.r_Sampler	("s_base",	C.L_textures[0]);
			C.r_End		();
			break;
		case SE_R1_LPOINT:
			break;
		case SE_R1_LSPOT:
			break;
		case SE_R1_LMODELS:
			break;
		}
	}
}
#elif RENDER==R_R2
//////////////////////////////////////////////////////////////////////////
// R2
//////////////////////////////////////////////////////////////////////////
#include "uber_deffer.h"
void	CBlender_Detail_Still::Compile	(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	switch(C.iElement) 
	{
	case SE_R2_NORMAL_HQ: 		// deffer wave
		uber_deffer				(C,false,"detail_w","base",true);
		break;
	case SE_R2_NORMAL_LQ: 		// deffer still
		uber_deffer				(C,false,"detail_s","base",true);
		break;
	}
}
#else
//////////////////////////////////////////////////////////////////////////
// R3
//////////////////////////////////////////////////////////////////////////
#include "uber_deffer.h"
void	CBlender_Detail_Still::Compile	(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	bool bUseATOC = (RImplementation.o.dx10_msaa_alphatest==CRender::MSAA_ATEST_DX10_0_ATOC);

	switch(C.iElement) 
	{
	case SE_R2_NORMAL_HQ: 		// deffer wave
		if (bUseATOC)
		{
			uber_deffer		(C,false,"detail_w","base_atoc",true,0,true);
			C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
			C.r_StencilRef	(0x01);
			C.r_ColorWriteEnable(false, false, false, false);
			C.r_CullMode	(D3DCULL_NONE);
			//	Alpha to coverage.
			C.RS.SetRS	(XRDX10RS_ALPHATOCOVERAGE,	TRUE);
			C.r_End			();
		}
		

		uber_deffer		(C,false,"detail_w","base",true, 0, true);
		C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		C.r_StencilRef	(0x01);
		C.r_CullMode	(D3DCULL_NONE);
		if (bUseATOC)
			C.RS.SetRS	( D3DRS_ZFUNC, D3DCMP_EQUAL);
		C.r_End			();
		break;
	case SE_R2_NORMAL_LQ: 		// deffer still
		if (bUseATOC)
		{
			uber_deffer		(C,false,"detail_s","base_atoc",true,0,true);
			C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
			C.r_StencilRef	(0x01);
			C.r_CullMode	(D3DCULL_NONE);
			C.r_ColorWriteEnable(false, false, false, false);
			//	Alpha to coverage.
			C.RS.SetRS	(XRDX10RS_ALPHATOCOVERAGE,	TRUE);
			C.r_End			();
		}

		uber_deffer		(C,false,"detail_s","base",true, 0, true);
		C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		C.r_StencilRef	(0x01);
		C.r_CullMode	(D3DCULL_NONE);
		//	Need this for ATOC
		if (bUseATOC)
			C.RS.SetRS	( D3DRS_ZFUNC, D3DCMP_EQUAL);
		C.r_End			();
		break;
	}
}
#endif
