#include "stdafx.h"
#pragma hdrstop

#include "blender_light_mask.h"

namespace xray::render::fg
{
CBlender_accum_direct_mask::CBlender_accum_direct_mask() { description.CLS = 0; }
CBlender_accum_direct_mask::~CBlender_accum_direct_mask() {}
void CBlender_accum_direct_mask::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_R2
    switch (C.iElement)
    {
    case SE_MASK_SPOT:
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_End();
        break;
    case SE_MASK_POINT:
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_End();
        break;
    case SE_MASK_DIRECT:
        C.r_Pass("null", "accum_sun_mask", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE, TRUE, 1);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL:
        C.r_Pass("accum_volume", "copy_p", false, FALSE, FALSE);
        C.r_Sampler_rtf("s_base", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D:
        C.r_Pass("null", "copy", false, FALSE, FALSE);
        C.r_Sampler_rtf("s_base", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ALBEDO:
        C.r_Pass("null", "copy", false, FALSE, FALSE);
        C.r_Sampler_rtf("s_base", r2_RT_accum);
        C.r_End();
        break;
    }
#elif RENDER == R_GL
    switch (C.iElement)
    {
    case SE_MASK_SPOT:
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT:
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT:
        C.r_Pass("stub_notransform_t", "accum_sun_mask_nomsaa", false, FALSE,FALSE,TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,
                 TRUE, 1);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL:
        C.r_Pass("accum_volume", "copy_p_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D:
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ALBEDO:
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case SE_MASK_SPOT:
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT:
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT:
        C.r_Pass("stub_notransform_t", "accum_sun_mask_nomsaa", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,
            TRUE, 1);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Sampler("smp_nofilter");
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL:
        C.r_Pass("accum_volume", "copy_p_nomsaa", false, FALSE, FALSE);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D:
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE, FALSE);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ALBEDO:
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE, FALSE);
        C.r_dx11Texture("s_generic", r2_RT_accum);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    }
#endif
}

#if RENDER != R_R2
void CBlender_accum_direct_mask_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (Name)
        RImplementation.m_MSAASample = atoi(Definition);
    else
        RImplementation.m_MSAASample = -1;

#if RENDER == R_GL
    switch (C.iElement)
    {
    case SE_MASK_SPOT:
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT:
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT:
        C.r_Pass("stub_notransform_t", "accum_sun_mask_msaa", false, FALSE,FALSE,TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,TRUE,
                 1);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL:
        C.r_Pass("accum_volume", "copy_p_msaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D:
        C.r_Pass("stub_notransform_t", "copy_msaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ALBEDO:
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case SE_MASK_SPOT:
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT:
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT:
        C.r_Pass("stub_notransform_t", "accum_sun_mask_msaa", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,
            TRUE, 1);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Sampler("smp_nofilter");
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL:
        C.r_Pass("accum_volume", "copy_p_msaa", false, FALSE, FALSE);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D:
        C.r_Pass("stub_notransform_t", "copy_msaa", false, FALSE, FALSE);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ALBEDO:
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE, FALSE);
        C.r_dx11Texture("s_generic", r2_RT_accum);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    }
#endif
    RImplementation.m_MSAASample = -1;
}
#endif
}
