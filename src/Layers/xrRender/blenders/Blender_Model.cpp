#include "stdafx.h"
#pragma hdrstop

#include "Blender_Model.h"

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

LPCSTR CBlender_Model::getComment()
{
    return "MODEL: Default";
}

void CBlender_Model::CompileForEditor(CBlender_Compile& C)
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

void CBlender_Model::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bEditor)
    {
        CompileForEditor(C);
        return;
    }

    LPCSTR vsname = nullptr;
    LPCSTR psname = nullptr;
    switch (C.iElement)
    {
    case SE_R1_NORMAL_HQ:
        vsname = psname = "model_def_hq";
        if (oBlend.value)
            C.r_Pass(
                vsname, psname, TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", "$user$projector", true);
        C.r_End();
        break;
    case SE_R1_NORMAL_LQ:
        vsname = psname = "model_def_lq";
        if (oBlend.value)
            C.r_Pass(
                vsname, psname, TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_End();
        break;
    case SE_R1_LPOINT:
        vsname = "model_def_point";
        psname = "add_point";
        if (oBlend.value)
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
        C.r_Sampler_clf("s_att", TEX_POINT_ATT);
        C.r_End();
        break;
    case SE_R1_LSPOT:
        vsname = "model_def_spot";
        psname = "add_spot";
        if (oBlend.value)
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", "internal" DELIMITER "internal_light_att", true);
        C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
        C.r_End();
        break;
    case SE_R1_LMODELS:
        vsname = "model_def_shadow";
        psname = "model_shadow";
        C.r_Pass(vsname, psname, FALSE, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, FALSE, 0);
        C.r_End();
        break;
    }
}
