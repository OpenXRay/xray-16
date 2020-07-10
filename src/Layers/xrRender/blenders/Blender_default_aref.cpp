#include "stdafx.h"
#pragma hdrstop

#include "Blender_default_aref.h"


CBlender_default_aref::CBlender_default_aref()
{
    description.CLS = B_DEFAULT_AREF;
    description.version = 1;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
}

LPCSTR CBlender_default_aref::getComment()
{
    return "LEVEL: lmap*base.aref";
}

BOOL CBlender_default_aref::canBeDetailed()
{
    return TRUE;
}

BOOL CBlender_default_aref::canBeLMAPped() 
{
    return TRUE;
}

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

    default:
        xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
        break;
    }
}

void CBlender_default_aref::CompileForEditor(CBlender_Compile& C)
{
    C.PassBegin();
    {
        const D3DBLEND blend_src = oBlend.value ? D3DBLEND_SRCALPHA : D3DBLEND_ONE;
        const D3DBLEND blend_dst = oBlend.value ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO;

        C.PassSET_Blend(true, blend_src, blend_dst, true, oAREF.value);
        C.PassSET_LightFog(true, true);

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

void CBlender_default_aref::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bEditor)
    {
        CompileForEditor(C);
        return;
    }

    R_ASSERT2(C.L_textures.size() >= 2, "Not enough textures for shader");

    const D3DBLEND blend_src = oBlend.value ? D3DBLEND_SRCALPHA : D3DBLEND_ONE;
    const D3DBLEND blend_dst = oBlend.value ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO;

    switch (C.iElement)
    {
    case SE_R1_NORMAL_HQ:
    {
        pcstr const tsv_hq = C.bDetail_Diffuse ? "lmap_dt" : "lmap";
        pcstr const tsp_hq = C.bDetail_Diffuse ? "lmap_dt" : "lmap";

        C.PassBegin();
        {
            C.PassSET_VS(tsv_hq);
            C.PassSET_PS(tsp_hq);

            C.PassSET_LightFog(false, true);
            C.PassSET_ZB(true, true);
            C.PassSET_ablend_mode(true, blend_src, blend_dst);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            C.SampledImage("s_lmap", "s_lmap", C.L_textures[1]);
            C.SampledImage("smp_rtlinear", "s_hemi", C.L_textures[2]);
            if (C.bDetail_Diffuse)
            {
                C.SampledImage("s_detail", "s_detail", C.detail_texture);
            }
        }
        C.PassEnd();
        break;
    }

    case SE_R1_NORMAL_LQ:
        C.PassBegin();
        {
            C.PassSET_VS("lmap");
            C.PassSET_PS("lmap");

            C.PassSET_LightFog(false, true);
            C.PassSET_ZB(true, true);
            C.PassSET_ablend_mode(true, blend_src, blend_dst);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            C.SampledImage("s_lmap", "s_lmap", C.L_textures[1]);
            C.SampledImage("smp_rtlinear", "s_hemi", C.L_textures[2]);
        }
        C.PassEnd();
        break;


    case SE_R1_LPOINT:
    {
        if (oBlend.value)
            break;

        C.PassBegin();
        {
            C.PassSET_VS("lmap_point");
            C.PassSET_PS("add_point");

            C.PassSET_ZB(true, false);
            C.PassSET_ablend_mode(true, D3DBLEND_ONE, D3DBLEND_ONE);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            C.SampledImage("smp_rtlinear", "s_lmap", TEX_POINT_ATT);
            C.SampledImage("smp_rtlinear", "s_att", TEX_POINT_ATT);
        }
        C.PassEnd();
        break;
    }

    case SE_R1_LSPOT:
    {
        if (oBlend.value)
            break;

        C.PassBegin();
        {
            C.PassSET_VS("lmap_spot");
            C.PassSET_PS("add_spot");

            C.PassSET_ZB(true, false);
            C.PassSET_ablend_mode(true, D3DBLEND_ONE, D3DBLEND_ONE);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            u32 stage = C.SampledImage("smp_rtlinear", "s_lmap", "internal" DELIMITER "internal_light_att");
            {
                C.i_Projective(stage, true);
            }
            C.SampledImage("smp_rtlinear", "s_att", TEX_SPOT_ATT);
        }
        C.PassEnd();
        break;
    }

    case SE_R1_LMODELS:
        C.PassBegin();
        {
            C.PassSET_VS("lmap_l");
            C.PassSET_PS("lmap_l");

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            C.SampledImage("s_lmap", "s_lmap", C.L_textures[1]);
            C.SampledImage("smp_rtlinear", "s_hemi", C.L_textures[2]);
        }
        C.PassEnd();
        break;
    }
}
