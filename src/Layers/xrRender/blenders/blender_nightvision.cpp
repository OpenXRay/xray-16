#include "stdafx.h"

#include "blender_nightvision.h"

CBlender_nightvision::CBlender_nightvision()
{
    description.CLS = B_BLUR;
}

LPCSTR CBlender_nightvision::getComment()
{
    return "INTERNAL: beef's nightvision";
}

void CBlender_nightvision::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
#if RENDER == R_R4
    switch (C.iElement)
    {
    case 0: //Dummy shader - because IDK what gonna happen when r2_nightvision will be 0
        C.r_Pass("stub_screen_space", "copy_nomsaa", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_generic", r2_RT_generic0);

        C.r_dx11Sampler("smp_base");
        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1:
        C.r_Pass("stub_screen_space", "nightvision_gen_1", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        //C.r_dx11Texture("s_bloom_new", r2_RT_pp_bloom);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_base");
        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2:
        C.r_Pass("stub_screen_space", "nightvision_gen_2", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        //C.r_dx11Texture("s_bloom_new", r2_RT_pp_bloom);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_base");
        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 3:
        C.r_Pass("stub_screen_space", "nightvision_gen_3", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        //C.r_dx11Texture("s_bloom_new", r2_RT_pp_bloom);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_base");
        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    }
#endif
}
