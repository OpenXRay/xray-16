#include "stdafx.h"
#pragma hdrstop

#include "Blender_Model.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Model::CBlender_Model()
{
    description.CLS = B_MODEL;
    description.version = 2;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
}

CBlender_Model::~CBlender_Model() {}
void CBlender_Model::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Use alpha-channel", xrPID_BOOL, oBlend);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
    xrP_TOKEN::Item I;
    xrPWRITE_PROP(fs, "Tessellation", xrPID_TOKEN, oTessellation);
    I.ID = 0;
    xr_strcpy(I.str, "NO_TESS");
    fs.w(&I, sizeof(I));
    I.ID = 1;
    xr_strcpy(I.str, "TESS_PN");
    fs.w(&I, sizeof(I));
    I.ID = 2;
    xr_strcpy(I.str, "TESS_HM");
    fs.w(&I, sizeof(I));
    I.ID = 3;
    xr_strcpy(I.str, "TESS_PN+HM");
    fs.w(&I, sizeof(I));
}

void CBlender_Model::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    switch (version)
    {
    case 0:
        oAREF.value = 32;
        oAREF.min = 0;
        oAREF.max = 255;
        oBlend.value = FALSE;
        break;
    case 1:
    default:
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
        xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
        break;
    }
    if (version > 1)
    {
        xrPREAD_PROP(fs, xrPID_TOKEN, oTessellation);
    }
}

void CBlender_Model::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    if (C.bEditor)
    {
        C.PassBegin();
        {
            C.PassSET_ZB(TRUE, oBlend.value && (oAREF.value < 200) ? FALSE : TRUE);
            if (oBlend.value)
                C.PassSET_Blend_BLEND(TRUE, oAREF.value);
            else
                C.PassSET_Blend_SET();
            C.PassSET_LightFog(TRUE, TRUE);
            C.StageBegin();
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
            C.StageSET_TMC(oT_Name, "$null", "$null", 0);
            C.StageEnd();
        }
        C.PassEnd();
    }
    else
    {
        LPCSTR tsv_hq, tsp_hq;
        LPCSTR tsv_point, tsv_spot, tsp_point, tsp_spot;
        if (C.bDetail_Diffuse)
        {
            tsv_hq = "model_def_hq_dt";
            tsv_point = "model_def_point_dt";
            tsv_spot = "model_def_spot_dt";

            tsp_hq = "model_def_hq_dt";
            tsp_point = "add_point_dt";
            tsp_spot = "add_spot_dt";
        }
        else
        {
            tsv_hq = "model_def_hq";
            tsv_point = "model_def_point";
            tsv_spot = "model_def_spot";

            tsp_hq = "model_def_hq";
            tsp_point = "add_point";
            tsp_spot = "add_spot";
        }

        switch (C.iElement)
        {
        case SE_R1_NORMAL_HQ:
            if (oBlend.value)
                C.r_Pass(
                    tsv_hq, tsp_hq, TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
            else
                C.r_Pass(tsv_hq, tsp_hq, TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", "$user$projector", true);
            if (C.bDetail_Diffuse)
                C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
            break;
        case SE_R1_NORMAL_LQ:
            if (oBlend.value)
                C.r_Pass("model_def_lq", "model_def_lq", TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA,
                    D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
            else
                C.r_Pass("model_def_lq", "model_def_lq", TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_End();
            break;
        case SE_R1_LPOINT:
            if (oBlend.value)
                C.r_Pass(tsv_point, tsp_point, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
            else
                C.r_Pass(tsv_point, tsp_point, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
            C.r_Sampler_clf("s_att", TEX_POINT_ATT);
            if (C.bDetail_Diffuse)
                C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
            break;
        case SE_R1_LSPOT:
            if (oBlend.value)
                C.r_Pass(tsv_spot, tsp_spot, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
            else
                C.r_Pass(tsv_spot, tsp_spot, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", "internal\\internal_light_att", true);
            C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
            if (C.bDetail_Diffuse)
                C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
            break;
        case SE_R1_LMODELS:
            C.r_Pass("model_def_shadow", "model_shadow", FALSE, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_SRCCOLOR,
                FALSE, 0);
            C.r_End();
            break;
        }
    }
}
