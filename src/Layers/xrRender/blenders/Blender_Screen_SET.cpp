#include "stdafx.h"
#pragma hdrstop

#include "Blender_Screen_SET.h"

#define VER_2_oBlendCount 7
#define VER_4_oBlendCount 9
#define VER_5_oBlendCount 10

CBlender_Screen_SET::CBlender_Screen_SET()
{
    description.CLS = B_SCREEN_SET;
    description.version = 4;
    oBlend.Count = VER_4_oBlendCount;
    oBlend.IDselected = 0;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oZTest.value = FALSE;
    oZWrite.value = FALSE;
    oLighting.value = FALSE;
    oFog.value = FALSE;
    oClamp.value = TRUE;
}

LPCSTR CBlender_Screen_SET::getComment()
{
    return "basic (simple)";
}

void CBlender_Screen_SET::Save(IWriter& fs)
{
    IBlender::Save(fs);

    // Blend mode
    xrP_TOKEN::Item I;
    xrPWRITE_PROP(fs, "Blending", xrPID_TOKEN, oBlend);
    I.ID = 0;
    xr_strcpy(I.str, "SET");
    fs.w(&I, sizeof(I));
    I.ID = 1;
    xr_strcpy(I.str, "BLEND");
    fs.w(&I, sizeof(I));
    I.ID = 2;
    xr_strcpy(I.str, "ADD");
    fs.w(&I, sizeof(I));
    I.ID = 3;
    xr_strcpy(I.str, "MUL");
    fs.w(&I, sizeof(I));
    I.ID = 4;
    xr_strcpy(I.str, "MUL_2X");
    fs.w(&I, sizeof(I));
    I.ID = 5;
    xr_strcpy(I.str, "ALPHA-ADD");
    fs.w(&I, sizeof(I));
    I.ID = 6;
    xr_strcpy(I.str, "MUL_2X (B^D)");
    fs.w(&I, sizeof(I));
    I.ID = 7;
    xr_strcpy(I.str, "SET (2r)");
    fs.w(&I, sizeof(I));
    I.ID = 8;
    xr_strcpy(I.str, "BLEND (2r)");
    fs.w(&I, sizeof(I));
    I.ID = 9;
    xr_strcpy(I.str, "BLEND (4r)");
    fs.w(&I, sizeof(I));

    // Params
    xrPWRITE_PROP(fs, "Texture clamp", xrPID_BOOL, oClamp);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
    xrPWRITE_PROP(fs, "Z-test", xrPID_BOOL, oZTest);
    xrPWRITE_PROP(fs, "Z-write", xrPID_BOOL, oZWrite);
    xrPWRITE_PROP(fs, "Lighting", xrPID_BOOL, oLighting);
    xrPWRITE_PROP(fs, "Fog", xrPID_BOOL, oFog);
}

void CBlender_Screen_SET::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    xrPREAD_PROP(fs, xrPID_TOKEN, oBlend);

    switch (version)
    {
    case 2:
        oBlend.Count = VER_5_oBlendCount;
        break;

    default:
        oBlend.Count = VER_5_oBlendCount;
        xrPREAD_PROP(fs, xrPID_BOOL, oClamp);
        break;
    }

    xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
    xrPREAD_PROP(fs, xrPID_BOOL, oZTest);
    xrPREAD_PROP(fs, xrPID_BOOL, oZWrite);
    xrPREAD_PROP(fs, xrPID_BOOL, oLighting);
    xrPREAD_PROP(fs, xrPID_BOOL, oFog);
}

void CBlender_Screen_SET::CompileFixed(CBlender_Compile& C)
{
    if (oBlend.IDselected == 6)
    {
        // Usually for wallmarks
        C.StageBegin();
        {
            if (oClamp.value)
            {
                C.StageSET_Address(D3DTADDRESS_CLAMP);
            }
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix("$null", 0);
            C.Stage_Constant("$null");
        }
        C.StageEnd();

        C.StageBegin();
        {
            if (oClamp.value)
            {
                C.StageSET_Address(D3DTADDRESS_CLAMP);
            }
            C.StageSET_Color(D3DTA_DIFFUSE, D3DTOP_BLENDDIFFUSEALPHA, D3DTA_CURRENT);
            C.StageSET_Alpha(D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_CURRENT);
            C.Stage_Texture("$null");
            C.Stage_Matrix("$null", 0);
            C.Stage_Constant("$null");
        }
        C.StageEnd();
    }
    else
    {
        C.StageBegin();
        {
            if (9 == oBlend.IDselected)
            {
                // 4x R
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE4X, D3DTA_DIFFUSE);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
            }
            else
            {
                if ((7 == oBlend.IDselected) || (8 == oBlend.IDselected))
                {
                    // 2x R
                    C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_DIFFUSE);
                    C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
                }
                else
                {
                    // 1x R
                    C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
                    C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
                }
            }

            if (oClamp.value)
            {
                C.StageSET_Address(D3DTADDRESS_CLAMP);
            }
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix(oT_xform, 0);
            C.Stage_Constant("$null");
        }
        C.StageEnd();
    }
}

void CBlender_Screen_SET::CompileProgrammed(CBlender_Compile& C)
{
    switch (oBlend.IDselected)
    {
    case 6:
        // Usually for wallmarks
        C.PassSET_VS("stub_notransform_t");
        C.PassSET_PS("stub_default_ma");

    case 9:
        // 4x R
        C.PassSET_VS("stub_notransform_t_m4");
        C.PassSET_PS("stub_default");

    case 7:
    case 8:
        // 2x R
        C.PassSET_VS("stub_notransform_t_m2");
        C.PassSET_PS("stub_default");

    default:
        // 1x R
        C.PassSET_VS("stub_notransform_t");
        C.PassSET_PS("stub_default");
    }

    VERIFY2(C.L_textures.size() > 0, "Not enough textures");
    const u32 stage = C.SampledImage("s_base", "s_base", C.L_textures[0]);
    if (oClamp.value)
    {
        C.i_Address(stage, D3DTADDRESS_CLAMP);
    }
}

void CBlender_Screen_SET::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    C.PassBegin();
    {
        if (HW.Caps.hasFixedPipeline)
        {
            CompileFixed(C);
        }
        else
        {
            CompileProgrammed(C);
        }

        C.PassSET_LightFog(oLighting.value, oFog.value);
        C.PassSET_ZB(oZTest.value, oZWrite.value);

        switch (oBlend.IDselected)
        {
        case 0: // SET
            C.PassSET_Blend(false, D3DBLEND_ONE, D3DBLEND_ZERO, false, 0);
            break;

        case 1: // BLEND
            C.PassSET_Blend(true, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, true, oAREF.value);
            break;

        case 2: // ADD
            C.PassSET_Blend(true, D3DBLEND_ONE, D3DBLEND_ONE, false, oAREF.value);
            break;

        case 3: // MUL
            C.PassSET_Blend(true, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, false, oAREF.value);
            break;

        case 4: // MUL_2X
            C.PassSET_Blend(true, D3DBLEND_DESTCOLOR, D3DBLEND_SRCCOLOR, false, oAREF.value);
            break;

        case 5: // ALPHA-ADD
            C.PassSET_Blend(true, D3DBLEND_SRCALPHA, D3DBLEND_ONE, true, oAREF.value);
            break;

        case 6: // MUL_2X + A-test
            C.PassSET_Blend(true, D3DBLEND_DESTCOLOR, D3DBLEND_SRCCOLOR, false, oAREF.value);
            break;

        case 7: // SET (2r)
            C.PassSET_Blend(true, D3DBLEND_ONE, D3DBLEND_ZERO, true, 0);
            break;

        case 8: // BLEND (2r)
            C.PassSET_Blend(true, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, true, oAREF.value);
            break;

        case 9: // BLEND (2r)
            C.PassSET_Blend(true, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, true, oAREF.value);
            break;
        }
    }
    C.PassEnd();
}
