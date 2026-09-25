#include "stdafx.h"
#pragma hdrstop

#include "Blender_Screen_SET.h"

#define VER_2_oBlendCount 7
#define VER_4_oBlendCount 9
#define VER_5_oBlendCount 10

namespace xray::render::fg
{
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
} // namespace xray::render::fg
