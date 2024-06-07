#include "stdafx.h"
#pragma hdrstop

#include "blender_light_reflected.h"

CBlender_accum_reflected::CBlender_accum_reflected() { description.CLS = 0; }
CBlender_accum_reflected::~CBlender_accum_reflected() {}
void CBlender_accum_reflected::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    BOOL blend = RImplementation.o.fp16_blend;
    D3DBLEND dest = blend ? D3DBLEND_ONE : D3DBLEND_ZERO;

#if RENDER == R_R2
    C.r_Pass("accum_volume", "accum_indirect", false, FALSE, FALSE, blend, D3DBLEND_ONE, dest);
    C.r_Sampler_rtf("s_position", r2_RT_P);
    C.r_Sampler_rtf("s_normal", r2_RT_N);
    C.r_Sampler_clw("s_material", r2_material);
    C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
#elif RENDER == R_GL
    C.r_Pass("accum_volume", "accum_indirect_nomsaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
    C.r_Sampler_rtf("s_position", r2_RT_P);
    C.r_Sampler_rtf("s_normal", r2_RT_N);
    C.r_Sampler_clw("s_material", r2_material);
    C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
#else
    C.r_Pass("accum_volume", "accum_indirect_nomsaa", false, FALSE, FALSE, blend, D3DBLEND_ONE, dest);
    // C.r_Sampler_rtf		("s_position",		r2_RT_P);
    // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
    // C.r_Sampler_clw		("s_material",		r2_material);
    // C.r_Sampler_rtf		("s_accumulator",	r2_RT_accum		);
    C.r_dx11Texture("s_position", r2_RT_P);
    C.r_dx11Texture("s_normal", r2_RT_N);
    C.r_dx11Texture("s_material", r2_material);
    C.r_dx11Texture("s_accumulator", r2_RT_accum);
    C.r_dx11Texture("s_diffuse", r2_RT_albedo);

    C.r_dx11Sampler("smp_nofilter");
    C.r_dx11Sampler("smp_material");
#endif
    C.r_End();
}

#if RENDER != R_R2
void CBlender_accum_reflected_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    BOOL blend = RImplementation.o.fp16_blend;
    D3DBLEND dest = blend ? D3DBLEND_ONE : D3DBLEND_ZERO;

    if (Name)
        RImplementation.m_MSAASample = atoi(Definition);
    else
        RImplementation.m_MSAASample = -1;

#if RENDER == R_GL
    C.r_Pass("accum_volume", "accum_indirect_msaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
    C.r_Sampler_rtf("s_position", r2_RT_P);
    C.r_Sampler_rtf("s_normal", r2_RT_N);
    C.r_Sampler_clw("s_material", r2_material);
    C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
#else
    C.r_Pass("accum_volume", "accum_indirect_msaa", false, FALSE, FALSE, blend, D3DBLEND_ONE, dest);
    // C.r_Sampler_rtf		("s_position",		r2_RT_P);
    // C.r_Sampler_rtf		("s_normal",		r2_RT_N);
    // C.r_Sampler_clw		("s_material",		r2_material);
    // C.r_Sampler_rtf		("s_accumulator",	r2_RT_accum		);
    C.r_dx11Texture("s_position", r2_RT_P);
    C.r_dx11Texture("s_normal", r2_RT_N);
    C.r_dx11Texture("s_material", r2_material);
    C.r_dx11Texture("s_accumulator", r2_RT_accum);
    C.r_dx11Texture("s_diffuse", r2_RT_albedo);

    C.r_dx11Sampler("smp_nofilter");
    C.r_dx11Sampler("smp_material");
#endif
    C.r_End();

    RImplementation.m_MSAASample = -1;
}
#endif
