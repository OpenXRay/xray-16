#include "stdafx.h"
#include "./dx11RainBlender.h"

void CBlender_rain::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    RImplementation.m_SMAPSize = RImplementation.o.rain_smapsize;

    switch (C.iElement)
    {
    case 0: // Test
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_SRCALPHA,
        // D3DBLEND_INVSRCALPHA);
        C.r_Pass("stub_notransform_2uv", "rain_layer", false, TRUE, FALSE, FALSE);
        C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_material", r2_material);
        C.r_dx11Texture("s_accumulator", r2_RT_accum);
        C.r_dx11Texture("s_lmap", r2_sunmask);
        C.r_dx11Texture("s_smap", r2_RT_smap_rain);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_linear");
        jitter(C);
        C.r_dx11Sampler("smp_smap");

        //		C.r_dx11Texture		("s_water",	"water\\water_water");

        // C.r_dx11Texture		("s_water",	"water\\water_studen");
        C.r_dx11Texture("s_water", "water\\water_normal");

        C.r_End();

        break;

    case 1: // Patch normals
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_SRCALPHA,
        // D3DBLEND_INVSRCALPHA);
        C.r_Pass("stub_notransform_2uv", "rain_patch_normal_nomsaa", false, TRUE, FALSE, FALSE);
        C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_material", r2_material);
        // C.r_dx11Texture		("s_accumulator",	r2_RT_accum);
        C.r_dx11Texture("s_lmap", r2_sunmask);
        C.r_dx11Texture("s_smap", r2_RT_smap_rain);

        C.r_dx11Texture("s_diffuse", r2_RT_albedo);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_linear");
        C.r_dx11Sampler("smp_base");
        jitter(C);
        C.r_dx11Sampler("smp_smap");

        //		C.r_dx11Texture		("s_water",	"water\\water_water");

        // C.r_dx11Texture		("s_water",	"water\\water_studen");
        // C.r_dx11Texture		("s_water",	"water\\water_normal");

        C.r_dx11Texture("s_water", "water\\water_SBumpVolume");
        // C.r_dx11Texture		("s_waterFall",	"water\\water_normal");
        C.r_dx11Texture("s_waterFall", "water\\water_flowing_nmap");

        C.r_End();

        break;

    case 2: // Apply normals
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_SRCALPHA,
        // D3DBLEND_INVSRCALPHA);
        C.r_Pass("stub_notransform_2uv", "rain_apply_normal_nomsaa", false, TRUE, FALSE, FALSE);
        C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

        C.r_dx11Texture("s_position", r2_RT_P);
        // C.r_dx11Texture		("s_normal",		r2_RT_N);
        C.r_dx11Texture("s_material", r2_material);
        // C.r_dx11Texture		("s_accumulator",	r2_RT_accum);
        C.r_dx11Texture("s_lmap", r2_sunmask);
        C.r_dx11Texture("s_smap", r2_RT_smap_rain);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_linear");
        jitter(C);
        C.r_dx11Sampler("smp_smap");

        //		C.r_dx11Texture		("s_water",	"water\\water_water");

        // C.r_dx11Texture		("s_water",	"water\\water_studen");
        C.r_dx11Texture("s_patched_normal", r2_RT_accum);

        //	Normal can be packed into R and G
        if (RImplementation.o.gbuffer_opt)
            C.r_ColorWriteEnable(true, true, false, false);
        else
            C.r_ColorWriteEnable(true, true, true, false);

        C.r_End();

        break;

    case 3: // Apply gloss
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_SRCALPHA,
        // D3DBLEND_INVSRCALPHA);
        C.r_Pass(
        "stub_notransform_2uv", "rain_apply_gloss_nomsaa", false, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

        C.r_dx11Texture("s_position", r2_RT_P);
        // C.r_dx11Texture		("s_normal",		r2_RT_N);
        C.r_dx11Texture("s_material", r2_material);
        // C.r_dx11Texture		("s_accumulator",	r2_RT_accum);
        C.r_dx11Texture("s_lmap", r2_sunmask);
        C.r_dx11Texture("s_smap", r2_RT_smap_rain);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_linear");
        jitter(C);
        C.r_dx11Sampler("smp_smap");

        //		C.r_dx11Texture		("s_water",	"water\\water_water");

        // C.r_dx11Texture		("s_water",	"water\\water_studen");
        C.r_dx11Texture("s_patched_normal", r2_RT_accum);

        // C.r_ColorWriteEnable( false, false, false, true );

        C.RS.SetRS(D3DRS_SRCBLEND, D3DBLEND_ZERO);
        C.RS.SetRS(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);

        C.r_End();

        break;
    }

    RImplementation.m_SMAPSize = RImplementation.o.smapsize;
}

void CBlender_rain_msaa::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (Name)
        RImplementation.m_MSAASample = atoi(Definition);
    else
        RImplementation.m_MSAASample = -1;

    RImplementation.m_SMAPSize = RImplementation.o.rain_smapsize;

    switch (C.iElement)
    {
    case 0: // Patch normals
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_SRCALPHA,
        // D3DBLEND_INVSRCALPHA);
        C.r_Pass("stub_notransform_2uv", "rain_patch_normal_msaa", false, TRUE, FALSE, FALSE);
        C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Texture("s_material", r2_material);
        // C.r_dx11Texture		("s_accumulator",	r2_RT_accum);
        C.r_dx11Texture("s_lmap", r2_sunmask);
        C.r_dx11Texture("s_smap", r2_RT_smap_rain);

        C.r_dx11Texture("s_diffuse", r2_RT_albedo);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_linear");
        C.r_dx11Sampler("smp_base");
        jitter(C);
        C.r_dx11Sampler("smp_smap");

        //		C.r_dx11Texture		("s_water",	"water\\water_water");

        // C.r_dx11Texture		("s_water",	"water\\water_studen");
        // C.r_dx11Texture		("s_water",	"water\\water_normal");

        C.r_dx11Texture("s_water", "water\\water_SBumpVolume");
        // C.r_dx11Texture		("s_waterFall",	"water\\water_normal");
        C.r_dx11Texture("s_waterFall", "water\\water_flowing_nmap");

        C.r_End();

        break;

    case 1: // Apply normals
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_SRCALPHA,
        // D3DBLEND_INVSRCALPHA);
        C.r_Pass("stub_notransform_2uv", "rain_apply_normal_msaa", false, TRUE, FALSE, FALSE);
        C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

        C.r_dx11Texture("s_position", r2_RT_P);
        // C.r_dx11Texture		("s_normal",		r2_RT_N);
        C.r_dx11Texture("s_material", r2_material);
        // C.r_dx11Texture		("s_accumulator",	r2_RT_accum);
        C.r_dx11Texture("s_lmap", r2_sunmask);
        C.r_dx11Texture("s_smap", r2_RT_smap_rain);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_linear");
        jitter(C);
        C.r_dx11Sampler("smp_smap");

        //		C.r_dx11Texture		("s_water",	"water\\water_water");

        // C.r_dx11Texture		("s_water",	"water\\water_studen");
        C.r_dx11Texture("s_patched_normal", r2_RT_accum);

        //	Normal can be packed into R and G
        if (RImplementation.o.gbuffer_opt)
            C.r_ColorWriteEnable(true, true, false, false);
        else
            C.r_ColorWriteEnable(true, true, true, false);

        C.r_End();

        break;

    case 2: // Apply gloss
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // C.r_Pass	("stub_notransform_2uv", "rain_layer", false,	TRUE,	FALSE, TRUE, D3DBLEND_SRCALPHA,
        // D3DBLEND_INVSRCALPHA);
        C.r_Pass("stub_notransform_2uv", "rain_apply_gloss_msaa", false, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

        C.r_dx11Texture("s_position", r2_RT_P);
        // C.r_dx11Texture		("s_normal",		r2_RT_N);
        C.r_dx11Texture("s_material", r2_material);
        // C.r_dx11Texture		("s_accumulator",	r2_RT_accum);
        C.r_dx11Texture("s_lmap", r2_sunmask);
        C.r_dx11Texture("s_smap", r2_RT_smap_rain);

        C.r_dx11Sampler("smp_nofilter");
        C.r_dx11Sampler("smp_material");
        C.r_dx11Sampler("smp_linear");
        jitter(C);
        C.r_dx11Sampler("smp_smap");

        //		C.r_dx11Texture		("s_water",	"water\\water_water");

        // C.r_dx11Texture		("s_water",	"water\\water_studen");
        C.r_dx11Texture("s_patched_normal", r2_RT_accum);

        // C.r_ColorWriteEnable( false, false, false, true );

        C.RS.SetRS(D3DRS_SRCBLEND, D3DBLEND_ZERO);
        C.RS.SetRS(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);

        C.r_End();

        break;
    }
    RImplementation.m_MSAASample = -1;
    RImplementation.m_SMAPSize = RImplementation.o.smapsize;
}
