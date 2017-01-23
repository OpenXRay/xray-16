// Blender_Vertex_aref.cpp: implementation of the CBlender_Tree class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Blender_tree.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Tree::CBlender_Tree()
{
	description.CLS		= B_TREE;
	description.version	= 1;
	oBlend.value		= FALSE;
	oNotAnTree.value	= FALSE;
}

CBlender_Tree::~CBlender_Tree()
{

}

void	CBlender_Tree::Save		(IWriter& fs )
{
	IBlender::Save		(fs);
	xrPWRITE_PROP		(fs,"Alpha-blend",	xrPID_BOOL,		oBlend);
	xrPWRITE_PROP		(fs,"Object LOD",	xrPID_BOOL,		oNotAnTree);
}

void	CBlender_Tree::Load		(IReader& fs, u16 version )
{
	IBlender::Load		(fs,version);
	xrPREAD_PROP		(fs,xrPID_BOOL,		oBlend);
	if (version>=1)		{
		xrPREAD_PROP		(fs,xrPID_BOOL,		oNotAnTree);
	}
}

#if RENDER==R_R1
//////////////////////////////////////////////////////////////////////////
// R1
//////////////////////////////////////////////////////////////////////////
void	CBlender_Tree::Compile	(CBlender_Compile& C)
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
		u32							tree_aref		= 200;
		if (oNotAnTree.value)		tree_aref		= 0;

		switch (C.iElement)
		{
		case SE_R1_NORMAL_HQ:
			if (oNotAnTree.value)	{
				// Level view
				LPCSTR tsv	= "tree_s", tsp="vert";
				if (C.bDetail_Diffuse)	{ tsv="tree_s_dt"; tsp="vert_dt";}
				if (oBlend.value)	C.r_Pass	(tsv,	tsp,	TRUE,TRUE,TRUE,TRUE,D3DBLEND_SRCALPHA,	D3DBLEND_INVSRCALPHA,	TRUE,tree_aref);
				else				C.r_Pass	(tsv,	tsp,	TRUE,TRUE,TRUE,TRUE,D3DBLEND_ONE,		D3DBLEND_ZERO,			TRUE,tree_aref);
				C.r_Sampler			("s_base",	C.L_textures[0]);
				C.r_Sampler			("s_detail",C.detail_texture);
				C.r_End				();
			} else {
				// Level view
				if (C.bDetail_Diffuse)
				{
					if (oBlend.value)	C.r_Pass	("tree_w_dt","vert_dt",	TRUE,TRUE,TRUE,TRUE,D3DBLEND_SRCALPHA,	D3DBLEND_INVSRCALPHA,	TRUE,tree_aref);
					else				C.r_Pass	("tree_w_dt","vert_dt",	TRUE,TRUE,TRUE,TRUE,D3DBLEND_ONE,		D3DBLEND_ZERO,			TRUE,tree_aref);
					C.r_Sampler			("s_base",	C.L_textures[0]);
					C.r_Sampler			("s_detail",C.detail_texture);
					C.r_End				();
				} else {
					if (oBlend.value)	C.r_Pass	("tree_w",	"vert",		TRUE,TRUE,TRUE,TRUE,D3DBLEND_SRCALPHA,	D3DBLEND_INVSRCALPHA,	TRUE,tree_aref);
					else				C.r_Pass	("tree_w",	"vert",		TRUE,TRUE,TRUE,TRUE,D3DBLEND_ONE,		D3DBLEND_ZERO,			TRUE,tree_aref);
					C.r_Sampler			("s_base",	C.L_textures[0]);
					C.r_Sampler			("s_detail",C.detail_texture);
					C.r_End				();
				}
			}
			break;
		case SE_R1_NORMAL_LQ:
			// Level view
			if (oBlend.value)	C.r_Pass	("tree_s",	"vert",		TRUE,TRUE,TRUE,TRUE,D3DBLEND_SRCALPHA,	D3DBLEND_INVSRCALPHA,	TRUE,tree_aref);
			else				C.r_Pass	("tree_s",	"vert",		TRUE,TRUE,TRUE,TRUE,D3DBLEND_ONE,		D3DBLEND_ZERO,			TRUE,tree_aref);
			C.r_Sampler			("s_base",	C.L_textures[0]);
			C.r_End				();
			break;
		case SE_R1_LPOINT:
			C.r_Pass		((oNotAnTree.value)?"tree_s_point":"tree_w_point",	"add_point",FALSE,TRUE,FALSE,TRUE,D3DBLEND_ONE,D3DBLEND_ONE,TRUE,0);
			C.r_Sampler		("s_base",	C.L_textures[0]);
			C.r_Sampler_clf	("s_lmap",	TEX_POINT_ATT	);
			C.r_Sampler_clf	("s_att",	TEX_POINT_ATT	);
			C.r_End			();
			break;
		case SE_R1_LSPOT:
			C.r_Pass		((oNotAnTree.value)?"tree_s_spot":"tree_w_spot",	"add_spot",	FALSE,TRUE,FALSE,TRUE,D3DBLEND_ONE,D3DBLEND_ONE,TRUE,0);
			C.r_Sampler		("s_base",	C.L_textures[0]);
			C.r_Sampler_clf	("s_lmap",	"internal\\internal_light_att",		true);
			C.r_Sampler_clf	("s_att",	TEX_SPOT_ATT	);
			C.r_End			();
			break;
		case SE_R1_LMODELS:
			/*	Don't use lighting from flora - strange visual results
			//	Lighting only
			C.r_Pass		("tree_wave","vert_l",FALSE);
			C.r_Sampler		("s_base",C.L_textures[0]);
			C.r_End			();
			*/
			break;
		}
	}
}
#elif RENDER==R_R2
//////////////////////////////////////////////////////////////////////////
// R2
//////////////////////////////////////////////////////////////////////////
#include "uber_deffer.h"
void	CBlender_Tree::Compile	(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	//*************** codepath is the same, only shaders differ
	LPCSTR	tvs				= "tree";
	LPCSTR	tvs_s			= "shadow_direct_tree";
	if (oNotAnTree.value)	{ tvs="tree_s"; tvs_s="shadow_direct_tree_s"; }
	switch (C.iElement)
	{
	case SE_R2_NORMAL_HQ:	// deffer
		uber_deffer			(C,true,tvs,"base",oBlend.value);
		break;
	case SE_R2_NORMAL_LQ:	// deffer
		uber_deffer			(C,false,tvs,"base",oBlend.value);
		break;
	case SE_R2_SHADOW:		// smap-spot
//	TODO: DX10: Use dumb shader for shadowmap since shadows are drawn using hardware PCF
		if (oBlend.value)	C.r_Pass	(tvs_s,"shadow_direct_base_aref",	FALSE,TRUE,TRUE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,200);
		else				C.r_Pass	(tvs_s,"shadow_direct_base",		FALSE);
		C.r_Sampler			("s_base",	C.L_textures[0]);
		C.r_End				();
		break;
	}
}
#else
//////////////////////////////////////////////////////////////////////////
// R3
//////////////////////////////////////////////////////////////////////////
#include "uber_deffer.h"
void	CBlender_Tree::Compile	(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	//*************** codepath is the same, only shaders differ
	LPCSTR	tvs;
	LPCSTR	tvs_s;
	if (oNotAnTree.value)	
	{ 
		tvs="tree_s";
		if (oBlend.value)	tvs_s="shadow_direct_tree_s_aref"; 
		else	tvs_s="shadow_direct_tree_s"; 
	}
	else
	{
		tvs				= "tree";
		if (oBlend.value)	tvs_s="shadow_direct_tree_aref"; 
		else	tvs_s="shadow_direct_tree"; 
	}

	bool bUseATOC = (oBlend.value && (RImplementation.o.dx10_msaa_alphatest==CRender::MSAA_ATEST_DX10_0_ATOC));

	switch (C.iElement)
	{
	case SE_R2_NORMAL_HQ:	// deffer
		if (bUseATOC)
		{
			uber_deffer		(C,true,tvs,"base_atoc",oBlend.value,0,true);
			C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
			C.r_ColorWriteEnable(false, false, false, false);
			C.r_StencilRef	(0x01);
			//	Alpha to coverage.
			C.RS.SetRS	(XRDX10RS_ALPHATOCOVERAGE,	TRUE);
			C.r_End			();
		}

		uber_deffer		(C,true,tvs,"base",oBlend.value,0,true);
		C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		C.r_StencilRef	(0x01);
		//C.PassSET_ZB		(true,false);
		//	Need only for ATOC to emulate stencil test
		if (bUseATOC)
			C.RS.SetRS	( D3DRS_ZFUNC, D3DCMP_EQUAL);
		C.r_End			();
		
		break;
	case SE_R2_NORMAL_LQ:	// deffer
		if (bUseATOC)
		{
			uber_deffer		(C,false,tvs,"base_atoc",oBlend.value,0,true);
			C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
			C.r_StencilRef	(0x01);
			C.r_ColorWriteEnable(false, false, false, false);
			//	Alpha to coverage.
			C.RS.SetRS	(XRDX10RS_ALPHATOCOVERAGE,	TRUE);
			C.r_End			();
		}

		uber_deffer		(C,false,tvs,"base",oBlend.value,0,true);
		C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		C.r_StencilRef	(0x01);
		//	Need only for ATOC to emulate stencil test
		if (bUseATOC)
			C.RS.SetRS	( D3DRS_ZFUNC, D3DCMP_EQUAL);
		C.r_End			();
		break;
	case SE_R2_SHADOW:		// smap-spot
		//	TODO: DX10: Use dumb shader for shadowmap since shadows are drawn using hardware PCF
		if (oBlend.value)	C.r_Pass	(tvs_s,"shadow_direct_base_aref",	FALSE,TRUE,TRUE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,200);
		else				C.r_Pass	(tvs_s,"shadow_direct_base",		FALSE);
		//C.r_Sampler			("s_base",	C.L_textures[0]);
		C.r_dx10Texture			("s_base",	C.L_textures[0]);
		C.r_dx10Sampler			("smp_base");
		C.r_dx10Sampler			("smp_linear");
		C.r_ColorWriteEnable	(false, false, false, false);
		C.r_End				();
		break;
	}
}
#endif
