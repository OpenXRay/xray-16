// Blender_Vertex_aref.cpp: implementation of the CBlender_Vertex_aref class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Blender_Vertex_aref.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Vertex_aref::CBlender_Vertex_aref()
{
    description.CLS = B_VERT_AREF;
    description.version = 1;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
}

CBlender_Vertex_aref::~CBlender_Vertex_aref() {}
void CBlender_Vertex_aref::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_Vertex_aref::Load(IReader& fs, u16 version)
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

void CBlender_Vertex_aref::Compile(CBlender_Compile& C)
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

            // Stage1 - Base texture
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
            C.r_Pass(tsv_hq, tsp_hq, TRUE, TRUE, TRUE, TRUE, blend_src, blend_dst, TRUE, oAREF.value);
            C.r_Sampler("s_base", C.L_textures[0]);
            if (C.bDetail_Diffuse)
                C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
        }
        break;
        case SE_R1_NORMAL_LQ:
        {
            // Level view
            C.r_Pass("vert", "vert", TRUE, TRUE, TRUE, TRUE, blend_src, blend_dst, TRUE, oAREF.value);
            C.r_Sampler("s_base", C.L_textures[0]);
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
            // Lighting only
            C.r_Pass("vert_l", "vert_l", FALSE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_End();
        }
        break;
        }
    }
}
