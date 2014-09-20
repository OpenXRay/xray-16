#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/ResourceManager.h"
#include "../xrRender/blenders/Blender_Recorder.h"
#include "../xrRender/blenders/Blender.h"

#include "../xrRender/dxRenderDeviceRender.h"

#include "../xrRender/tss.h"

void fix_texture_name(LPSTR fn);

void CBlender_Compile::r_Stencil(BOOL Enable, u32 Func, u32 Mask, u32 WriteMask, u32 Fail, u32 Pass, u32 ZFail)
{
	RS.SetRS(	D3DRS_STENCILENABLE,	BC(Enable) );
	if (!Enable) return;
	RS.SetRS(	D3DRS_STENCILFUNC,		Func);
	RS.SetRS(	D3DRS_STENCILMASK,		Mask);
	RS.SetRS(	D3DRS_STENCILWRITEMASK,	WriteMask);
	RS.SetRS(	D3DRS_STENCILFAIL,		Fail);
	RS.SetRS(	D3DRS_STENCILPASS,		Pass);
	RS.SetRS(	D3DRS_STENCILZFAIL,		ZFail);
	//	Since we never really support different options for
	//	CW/CCW stencil use it to mimic DX9 behaviour for 
	//	single-sided stencil
	RS.SetRS(	D3DRS_CCW_STENCILFUNC,		Func);
	RS.SetRS(	D3DRS_CCW_STENCILFAIL,		Fail);
	RS.SetRS(	D3DRS_CCW_STENCILPASS,		Pass);
	RS.SetRS(	D3DRS_CCW_STENCILZFAIL,		ZFail);
}

void CBlender_Compile::r_StencilRef(u32 Ref)
{
	RS.SetRS(	D3DRS_STENCILREF,		Ref);
}

void CBlender_Compile::r_CullMode(D3DCULL Mode)
{
	RS.SetRS(	D3DRS_CULLMODE,	(u32)Mode);
}

void CBlender_Compile::r_dx10Texture(LPCSTR ResourceName,	LPCSTR texture)
{
	VERIFY(ResourceName);
	if (!texture) return;
	//
	string256				TexName;
	xr_strcpy				(TexName,texture);
	fix_texture_name		(TexName);

	// Find index
	ref_constant C			= ctable.get(ResourceName);
	//VERIFY(C);
	if (!C)					return;

	R_ASSERT				(C->type == RC_dx10texture);
	u32 stage				= C->samp.index;

	passTextures.push_back	(mk_pair(stage, ref_texture(DEV->_CreateTexture(TexName))));
}

void CBlender_Compile::i_dx10Address(u32 s, u32 address)
{
	//VERIFY(s!=u32(-1));
   if( s == u32(-1) )
   {
      Msg( "s != u32(-1)" );
   }
	RS.SetSAMP			(s,D3DSAMP_ADDRESSU,	address);
	RS.SetSAMP			(s,D3DSAMP_ADDRESSV,	address);
	RS.SetSAMP			(s,D3DSAMP_ADDRESSW,	address);
}

void CBlender_Compile::i_dx10BorderColor(u32 s, u32 color)
{
	RS.SetSAMP			(s,D3DSAMP_BORDERCOLOR,	color);
}
void CBlender_Compile::i_dx10Filter_Min(u32 s, u32 f)
{
	VERIFY(s!=u32(-1));
	RS.SetSAMP			(s,D3DSAMP_MINFILTER,	f);
}

void CBlender_Compile::i_dx10Filter_Mip(u32 s, u32 f)
{
	VERIFY(s!=u32(-1));
	RS.SetSAMP			(s,D3DSAMP_MIPFILTER,	f);
}

void CBlender_Compile::i_dx10Filter_Mag(u32 s, u32 f)
{
	VERIFY(s!=u32(-1));
	RS.SetSAMP			(s,D3DSAMP_MAGFILTER,	f);
}

void CBlender_Compile::i_dx10FilterAnizo(u32 s, BOOL value)
{
	VERIFY(s!=u32(-1));
	RS.SetSAMP			(s,XRDX10SAMP_ANISOTROPICFILTER, value);
}

void CBlender_Compile::i_dx10Filter(u32 s, u32 _min, u32 _mip, u32 _mag)
{
	VERIFY(s!=u32(-1));
	i_dx10Filter_Min	(s,_min);
	i_dx10Filter_Mip	(s,_mip);
	i_dx10Filter_Mag	(s,_mag);
}

u32 CBlender_Compile::r_dx10Sampler(LPCSTR ResourceName)
{
	//	TEST
	//return ((u32)-1);
	VERIFY(ResourceName);
	string256				name;
	xr_strcpy				(name,ResourceName);
	fix_texture_name		(name);

	// Find index
	//ref_constant C			= ctable.get(ResourceName);
	ref_constant C			= ctable.get(name);
	//VERIFY(C);
	if (!C)					return	u32(-1);

	R_ASSERT				(C->type == RC_sampler);
	u32 stage				= C->samp.index;

	//	init defaults here

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT 
	if (0==xr_strcmp(ResourceName,"smp_nofilter"))
	{
		i_dx10Address( stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	}

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR 
	if (0==xr_strcmp(ResourceName,"smp_rtlinear"))
	{
		i_dx10Address( stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
	}

	//	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
	if (0==xr_strcmp(ResourceName,"smp_linear"))
	{
		i_dx10Address( stage, D3DTADDRESS_WRAP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
	}

	//	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC
	if (0==xr_strcmp(ResourceName,"smp_base"))
	{
		i_dx10Address( stage, D3DTADDRESS_WRAP);
		i_dx10FilterAnizo( stage, TRUE);
		//i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
	}

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
	if (0==xr_strcmp(ResourceName,"smp_material"))
	{
		i_dx10Address( stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
		RS.SetSAMP(stage,D3DSAMP_ADDRESSW,	D3DTADDRESS_WRAP);
	}

	if (0==xr_strcmp(ResourceName,"smp_smap"))
	{
		i_dx10Address( stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
		RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFILTER, TRUE);
		RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFUNC, D3D_COMPARISON_LESS_EQUAL);
	}

	if (0==xr_strcmp(ResourceName,"smp_jitter"))
	{
		i_dx10Address( stage, D3DTADDRESS_WRAP);
		i_dx10Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	}

	return					stage;
}

void	CBlender_Compile::r_Pass		(LPCSTR _vs, LPCSTR _gs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite,	BOOL bABlend, D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
	RS.Invalidate			();
	ctable.clear			();
	passTextures.clear		();
	passMatrices.clear		();
	passConstants.clear		();
	dwStage					= 0;

	// Setup FF-units (Z-buffer, blender)
	PassSET_ZB				(bZtest,bZwrite);
	PassSET_Blend			(bABlend,abSRC,abDST,aTest,aRef);
	PassSET_LightFog		(FALSE,bFog);

	// Create shaders
	SPS* ps	= DEV->_CreatePS(_ps);
	SVS* vs	= DEV->_CreateVS(_vs);
	SGS* gs	= DEV->_CreateGS(_gs);
	dest.ps	= ps;
	dest.vs	= vs;
	dest.gs	= gs;
#ifdef USE_DX11
	dest.hs = DEV->_CreateHS("null");
	dest.ds = DEV->_CreateDS("null");
#endif
	ctable.merge			(&ps->constants);
	ctable.merge			(&vs->constants);
	ctable.merge			(&gs->constants);

	// Last Stage - disable
	if (0==stricmp(_ps,"null"))	{
		RS.SetTSS				(0,D3DTSS_COLOROP,D3DTOP_DISABLE);
		RS.SetTSS				(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
	}
}

#ifdef USE_DX11
void CBlender_Compile::r_TessPass(LPCSTR vs, LPCSTR hs, LPCSTR ds, LPCSTR gs, LPCSTR ps, bool bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend, D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
	r_Pass(vs, gs, ps, bFog, bZtest, bZwrite, bABlend, abSRC, abDST, aTest, aRef);

	dest.hs = DEV->_CreateHS(hs);
	dest.ds = DEV->_CreateDS(ds);

	ctable.merge(&dest.hs->constants);
	ctable.merge(&dest.ds->constants);
}

void CBlender_Compile::r_ComputePass(LPCSTR cs)
{
	dest.cs = DEV->_CreateCS(cs);

	ctable.merge(&dest.cs->constants);
}
#endif

void	CBlender_Compile::r_End			()
{
	SetMapping				();
	dest.constants			= DEV->_CreateConstantTable(ctable);
	dest.state				= DEV->_CreateState		(RS.GetContainer());
	dest.T					= DEV->_CreateTextureList	(passTextures);
	dest.C					= 0;
	ref_matrix_list			temp(0);
	SH->passes.push_back	(DEV->_CreatePass(dest));
	//SH->passes.push_back	(DEV->_CreatePass(dest.state,dest.ps,dest.vs,dest.gs,dest.constants,dest.T,temp,dest.C));
}