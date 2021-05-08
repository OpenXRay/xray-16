#include "stdafx.h"
#pragma hdrstop

#include "blender_luminance.h"

CBlender_luminance::CBlender_luminance() { description.CLS = 0; }
CBlender_luminance::~CBlender_luminance() {}
void CBlender_luminance::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_R2
    switch (C.iElement)
    {
    case 0: // 256x256	=> 64x64
        C.r_Pass("null", "bloom_luminance_1", false, FALSE, FALSE, FALSE);
        C.r_Sampler_clf("s_image", r2_RT_bloom1);
        C.r_End();
        break;
    case 1: // 64x64	=> 8x8
        C.r_Pass("null", "bloom_luminance_2", false, FALSE, FALSE, FALSE);
        C.r_Sampler_clf("s_image", r2_RT_luminance_t64);
        C.r_End();
        break;
    case 2: // 8x8		=> 1x1, blending with old result
        C.r_Pass("null", "bloom_luminance_3", false, FALSE, FALSE, FALSE);
        C.r_Sampler_clf("s_image", r2_RT_luminance_t8);
        C.r_Sampler_clf("s_tonemap", r2_RT_luminance_src);
        C.r_End();
        break;
    }
#elif RENDER == R_GL
    switch (C.iElement)
    {
    case 0: // 256x256	=> 64x64
        C.r_Pass("stub_notransform_build", "bloom_luminance_1", false,FALSE,FALSE, FALSE);
        C.r_Sampler_clf("s_image", r2_RT_bloom1);
        C.r_End();
        break;
    case 1: // 64x64	=> 8x8
        C.r_Pass("stub_notransform_filter", "bloom_luminance_2", false,FALSE,FALSE, FALSE);
        C.r_Sampler_clf("s_image", r2_RT_luminance_t64);
        C.r_End();
        break;
    case 2: // 8x8		=> 1x1, blending with old result
        C.r_Pass("stub_notransform_filter", "bloom_luminance_3", false,FALSE,FALSE, FALSE);
        C.r_Sampler_clf("s_image", r2_RT_luminance_t8);
        C.r_Sampler_clf("s_tonemap", r2_RT_luminance_src);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case 0: // 256x256	=> 64x64
        C.r_Pass("stub_notransform_build", "bloom_luminance_1", false, FALSE, FALSE, FALSE);
        // C.r_Sampler_clf	("s_image",	r2_RT_bloom1);
        C.r_dx10Texture("s_image", r2_RT_bloom1);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1: // 64x64	=> 8x8
        C.r_Pass("stub_notransform_filter", "bloom_luminance_2", false, FALSE, FALSE, FALSE);
        // C.r_Sampler_clf	("s_image",	r2_RT_luminance_t64);
        C.r_dx10Texture("s_image", r2_RT_luminance_t64);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2: // 8x8		=> 1x1, blending with old result
        C.r_Pass("stub_notransform_filter", "bloom_luminance_3", false, FALSE, FALSE, FALSE);
        // C.r_Sampler_clf	("s_image",		r2_RT_luminance_t8	);
        // C.r_Sampler_clf	("s_tonemap",	r2_RT_luminance_src	);
        C.r_dx10Texture("s_image", r2_RT_luminance_t8);
        C.r_dx10Texture("s_tonemap", r2_RT_luminance_src);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_dx10Sampler("smp_nofilter");
        C.r_End();
        break;
    }
#endif
}
