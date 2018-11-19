#include "stdafx.h"
#pragma hdrstop

#include "blender_light_spot.h"

CBlender_accum_spot::CBlender_accum_spot() { description.CLS = 0; }
CBlender_accum_spot::~CBlender_accum_spot() { }

void CBlender_accum_spot::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    //	BOOL		b_HW_smap	= RImplementation.o.HW_smap;
    //	BOOL		b_HW_PCF	= RImplementation.o.HW_smap_PCF;
    BOOL blend = RImplementation.o.fp16_blend;
    D3DBLEND dest = blend ? D3DBLEND_ONE : D3DBLEND_ZERO;

    switch (C.iElement)
    {
    case SE_L_FILL: // masking
        C.r_Pass("stub_notransform", "copy_nomsaa", false, FALSE, FALSE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_End();
        break;
    case SE_L_UNSHADOWED: // unshadowed
        C.r_Pass("accum_volume", "accum_spot_unshadowed_nomsaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        C.r_End();
        break;
    case SE_L_NORMAL: // normal
        C.r_Pass("accum_volume", "accum_spot_normal_nomsaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
        C.r_Sampler_cmp("s_smap", r2_RT_smap_depth);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        jitter(C);
        C.r_End();
        break;
    case SE_L_FULLSIZE: // normal-fullsize
        C.r_Pass("accum_volume", "accum_spot_fullsize_nomsaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
        C.r_Sampler_cmp("s_smap", r2_RT_smap_depth);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        jitter(C);
        C.r_End();
        break;
    case SE_L_TRANSLUENT: // shadowed + transluency
        C.r_Pass("accum_volume", "accum_spot_fullsize_nomsaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler_clf("s_lmap", r2_RT_smap_surf); // diff here
        C.r_Sampler_cmp("s_smap", r2_RT_smap_depth);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        jitter(C);
        C.r_End();
        break;
    }
}

CBlender_accum_spot_msaa::CBlender_accum_spot_msaa() { description.CLS = 0; }
CBlender_accum_spot_msaa::~CBlender_accum_spot_msaa() { }

void CBlender_accum_spot_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    //	BOOL		b_HW_smap	= RImplementation.o.HW_smap;
    //	BOOL		b_HW_PCF	= RImplementation.o.HW_smap_PCF;
    BOOL blend = RImplementation.o.fp16_blend;
    D3DBLEND dest = blend ? D3DBLEND_ONE : D3DBLEND_ZERO;

    if (Name)
        GEnv.Render->m_MSAASample = atoi(Definition);
    else
        GEnv.Render->m_MSAASample = -1;

    switch (C.iElement)
    {
    case SE_L_FILL: // masking
        C.r_Pass("stub_notransform", "copy_msaa", false, FALSE, FALSE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_End();
        break;
    case SE_L_UNSHADOWED: // unshadowed
        C.r_Pass("accum_volume", "accum_spot_unshadowed_msaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        C.r_End();
        break;
    case SE_L_NORMAL: // normal
        C.r_Pass("accum_volume", "accum_spot_normal_msaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
        C.r_Sampler_cmp("s_smap", r2_RT_smap_depth);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        jitter(C);
        C.r_End();
        break;
    case SE_L_FULLSIZE: // normal-fullsize
        C.r_Pass("accum_volume", "accum_spot_fullsize_msaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
        C.r_Sampler_cmp("s_smap", r2_RT_smap_depth);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        jitter(C);
        C.r_End();
        break;
    case SE_L_TRANSLUENT: // shadowed + transluency
        C.r_Pass("accum_volume", "accum_spot_fullsize_msaa", false, FALSE,FALSE, blend, D3DBLEND_ONE, dest);
        C.r_Sampler_rtf("s_position", r2_RT_P);
        C.r_Sampler_rtf("s_normal", r2_RT_N);
        C.r_Sampler_clw("s_material", r2_material);
        C.r_Sampler_clf("s_lmap", r2_RT_smap_surf); // diff here
        C.r_Sampler_cmp("s_smap", r2_RT_smap_depth);
        C.r_Sampler_rtf("s_accumulator", r2_RT_accum);
        jitter(C);
        C.r_End();
        break;
    }
    GEnv.Render->m_MSAASample = -1;
}

CBlender_accum_volumetric_msaa::CBlender_accum_volumetric_msaa() { description.CLS = 0; }
CBlender_accum_volumetric_msaa::~CBlender_accum_volumetric_msaa() { }

void CBlender_accum_volumetric_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (Name)
        GEnv.Render->m_MSAASample = atoi(Definition);
    else
        GEnv.Render->m_MSAASample = -1;

    switch (C.iElement)
    {
    case 0: // masking
        C.r_Pass("accum_volumetric", "accum_volumetric_msaa", false, FALSE, FALSE);

        C.r_Sampler("s_lmap", C.L_textures[0]);
        C.r_Sampler_cmp("s_smap", r2_RT_smap_depth);
        C.r_Sampler("s_noise", "fx" DELIMITER "fx_noise");
        C.r_End();
        break;
    }
    GEnv.Render->m_MSAASample = -1;
}
