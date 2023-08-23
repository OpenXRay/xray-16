#include "stdafx.h"
#pragma hdrstop

#include "Blender_Lm(EbB).h"

CBlender_LmEbB::CBlender_LmEbB()
{
    description.CLS = B_LmEbB;
    description.version = 0x1;
    xr_strcpy(oT2_Name, "$null");
    xr_strcpy(oT2_xform, "$null");
    oBlend.value = FALSE;
}

void CBlender_LmEbB::Save(IWriter& fs)
{
    description.version = 0x1;
    IBlender::Save(fs);
    xrPWRITE_MARKER(fs, "Environment map");
    xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
    xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
    xrPWRITE_PROP(fs, "Alpha-Blend", xrPID_BOOL, oBlend);
}

void CBlender_LmEbB::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_MARKER(fs);
    xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
    xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
    if (version >= 0x1)
    {
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
    }
}

LPCSTR CBlender_LmEbB::getComment()
{
    return "LEVEL: lmap*(env^base)";
}

BOOL CBlender_LmEbB::canBeLMAPped()
{
    return TRUE;
}

void CBlender_LmEbB::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bFFP)
        CompileFFP(C);
    else
        CompileProgrammable(C);
}

void CBlender_LmEbB::CompileFFP(CBlender_Compile& C) const
{
    if (!ps_r1_flags.is_any(R1FLAG_FFP_LIGHTMAPS | R1FLAG_DLIGHTS))
        compile_ED(C);
    else
    {
        switch (C.iElement)
        {
        case SE_R1_NORMAL_HQ:
        case SE_R1_NORMAL_LQ:
        {
            switch (HW.Caps.raster.dwStages)
            {
            case 2: // Geforce1/2/MX
                compile_2(C);
                break;
            case 3: // Kyro, Radeon, Radeon2, Geforce3/4
            default:
                compile_3(C);
                break;
            } // switch (HW.Caps.raster.dwStages)
            break;
        }
        case SE_R1_LMODELS:
        {
            compile_L(C);
            break;
        }
        } // switch (C.iElement)
    }
}

void CBlender_LmEbB::compile_ED(CBlender_Compile& C) const
{
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(TRUE, TRUE);

        // Stage1 - Env texture
        C.StageBegin();
        C.StageSET_Address(D3DTADDRESS_CLAMP);
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
        C.StageEnd();

        // Stage2 - Base texture
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_BLENDTEXTUREALPHA, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();

        // Stage3 - Lighting - should work on all 2tex hardware
        C.StageBegin();
        C.StageSET_Color(D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_CURRENT);
        C.Stage_Texture("$null");
        C.Stage_Matrix("$null", 0);
        C.Stage_Constant("$null");
        C.StageEnd();
    }
    C.PassEnd();
}

void CBlender_LmEbB::compile_2(CBlender_Compile& C) const
{
    // Pass1 - Lighting
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - Detail
        if (ps_r1_flags.test(R1FLAG_FFP_LIGHTMAPS))
        {
            C.StageBegin();
            C.StageTemplate_LMAP0();
            C.StageEnd();
        }
    }
    C.PassEnd();

    // Pass2 - (env^base)
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, FALSE);
        C.PassSET_Blend_MUL2X();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - Environment map
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
        C.StageEnd();

        // Stage1 - Base map
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_BLENDTEXTUREALPHA, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

void CBlender_LmEbB::compile_3(CBlender_Compile& C) const
{
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - Environment map
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
        C.StageEnd();

        // Stage1 - [^] Base map
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_BLENDTEXTUREALPHA, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();

        // Stage2 - [*] Lightmap
        if (ps_r1_flags.test(R1FLAG_FFP_LIGHTMAPS))
        {
            C.StageBegin();
            C.StageSET_Address(D3DTADDRESS_CLAMP);
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_CURRENT);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG2, D3DTA_CURRENT);
            C.Stage_Texture("$base1");
            C.Stage_Matrix("$null", 1);
            C.Stage_Constant("$null");
            C.StageEnd();
        }
    }
    C.PassEnd();
}

void CBlender_LmEbB::compile_L(CBlender_Compile& C) const
{
    // Pass1 - Lighting
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, FALSE);

        // Stage0 - Detail
        if (ps_r1_flags.test(R1FLAG_FFP_LIGHTMAPS))
        {
            C.StageBegin();
            C.StageTemplate_LMAP0();
            C.StageEnd();
        }
    }
    C.PassEnd();
}

void CBlender_LmEbB::CompileProgrammable(CBlender_Compile& C) const
{
    R_ASSERT3(C.L_textures.size() >= 2, "Not enought textures for shader, base tex: %s", *C.L_textures[0]);

    switch (C.iElement)
    {
    case SE_R1_NORMAL_HQ:
    case SE_R1_NORMAL_LQ:
        // Level view
        if (oBlend.value)
            C.r_Pass("lmapE", "lmapE", TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, 0);
        else
            C.r_Pass("lmapE", "lmapE", TRUE);
#if RENDER == R_R3 || RENDER == R_R4
        C.r_dx11Texture("s_base", C.L_textures[0]);
        C.r_dx11Sampler("smp_base");
        C.r_dx11Texture("s_lmap", C.L_textures[1]);
        C.r_dx11Sampler("smp_linear");
        C.r_dx11Texture("s_hemi", *C.L_textures[2]);
        C.r_dx11Sampler("smp_rtlinear");
        C.r_dx11Texture("s_env", oT2_Name);
#else
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler("s_lmap", C.L_textures[1]);
        C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
        C.r_Sampler("s_env", oT2_Name, false, D3DTADDRESS_CLAMP);
#endif
        C.r_End();
        break;
#if RENDER == R_R1
    case SE_R1_LPOINT:
        C.r_Pass("lmap_point", "add_point", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
        C.r_Sampler_clf("s_att", TEX_POINT_ATT);
        C.r_End();
        break;
    case SE_R1_LSPOT:
        C.r_Pass("lmap_spot", "add_spot", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", "internal" DELIMITER "internal_light_att", true);
        C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
        C.r_End();
        break;
    case SE_R1_LMODELS:
        // Lighting only, not use alpha-channel
        C.r_Pass("lmap_l", "lmap_l", FALSE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler("s_lmap", C.L_textures[1]);
        C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
        C.r_End();
        break;
#endif
    }
}
