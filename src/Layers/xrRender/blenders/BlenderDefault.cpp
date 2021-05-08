#include "stdafx.h"
#pragma hdrstop

#include "BlenderDefault.h"


#if RENDER != R_R1
#error "The blender can't be used in this renderer generation"
#endif

CBlender_default::CBlender_default()
{
    description.CLS = B_DEFAULT;
    description.version = 1;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
}

LPCSTR CBlender_default::getComment()
{
    return "LEVEL: lmap*base (default)";
}

BOOL CBlender_default::canBeDetailed()
{
    return TRUE;
}

BOOL CBlender_default::canBeLMAPped() 
{
    return TRUE;
}

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

void CBlender_default::CompileForEditor(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_LightFog(true, true);

        // Stage1 - Base texture
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

void CBlender_default::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bEditor)
    {
        CompileForEditor(C);
        return;
    }

    R_ASSERT2(C.L_textures.size() >= 3, "Not enought textures for shader");

    switch (C.iElement)
    {
    case SE_R1_NORMAL_HQ:
    {
        pcstr const tsv_hq = C.bDetail_Diffuse ? "lmap_dt" : "lmap";
        pcstr const tsp_hq = C.bDetail_Diffuse ? "lmap_dt" : "lmap";

        // Level view
        C.PassBegin();
        {
            C.PassSET_VS(tsv_hq);
            C.PassSET_PS(tsp_hq);

            C.PassSET_LightFog(false, true);

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

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            C.SampledImage("s_lmap", "s_lmap", C.L_textures[1]);
            C.SampledImage("smp_rtlinear", "s_hemi", C.L_textures[2]);
        }
        C.PassEnd();
        break;

    case SE_R1_LPOINT:
    {
        C.PassBegin();
        {
            C.PassSET_VS("lmap_point");
            C.PassSET_PS("add_point");

            C.PassSET_ZB(true, false);
            C.PassSET_ablend_mode(true, D3DBLEND_ONE, D3DBLEND_ONE);
            C.PassSET_ablend_aref(true, 0);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            C.SampledImage("smp_rtlinear", "s_lmap", TEX_POINT_ATT);
            C.SampledImage("smp_rtlinear", "s_att", TEX_POINT_ATT);
        }
        C.PassEnd();
        break;
    }

    case SE_R1_LSPOT:
    {
        C.PassBegin();
        {
            C.PassSET_VS("lmap_spot");
            C.PassSET_PS("add_spot");

            C.PassSET_ZB(true, false);
            C.PassSET_ablend_mode(true, D3DBLEND_ONE, D3DBLEND_ONE);
            C.PassSET_ablend_aref(true, 0);

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
        // Lighting only, not use alpha-channel
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

    default:
        break;
    }
}
