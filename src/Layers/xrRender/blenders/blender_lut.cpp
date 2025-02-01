#include "stdafx.h"

#include "blender_lut.h"

CBlender_lut::CBlender_lut() { description.CLS = 0; }

LPCSTR CBlender_lut::getComment()
{
    return "INTERNAL: LUT";
}

void CBlender_lut::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

    C.r_Pass("stub_screen_space", "pp_lut", FALSE, FALSE, FALSE);
    C.r_dx11Texture("s_image", r2_RT_generic0);
    C.r_dx11Texture("s_lut_atlas", "shaders\\lut_atlas");

    C.r_dx11Sampler("smp_base");
    C.r_dx11Sampler("smp_nofilter");
    C.r_dx11Sampler("smp_rtlinear");
    C.r_dx11Sampler("smp_linear");
    C.r_End();
}
