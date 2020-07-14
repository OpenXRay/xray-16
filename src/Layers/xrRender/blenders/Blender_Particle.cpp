#include "stdafx.h"
#pragma hdrstop

#include "Blender_Particle.h"

#define oBlendCount 6

CBlender_Particle::CBlender_Particle()
{
    description.CLS = B_PARTICLE;
    description.version = 0;
    oBlend.Count = oBlendCount;
    oBlend.IDselected = 0;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oClamp.value = TRUE;
}

void CBlender_Particle::Save(IWriter& fs)
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

    // Params
    xrPWRITE_PROP(fs, "Texture clamp", xrPID_BOOL, oClamp);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
}

void CBlender_Particle::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    xrPREAD_PROP(fs, xrPID_TOKEN, oBlend);
    oBlend.Count = oBlendCount;
    xrPREAD_PROP(fs, xrPID_BOOL, oClamp);
    xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
}

LPCSTR CBlender_Particle::getComment()
{
    return "particles";
}

void CBlender_Particle::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    switch (oBlend.IDselected)
    {
    case 0:
        C.r_Pass("particle", "particle", TRUE, TRUE, TRUE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE, 200);
        break; // SET
    case 1:
        C.r_Pass("particle", "particle", FALSE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, 0);
        break; // BLEND
    case 2:
        C.r_Pass("particle", "particle", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, 0);
        break; // ADD
    case 3:
        C.r_Pass("particle", "particle", FALSE, TRUE, FALSE, TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, TRUE, 0);
        break; // MUL
    case 4:
        C.r_Pass("particle", "particle", FALSE, TRUE, FALSE, TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_SRCCOLOR, TRUE, 0);
        break; // MUL_2X
    case 5:
        C.r_Pass("particle", "particle", FALSE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_ONE, TRUE, 0);
        break; // ALPHA-ADD
    }
    C.r_Sampler("s_base", C.L_textures[0], false, oClamp.value ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
    C.r_End();
}
