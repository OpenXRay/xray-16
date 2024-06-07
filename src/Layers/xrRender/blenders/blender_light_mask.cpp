#include "stdafx.h"
#pragma hdrstop

#include "blender_light_mask.h"

CBlender_accum_direct_mask::CBlender_accum_direct_mask() { description.CLS = 0; }
CBlender_accum_direct_mask::~CBlender_accum_direct_mask() {}
void CBlender_accum_direct_mask::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_R2
    switch (C.iElement)
    {
    case SE_MASK_SPOT: // spot or omni-part
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_End();
        break;
    case SE_MASK_POINT: // point
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_End();
        break;
    case SE_MASK_DIRECT: // stencil mask for directional light
        C.r_Pass("null", "accum_sun_mask", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE, TRUE, 1);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL: // copy accumulator (temp -> real), volumetric (usually after blend)
        C.r_Pass("accum_volume", "copy_p", false, FALSE, FALSE);
        C.r_Sampler_rtf("s_base", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D: // copy accumulator (temp -> real), 2D (usually after sun-blend)
        C.r_Pass("null", "copy", false, FALSE, FALSE);
        C.r_Sampler_rtf("s_base", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ALBEDO: // copy accumulator, 2D (for accum->color, albedo_wo)
        C.r_Pass("null", "copy", false, FALSE, FALSE);
        C.r_Sampler_rtf("s_base", r2_RT_accum);
        C.r_End();
        break;
    }
#elif RENDER == R_GL
    switch (C.iElement)
    {
    case SE_MASK_SPOT: // spot or omni-part
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P); //	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT: // point
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P); //	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT: // stencil mask for directional light
        //	FVF::F_TL
        //C.r_Pass			("null",			"accum_sun_mask",	false,	FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        //C.r_Pass			("stub_notransform","accum_sun_mask",	false,	FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        C.r_Pass("stub_notransform_t", "accum_sun_mask_nomsaa", false, FALSE,FALSE,TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,
                 TRUE, 1);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL: // copy accumulator (temp -> real), volumetric (usually after blend)
        C.r_Pass("accum_volume", "copy_p_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D: // copy accumulator (temp -> real), 2D (usually after sun-blend)
        //	FVF::F_TL2uv but only uv0 is used
        //C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        //C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ALBEDO: // copy accumulator, 2D (for accum->color, albedo_wo)
        //	FVF::F_TL2uv but only uv0 is used
        //C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        //C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case SE_MASK_SPOT: // spot or omni-part
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Texture		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Sampler		("smp_nofilter");	//	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT: // point
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Texture		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Sampler		("smp_nofilter");	//	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT: // stencil mask for directional light
        //	FVF::F_TL
        // C.r_Pass			("null",			"accum_sun_mask",	false,
        // FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        // C.r_Pass			("stub_notransform","accum_sun_mask",	false,
        // FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        C.r_Pass("stub_notransform_t", "accum_sun_mask_nomsaa", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,
            TRUE, 1);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_diffuse", r2_RT_albedo);
        C.r_dx11Sampler("smp_nofilter");
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL: // copy accumulator (temp -> real), volumetric (usually after blend)
        C.r_Pass("accum_volume", "copy_p_nomsaa", false, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_base",			r2_RT_accum_temp	);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D: // copy accumulator (temp -> real), 2D (usually after sun-blend)
        //	FVF::F_TL2uv but only uv0 is used
        // C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        // C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_base",			r2_RT_accum_temp	);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ALBEDO: // copy accumulator, 2D (for accum->color, albedo_wo)
        //	FVF::F_TL2uv but only uv0 is used
        // C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        // C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_base",			r2_RT_accum			);
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
    case SE_MASK_SPOT: // spot or omni-part
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P); //	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT: // point
        C.r_Pass("accum_mask", "dumb", false, TRUE,FALSE);
        C.r_Sampler_rtf("s_position", r2_RT_P); //	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT: // stencil mask for directional light
        //	FVF::F_TL
        //C.r_Pass			("null",			"accum_sun_mask",	false,	FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        //C.r_Pass			("stub_notransform","accum_sun_mask",	false,	FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        C.r_Pass("stub_notransform_t", "accum_sun_mask_msaa", false, FALSE,FALSE,TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,TRUE,
                 1);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL: // copy accumulator (temp -> real), volumetric (usually after blend)
        C.r_Pass("accum_volume", "copy_p_msaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D: // copy accumulator (temp -> real), 2D (usually after sun-blend)
        //	FVF::F_TL2uv but only uv0 is used
        //C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        //C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_msaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum_temp);
        C.r_End();
        break;
    case SE_MASK_ALBEDO: // copy accumulator, 2D (for accum->color, albedo_wo)
        //	FVF::F_TL2uv but only uv0 is used
        //C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        //C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE,FALSE);
        C.r_Sampler_rtf("s_generic", r2_RT_accum);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case SE_MASK_SPOT: // spot or omni-part
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Texture		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Sampler		("smp_nofilter");	//	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_POINT: // point
        C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
        // C.r_Sampler_rtf		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Texture		("s_position",		r2_RT_P);	//	???
        // C.r_dx11Sampler		("smp_nofilter");	//	???
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_DIRECT: // stencil mask for directional light
        //	FVF::F_TL
        // C.r_Pass			("null",			"accum_sun_mask",	false,
        // FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        // C.r_Pass			("stub_notransform","accum_sun_mask",	false,
        // FALSE,FALSE,TRUE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,1);
        C.r_Pass("stub_notransform_t", "accum_sun_mask_msaa", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,
            TRUE, 1);
        // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_diffuse", r2_RT_albedo);
        C.r_dx11Sampler("smp_nofilter");
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    case SE_MASK_ACCUM_VOL: // copy accumulator (temp -> real), volumetric (usually after blend)
        C.r_Pass("accum_volume", "copy_p_msaa", false, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_base",			r2_RT_accum_temp	);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ACCUM_2D: // copy accumulator (temp -> real), 2D (usually after sun-blend)
        //	FVF::F_TL2uv but only uv0 is used
        // C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        // C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_msaa", false, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_base",			r2_RT_accum_temp	);
        C.r_dx11Texture("s_generic", r2_RT_accum_temp);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    case SE_MASK_ALBEDO: // copy accumulator, 2D (for accum->color, albedo_wo)
        //	FVF::F_TL2uv but only uv0 is used
        // C.r_Pass			("null",			"copy",				false,	FALSE,FALSE);
        // C.r_Pass			("stub_notransform","copy",				false,	FALSE,FALSE);
        C.r_Pass("stub_notransform_t", "copy_nomsaa", false, FALSE, FALSE);
        // C.r_Sampler_rtf		("s_base",			r2_RT_accum			);
        C.r_dx11Texture("s_generic", r2_RT_accum);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();
        break;
    }
#endif
    RImplementation.m_MSAASample = -1;
}
#endif
