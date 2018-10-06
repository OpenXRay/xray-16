// BlenderDefault.cpp: implementation of the CBlender_default class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "BlenderDefault.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_default::CBlender_default()
{
    description.CLS = B_DEFAULT;
    description.version = 1;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
}

CBlender_default::~CBlender_default() {}
void CBlender_default::Save(IWriter& fs)
{
    IBlender::Save(fs);
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
void CBlender_default::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (version > 0)
    {
        xrPREAD_PROP(fs, xrPID_TOKEN, oTessellation);
    }
}
void CBlender_default::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    if (C.bEditor)
    {
        C.PassBegin();
        {
            C.PassSET_ZB(TRUE, TRUE);
            C.PassSET_Blend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO, FALSE, 0);
            C.PassSET_LightFog(TRUE, TRUE);

            // Stage1 - Base texture
            C.StageBegin();
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
            C.StageEnd();
        }
        C.PassEnd();
    }
    else
    {
        R_ASSERT3(C.L_textures.size() >= 3, "Not enought textures for shader, base tex: %s", *C.L_textures[0]);

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
            // Level view
            C.r_Pass(tsv_hq, tsp_hq, TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_Sampler("s_detail", C.detail_texture);
            C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
            C.r_End();
        }
        break;
        case SE_R1_NORMAL_LQ:
        {
            C.r_Pass("lmap", "lmap", TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_lmap", C.L_textures[1]);
            C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
            C.r_End();
        }
        break;
        case SE_R1_LPOINT:
        {
            C.r_Pass(tsv_point, tsp_point, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
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
            C.r_Pass(tsv_spot, tsp_spot, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler_clf("s_lmap", "internal" DELIMITER "internal_light_att", true);
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
        break;
        }
    }
}
