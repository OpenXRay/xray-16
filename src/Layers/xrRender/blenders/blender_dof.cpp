#include "stdafx.h"

#include "blender_dof.h"

CBlender_dof::CBlender_dof()
{
    description.CLS = B_BLUR;
}

LPCSTR CBlender_dof::getComment()
{
    return "INTERNAL: dof";
}

void CBlender_dof::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
#if RENDER == R_R4
    switch (C.iElement)
    {
    case 0:
        C.r_Pass("stub_screen_space", "depth_of_field", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);

        C.r_dx11Sampler("smp_base");
        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1:
        C.r_Pass("stub_screen_space", "post_processing", FALSE, FALSE, FALSE);
        C.r_dx11Texture("samplero_pepero", r2_RT_dof);

        C.r_dx11Sampler("smp_base");
        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    }
#endif
}
