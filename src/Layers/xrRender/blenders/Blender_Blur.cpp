#include "stdafx.h"
#pragma hdrstop

#include "Blender_Blur.h"

/*
 * TODO: Seems there is no use for this blender even in R1.
 * Consider removing.
 */

CBlender_Blur::CBlender_Blur()
{
    description.CLS = B_BLUR;
}

LPCSTR CBlender_Blur::getComment()
{
    return "INTERNAL: blur";
}

#if RENDER != R_R4
void CBlender_Blur::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    C.PassBegin();
    {
        C.PassSET_ZB(FALSE, FALSE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, FALSE);

        // Stage0 - B0*F
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_TFACTOR);
        C.Stage_Texture("$base0");
        C.Stage_Matrix("$null", 0);
        C.Stage_Constant("$null");
        C.StageEnd();

        // Stage1 - B1*F + current
        C.StageBegin();
        C.StageSET_Color3(D3DTA_TEXTURE, D3DTOP_MULTIPLYADD, D3DTA_TFACTOR, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_CURRENT, D3DTOP_SELECTARG1, D3DTA_TFACTOR);
        C.Stage_Texture("$base1");
        C.Stage_Matrix("$null", 1);
        C.Stage_Constant("$null");
        C.StageEnd();

        C.R().SetRS(D3DRS_TEXTUREFACTOR, color_rgba(127, 127, 127, 127));
    }
    C.PassEnd();
}
#else
void CBlender_Blur::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: //Fullres Horizontal
        C.r_Pass("stub_screen_space", "pp_blur", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_lut_atlas", "shaders\\lut_atlas");

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1: //Fullres Vertical
        C.r_Pass("stub_screen_space", "pp_blur", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_image", r2_RT_blur_h_2);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_lut_atlas", "shaders\\lut_atlas");

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2: //Halfres Horizontal
        C.r_Pass("stub_screen_space", "pp_blur", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_lut_atlas", "shaders\\lut_atlas");

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 3: //Halfres Vertical
        C.r_Pass("stub_screen_space", "pp_blur", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_image", r2_RT_blur_h_4);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_lut_atlas", "shaders\\lut_atlas");

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 4: //Quarterres Horizontal
        C.r_Pass("stub_screen_space", "pp_blur", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_lut_atlas", "shaders\\lut_atlas");

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 5: //Quarterres Vertical
        C.r_Pass("stub_screen_space", "pp_blur", FALSE, FALSE, FALSE);
        C.r_dx11Texture("s_image", r2_RT_blur_h_8);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_lut_atlas", "shaders\\lut_atlas");

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    }
}
#endif
