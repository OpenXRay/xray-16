// Blender_default_aref.cpp: implementation of the CBlender_default_aref class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Blender_default_aref.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_default_aref::CBlender_default_aref()
{
    description.CLS = B_DEFAULT_AREF;
    description.version = 1;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
}

CBlender_default_aref::~CBlender_default_aref() {}
void CBlender_default_aref::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_default_aref::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    switch (version)
    {
    case 0:
        xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
        oBlend.value = FALSE;
        break;
    case 1:
    default:
        xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
        break;
    }
}

void CBlender_default_aref::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    if (C.bEditor)
    {
        C.PassBegin();
        {
            C.PassSET_ZB(TRUE, TRUE);
            if (oBlend.value)
                C.PassSET_Blend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
            else
                C.PassSET_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE, oAREF.value);
            C.PassSET_LightFog(TRUE, TRUE);

            // Stage0 - Base texture
            C.StageBegin();
            C.StageSET_Address(D3DTADDRESS_WRAP);
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
            C.StageEnd();
        }
        C.PassEnd();
    }
    else
    {
        R_ASSERT3(C.L_textures.size() >= 2, "Not enought textures for shader, base tex: %s", *C.L_textures[0]);

        D3DBLEND blend_src, blend_dst;
        if (oBlend.value)
        {
            blend_src = D3DBLEND_SRCALPHA;
            blend_dst = D3DBLEND_INVSRCALPHA;
        }
        else
        {
            blend_src = D3DBLEND_ONE;
            blend_dst = D3DBLEND_ZERO;
        }

        LPCSTR tsv_hq, tsp_hq;
        LPCSTR tsv_point, tsv_spot, tsp_point, tsp_spot;
        if (C.bDetail_Diffuse)
        {
            tsv_hq = "lmap_dt";
            tsv_point = "lmap_point_dt";
            tsv_spot = "lmap_spot_dt";

            tsp_hq = "lmap_dt";
            tsp_point = "add_point_dt";
            tsp_spot = "add_spot_dt";
        }
        else
        {
            tsv_hq = "lmap";
            tsv_point = "lmap_point";
            tsv_spot = "lmap_spot";

            tsp_hq = "lmap";
            tsp_point = "add_point";
            tsp_spot = "add_spot";
        }

        switch (C.iElement)
        {
        case SE_R1_NORMAL_HQ:
        {
            C.r_Pass(tsv_hq, tsp_hq, TRUE, TRUE, TRUE, TRUE, blend_src, blend_dst, TRUE, oAREF.value);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_Sampler("s_detail", C.detail_texture);
            C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
            C.r_End();
        }
        break;
        case SE_R1_NORMAL_LQ:
        {
            C.r_Pass("lmap", "lmap", TRUE, TRUE, TRUE, TRUE, blend_src, blend_dst, TRUE, oAREF.value);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
            C.r_End();
        }
        break;
        case SE_R1_LPOINT:
        {
            C.r_Pass(tsv_point, tsp_point, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
            C.r_Sampler_clf("s_att", TEX_POINT_ATT);
            if (C.bDetail_Diffuse)
                C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
        }
        break;
        case SE_R1_LSPOT:
        {
            C.r_Pass(tsv_spot, tsp_spot, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", "internal\\internal_light_att", true);
            C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
            if (C.bDetail_Diffuse)
                C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
        }
        break;
        case SE_R1_LMODELS:
        {
            // Lighting only, not use alpha-channel
            C.r_Pass("lmap_l", "lmap_l", FALSE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
            C.r_End();
        }
        }
    }
}
