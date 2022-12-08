#include "stdafx.h"
#pragma hdrstop

#include "Blender_BmmD.h"

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

void CBlender_BmmD::CompileForEditor(CBlender_Compile& C)
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

void CBlender_BmmD::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bEditor)
    {
        CompileForEditor(C);
        return;
    }

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
        C.r_Pass("impl_point", "add_point", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
        C.r_Sampler_clf("s_att", TEX_POINT_ATT);
        C.r_End();
        break;
    case SE_R1_LSPOT:
        C.r_Pass("impl_spot", "add_spot", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", "internal" DELIMITER "internal_light_att", true);
        C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
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
