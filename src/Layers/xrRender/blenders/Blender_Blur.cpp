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


CBlender_ssfx_ssr::CBlender_ssfx_ssr() { description.CLS = 0; }

void CBlender_ssfx_ssr::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0:	// Do SSR
        C.r_Pass("stub_screen_space", "ssfx_ssr", FALSE, FALSE, FALSE);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("ssr_image", r2_RT_ssfx_ssr); // Prev Frame
        C.r_dx11Texture("s_rimage", "$user$generic_temp");
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);
        C.r_dx11Texture("s_prev_pos", r2_RT_ssfx_prevPos);
        C.r_dx11Texture("s_gloss_data", r2_RT_ssfx_temp3);

        C.r_dx11Texture("blue_noise", "fx\\blue_noise");

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_dx11Sampler("smp_linear");

        C.r_End();
        break;

    case 1:	// Blur Phase 1
        C.r_Pass("stub_screen_space", "ssfx_ssr_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("ssr_image", r2_RT_ssfx);
        C.r_dx11Texture("s_diffuse", r2_RT_albedo);

        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;

    case 2:	// Blur Phase 2
        C.r_Pass("stub_screen_space", "ssfx_ssr_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("ssr_image", r2_RT_ssfx_temp);
        C.r_dx11Texture("s_diffuse", r2_RT_albedo);

        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;

    case 3:	// Combine
        C.r_Pass("stub_screen_space", "ssfx_ssr_combine", FALSE, FALSE, FALSE);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_rimage", "$user$generic_temp");

        C.r_dx11Texture("ssr_image", r2_RT_ssfx_temp2);
        C.r_dx11Texture("s_gloss_data", r2_RT_ssfx_temp3);

        C.r_dx11Sampler("smp_linear");
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();

        break;

    case 4:	// No blur just direct to [r2_RT_ssfx_temp2]
        C.r_Pass("stub_screen_space", "ssfx_ssr_noblur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("ssr_image", r2_RT_ssfx);

        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;

    case 5:
        C.r_Pass("stub_screen_space", "ssfx_ssr_gloss", FALSE, FALSE, FALSE);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_diffuse", r2_RT_albedo);
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);

        C.r_dx11Texture("env_s0", r2_T_envs0);
        C.r_dx11Texture("env_s1", r2_T_envs1);
        C.r_dx11Texture("sky_s0", r2_T_sky0);
        C.r_dx11Texture("sky_s1", r2_T_sky1);

        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    }
}



CBlender_ssfx_volumetric_blur::CBlender_ssfx_volumetric_blur() { description.CLS = 0; }

void CBlender_ssfx_volumetric_blur::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0:	// Blur Phase 1
        C.r_Pass("stub_screen_space", "ssfx_volumetric_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("vol_buffer", r2_RT_generic2);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;

    case 1:	// Blur Phase 2
        C.r_Pass("stub_screen_space", "ssfx_volumetric_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("vol_buffer", r2_RT_ssfx_accum);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;

    case 2:	// Blur Phase 1
        C.r_Pass("stub_screen_space", "ssfx_water_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("water_buffer", r2_RT_ssfx_temp);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;

    case 3:	// Blur Phase 2
        C.r_Pass("stub_screen_space", "ssfx_water_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("water_buffer", r2_RT_ssfx_temp2);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;

    case 4:	// No Blur
        C.r_Pass("stub_screen_space", "ssfx_water_noblur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("water_buffer", r2_RT_ssfx_temp2);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;

    case 5:	// Water Waves
        C.r_Pass("stub_screen_space", "ssfx_water_waves", FALSE, FALSE, FALSE);

        C.r_dx11Texture("water_waves", "fx\\water_height");

        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;
    }
}

CBlender_ssfx_ao::CBlender_ssfx_ao() { description.CLS = 0; }

void CBlender_ssfx_ao::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0:	// AO
        C.r_Pass("stub_screen_space", "ssfx_ao", FALSE, FALSE, FALSE);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);
        C.r_dx11Texture("ssfx_ao", r2_RT_ssfx_ao);

        C.r_dx11Texture("s_prev_pos", r2_RT_ssfx_prevPos);

        C.r_dx11Texture("jitter0", JITTER(0));

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_dx11Sampler("smp_jitter");

        C.r_End();

        break;

    case 1:	// Blur Phase 1
        C.r_Pass("stub_screen_space", "ssfx_ao_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("ao_image", r2_RT_ssfx_temp);
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;

    case 2:	// Blur Phase 2
        C.r_Pass("stub_screen_space", "ssfx_ao_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("ao_image", r2_RT_ssfx_temp3);
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;


    case 3:	// IL
        C.r_Pass("stub_screen_space", "ssfx_il", FALSE, FALSE, FALSE);

        C.r_dx11Texture("s_accumulator", r2_RT_accum);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);

        C.r_dx11Texture("ssfx_ao", r2_RT_ssfx_il);



        C.r_dx11Texture("s_prev_pos", r2_RT_ssfx_prevPos);

        C.r_dx11Texture("jitter0", JITTER(0));

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_dx11Sampler("smp_jitter");

        C.r_End();

        break;

    case 4:	// Blur Phase 1
        C.r_Pass("stub_screen_space", "ssfx_il_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("ao_image", r2_RT_ssfx_temp2);
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;

    case 5:	// Blur Phase 2
        C.r_Pass("stub_screen_space", "ssfx_il_blur", FALSE, FALSE, FALSE);

        C.r_dx11Texture("ao_image", r2_RT_ssfx_temp3);
        C.r_dx11Texture("s_hud_mask", r2_RT_ssfx_hud);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_linear");
        C.r_End();
        break;
    }
}
