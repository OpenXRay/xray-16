#include "stdafx.h"
#pragma hdrstop

#include "Blender_ss_sunshafts.h"

CBlender_sunshafts::CBlender_sunshafts() { description.CLS = 0; }
CBlender_sunshafts::~CBlender_sunshafts() {	}

void	CBlender_sunshafts::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0:	// generation of sunshafts mask
        C.r_Pass("ogse_ssss_notransform", "ogse_sunshafts_mask", FALSE, FALSE, FALSE);
        C.r_dx10Texture("s_position", r2_RT_P);
        C.r_dx10Texture("s_image", r2_RT_generic0);

        C.r_dx10Sampler("smp_nofilter");
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 1:	// first pass
        C.r_Pass("ogse_ssss_notransform", "ogse_sunshafts_blur", FALSE, FALSE, FALSE);
        C.r_dx10Texture("s_sun_shafts", r2_RT_sunshafts0);

        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 2:	// second pass
        C.r_Pass("ogse_ssss_notransform", "ogse_sunshafts_blur", FALSE, FALSE, FALSE);
        C.r_dx10Texture("s_sun_shafts", r2_RT_sunshafts1);

        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 3:	// third pass
        C.r_Pass("ogse_ssss_notransform", "ogse_sunshafts_blur", FALSE, FALSE, FALSE);
        C.r_dx10Texture("s_sun_shafts", r2_RT_sunshafts0);

        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    case 4:	// combine pass
        C.r_Pass("ogse_ssss_notransform", "ogse_sunshafts_final", FALSE, FALSE, FALSE);
        C.r_dx10Texture("s_position", r2_RT_P);
        C.r_dx10Texture("s_sun_shafts", r2_RT_sunshafts1);
        C.r_dx10Texture("s_image", r2_RT_generic0);

        jitter(C);

        C.r_dx10Sampler("smp_nofilter");
        C.r_dx10Sampler("smp_rtlinear");
        C.r_End();
        break;
    }
}
