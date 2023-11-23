#include "stdafx.h"
#pragma hdrstop

#include "blender_combine.h"

CBlender_combine::CBlender_combine() { description.CLS = 0; }
CBlender_combine::~CBlender_combine() {}
void CBlender_combine::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_R2
    switch (C.iElement)
    {
    case 0: // combine
        C.r_Pass("combine_1", "combine_1", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA,
            D3DBLEND_SRCALPHA); //. MRT-blend?
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_rtf("s_diffuse", r2_RT_albedo);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        C.r_Sampler_rtf("s_depth", r2_RT_depth);
        C.r_Sampler_rtf("s_tonemap", r2_RT_luminance_cur);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler_clw("s_occ", r2_RT_ssao_temp);
        C.r_Sampler_rtf("s_half_depth", r2_RT_half_depth);
        C.r_Sampler_clf("env_s0", r2_T_envs0);
        C.r_Sampler_clf("env_s1", r2_T_envs1);
        C.r_Sampler_clf("sky_s0", r2_T_sky0);
        C.r_Sampler_clf("sky_s1", r2_T_sky1);
        jitter(C);
        C.r_End();
        break;
    case 1: // aa-edge-detection + AA :)
        C.r_Pass("null", "combine_2_AA", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);

        C.r_End();
        break;
    case 2: // non-AA
        C.r_Pass("null", "combine_2_NAA", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);
        C.r_End();
        break;
    case 3: // aa-edge-detection + AA :) + DISTORTION
        C.r_Pass("null", "combine_2_AA_D", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);
        C.r_End();
        break;
    case 4: // non-AA + DISTORTION
        C.r_Pass("null", "combine_2_NAA_D", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);
        C.r_End();
        break;
    case 5: // post-processing
        break;
    }
#elif RENDER == R_GL
    switch (C.iElement)
    {
    case 0: // combine
        C.r_Pass("combine_1", "combine_1_nomsaa", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA,
                 D3DBLEND_SRCALPHA); //. MRT-blend?
        C.r_Stencil(TRUE, D3DCMP_LESSEQUAL, 0xff, 0x00); // stencil should be >= 1
        C.r_StencilRef(0x01);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_rtf("s_diffuse", r2_RT_albedo);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        C.r_Sampler_rtf("s_depth", r2_RT_depth);
        C.r_Sampler_rtf("s_tonemap", r2_RT_luminance_cur);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler_clf("env_s0", r2_T_envs0);
        C.r_Sampler_clf("env_s1", r2_T_envs1);
        C.r_Sampler_clf("sky_s0", r2_T_sky0);
        C.r_Sampler_clf("sky_s1", r2_T_sky1);
        C.r_Sampler_rtf("s_occ", r2_RT_ssao_temp);
        C.r_Sampler_rtf("s_half_depth", r2_RT_half_depth);

        jitter(C);

        C.r_End();
        break;
    case 1: // aa-edge-detection + AA :)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);
        C.r_End();
        break;
    case 2: // non-AA
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);
        C.r_End();
        break;
    case 3: // aa-edge-detection + AA :) + DISTORTION
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA_D", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);
        C.r_End();
        break;
    case 4: // non-AA + DISTORTION
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA_D", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1);
        C.r_End();
        break;
    case 5: // post-processing
        break;
    }
#else
    switch (C.iElement)
    {
    case 0: // combine
        C.r_Pass("combine_1", "combine_1_nomsaa", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA,
            D3DBLEND_SRCALPHA); //. MRT-blend?
        C.r_Stencil(TRUE, D3DCMP_LESSEQUAL, 0xff, 0x00); // stencil should be >= 1
        C.r_StencilRef(0x01);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P				);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N				);
        // C.r_Sampler_rtf		("s_diffuse",		r2_RT_albedo		);
        // C.r_Sampler_rtf		("s_accumulator",	r2_RT_accum			);
        // C.r_Sampler_rtf		("s_depth",			r2_RT_depth			);
        // C.r_Sampler_rtf		("s_tonemap",		r2_RT_luminance_cur	);
        // C.r_Sampler_clw		("s_material",		r2_material			);
        // C.r_Sampler_clf		("env_s0",			r2_T_envs0			);
        // C.r_Sampler_clf		("env_s1",			r2_T_envs1			);
        // C.r_Sampler_clf		("sky_s0",			r2_T_sky0			);
        // C.r_Sampler_clf		("sky_s1",			r2_T_sky1			);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_diffuse", r2_RT_albedo);
        C.r_dx11Texture("s_accumulator", r2_RT_accum);
        C.r_dx11Texture("s_depth", r2_RT_depth);
        C.r_dx11Texture("s_tonemap", r2_RT_luminance_cur);
        C.r_dx11Texture("s_material", r2_material);
        C.r_dx11Texture("env_s0", r2_T_envs0);
        C.r_dx11Texture("env_s1", r2_T_envs1);
        C.r_dx11Texture("sky_s0", r2_T_sky0);
        C.r_dx11Texture("sky_s1", r2_T_sky1);
        C.r_dx11Texture("s_occ", r2_RT_ssao_temp);
        C.r_dx11Texture("s_half_depth", r2_RT_half_depth);

        jitter(C);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1: // aa-edge-detection + AA :)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA", FALSE, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2: // non-AA
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA", FALSE, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 3: // aa-edge-detection + AA :) + DISTORTION
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA_D", FALSE, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 4: // non-AA + DISTORTION
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA_D", FALSE, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 5: // post-processing
        break;
    }
#endif
}

#if RENDER != R_R2
void CBlender_combine_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (Name)
        RImplementation.m_MSAASample = atoi(Definition);
    else
        RImplementation.m_MSAASample = -1;

#if RENDER == R_GL
    switch (C.iElement)
    {
    case 0: // combine
        C.r_Pass("combine_1", "combine_1_msaa", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA,
                 D3DBLEND_SRCALPHA); //. MRT-blend?
        C.r_Stencil(TRUE, D3DCMP_LESSEQUAL, 0xff, 0x00); // stencil should be >= 1
        C.r_StencilRef(0x01);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_rtf("s_diffuse", r2_RT_albedo);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        C.r_Sampler_rtf("s_depth", r2_RT_depth);
        C.r_Sampler_rtf("s_tonemap", r2_RT_luminance_cur);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler_clf("env_s0", r2_T_envs0);
        C.r_Sampler_clf("env_s1", r2_T_envs1);
        C.r_Sampler_clf("sky_s0", r2_T_sky0);
        C.r_Sampler_clf("sky_s1", r2_T_sky1);
        C.r_Sampler_rtf("s_occ", r2_RT_ssao_temp);
        C.r_Sampler_rtf("s_half_depth", r2_RT_half_depth);

        jitter(C);

        C.r_End();
        break;
    case 1: // aa-edge-detection + AA :)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1_r);
        C.r_End();
        break;
    case 2: // non-AA
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA", FALSE, FALSE, TRUE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1_r);
        C.r_End();
        break;
    case 3: // aa-edge-detection + AA :) + DISTORTION
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA_D", FALSE, FALSE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1_r);
        C.r_End();
        break;
    case 4: // non-AA + DISTORTION
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA_D", FALSE, FALSE, TRUE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clf("s_image", r2_RT_generic0);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_Sampler_clf("s_distort", r2_RT_generic1_r);
        C.r_End();
        break;
    case 5: // post-processing
        break;
    }
#else
    switch (C.iElement)
    {
    case 0: // combine
        C.r_Pass("combine_1", "combine_1_msaa", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA,
            D3DBLEND_SRCALPHA); //. MRT-blend?
        C.r_Stencil(TRUE, D3DCMP_LESSEQUAL, 0xff, 0x00); // stencil should be >= 1
        C.r_StencilRef(0x01);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P				);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N				);
        // C.r_Sampler_rtf		("s_diffuse",		r2_RT_albedo		);
        // C.r_Sampler_rtf		("s_accumulator",	r2_RT_accum			);
        // C.r_Sampler_rtf		("s_depth",			r2_RT_depth			);
        // C.r_Sampler_rtf		("s_tonemap",		r2_RT_luminance_cur	);
        // C.r_Sampler_clw		("s_material",		r2_material			);
        // C.r_Sampler_clf		("env_s0",			r2_T_envs0			);
        // C.r_Sampler_clf		("env_s1",			r2_T_envs1			);
        // C.r_Sampler_clf		("sky_s0",			r2_T_sky0			);
        // C.r_Sampler_clf		("sky_s1",			r2_T_sky1			);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_diffuse", r2_RT_albedo);
        C.r_dx11Texture("s_accumulator", r2_RT_accum);
        C.r_dx11Texture("s_depth", r2_RT_depth);
        C.r_dx11Texture("s_tonemap", r2_RT_luminance_cur);
        C.r_dx11Texture("s_material", r2_material);
        C.r_dx11Texture("s_occ", r2_RT_ssao_temp);
        C.r_dx11Texture("s_half_depth", r2_RT_half_depth);
        C.r_dx11Texture("env_s0", r2_T_envs0);
        C.r_dx11Texture("env_s1", r2_T_envs1);
        C.r_dx11Texture("sky_s0", r2_T_sky0);
        C.r_dx11Texture("sky_s1", r2_T_sky1);

        jitter(C);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1: // aa-edge-detection + AA :)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA", FALSE, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1_r);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2: // non-AA
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA", FALSE, FALSE, TRUE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1_r);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 3: // aa-edge-detection + AA :) + DISTORTION
        C.r_Pass("stub_notransform_aa_AA", "combine_2_AA_D", FALSE, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1_r);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 4: // non-AA + DISTORTION
        //	Can use simpler VS (need only Tex0)
        C.r_Pass("stub_notransform_aa_AA", "combine_2_NAA_D", FALSE, FALSE, TRUE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic0);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        // C.r_Sampler_clf		("s_distort",		r2_RT_generic1);

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_image", r2_RT_generic0);
        C.r_dx11Texture("s_bloom", r2_RT_bloom1);
        C.r_dx11Texture("s_distort", r2_RT_generic1_r);
        C.r_dx11Texture("s_blur_2", r2_RT_blur_2);
        C.r_dx11Texture("s_blur_4", r2_RT_blur_4);
        C.r_dx11Texture("s_blur_8", r2_RT_blur_8);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 5: // post-processing
        break;
    }
#endif
    RImplementation.m_MSAASample = -1;
}
#endif
