#include "stdafx.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/blenders/blender_light_occq.h"
#include "Layers/xrRender/blenders/blender_light_mask.h"
#include "Layers/xrRender/blenders/blender_light_direct.h"
#include "Layers/xrRender/blenders/blender_light_point.h"
#include "Layers/xrRender/blenders/blender_light_spot.h"
#include "Layers/xrRender/blenders/blender_light_reflected.h"
#include "Layers/xrRender/blenders/blender_combine.h"
#include "Layers/xrRender/blenders/blender_bloom_build.h"
#include "Layers/xrRender/blenders/blender_luminance.h"
#include "Layers/xrRender/blenders/blender_ssao.h"

#if RENDER == R_R2 // XXX: merge old/new cascade blenders into one file
#include "Layers/xrRender/blenders/blender_light_direct_cascade.h"
#endif

#ifndef USE_DX9
#   include "Layers/xrRender/blenders/dx10MSAABlender.h"
#   include "Layers/xrRender/blenders/dx10RainBlender.h"

#   ifdef USE_DX11
#       include "Layers/xrRender/blenders/dx11MinMaxSMBlender.h"
#       include "Layers/xrRender/blenders/dx11HDAOCSBlender.h"
#   else
#       include "Layers/xrRender/blenders/dx10MinMaxSMBlender.h"
#   endif
#endif

#ifdef USE_DX9
void CRenderTarget::u_stencil_optimize(BOOL common_stencil)
#else
void CRenderTarget::u_stencil_optimize(eStencilOptimizeMode eSOM)
#endif
{
#ifdef USE_OGL
    //	TODO: OGL: should we implement stencil optimization?
    VERIFY(RImplementation.o.nvstencil);
    VERIFY(!"CRenderTarget::u_stencil_optimize no implemented");
    UNUSED(eSOM);
#else
    // TODO: DX10: remove half pixel offset?
    VERIFY(RImplementation.o.nvstencil);
#   ifdef USE_DX9
    RCache.set_ColorWriteEnable(false);
#   endif
    u32 Offset;
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    u32 C = color_rgba(255, 255, 255, 255);
    FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
#   ifdef USE_DX9
    float eps = EPS_S;
    pv->set(eps, float(_h + eps), eps, 1.f, C, 0, 0);
    pv++;
    pv->set(eps, eps, eps, 1.f, C, 0, 0);
    pv++;
    pv->set(float(_w + eps), float(_h + eps), eps, 1.f, C, 0, 0);
    pv++;
    pv->set(float(_w + eps), eps, eps, 1.f, C, 0, 0);
    pv++;
#   else
    float eps = 0;
    float _dw = 0.5f;
    float _dh = 0.5f;
    pv->set(-_dw, _h - _dh, eps, 1.f, C, 0, 0);
    pv++;
    pv->set(-_dw, -_dh, eps, 1.f, C, 0, 0);
    pv++;
    pv->set(_w - _dw, _h - _dh, eps, 1.f, C, 0, 0);
    pv++;
    pv->set(_w - _dw, -_dh, eps, 1.f, C, 0, 0);
    pv++;
#   endif
    RCache.Vertex.Unlock(4, g_combine->vb_stride);
#   ifdef USE_DX9
    RCache.set_CullMode(CULL_NONE);
    if (common_stencil)
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00); // keep/keep/keep
#   endif
    RCache.set_Element(s_occq->E[1]);

#   ifndef USE_DX9
    switch (eSOM)
    {
    case SO_Light: StateManager.SetStencilRef(dwLightMarkerID); break;
    case SO_Combine: StateManager.SetStencilRef(0x01); break;
    default: VERIFY(!"CRenderTarget::u_stencil_optimize. switch no default!");
    }
#   endif

    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
#endif
}

// 2D texgen (texture adjustment matrix)
void CRenderTarget::u_compute_texgen_screen(Fmatrix& m_Texgen)
{
#ifdef USE_DX9
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    float o_w = (.5f / _w);
    float o_h = (.5f / _h);
#endif
    Fmatrix m_TexelAdjust =
    {
        0.5f, 0.0f, 0.0f, 0.0f,
#ifdef USE_OGL
        0.0f, 0.5f, 0.0f, 0.0f,
#else
        0.0f, -0.5f, 0.0f, 0.0f,
#endif
        0.0f, 0.0f, 1.0f, 0.0f,
#ifdef USE_DX9
        0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f
#else
        0.5f, 0.5f, 0.0f, 1.0f
#endif
    };
    m_Texgen.mul(m_TexelAdjust, RCache.xforms.m_wvp);
}

// 2D texgen for jitter (texture adjustment matrix)
void CRenderTarget::u_compute_texgen_jitter(Fmatrix& m_Texgen_J)
{
    // place into 0..1 space
    Fmatrix m_TexelAdjust =
    {
        0.5f, 0.0f, 0.0f, 0.0f,
#ifdef USE_OGL
        0.0f, 0.5f, 0.0f, 0.0f,
#else
        0.0f, -0.5f, 0.0f, 0.0f,
#endif
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    };
    m_Texgen_J.mul(m_TexelAdjust, RCache.xforms.m_wvp);

    // rescale - tile it
    float scale_X = float(Device.dwWidth) / float(TEX_jitter);
    float scale_Y = float(Device.dwHeight) / float(TEX_jitter);
    m_TexelAdjust.scale(scale_X, scale_Y, 1.f);
#ifdef USE_DX9
    float offset = (.5f / float(TEX_jitter));
    m_TexelAdjust.translate_over(offset, offset, 0);
#endif
    m_Texgen_J.mulA_44(m_TexelAdjust);
}

u8 fpack(float v)
{
    s32 _v = iFloor(((v + 1) * .5f) * 255.f + .5f);
    clamp(_v, 0, 255);
    return u8(_v);
}

u8 fpackZ(float v)
{
    s32 _v = iFloor(_abs(v) * 255.f + .5f);
    clamp(_v, 0, 255);
    return u8(_v);
}

Fvector vunpack(s32 x, s32 y, s32 z)
{
    Fvector pck;
    pck.x = (float(x) / 255.f - .5f) * 2.f;
    pck.y = (float(y) / 255.f - .5f) * 2.f;
    pck.z = -float(z) / 255.f;
    return pck;
}

Fvector vunpack(const Ivector& src)
{
    return vunpack(src.x, src.y, src.z);
}

Ivector vpack(const Fvector& src)
{
    Fvector _v;
    int bx = fpack(src.x);
    int by = fpack(src.y);
    int bz = fpackZ(src.z);
    // dumb test
    float e_best = flt_max;
    int r = bx, g = by, b = bz;
#ifdef DEBUG
    int d = 0;
#else
    int d = 3;
#endif
    for (int x = _max(bx - d, 0); x <= _min(bx + d, 255); x++)
        for (int y = _max(by - d, 0); y <= _min(by + d, 255); y++)
            for (int z = _max(bz - d, 0); z <= _min(bz + d, 255); z++)
            {
                _v = vunpack(x, y, z);
                float m = _v.magnitude();
                float me = _abs(m - 1.f);
                if (me > 0.03f)
                    continue;
                _v.div(m);
                float e = _abs(src.dotproduct(_v) - 1.f);
                if (e < e_best)
                {
                    e_best = e;
                    r = x, g = y, b = z;
                }
            }
    Ivector ipck;
    ipck.set(r, g, b);
    return ipck;
}

void manually_assign_texture(ref_shader& shader, pcstr textureName, pcstr rendertargetTextureName)
{
    SPass& pass = *shader->E[0]->passes[0];
    if (!pass.constants)
        return;

    const ref_constant constant = pass.constants->get(textureName);
    if (!constant)
        return;

    const auto index = constant->samp.index;
    pass.T->create_texture(index, rendertargetTextureName, false);
}

CRenderTarget::CRenderTarget()
{
    u32 SampleCount = 1;

    if (ps_r_ssao_mode != 2 /*hdao*/)
        ps_r_ssao = _min(ps_r_ssao, 3);

#ifndef USE_DX9
    RImplementation.o.ssao_ultra = ps_r_ssao > 3 && HW.ComputeShadersSupported;
#endif
    if (RImplementation.o.dx10_msaa)
        SampleCount = RImplementation.o.dx10_msaa_samples;

#ifdef DEBUG
    Msg("MSAA samples = %d", SampleCount);
    if (RImplementation.o.dx10_msaa_opt)
        Msg("dx10_MSAA_opt = on");
    if (RImplementation.o.dx10_gbuffer_opt)
        Msg("dx10_gbuffer_opt = on");
#endif
    param_blur = 0.f;
    param_gray = 0.f;
    param_noise = 0.f;
    param_duality_h = 0.f;
    param_duality_v = 0.f;
    param_noise_fps = 25.f;
    param_noise_scale = 1.f;

    im_noise_time = 1.0f / 100.0f;
    im_noise_shift_w = 0;
    im_noise_shift_h = 0;

    param_color_base = color_rgba(127, 127, 127, 0);
    param_color_gray = color_rgba(85, 85, 85, 0);
    param_color_add.set(0.0f, 0.0f, 0.0f);

    dwAccumulatorClearMark = 0;
    RImplementation.Resources->Evict();

    // Blenders
    b_accum_spot = xr_new<CBlender_accum_spot>();

#ifndef USE_DX9
    if (RImplementation.o.dx10_msaa)
    {
        int bound = RImplementation.o.dx10_msaa_samples;

        if (RImplementation.o.dx10_msaa_opt)
            bound = 1;

        for (int i = 0; i < bound; ++i)
        {
            static pcstr SampleDefs[] = { "0", "1", "2", "3", "4", "5", "6", "7" };
            b_combine_msaa[i] = xr_new<CBlender_combine_msaa>();
            b_accum_mask_msaa[i] = xr_new<CBlender_accum_direct_mask_msaa>();
            b_accum_direct_msaa[i] = xr_new<CBlender_accum_direct_msaa>();
            b_accum_direct_volumetric_msaa[i] = xr_new<CBlender_accum_direct_volumetric_msaa>();
            // b_accum_direct_volumetric_sun_msaa[i]	= new CBlender_accum_direct_volumetric_sun_msaa			();
            b_accum_spot_msaa[i] = xr_new<CBlender_accum_spot_msaa>();
            b_accum_volumetric_msaa[i] = xr_new<CBlender_accum_volumetric_msaa>();
            b_accum_point_msaa[i] = xr_new<CBlender_accum_point_msaa>();
            b_accum_reflected_msaa[i] = xr_new<CBlender_accum_reflected_msaa>();
            b_ssao_msaa[i] = xr_new<CBlender_SSAO_MSAA>();
            static_cast<CBlender_accum_direct_mask_msaa*>(b_accum_mask_msaa[i])->SetDefine("ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_accum_direct_volumetric_msaa*>(b_accum_direct_volumetric_msaa[i])
                ->SetDefine("ISAMPLE", SampleDefs[i]);
            // static_cast<CBlender_accum_direct_volumetric_sun_msaa*>(b_accum_direct_volumetric_sun_msaa[i])->SetDefine( "ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_accum_direct_msaa*>(b_accum_direct_msaa[i])->SetDefine("ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_accum_volumetric_msaa*>(b_accum_volumetric_msaa[i])
                ->SetDefine("ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_accum_spot_msaa*>(b_accum_spot_msaa[i])->SetDefine("ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_accum_point_msaa*>(b_accum_point_msaa[i])->SetDefine("ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_accum_reflected_msaa*>(b_accum_reflected_msaa[i])->SetDefine("ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_combine_msaa*>(b_combine_msaa[i])->SetDefine("ISAMPLE", SampleDefs[i]);
            static_cast<CBlender_SSAO_MSAA*>(b_ssao_msaa[i])->SetDefine("ISAMPLE", SampleDefs[i]);
        }
    }
#endif
    // NORMAL
    {
        u32 w = Device.dwWidth, h = Device.dwHeight;
        rt_Base.resize(HW.BackBufferCount);
        for (u32 i = 0; i < HW.BackBufferCount; i++)
        {
            string32 temp;
            xr_sprintf(temp, "%s%d", r2_RT_base, i);
            rt_Base[i].create(temp, w, h, HW.Caps.fTarget, 1, { CRT::CreateBase });
        }
        rt_Base_Depth.create(r2_RT_base_depth, w, h, HW.Caps.fDepth, 1, { CRT::CreateBase });

        if (!RImplementation.o.dx10_msaa)
            rt_MSAADepth = rt_Base_Depth;
        else
            rt_MSAADepth.create(r2_RT_MSAAdepth, w, h, D3DFMT_D24S8, SampleCount);

        rt_Position.create(r2_RT_P, w, h, D3DFMT_A16B16G16R16F, SampleCount);
        if (!RImplementation.o.dx10_gbuffer_opt)
            rt_Normal.create(r2_RT_N, w, h, D3DFMT_A16B16G16R16F, SampleCount);

        // select albedo & accum
        if (RImplementation.o.mrtmixdepth)
        {
            // NV50
            rt_Color.create(r2_RT_albedo, w, h, D3DFMT_A8R8G8B8, SampleCount);
            rt_Accumulator.create(r2_RT_accum, w, h, D3DFMT_A16B16G16R16F, SampleCount);
        }
        else
        {
            // can't - mix-depth
            if (RImplementation.o.fp16_blend)
            {
                // NV40
                if (!RImplementation.o.dx10_gbuffer_opt)
                {
                    rt_Color.create(r2_RT_albedo, w, h, D3DFMT_A16B16G16R16F, SampleCount); // expand to full
                    rt_Accumulator.create(r2_RT_accum, w, h, D3DFMT_A16B16G16R16F, SampleCount);
                }
                else
                {
                    rt_Color.create(r2_RT_albedo, w, h, D3DFMT_A8R8G8B8, SampleCount); // expand to full
                    rt_Accumulator.create(r2_RT_accum, w, h, D3DFMT_A16B16G16R16F, SampleCount);
                }
            }
            else
            {
                // R4xx, no-fp-blend,-> albedo_wo
                VERIFY(RImplementation.o.albedo_wo);
                rt_Color.create(r2_RT_albedo, w, h, D3DFMT_A8R8G8B8, SampleCount); // normal
                rt_Accumulator.create(r2_RT_accum, w, h, D3DFMT_A16B16G16R16F, SampleCount);
                rt_Accumulator_temp.create(r2_RT_accum_temp, w, h, D3DFMT_A16B16G16R16F, SampleCount);
            }
        }

        // generic(LDR) RTs
        rt_Generic_0.create(r2_RT_generic0, w, h, D3DFMT_A8R8G8B8, 1);
        rt_Generic_1.create(r2_RT_generic1, w, h, D3DFMT_A8R8G8B8, 1);
#ifndef USE_DX9
        rt_Generic.create(r2_RT_generic, w, h, D3DFMT_A8R8G8B8, 1);
#endif
        if (!RImplementation.o.dx10_msaa)
        {
            rt_Generic_0_r = rt_Generic_0;
            rt_Generic_1_r = rt_Generic_1;
        }
        else
        {
            rt_Generic_0_r.create(r2_RT_generic0_r, w, h, D3DFMT_A8R8G8B8, SampleCount);
            rt_Generic_1_r.create(r2_RT_generic1_r, w, h, D3DFMT_A8R8G8B8, SampleCount);
        }
        //	Igor: for volumetric lights
        // rt_Generic_2.create			(r2_RT_generic2,w,h,D3DFMT_A8R8G8B8		);
        //	temp: for higher quality blends
        if (RImplementation.o.advancedpp)
            rt_Generic_2.create(r2_RT_generic2, w, h, D3DFMT_A16B16G16R16F, SampleCount);
    }

    // OCCLUSION
    {
        CBlender_light_occq b_occq;
        s_occq.create(&b_occq, "r2" DELIMITER "occq");
    }

    // DIRECT (spot)
    pcstr smapTarget = r2_RT_smap_depth;
    {
        const u32 smapsize = RImplementation.o.smapsize;

        D3DFORMAT depth_format = D3DFMT_D24X8;
        D3DFORMAT surf_format = D3DFMT_R32F;

        Flags32 flags{};
        if (!RImplementation.o.HW_smap)
        {
            flags.flags = CRT::CreateSurface;
            smapTarget = r2_RT_smap_surf;
        }
        else
        {
            depth_format = (D3DFORMAT)RImplementation.o.HW_smap_FORMAT;
            if (RImplementation.o.nullrt) // use nullrt if possible
                surf_format = (D3DFORMAT)MAKEFOURCC('N', 'U', 'L', 'L');
            else
                surf_format = D3DFMT_R5G6B5;
        }

        // We only need to create rt_smap_surf on DX9, on DX10+ it's always a NULL render target
        // TODO: OGL: Don't create a color buffer for the shadow map.
#if defined(USE_DX9) || defined(USE_OGL)
        rt_smap_surf.create(r2_RT_smap_surf, smapsize, smapsize, surf_format);
#endif

        // Create D3DFMT_D24X8 depth-stencil surface if HW smap is not supported,
        // otherwise - create texture with specified HW_smap_FORMAT
        rt_smap_depth.create(r2_RT_smap_depth, smapsize, smapsize, depth_format, 1, flags);
#ifndef USE_DX9
        if (RImplementation.o.dx10_minmax_sm)
        {
            rt_smap_depth_minmax.create(r2_RT_smap_depth_minmax, smapsize / 4, smapsize / 4, D3DFMT_R32F);
            CBlender_createminmax TempBlender;
            s_create_minmax_sm.create(&TempBlender, "null");
        }
#endif
        // Accum mask
        {
            CBlender_accum_direct_mask b_accum_mask;
            s_accum_mask.create(&b_accum_mask, "r2" DELIMITER "accum_mask");
        }
        // Accum direct
        {
#if RENDER == R_R2
            if (RImplementation.o.oldshadowcascades)
            {
                CBlender_accum_direct b_accum_direct;
                s_accum_direct.create(&b_accum_direct, "r2" DELIMITER "accum_direct");
            }
            else
            {
                CBlender_accum_direct_cascade b_accum_direct;
                s_accum_direct.create(&b_accum_direct, "r2" DELIMITER "accum_direct_cascade");
            }
#else
            CBlender_accum_direct b_accum_direct;
            s_accum_direct.create(&b_accum_direct, "r2" DELIMITER "accum_direct");
#endif // RENDER == R_R2
        }
        // Accum direct/mask MSAA
#ifndef USE_DX9
        if (RImplementation.o.dx10_msaa)
        {
            int bound = RImplementation.o.dx10_msaa_samples;

            if (RImplementation.o.dx10_msaa_opt)
                bound = 1;

            for (int i = 0; i < bound; ++i)
            {
                s_accum_direct_msaa[i].create(b_accum_direct_msaa[i], "r2" DELIMITER "accum_direct");
                s_accum_mask_msaa[i].create(b_accum_mask_msaa[i], "r2" DELIMITER "accum_direct");
            }
        }
#endif // !USE_DX9
        // Accum volumetric
        if (RImplementation.o.advancedpp)
        {
#ifdef USE_DX9
            if (RImplementation.o.oldshadowcascades)
                s_accum_direct_volumetric.create("accum_volumetric_sun");
            else
                s_accum_direct_volumetric.create("accum_volumetric_sun_cascade");
#else
            s_accum_direct_volumetric.create("accum_volumetric_sun_nomsaa");
#endif
            manually_assign_texture(s_accum_direct_volumetric, "s_smap", smapTarget);

#ifndef USE_DX9
            if (RImplementation.o.dx10_minmax_sm)
            {
                s_accum_direct_volumetric_minmax.create("accum_volumetric_sun_nomsaa_minmax");
                manually_assign_texture(s_accum_direct_volumetric_minmax, "s_smap", smapTarget);
            }

            if (RImplementation.o.dx10_msaa)
            {
                static pcstr snames[] =
                {
                    "accum_volumetric_sun_msaa0", "accum_volumetric_sun_msaa1",
                    "accum_volumetric_sun_msaa2", "accum_volumetric_sun_msaa3",
                    "accum_volumetric_sun_msaa4", "accum_volumetric_sun_msaa5",
                    "accum_volumetric_sun_msaa6", "accum_volumetric_sun_msaa7"
                };
                int bound = RImplementation.o.dx10_msaa_samples;

                if (RImplementation.o.dx10_msaa_opt)
                    bound = 1;

                for (int i = 0; i < bound; ++i)
                {
                    // s_accum_direct_volumetric_msaa[i].create		(b_accum_direct_volumetric_sun_msaa[i],			"r2" DELIMITER "accum_direct");
                    s_accum_direct_volumetric_msaa[i].create(snames[i]);
                    manually_assign_texture(s_accum_direct_volumetric_msaa[i], "s_smap", smapTarget);
                }
            }
#endif // !USE_DX9
        }
    }

    // RAIN
    // TODO: DX10: Create resources only when DX10 rain is enabled.
    // Or make DX10 rain switch dynamic?
#ifndef USE_DX9
    {
        CBlender_rain TempBlender;
        s_rain.create(&TempBlender, "null");

        if (RImplementation.o.dx10_msaa)
        {
            static pcstr SampleDefs[] = { "0", "1", "2", "3", "4", "5", "6", "7" };
            CBlender_rain_msaa TempBlenderMSAA[8];

            int bound = RImplementation.o.dx10_msaa_samples;

            if (RImplementation.o.dx10_msaa_opt)
                bound = 1;

            for (int i = 0; i < bound; ++i)
            {
                TempBlenderMSAA[i].SetDefine("ISAMPLE", SampleDefs[i]);
                s_rain_msaa[i].create(&TempBlenderMSAA[i], "null");
                s_accum_spot_msaa[i].create(b_accum_spot_msaa[i], "r2" DELIMITER "accum_spot_s", "lights" DELIMITER "lights_spot01");
                s_accum_point_msaa[i].create(b_accum_point_msaa[i], "r2" DELIMITER "accum_point_s");
                // s_accum_volume_msaa[i].create(b_accum_direct_volumetric_msaa[i], "lights" DELIMITER "lights_spot01");
                s_accum_volume_msaa[i].create(b_accum_volumetric_msaa[i], "lights" DELIMITER "lights_spot01");
                s_combine_msaa[i].create(b_combine_msaa[i], "r2" DELIMITER "combine");
            }
        }
    }
#endif // !USE_DX9

#ifndef USE_DX9
    if (RImplementation.o.dx10_msaa)
    {
        CBlender_msaa TempBlender;
        s_mark_msaa_edges.create(&TempBlender, "null");
    }
#endif // !USE_DX9

    // POINT
    {
        CBlender_accum_point b_accum_point;
        s_accum_point.create(&b_accum_point, "r2" DELIMITER "accum_point_s");
        accum_point_geom_create();
        g_accum_point.create(D3DFVF_XYZ, g_accum_point_vb, g_accum_point_ib);
        accum_omnip_geom_create();
        g_accum_omnipart.create(D3DFVF_XYZ, g_accum_omnip_vb, g_accum_omnip_ib);
    }

    // SPOT
    {
        s_accum_spot.create(b_accum_spot, "r2" DELIMITER "accum_spot_s", "lights" DELIMITER "lights_spot01");
        accum_spot_geom_create();
        g_accum_spot.create(D3DFVF_XYZ, g_accum_spot_vb, g_accum_spot_ib);
    }

    // SPOT VOLUMETRIC
    if (RImplementation.o.advancedpp)
    {
        s_accum_volume.create("accum_volumetric", "lights" DELIMITER "lights_spot01");
        manually_assign_texture(s_accum_volume, "s_smap", smapTarget);
        accum_volumetric_geom_create();
        g_accum_volumetric.create(D3DFVF_XYZ, g_accum_volumetric_vb, g_accum_volumetric_ib);
    }

    // REFLECTED
    {
        CBlender_accum_reflected b_accum_reflected;
        s_accum_reflected.create(&b_accum_reflected, "r2" DELIMITER "accum_refl");
#ifndef USE_DX9
        if (RImplementation.o.dx10_msaa)
        {
            int bound = RImplementation.o.dx10_msaa_samples;

            if (RImplementation.o.dx10_msaa_opt)
                bound = 1;

            for (int i = 0; i < bound; ++i)
            {
                s_accum_reflected_msaa[i].create(b_accum_reflected_msaa[i], "null");
            }
        }
#endif // !USE_DX9
    }

    // BLOOM
    {
        D3DFORMAT fmt = D3DFMT_A8R8G8B8; // D3DFMT_X8R8G8B8;
        u32 w = BLOOM_size_X, h = BLOOM_size_Y;
        u32 fvf_build = D3DFVF_XYZRHW | D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) |
            D3DFVF_TEXCOORDSIZE2(2) | D3DFVF_TEXCOORDSIZE2(3);
        u32 fvf_filter = (u32)D3DFVF_XYZRHW | D3DFVF_TEX8 | D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEXCOORDSIZE4(1) |
            D3DFVF_TEXCOORDSIZE4(2) | D3DFVF_TEXCOORDSIZE4(3) | D3DFVF_TEXCOORDSIZE4(4) | D3DFVF_TEXCOORDSIZE4(5) |
            D3DFVF_TEXCOORDSIZE4(6) | D3DFVF_TEXCOORDSIZE4(7);
        rt_Bloom_1.create(r2_RT_bloom1, w, h, fmt);
        rt_Bloom_2.create(r2_RT_bloom2, w, h, fmt);
        g_bloom_build.create(fvf_build, RCache.Vertex.Buffer(), RCache.QuadIB);
        g_bloom_filter.create(fvf_filter, RCache.Vertex.Buffer(), RCache.QuadIB);
        s_bloom_dbg_1.create("effects" DELIMITER "screen_set", r2_RT_bloom1);
        s_bloom_dbg_2.create("effects" DELIMITER "screen_set", r2_RT_bloom2);

        CBlender_bloom_build b_bloom;
        s_bloom.create(&b_bloom, "r2" DELIMITER "bloom");
        if (!RImplementation.o.dx10_msaa)
            s_bloom_msaa = s_bloom;
        else
        {
#ifdef USE_DX9
            NODEFAULT;
#else
            CBlender_bloom_build_msaa b_bloom_msaa;
            s_bloom_msaa.create(&b_bloom_msaa, "r2" DELIMITER "bloom");
#endif
        }
        f_bloom_factor = 0.5f;
    }

    // HBAO
    if (RImplementation.o.ssao_opt_data)
    {
        u32 w = 0;
        u32 h = 0;
        if (RImplementation.o.ssao_half_data)
        {
            w = Device.dwWidth / 2;
            h = Device.dwHeight / 2;
        }
        else
        {
            w = Device.dwWidth;
            h = Device.dwHeight;
        }

        D3DFORMAT fmt = HW.Caps.id_vendor == 0x10DE ? D3DFMT_R32F : D3DFMT_R16F;
        rt_half_depth.create(r2_RT_half_depth, w, h, fmt);

        CBlender_SSAO_noMSAA b_ssao;
        s_ssao.create(&b_ssao, "r2" DELIMITER "ssao");
    }

    // HDAO/SSAO
    const bool ssao_blur_on = RImplementation.o.ssao_blur_on;
#ifdef USE_DX9
    constexpr bool ssao_hdao_ultra = false;
#else
    const bool ssao_hdao_ultra = RImplementation.o.ssao_hdao && RImplementation.o.ssao_ultra;
#endif
    if (ssao_blur_on || ssao_hdao_ultra)
    {
        const u32 w = Device.dwWidth, h = Device.dwHeight;

        if (ssao_hdao_ultra)
        {
#if !defined(USE_DX9) && !defined(USE_OGL) // XXX: support compute shaders for OpenGL
            CBlender_CS_HDAO b_hdao_cs;
            s_hdao_cs.create(&b_hdao_cs, "r2" DELIMITER "ssao");
            if (RImplementation.o.dx10_msaa)
            {
                CBlender_CS_HDAO_MSAA b_hdao_msaa_cs;
                s_hdao_cs_msaa.create(&b_hdao_msaa_cs, "r2" DELIMITER "ssao");
            }
            rt_ssao_temp.create(r2_RT_ssao_temp, w, h, D3DFMT_R16F, 1, { CRT::CreateUAV });
#endif
        }
        else if (ssao_blur_on)
        {
            CBlender_SSAO_noMSAA b_ssao;
            s_ssao.create(&b_ssao, "r2" DELIMITER "ssao");

            /* Should be used in r*_rendertarget_phase_ssao.cpp but it's commented there.
            if (RImplementation.o.dx10_msaa)
            {
                const int bound = RImplementation.o.dx10_msaa_opt ? 1 : RImplementation.o.dx10_msaa_samples;

                for (int i = 0; i < bound; ++i)
                    s_ssao_msaa[i].create(b_ssao_msaa[i], "null");
            }*/
            rt_ssao_temp.create(r2_RT_ssao_temp, w, h, D3DFMT_G16R16F, SampleCount);
        }
    }

    // TONEMAP
    {
        rt_LUM_64.create(r2_RT_luminance_t64, 64, 64, D3DFMT_A16B16G16R16F);
        rt_LUM_8.create(r2_RT_luminance_t8, 8, 8, D3DFMT_A16B16G16R16F);

        CBlender_luminance b_luminance;
        s_luminance.create(&b_luminance, "r2" DELIMITER "luminance");
        f_luminance_adapt = 0.5f;

        t_LUM_src.create(r2_RT_luminance_src);
        t_LUM_dest.create(r2_RT_luminance_cur);

        // create pool
        for (u32 it = 0; it < HW.Caps.iGPUNum * 2; it++)
        {
            string256 name;
            xr_sprintf(name, "%s_%d", r2_RT_luminance_pool, it);
            rt_LUM_pool[it].create(name, 1, 1, D3DFMT_R32F);
#ifdef USE_DX9
            u_setrt(rt_LUM_pool[it], 0, 0, 0);
#endif
            RCache.ClearRT(rt_LUM_pool[it], 0x7f7f7f7f);
        }
        u_setrt(Device.dwWidth, Device.dwHeight, get_base_rt(), 0, 0, get_base_zb());
    }

    // COMBINE
    {
#ifdef USE_DX9
        static D3DVERTEXELEMENT9 dwDecl[] =
        {
            { 0, 0,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, // pos+uv
            { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
            { 0, 20, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };
#else
        static D3DVERTEXELEMENT9 dwDecl[] =
        {
            { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, // pos+uv
            D3DDECL_END()
        };
#endif
        CBlender_combine b_combine;
        s_combine.create(&b_combine, "r2" DELIMITER "combine");
        s_combine_volumetric.create("combine_volumetric");
        s_combine_dbg_0.create("effects" DELIMITER "screen_set", r2_RT_smap_surf);
        s_combine_dbg_1.create("effects" DELIMITER "screen_set", r2_RT_luminance_t8);
        s_combine_dbg_Accumulator.create("effects" DELIMITER "screen_set", r2_RT_accum);
        g_combine_VP.create(dwDecl, RCache.Vertex.Buffer(), RCache.QuadIB);
        g_combine.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
        g_combine_2UV.create(FVF::F_TL2uv, RCache.Vertex.Buffer(), RCache.QuadIB);
#ifdef USE_DX9
        g_combine_cuboid.create(FVF::F_L, RCache.Vertex.Buffer(), RCache.Index.Buffer());
#else
        g_combine_cuboid.create(dwDecl, RCache.Vertex.Buffer(), RCache.Index.Buffer());
#endif
        u32 fvf_aa_blur = D3DFVF_XYZRHW | D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) |
            D3DFVF_TEXCOORDSIZE2(2) | D3DFVF_TEXCOORDSIZE2(3);
        g_aa_blur.create(fvf_aa_blur, RCache.Vertex.Buffer(), RCache.QuadIB);

        u32 fvf_aa_AA = D3DFVF_XYZRHW | D3DFVF_TEX7 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) |
            D3DFVF_TEXCOORDSIZE2(2) | D3DFVF_TEXCOORDSIZE2(3) | D3DFVF_TEXCOORDSIZE2(4) | D3DFVF_TEXCOORDSIZE4(5) |
            D3DFVF_TEXCOORDSIZE4(6);
        g_aa_AA.create(fvf_aa_AA, RCache.Vertex.Buffer(), RCache.QuadIB);

        t_envmap_0.create(r2_T_envs0);
        t_envmap_1.create(r2_T_envs1);
    }

    // Build textures
    build_textures();

    // PP
    s_postprocess.create("postprocess");
    g_postprocess.create(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX3,
        RCache.Vertex.Buffer(), RCache.QuadIB);
    if (!RImplementation.o.dx10_msaa)
        s_postprocess_msaa = s_postprocess;
    else
    {
#ifdef USE_DX9
        NODEFAULT;
#else
        CBlender_postprocess_msaa b_postprocess_msaa;
        s_postprocess_msaa.create(&b_postprocess_msaa, "r2" DELIMITER "post");
#endif
    }

    // Menu
    s_menu.create("distort");
    g_menu.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);

#if 0 // OpenGL: kept for historical reasons
    // Flip
    t_base = RImplementation.Resources->_CreateTexture(r2_base);
    t_base->surface_set(GL_TEXTURE_2D, get_base_rt());
    s_flip.create("effects" DELIMITER "screen_set", r2_base);
    g_flip.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
#endif

    //
    dwWidth = Device.dwWidth;
    dwHeight = Device.dwHeight;
}

CRenderTarget::~CRenderTarget()
{
#ifdef USE_OGL
    glDeleteTextures(1, &t_ss_async);

    // Textures
    t_material->surface_set(GL_TEXTURE_3D, 0);
    glDeleteTextures(1, &t_material_surf);
    t_material.destroy();

    t_LUM_src->surface_set(GL_TEXTURE_2D, 0);
    t_LUM_dest->surface_set(GL_TEXTURE_2D, 0);
    t_LUM_src.destroy();
    t_LUM_dest.destroy();

    t_envmap_0->surface_set(GL_TEXTURE_CUBE_MAP, 0);
    t_envmap_1->surface_set(GL_TEXTURE_CUBE_MAP, 0);
    t_envmap_0.destroy();
    t_envmap_1.destroy();

    // Jitter
    for (int it = 0; it < TEX_jitter_count; it++)
    {
        t_noise[it]->surface_set(GL_TEXTURE_2D, 0);
    }
    glDeleteTextures(TEX_jitter_count, t_noise_surf);

    t_noise_mipped->surface_set(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &t_noise_surf_mipped);
#else
#   ifndef USE_DX9
    _RELEASE(t_ss_async);
#   endif
    // Textures
    t_material->surface_set(NULL);

#ifdef DEBUG
    _SHOW_REF("t_material_surf", t_material_surf);
#endif // DEBUG
    _RELEASE(t_material_surf);

    t_LUM_src->surface_set(NULL);
    t_LUM_dest->surface_set(NULL);

#   ifdef DEBUG
    ID3DBaseTexture* pSurf = 0;

    pSurf = t_envmap_0->surface_get();
    if (pSurf)
        pSurf->Release();
    _SHOW_REF("t_envmap_0 - #small", pSurf);

    pSurf = t_envmap_1->surface_get();
    if (pSurf)
        pSurf->Release();
    _SHOW_REF("t_envmap_1 - #small", pSurf);
#   endif // DEBUG
    t_envmap_0->surface_set(NULL);
    t_envmap_1->surface_set(NULL);
    t_envmap_0.destroy();
    t_envmap_1.destroy();

    // Jitter
    for (int it = 0; it < TEX_jitter_count; it++)
    {
        t_noise[it]->surface_set(NULL);
#   ifdef DEBUG
        _SHOW_REF("t_noise_surf[it]", t_noise_surf[it]);
#   endif // DEBUG
        _RELEASE(t_noise_surf[it]);
    }

#   ifndef USE_DX9
    t_noise_mipped->surface_set(NULL);
#       ifdef DEBUG
    _SHOW_REF("t_noise_surf_mipped", t_noise_surf_mipped);
#       endif // DEBUG
    _RELEASE(t_noise_surf_mipped);
#   endif
#endif
    //
    accum_spot_geom_destroy();
    accum_omnip_geom_destroy();
    accum_point_geom_destroy();
    accum_volumetric_geom_destroy();

    // Blenders
    xr_delete(b_accum_spot);

#ifndef USE_DX9
    if (RImplementation.o.dx10_msaa)
    {
        int bound = RImplementation.o.dx10_msaa_samples;

        if (RImplementation.o.dx10_msaa_opt)
            bound = 1;

        for (int i = 0; i < bound; ++i)
        {
            xr_delete(b_combine_msaa[i]);
            xr_delete(b_accum_direct_msaa[i]);
            xr_delete(b_accum_mask_msaa[i]);
            xr_delete(b_accum_direct_volumetric_msaa[i]);
            // xr_delete					(b_accum_direct_volumetric_sun_msaa[i]);
            xr_delete(b_accum_spot_msaa[i]);
            xr_delete(b_accum_volumetric_msaa[i]);
            xr_delete(b_accum_point_msaa[i]);
            xr_delete(b_accum_reflected_msaa[i]);
            xr_delete(b_ssao_msaa[i]);
        }
    }
#endif
}

void CRenderTarget::reset_light_marker(bool bResetStencil)
{
    dwLightMarkerID = 5;
    if (bResetStencil)
    {
#ifdef USE_DX9
        RCache.set_ColorWriteEnable(FALSE);
#endif
        u32 Offset;
        float _w = float(Device.dwWidth);
        float _h = float(Device.dwHeight);
        u32 C = color_rgba(255, 255, 255, 255);
#ifdef USE_DX9
        float eps = EPS_S;
        FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
        pv->set(eps, float(_h + eps), eps, 1.f, C, 0, 0);
        pv++;
        pv->set(eps, eps, eps, 1.f, C, 0, 0);
        pv++;
        pv->set(float(_w + eps), float(_h + eps), eps, 1.f, C, 0, 0);
        pv++;
        pv->set(float(_w + eps), eps, eps, 1.f, C, 0, 0);
        pv++;
        RCache.Vertex.Unlock(4, g_combine->vb_stride);
        RCache.set_CullMode(CULL_NONE);
        //  Clear everything except last bit
        RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, dwLightMarkerID, 0x00, 0xFE,
            D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO);
        RCache.set_Element(s_occq->E[1]);
#else
        float eps = 0;
        float _dw = 0.5f;
        float _dh = 0.5f;
        FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
        pv->set(-_dw, _h - _dh, eps, 1.f, C, 0, 0);
        pv++;
        pv->set(-_dw, -_dh, eps, 1.f, C, 0, 0);
        pv++;
        pv->set(_w - _dw, _h - _dh, eps, 1.f, C, 0, 0);
        pv++;
        pv->set(_w - _dw, -_dh, eps, 1.f, C, 0, 0);
        pv++;
        RCache.Vertex.Unlock(4, g_combine->vb_stride);
        RCache.set_Element(s_occq->E[2]);
#endif
        RCache.set_Geometry(g_combine);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }
}

void CRenderTarget::increment_light_marker()
{
    dwLightMarkerID += 2;

    const u32 iMaxMarkerValue = RImplementation.o.dx10_msaa ? 127 : 255;

    if (dwLightMarkerID > iMaxMarkerValue)
        reset_light_marker(true);
}

bool CRenderTarget::need_to_render_sunshafts()
{
    if (!(RImplementation.o.advancedpp && ps_r_sun_shafts))
        return false;

    {
        CEnvDescriptor& E = *g_pGamePersistent->Environment().CurrentEnv;
        float fValue = E.m_fSunShaftsIntensity;
        // TODO: add multiplication by sun color here
        if (fValue < 0.0001)
            return false;
    }

    return true;
}

#ifndef USE_DX9
bool CRenderTarget::use_minmax_sm_this_frame()
{
    switch (RImplementation.o.dx10_minmax_sm)
    {
    case CRender::MMSM_ON: return true;
    case CRender::MMSM_AUTO: return need_to_render_sunshafts();
    case CRender::MMSM_AUTODETECT:
    {
        const auto& [width, height] = HW.GetSurfaceSize();
        u32 dwScreenArea = width * height;

        if (dwScreenArea >= RImplementation.o.dx10_minmax_sm_screenarea_threshold)
            return need_to_render_sunshafts();
        return false;
    }

    default: return false;
    }
}
#endif
