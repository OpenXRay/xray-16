#include "stdafx.h"
#pragma hdrstop

#include "blender_LaEmB.h"

/*
 * TODO: Seems there is no use for this blender even in R1.
 * Consider removing.
 */

#if RENDER != R_R1
#error "The blender can't be used in this renderer generation"
#endif

CBlender_LaEmB::CBlender_LaEmB()
{
    description.CLS = B_LaEmB;
    xr_strcpy(oT2_Name, "$null");
    xr_strcpy(oT2_xform, "$null");
    xr_strcpy(oT2_const, "$null");
}

LPCSTR CBlender_LaEmB::getComment() 
{
    return "LEVEL: (lmap+env*const)*base";
}

BOOL CBlender_LaEmB::canBeLMAPped()
{
    return TRUE;
}

void CBlender_LaEmB::Save(IWriter& fs)
{
    IBlender::Save(fs);

    xrPWRITE_MARKER(fs, "Environment map");
    xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
    xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
    xrPWRITE_PROP(fs, "Constant", xrPID_CONSTANT, oT2_const);
}

void CBlender_LaEmB::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    xrPREAD_MARKER(fs);
    xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
    xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
    xrPREAD_PROP(fs, xrPID_CONSTANT, oT2_const);
}

// EDITOR --- NO CONSTANT
void CBlender_LaEmB::compile_ED(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_ZB(true, true);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(true, true);

        // Stage1 - Env texture
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
        C.StageEnd();

        // Stage2 - Base texture
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_CURRENT);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

// EDITOR --- WITH CONSTANT
void CBlender_LaEmB::compile_EDc(CBlender_Compile& C)
{
    // Pass0 - (lmap+env*const)
    C.PassBegin();
    {
        C.PassSET_ZB(true, true);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(true, true);

        // Stage1 - Env texture * constant
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_TMC(oT2_Name, oT2_xform, oT2_const, 0);
        C.StageEnd();

        // Stage2 - Diffuse color
        C.StageBegin();
        C.StageSET_Color(D3DTA_DIFFUSE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_DIFFUSE, D3DTOP_ADD, D3DTA_CURRENT);
        C.Stage_Texture("$null");
        C.Stage_Matrix("$null", 0);
        C.Stage_Constant("$null");
        C.StageEnd();
    }
    C.PassEnd();

    // Pass1 - *base
    C.PassBegin();
    {
        C.PassSET_ZB(true, false);
        C.PassSET_Blend_MUL();
        C.PassSET_LightFog(false, true);

        // Stage2 - Diffuse color
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

void CBlender_LaEmB::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    const bool bConstant = (0 != xr_stricmp(oT2_const, "$null"));

    if (C.bEditor)
    {
        if (bConstant)
            compile_EDc(C);
        else
            compile_ED(C);
        return;
    }

    if (2 == C.iElement)
    {
        if (bConstant)
            compile_Lc(C);
        else
            compile_L(C);
    }
    else
    {
        switch (HW.Caps.raster.dwStages)
        {
        case 2: // Geforce1/2/MX
            if (bConstant)
                compile_2c(C);
            else
                compile_2(C);
            break;
        case 3: // Kyro, Radeon, Radeon2, Geforce3/4
        default:
            if (bConstant)
                compile_3c(C);
            else
                compile_3(C);
            break;
        }
    }
}
//
void CBlender_LaEmB::compile_2(CBlender_Compile& C)
{
    // Pass1 - Lmap+Env
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - Lightmap
        C.StageBegin();
        C.StageTemplate_LMAP0();
        C.StageEnd();

        // Stage1 - Environment map
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();

    // Pass2 - Base map
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, FALSE);
        C.PassSET_Blend_MUL2X();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - Base
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}
//
void CBlender_LaEmB::compile_2c(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - Environment map [*] const
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_TMC(oT2_Name, oT2_xform, oT2_const, 0);
        C.StageEnd();

        // Stage1 - [+] Lightmap
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_TMC("$base1", "$null", "$null", 1);
        C.StageEnd();
    }
    C.PassEnd();

    // Pass2 - Base map
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, FALSE);
        C.PassSET_Blend_MUL2X();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - Detail
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

//
void CBlender_LaEmB::compile_3(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage0 - [=] Lightmap
        C.StageBegin();
        C.StageTemplate_LMAP0();
        C.StageEnd();

        // Stage1 - [+] Env-map
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
        C.StageEnd();

        // Stage2 - [*] Base
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_CURRENT);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

//
void CBlender_LaEmB::compile_3c(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, TRUE);

        // Stage1 - [=] Env-map [*] const
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_TMC(oT2_Name, oT2_xform, oT2_const, 0);
        C.StageEnd();

        // Stage0 - [+] Lightmap
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_TMC("$base1", "$null", "$null", 1);
        C.StageEnd();

        // Stage2 - [*] Base
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_CURRENT);
        C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}

//
void CBlender_LaEmB::compile_L(CBlender_Compile& C)
{
    // Pass1 - Lmap+Env
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, FALSE);

        // Stage0 - Lightmap
        C.StageBegin();
        C.StageTemplate_LMAP0();
        C.StageEnd();

        // Stage1 - Environment map
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_TMC(oT2_Name, oT2_xform, "$null", 0);
        C.StageEnd();
    }
    C.PassEnd();
}
//
void CBlender_LaEmB::compile_Lc(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_ZB(TRUE, TRUE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, FALSE);

        // Stage0 - Environment map [*] const
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_TMC(oT2_Name, oT2_xform, oT2_const, 0);
        C.StageEnd();

        // Stage1 - [+] Lightmap
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
        C.StageSET_TMC("$base1", "$null", "$null", 1);
        C.StageEnd();
    }
    C.PassEnd();
}
