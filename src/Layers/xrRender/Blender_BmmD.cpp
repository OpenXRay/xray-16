// BlenderDefault.cpp: implementation of the CBlender_BmmD class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Blender_BmmD.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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

CBlender_BmmD::~CBlender_BmmD() {}
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

#if RENDER == R_R1
//////////////////////////////////////////////////////////////////////////
// R1
//////////////////////////////////////////////////////////////////////////
void CBlender_BmmD::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    if (C.bEditor)
    {
        C.PassBegin();
        {
            C.PassSET_ZB(TRUE, TRUE);
            C.PassSET_Blend_SET();
            C.PassSET_LightFog(TRUE, TRUE);

            // Stage1 - Base texture
            C.StageBegin();
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
            C.StageEnd();

            // Stage2 - Second texture
            C.StageBegin();
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_CURRENT);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG2, D3DTA_CURRENT);
            C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
            C.StageEnd();
        }
        C.PassEnd();
    }
    else
    {
        R_ASSERT3(C.L_textures.size() >= 2, "Not enought textures for shader, base tex: %s", *C.L_textures[0]);

        switch (C.iElement)
        {
        case SE_R1_NORMAL_HQ:
            C.r_Pass("impl_dt", "impl_dt", TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_Sampler("s_detail", oT2_Name);
            C.r_End();
            break;
        case SE_R1_NORMAL_LQ:
            C.r_Pass("impl_dt", "impl_dt", TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_Sampler("s_detail", oT2_Name);
            C.r_End();
            break;
        case SE_R1_LPOINT:
            C.r_Pass("impl_point_dt", "add_point_dt", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
            C.r_Sampler_clf("s_att", TEX_POINT_ATT);
            C.r_Sampler("s_detail", oT2_Name);
            C.r_End();
            break;
        case SE_R1_LSPOT:
            C.r_Pass("impl_spot_dt", "add_spot_dt", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", "internal" DELIMITER "internal_light_att", true);
            C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
            C.r_Sampler("s_detail", oT2_Name);
            C.r_End();
            break;
        case SE_R1_LMODELS:
            C.r_Pass("impl_l", "impl_l", FALSE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_End();
            break;
        }
    }
}
#elif RENDER == R_R2

//////////////////////////////////////////////////////////////////////////
// R2
//////////////////////////////////////////////////////////////////////////
#include "uber_deffer.h"
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
#include "uber_deffer.h"
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

		C.r_Sampler		("s_dn_r",	strconcat(sizeof(mask),mask,oR_Name,"_bump")	);
		C.r_Sampler		("s_dn_g",	strconcat(sizeof(mask),mask,oG_Name,"_bump") );
		C.r_Sampler		("s_dn_b",	strconcat(sizeof(mask),mask,oB_Name,"_bump") );
		C.r_Sampler		("s_dn_a",	strconcat(sizeof(mask),mask,oA_Name,"_bump") );

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
//////////////////////////////////////////////////////////////////////////
// R3
//////////////////////////////////////////////////////////////////////////
#include "uber_deffer.h"
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
