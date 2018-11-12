#include "stdafx.h"
#pragma hdrstop

#include "blender_fxaa.h"

CBlender_FXAA::CBlender_FXAA() { description.CLS = 0; }
CBlender_FXAA::~CBlender_FXAA() { }

void CBlender_FXAA::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0:
        C.r_Pass("fxaa_main", "fxaa_main", false, FALSE, FALSE);
        C.r_Sampler_rtf("s_base0", r2_RT_generic0);
        C.r_End();
        break;
    }
}
