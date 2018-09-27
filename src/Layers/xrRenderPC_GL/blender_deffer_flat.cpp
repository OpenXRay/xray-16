#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/uber_deffer.h"
#include "blender_deffer_flat.h"

CBlender_deffer_flat::CBlender_deffer_flat()
{
    description.CLS = B_DEFAULT;
    description.version = 1;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
}

CBlender_deffer_flat::~CBlender_deffer_flat() { }

void CBlender_deffer_flat::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrP_TOKEN::Item I;
    xrPWRITE_PROP (fs,"Tessellation", xrPID_TOKEN, oTessellation);
    I.ID = 0;
    xr_strcpy(I.str, "NO_TESS");
    fs.w(&I, sizeof I);
    I.ID = 1;
    xr_strcpy(I.str, "TESS_PN");
    fs.w(&I, sizeof I);
    I.ID = 2;
    xr_strcpy(I.str, "TESS_HM");
    fs.w(&I, sizeof I);
    I.ID = 3;
    xr_strcpy(I.str, "TESS_PN+HM");
    fs.w(&I, sizeof I);
}

void CBlender_deffer_flat::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (version > 0)
    {
        xrPREAD_PROP(fs,xrPID_TOKEN,oTessellation);
        oTessellation.Count = 4;
    }
}

void CBlender_deffer_flat::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    // codepath is the same, only the shaders differ
    switch (C.iElement)
    {
    case SE_R2_NORMAL_HQ: // deffer
        uber_deffer(C, true, "base", "base", false, nullptr, true);

        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);
        C.r_End();
        break;
    case SE_R2_NORMAL_LQ: // deffer
        uber_deffer(C, false, "base", "base", false, nullptr, true);

        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);
        C.r_End();
        break;
    case SE_R2_SHADOW: // smap-direct
        //if (RImplementation.o.HW_smap)	C.r_Pass	("shadow_direct_base","dumb",	FALSE,TRUE,TRUE,FALSE);
        //else							C.r_Pass	("shadow_direct_base","shadow_direct_base",FALSE);
        C.r_Pass("shadow_direct_base", "dumb", FALSE, TRUE,TRUE,FALSE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    }
}
