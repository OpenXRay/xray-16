#include "stdafx.h"
#pragma hdrstop

#include "blender_bloom_build.h"

CBlender_bloom_build::CBlender_bloom_build() { description.CLS = 0; }
CBlender_bloom_build::~CBlender_bloom_build() {}
void CBlender_bloom_build::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_R2
    switch (C.iElement)
    {
    case 0: // transfer into bloom-target
        C.r_Pass("null", "bloom_build", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        C.r_Sampler_clf("s_image", r2_RT_generic1);
        C.r_End();
        break;
    case 1: // X-filter
        C.r_Pass("null", "bloom_filter", FALSE, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_End();
        break;
    case 2: // Y-filter
        C.r_Pass("null", "bloom_filter", FALSE, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
        C.r_End();
        break;
    case 3: // FF-filter_P0
        C.r_Pass("null", "bloom_filter_f", FALSE, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_End();
        break;
    case 4: // FF-filter_P1
        C.r_Pass("null", "bloom_filter_f", FALSE, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
        C.r_End();
        break;
    }
#elif RENDER == R_GL
    switch (C.iElement)
    {
    case 0: // transfer into bloom-target
        C.r_Pass("stub_notransform_build", "bloom_build", false, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
                 D3DBLEND_INVSRCALPHA);
        C.r_Sampler_clf("s_image", r2_RT_generic1);
        C.r_End();
        break;
    case 1: // X-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_End();
        break;
    case 2: // Y-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
        C.r_End();
        break;
    case 3: // FF-filter_P0
        C.r_Pass("stub_notransform_build", "bloom_filter_f", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_End();
        break;
    case 4: // FF-filter_P1
        C.r_Pass("stub_notransform_build", "bloom_filter_f", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case 0: // transfer into bloom-target
        C.r_Pass("stub_notransform_build", "bloom_build", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic1);
        C.r_dx10Texture("s_image", r2_RT_generic1);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1: // X-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        C.r_dx10Texture("s_bloom", r2_RT_bloom1);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2: // Y-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom2);
        C.r_dx10Texture("s_bloom", r2_RT_bloom2);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 3: // FF-filter_P0
        C.r_Pass("stub_notransform_build", "bloom_filter_f", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        C.r_dx10Texture("s_bloom", r2_RT_bloom1);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 4: // FF-filter_P1
        C.r_Pass("stub_notransform_build", "bloom_filter_f", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom2);
        C.r_dx10Texture("s_bloom", r2_RT_bloom2);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    }
#endif
}

#if RENDER != R_R2
CBlender_bloom_build_msaa::CBlender_bloom_build_msaa() { description.CLS = 0; }
CBlender_bloom_build_msaa::~CBlender_bloom_build_msaa() {}
void CBlender_bloom_build_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_GL
    switch (C.iElement)
    {
    case 0: // transfer into bloom-target
        C.r_Pass("stub_notransform_build", "bloom_build", false, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
                 D3DBLEND_INVSRCALPHA);
        C.r_Sampler_clf("s_image", r2_RT_generic1_r);
        C.r_End();
        break;
    case 1: // X-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_End();
        break;
    case 2: // Y-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
        C.r_End();
        break;
    case 3: // FF-filter_P0
        C.r_Pass("stub_notransform_build", "bloom_filter_f", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
        C.r_End();
        break;
    case 4: // FF-filter_P1
        C.r_Pass("stub_notransform_build", "bloom_filter_f", false, FALSE, FALSE);
        C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case 0: // transfer into bloom-target
        C.r_Pass("stub_notransform_build", "bloom_build", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA);
        // C.r_Sampler_clf		("s_image",			r2_RT_generic1);
        C.r_dx10Texture("s_image", r2_RT_generic1_r);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1: // X-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        C.r_dx10Texture("s_bloom", r2_RT_bloom1);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2: // Y-filter
        C.r_Pass("stub_notransform_filter", "bloom_filter", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom2);
        C.r_dx10Texture("s_bloom", r2_RT_bloom2);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 3: // FF-filter_P0
        C.r_Pass("stub_notransform_build", "bloom_filter_f", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom1);
        C.r_dx10Texture("s_bloom", r2_RT_bloom1);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 4: // FF-filter_P1
        C.r_Pass("stub_notransform_build", "bloom_filter_f", FALSE, FALSE, FALSE);
        // C.r_Sampler_clf		("s_bloom",			r2_RT_bloom2);
        C.r_dx10Texture("s_bloom", r2_RT_bloom2);
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    }
#endif
}

CBlender_postprocess_msaa::CBlender_postprocess_msaa() { description.CLS = 0; }
CBlender_postprocess_msaa::~CBlender_postprocess_msaa() {}
void CBlender_postprocess_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_GL
    switch (C.iElement)
    {
    case 0: // transfer into bloom-target
        C.r_Pass("stub_notransform_postpr", "postprocess", false, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
                 D3DBLEND_INVSRCALPHA);
        C.r_Sampler_clf("s_base0", r2_RT_generic);
        C.r_Sampler_clf("s_base1", r2_RT_generic);
        C.r_Sampler("s_noise", "fx\\fx_noise2");
        C.r_End();
        break;

    case 4: // use color map
        C.r_Pass("stub_notransform_postpr", "postprocess_CM", false, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
                 D3DBLEND_INVSRCALPHA);
        C.r_Sampler_clf("s_base0", r2_RT_generic);
        C.r_Sampler_clf("s_base1", r2_RT_generic);
        C.r_Sampler("s_noise", "fx\\fx_noise2");
        C.r_Sampler("s_grad0", "$user$cmap0");
        C.r_Sampler("s_grad1", "$user$cmap1");

        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case 0: // transfer into bloom-target
        C.r_Pass("stub_notransform_postpr", "postprocess", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA);
        C.r_dx10Texture("s_base0", r2_RT_generic);
        C.r_dx10Texture("s_base1", r2_RT_generic);
        C.r_dx10Texture("s_noise", "fx" DELIMITER "fx_noise2");

        C.r_dx10Sampler("smp_rtlinear");
        C.r_dx10Sampler("smp_linear");
        C.r_End();
        break;

    case 4: // use color map
        C.r_Pass("stub_notransform_postpr", "postprocess_CM", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA);
        C.r_dx10Texture("s_base0", r2_RT_generic);
        C.r_dx10Texture("s_base1", r2_RT_generic);
        C.r_dx10Texture("s_noise", "fx" DELIMITER "fx_noise2");
        C.r_dx10Texture("s_grad0", "$user$cmap0");
        C.r_dx10Texture("s_grad1", "$user$cmap1");

        C.r_dx10Sampler("smp_rtlinear");
        C.r_dx10Sampler("smp_linear");
        C.r_End();
        break;
    }
#endif
}
#endif
