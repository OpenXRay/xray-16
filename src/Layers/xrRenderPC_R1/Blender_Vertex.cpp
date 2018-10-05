// Blender_Vertex.cpp: implementation of the CBlender_Vertex class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Blender_Vertex.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Vertex::CBlender_Vertex()
{
    description.CLS = B_VERT;
    description.version = 1;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
}

CBlender_Vertex::~CBlender_Vertex() {}
void CBlender_Vertex::Save(IWriter& fs)
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

void CBlender_Vertex::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (version > 0)
    {
        xrPREAD_PROP(fs, xrPID_TOKEN, oTessellation);
    }
}

void CBlender_Vertex::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bEditor)
    {
        // Editor shader
        C.PassBegin();
        {
            C.PassSET_ZB(TRUE, TRUE);
            C.PassSET_Blend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO, FALSE, 0);
            C.PassSET_LightFog(TRUE, TRUE);

            // Stage0 - Base texture
            C.StageBegin();
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix(oT_xform, 0);
            C.Stage_Constant("$null");
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
            tsv_hq = "vert_dt";
            tsv_point = "vert_point_dt";
            tsv_spot = "vert_spot_dt";

            tsp_hq = "vert_dt";
            tsp_point = "add_point_dt";
            tsp_spot = "add_spot_dt";
        }
        else
        {
            tsv_hq = "vert";
            tsv_point = "vert_point";
            tsv_spot = "vert_spot";

            tsp_hq = "vert";
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
            if (C.bDetail_Diffuse)
                C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
        }
        break;
        case SE_R1_NORMAL_LQ:
        {
            // Level view
            C.r_Pass("vert", "vert", TRUE);
            C.r_Sampler("s_base", C.L_textures[0]);
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
            // Lighting only
            C.r_Pass("vert_l", "vert_l", FALSE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_End();
        }
        break;
        }
    }
}
