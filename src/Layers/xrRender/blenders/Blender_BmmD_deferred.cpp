#include "stdafx.h"
#pragma hdrstop

#include "Blender_BmmD.h"
#include "uber_deffer.h"

CBlender_BmmD::CBlender_BmmD()
{
    description.CLS = B_BmmD;
    xr_strcpy(oT2_Name, "$null");
    xr_strcpy(oT2_xform, "$null");
    description.version = 3;
    xr_strcpy(oR_Name, "detail" DELIMITER "detail_grnd_grass"); //"$null");
    xr_strcpy(oG_Name, "detail" DELIMITER "detail_grnd_asphalt"); //"$null");
    xr_strcpy(oB_Name, "detail" DELIMITER "detail_grnd_earth"); //"$null");
    xr_strcpy(oA_Name, "detail" DELIMITER "detail_grnd_yantar"); //"$null");
}

void CBlender_BmmD::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_MARKER(fs, "Detail map");
    xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
    xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
    xrPWRITE_PROP(fs, "R2-R", xrPID_TEXTURE, oR_Name);
    xrPWRITE_PROP(fs, "R2-G", xrPID_TEXTURE, oG_Name);
    xrPWRITE_PROP(fs, "R2-B", xrPID_TEXTURE, oB_Name);
    xrPWRITE_PROP(fs, "R2-A", xrPID_TEXTURE, oA_Name);
}

void CBlender_BmmD::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (version < 3)
    {
        xrPREAD_MARKER(fs);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
        xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
    }
    else
    {
        xrPREAD_MARKER(fs);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
        xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oR_Name);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oG_Name);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oB_Name);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oA_Name);
    }
}

LPCSTR CBlender_BmmD::getComment()
{
    return "LEVEL: Implicit**detail";
}

BOOL CBlender_BmmD::canBeDetailed()
{
    return TRUE;
}

BOOL CBlender_BmmD::canBeLMAPped()
{
    return TRUE;
}

BOOL CBlender_BmmD::canUseSteepParallax()
{
    return TRUE;
}

#if RENDER == R_R2 
void CBlender_BmmD::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    // codepath is the same, only the shaders differ
    // ***only pixel shaders differ***
    string256 mask;
    strconcat(sizeof(mask), mask, C.L_textures[0].c_str(), "_mask");
    switch (C.iElement)
    {
    case SE_R2_NORMAL_HQ: // deffer
        uber_deffer(C, true, "impl", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);
        C.r_Sampler("s_mask", mask);
        C.r_Sampler("s_lmap", C.L_textures[1]);

        C.r_Sampler(
            "s_dt_r", oR_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
        C.r_Sampler(
            "s_dt_g", oG_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
        C.r_Sampler(
            "s_dt_b", oB_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
        C.r_Sampler(
            "s_dt_a", oA_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

        C.r_Sampler("s_dn_r", strconcat(sizeof(mask), mask, oR_Name, "_bump"));
        C.r_Sampler("s_dn_g", strconcat(sizeof(mask), mask, oG_Name, "_bump"));
        C.r_Sampler("s_dn_b", strconcat(sizeof(mask), mask, oB_Name, "_bump"));
        C.r_Sampler("s_dn_a", strconcat(sizeof(mask), mask, oA_Name, "_bump"));

        if (C.bUseSteepParallax)
        {
            C.r_Sampler("s_dn_rX", strconcat(sizeof(mask), mask, oR_Name, "_bump#"));
            C.r_Sampler("s_dn_gX", strconcat(sizeof(mask), mask, oG_Name, "_bump#"));
            C.r_Sampler("s_dn_bX", strconcat(sizeof(mask), mask, oB_Name, "_bump#"));
            C.r_Sampler("s_dn_aX", strconcat(sizeof(mask), mask, oA_Name, "_bump#"));
        }

        C.r_End();
        break;
    case SE_R2_NORMAL_LQ: // deffer
        uber_deffer(C, false, "base", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);
        C.r_Sampler("s_lmap", C.L_textures[1]);
        C.r_End();
        break;
    case SE_R2_SHADOW: // smap
        if (RImplementation.o.HW_smap)
            C.r_Pass("shadow_direct_base", "dumb", FALSE, TRUE, TRUE, FALSE);
        else
            C.r_Pass("shadow_direct_base", "shadow_direct_base", FALSE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_End();
        break;
    }
}
#elif RENDER==R_GL
//////////////////////////////////////////////////////////////////////////
// GL
//////////////////////////////////////////////////////////////////////////
void	CBlender_BmmD::Compile	(CBlender_Compile& C)
{
	IBlender::Compile		(C);
	// codepath is the same, only the shaders differ
	// ***only pixel shaders differ***
	string256				mask;
	strconcat				(sizeof(mask),mask,C.L_textures[0].c_str(),"_mask");
	switch(C.iElement) 
	{
	case SE_R2_NORMAL_HQ: 		// deffer
		uber_deffer		(C, true,	"impl","impl",false,oT2_Name[0]?oT2_Name:0,true);
		C.r_Sampler		("s_mask",	mask);
		C.r_Sampler		("s_lmap",	C.L_textures[1]);

		C.r_Sampler		("s_dt_r",	oR_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC);
		C.r_Sampler		("s_dt_g",	oG_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC);
		C.r_Sampler		("s_dt_b",	oB_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC);
		C.r_Sampler		("s_dt_a",	oA_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC);

		C.r_Sampler		("s_dn_r",	strconcat(sizeof(mask),mask,oR_Name,"_bump") );
		C.r_Sampler		("s_dn_g",	strconcat(sizeof(mask),mask,oG_Name,"_bump") );
		C.r_Sampler		("s_dn_b",	strconcat(sizeof(mask),mask,oB_Name,"_bump") );
		C.r_Sampler		("s_dn_a",	strconcat(sizeof(mask),mask,oA_Name,"_bump") );

        if (C.bUseSteepParallax)
        {
		    C.r_Sampler	("s_dn_rX",	strconcat(sizeof(mask),mask,oR_Name,"_bump#") );
		    C.r_Sampler	("s_dn_gX",	strconcat(sizeof(mask),mask,oG_Name,"_bump#") );
		    C.r_Sampler	("s_dn_bX",	strconcat(sizeof(mask),mask,oB_Name,"_bump#") );
		    C.r_Sampler	("s_dn_aX",	strconcat(sizeof(mask),mask,oA_Name,"_bump#") );
        }

		C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		C.r_StencilRef	(0x01);

		C.r_End			();
		break;
	case SE_R2_NORMAL_LQ: 		// deffer
		uber_deffer		(C, false,	"base","impl",false,oT2_Name[0]?oT2_Name:0,true);

		C.r_Sampler		("s_lmap",	C.L_textures[1]);

		C.r_Stencil		( TRUE,D3DCMP_ALWAYS,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		C.r_StencilRef	(0x01);

		C.r_End			();
		break;
	case SE_R2_SHADOW:			// smap
		//if (RImplementation.o.HW_smap)	C.r_Pass	("shadow_direct_base","dumb",	FALSE,TRUE,TRUE,FALSE);
		//else							C.r_Pass	("shadow_direct_base","shadow_direct_base",FALSE);
		C.r_Pass	("shadow_direct_base","dumb",	FALSE,TRUE,TRUE,FALSE);
		C.r_Sampler		("s_base",C.L_textures[0]);
		C.r_ColorWriteEnable(false, false, false, false);
		C.r_End			();
		break;
	}
}
#else
void CBlender_BmmD::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    // codepath is the same, only the shaders differ
    // ***only pixel shaders differ***
    string256 mask;
    strconcat(sizeof(mask), mask, C.L_textures[0].c_str(), "_mask");
    switch (C.iElement)
    {
    case SE_R2_NORMAL_HQ: // deffer
        uber_deffer(C, true, "impl", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);
        // C.r_Sampler		("s_mask",	mask);
        // C.r_Sampler		("s_lmap",	C.L_textures[1]);

        // C.r_Sampler		("s_dt_r",	oR_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
        // D3DTEXF_ANISOTROPIC);
        // C.r_Sampler		("s_dt_g",	oG_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
        // D3DTEXF_ANISOTROPIC);
        // C.r_Sampler		("s_dt_b",	oB_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
        // D3DTEXF_ANISOTROPIC);
        // C.r_Sampler		("s_dt_a",	oA_Name,	false,	D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
        // D3DTEXF_ANISOTROPIC);

        // C.r_Sampler		("s_dn_r",	strconcat(sizeof(mask),mask,oR_Name,"_bump")	);
        // C.r_Sampler		("s_dn_g",	strconcat(sizeof(mask),mask,oG_Name,"_bump") );
        // C.r_Sampler		("s_dn_b",	strconcat(sizeof(mask),mask,oB_Name,"_bump") );
        // C.r_Sampler		("s_dn_a",	strconcat(sizeof(mask),mask,oA_Name,"_bump") );

        C.r_dx10Texture("s_mask", mask);
        C.r_dx10Texture("s_lmap", C.L_textures[1]);

        C.r_dx10Texture("s_dt_r", oR_Name);
        C.r_dx10Texture("s_dt_g", oG_Name);
        C.r_dx10Texture("s_dt_b", oB_Name);
        C.r_dx10Texture("s_dt_a", oA_Name);

        C.r_dx10Texture("s_dn_r", strconcat(sizeof(mask), mask, oR_Name, "_bump"));
        C.r_dx10Texture("s_dn_g", strconcat(sizeof(mask), mask, oG_Name, "_bump"));
        C.r_dx10Texture("s_dn_b", strconcat(sizeof(mask), mask, oB_Name, "_bump"));
        C.r_dx10Texture("s_dn_a", strconcat(sizeof(mask), mask, oA_Name, "_bump"));

        if (C.bUseSteepParallax)
        {
            C.r_dx10Texture("s_dn_rX", strconcat(sizeof(mask), mask, oR_Name, "_bump#"));
            C.r_dx10Texture("s_dn_gX", strconcat(sizeof(mask), mask, oG_Name, "_bump#"));
            C.r_dx10Texture("s_dn_bX", strconcat(sizeof(mask), mask, oB_Name, "_bump#"));
            C.r_dx10Texture("s_dn_aX", strconcat(sizeof(mask), mask, oA_Name, "_bump#"));
        }

        C.r_dx10Sampler("smp_base");
        C.r_dx10Sampler("smp_linear");

        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);

        C.r_End();
        break;
    case SE_R2_NORMAL_LQ: // deffer
        uber_deffer(C, false, "base", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);

        // C.r_Sampler		("s_lmap",	C.L_textures[1]);

        C.r_dx10Texture("s_lmap", C.L_textures[1]);
        C.r_dx10Sampler("smp_linear");

        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);

        C.r_End();
        break;
    case SE_R2_SHADOW: // smap
        // if (RImplementation.o.HW_smap)	C.r_Pass	("shadow_direct_base","dumb",	FALSE,TRUE,TRUE,FALSE);
        // else							C.r_Pass	("shadow_direct_base","shadow_direct_base",FALSE);
        C.r_Pass("shadow_direct_base", "dumb", FALSE, TRUE, TRUE, FALSE);
        // C.r_Sampler		("s_base",C.L_textures[0]);
        C.r_dx10Texture("s_base", C.L_textures[0]);
        C.r_dx10Sampler("smp_base");
        C.r_dx10Sampler("smp_linear");
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    }
}
#endif
