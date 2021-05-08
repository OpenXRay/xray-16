#include "stdafx.h"
#pragma hdrstop

#include "Blender_tree.h"

CBlender_Tree::CBlender_Tree()
{
    description.CLS = B_TREE;
    description.version = 1;
    oBlend.value = FALSE;
    oNotAnTree.value = FALSE;
}

void CBlender_Tree::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
    xrPWRITE_PROP(fs, "Object LOD", xrPID_BOOL, oNotAnTree);
}

void CBlender_Tree::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
    if (version >= 1)
    {
        xrPREAD_PROP(fs, xrPID_BOOL, oNotAnTree);
    }
}

LPCSTR CBlender_Tree::getComment()
{
    return "LEVEL: trees/bushes";
}

BOOL CBlender_Tree::canBeDetailed()
{
    return TRUE;
}

void CBlender_Tree::CompileForEditor(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        if (oBlend.value)
            C.PassSET_Blend_BLEND(TRUE, 200);
        else
            C.PassSET_Blend_SET(TRUE, 200);
        C.PassSET_LightFog(TRUE, TRUE);

        // Stage1 - Base texture
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT_Name, "$null", "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

void CBlender_Tree::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bEditor)
    {
        CompileForEditor(C);
        return;
    }

    u32 tree_aref = 200;
    if (oNotAnTree.value)
        tree_aref = 0;

    switch (C.iElement)
    {
    case SE_R1_NORMAL_HQ:
        if (oNotAnTree.value)
        {
            // Level view
            LPCSTR tsv = "tree_s", tsp = "vert";
            if (C.bDetail_Diffuse)
            {
                tsv = "tree_s_dt";
                tsp = "vert_dt";
            }
            if (oBlend.value)
                C.r_Pass(
                    tsv, tsp, TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, tree_aref);
            else
                C.r_Pass(tsv, tsp, TRUE, TRUE, TRUE, TRUE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE, tree_aref);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_detail", C.detail_texture);
            C.r_End();
        }
        else
        {
            // Level view
            if (C.bDetail_Diffuse)
            {
                if (oBlend.value)
                    C.r_Pass("tree_w_dt", "vert_dt", TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA,
                        D3DBLEND_INVSRCALPHA, TRUE, tree_aref);
                else
                    C.r_Pass("tree_w_dt", "vert_dt", TRUE, TRUE, TRUE, TRUE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE,
                        tree_aref);
                C.r_Sampler("s_base", C.L_textures[0]);
                C.r_Sampler("s_detail", C.detail_texture);
                C.r_End();
            }
            else
            {
                if (oBlend.value)
                    C.r_Pass("tree_w", "vert", TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA,
                        TRUE, tree_aref);
                else
                    C.r_Pass(
                        "tree_w", "vert", TRUE, TRUE, TRUE, TRUE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE, tree_aref);
                C.r_Sampler("s_base", C.L_textures[0]);
                C.r_Sampler("s_detail", C.detail_texture);
                C.r_End();
            }
        }
        break;
    case SE_R1_NORMAL_LQ:
        // Level view
        if (oBlend.value)
            C.r_Pass(
                "tree_s", "vert", TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, tree_aref);
        else
            C.r_Pass("tree_s", "vert", TRUE, TRUE, TRUE, TRUE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE, tree_aref);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_End();
        break;
    case SE_R1_LPOINT:
        C.r_Pass((oNotAnTree.value) ? "tree_s_point" : "tree_w_point", "add_point", FALSE, TRUE, FALSE, TRUE,
            D3DBLEND_ONE, D3DBLEND_ONE, TRUE, 0);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
        C.r_Sampler_clf("s_att", TEX_POINT_ATT);
        C.r_End();
        break;
    case SE_R1_LSPOT:
        C.r_Pass((oNotAnTree.value) ? "tree_s_spot" : "tree_w_spot", "add_spot", FALSE, TRUE, FALSE, TRUE,
            D3DBLEND_ONE, D3DBLEND_ONE, TRUE, 0);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", "internal" DELIMITER "internal_light_att", true);
        C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
        C.r_End();
        break;
    case SE_R1_LMODELS:
        /*	Don't use lighting from flora - strange visual results
        //	Lighting only
        C.r_Pass		("tree_wave","vert_l",FALSE);
        C.r_Sampler		("s_base",C.L_textures[0]);
        C.r_End			();
        */
        break;
    }
}
