#pragma once

#include "Layers/xrRender/ColorMapManager.h"

class light;

//#define DU_SPHERE_NUMVERTEX 92
//#define DU_SPHERE_NUMFACES	180
//#define DU_CONE_NUMVERTEX	18
//#define DU_CONE_NUMFACES	32
//	no less than 2
#define VOLUMETRIC_SLICES 100

class CRenderTarget : public IRender_Target
{
private:
    u32 dwWidth;
    u32 dwHeight;
    u32 dwAccumulatorClearMark;

public:
    u32 dwLightMarkerID;
    //
    IBlender* b_accum_spot;

#ifdef DEBUG
    struct dbg_line_t
    {
        Fvector P0, P1;
        u32 color;
    };
    xr_vector<std::pair<Fsphere, Fcolor>> dbg_spheres;
    xr_vector<dbg_line_t> dbg_lines;
    xr_vector<Fplane> dbg_planes;
#endif

    // Base targets
    xr_vector<ref_rt> rt_Base;
    ref_rt rt_Base_Depth;

    // MRT-path
    ref_rt rt_Depth; // Z-buffer like - initial depth
    ref_rt rt_MSAADepth; // z-buffer for MSAA deferred shading. If MSAA is disabled, points to rt_Base_Depth so we can reduce branching
    ref_rt rt_Generic_0_r; // MRT generic 0, if MSAA is disabled, just an alias of rt_Generic_0
    ref_rt rt_Generic_1_r; // MRT generic 1, if MSAA is disabled, just an alias of rt_Generic_1
    ref_rt rt_Position; // 64bit,	fat	(x,y,z,?)				(eye-space)
    ref_rt rt_Normal; // 64bit,	fat	(x,y,z,hemi)			(eye-space)
    ref_rt rt_Color; // 64/32bit,fat	(r,g,b,specular-gloss)	(or decompressed MET-8-8-8-8)

    //
    ref_rt rt_Accumulator; // 64bit		(r,g,b,specular)
    ref_rt rt_Accumulator_temp; // only for HW which doesn't feature fp16 blend
    ref_rt rt_Generic_0; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
    ref_rt rt_Generic_1; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
    //	Igor: for volumetric lights
    ref_rt rt_Generic_2; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
    ref_rt rt_Bloom_1; // 32bit, dim/4	(r,g,b,?)
    ref_rt rt_Bloom_2; // 32bit, dim/4	(r,g,b,?)
    ref_rt rt_LUM_64; // 64bit, 64x64,	log-average in all components
    ref_rt rt_LUM_8; // 64bit, 8x8,		log-average in all components

    //	Igor: for async screenshots
    ref_rt rt_async_ss; // 32bit		(r,g,b,a) is situated in the system memory

    ref_rt rt_LUM_pool[CHWCaps::MAX_GPUS * 2]; // 1xfp32,1x1,		exp-result -> scaler
    ref_texture t_LUM_src; // source
    ref_texture t_LUM_dest; // destination & usage for current frame

    // env
    ref_texture t_envmap_0; // env-0
    ref_texture t_envmap_1; // env-1

    // smap
    ref_rt rt_smap_surf; // 32bit,		color
    ref_rt rt_smap_depth; // 24(32) bit,	depth

    // Textures
    IDirect3DVolumeTexture9* t_material_surf;
    ref_texture t_material;

    IDirect3DTexture9* t_noise_surf[TEX_jitter_count];
    ref_texture t_noise[TEX_jitter_count];

private:
    // OCCq
    ref_shader s_occq;

    // Accum
    ref_shader s_accum_mask;
    ref_shader s_accum_direct;
    ref_shader s_accum_direct_volumetric;
    ref_shader s_accum_point;
    ref_shader s_accum_spot;
    ref_shader s_accum_reflected;
    ref_shader s_accum_volume;

    ref_geom g_accum_point;
    ref_geom g_accum_spot;
    ref_geom g_accum_omnipart;
    ref_geom g_accum_volumetric;

    VertexStagingBuffer g_accum_point_vb;
    IndexStagingBuffer g_accum_point_ib;

    VertexStagingBuffer g_accum_omnip_vb;
    IndexStagingBuffer g_accum_omnip_ib;

    VertexStagingBuffer g_accum_spot_vb;
    IndexStagingBuffer g_accum_spot_ib;

    VertexStagingBuffer g_accum_volumetric_vb;
    IndexStagingBuffer g_accum_volumetric_ib;

    // SSAO
    ref_shader s_ssao;
    ref_rt rt_ssao_temp;
    ref_rt rt_half_depth;

    // Bloom
    ref_geom g_bloom_build;
    ref_geom g_bloom_filter;
    ref_shader s_bloom_dbg_1;
    ref_shader s_bloom_dbg_2;
    ref_shader s_bloom;
    ref_shader s_bloom_msaa; // if MSAA is disabled, just an alias of s_bloom
    float f_bloom_factor;

    // Luminance
    ref_shader s_luminance;
    float f_luminance_adapt;

    // Combine
    ref_geom g_combine;
    ref_geom g_combine_VP; // xy=p,zw=tc
    ref_geom g_combine_2UV;
    ref_geom g_combine_cuboid;
    ref_geom g_aa_blur;
    ref_geom g_aa_AA;
    ref_shader s_combine_dbg_0;
    ref_shader s_combine_dbg_1;
    ref_shader s_combine_dbg_Accumulator;
    ref_shader s_combine;
    ref_shader s_combine_volumetric;

public:
    ref_shader s_postprocess;
    ref_shader s_postprocess_msaa; // if MSAA is disabled, just an alias of s_bloom
    ref_geom g_postprocess;
    ref_shader s_menu;
    ref_geom g_menu;

private:
    float im_noise_time;
    u32 im_noise_shift_w;
    u32 im_noise_shift_h;

    float param_blur;
    float param_gray;
    float param_duality_h;
    float param_duality_v;
    float param_noise;
    float param_noise_scale;
    float param_noise_fps;
    u32 param_color_base;
    u32 param_color_gray;
    Fvector param_color_add;

    //	Color mapping
    float param_color_map_influence;
    float param_color_map_interpolate;
    ColorMapManager color_map_manager;

    //	Igor: used for volumetric lights
    bool m_bHasActiveVolumetric;

public:
    CRenderTarget();
    ~CRenderTarget();

    void build_textures();

    void accum_point_geom_create();
    void accum_point_geom_destroy();
    void accum_omnip_geom_create();
    void accum_omnip_geom_destroy();
    void accum_spot_geom_create();
    void accum_spot_geom_destroy();
    //	Igor: used for volumetric lights
    void accum_volumetric_geom_create();
    void accum_volumetric_geom_destroy();

    ID3DRenderTargetView* get_base_rt() { return rt_Base[HW.CurrentBackBuffer]->pRT; }
    ID3DDepthStencilView* get_base_zb() { return rt_Base_Depth->pRT; }

    void u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, IDirect3DSurface9* zb);
    void u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _zb)
    {
        u_setrt(_1, _2, _3, _zb ? _zb->pRT : nullptr);
    }
    void u_setrt(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3,
        IDirect3DSurface9* zb);

    void u_stencil_optimize(BOOL common_stencil = TRUE);
    void u_compute_texgen_screen(Fmatrix& dest);
    void u_compute_texgen_screen_asd(Fmatrix& dest);
    void u_compute_texgen_jitter(Fmatrix& dest);
    void u_calc_tc_noise(Fvector2& p0, Fvector2& p1);
    void u_calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);
    bool u_need_PP();
    bool u_need_CM();
    bool u_DBT_enable(float zMin, float zMax);
    void u_DBT_disable();

    void phase_ssao();
    void phase_downsamp();
    void phase_scene_prepare();
    void phase_scene_begin();
    void phase_scene_end();
    void phase_occq();
    void phase_wallmarks();
    void phase_smap_direct(light* L, u32 sub_phase);
    void phase_smap_direct_tsh(light* L, u32 sub_phase);
    void phase_smap_spot_clear();
    void phase_smap_spot(light* L);
    void phase_smap_spot_tsh(light* L);
    void phase_accumulator();
    void phase_vol_accumulator();
    void shadow_direct(light* L, u32 dls_phase);

    //	Generates min/max sm
    void create_minmax_SM() {}
    bool need_to_render_sunshafts();
    bool use_minmax_sm_this_frame() { return false; }

    bool enable_scissor(light* L); // true if intersects near plane
    void enable_dbt_bounds(light* L);

    void disable_aniso();

    void draw_volume(light* L);
    void accum_direct(u32 sub_phase);
    void accum_direct_cascade(u32 sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias);
    void accum_direct_f(u32 sub_phase);
    void accum_direct_lum();
    void accum_direct_blend();
    void accum_direct_volumetric(u32 sub_phase, const u32 Offset, const Fmatrix& mShadow);
    void accum_point(light* L);
    void accum_spot(light* L);
    void accum_reflected(light* L);
    //	Igor: for volumetric lights
    void accum_volumetric(light* L);
    void phase_bloom();
    void phase_luminance();
    void phase_combine();
    void phase_combine_volumetric();
    void phase_pp();

    virtual void set_blur(float f) { param_blur = f; }
    virtual void set_gray(float f) { param_gray = f; }
    virtual void set_duality_h(float f) { param_duality_h = _abs(f); }
    virtual void set_duality_v(float f) { param_duality_v = _abs(f); }
    virtual void set_noise(float f) { param_noise = f; }
    virtual void set_noise_scale(float f) { param_noise_scale = f; }
    virtual void set_noise_fps(float f) { param_noise_fps = _abs(f) + EPS_S; }
    virtual void set_color_base(u32 f) { param_color_base = f; }
    virtual void set_color_gray(u32 f) { param_color_gray = f; }
    virtual void set_color_add(const Fvector& f) { param_color_add = f; }
    virtual u32 get_width() { return dwWidth; }
    virtual u32 get_height() { return dwHeight; }
    virtual void set_cm_imfluence(float f) { param_color_map_influence = f; }
    virtual void set_cm_interpolate(float f) { param_color_map_interpolate = f; }
    virtual void set_cm_textures(const shared_str& tex0, const shared_str& tex1)
    {
        color_map_manager.SetTextures(tex0, tex1);
    }

    //	Need to reset stencil only when marker overflows.
    //	Don't clear when render for the first time
    void reset_light_marker(bool bResetStencil = false);
    void increment_light_marker();

    void DoAsyncScreenshot();

#ifdef DEBUG
    IC void dbg_addline(Fvector& P0, Fvector& P1, u32 c)
    {
        dbg_lines.push_back(dbg_line_t());
        dbg_lines.back().P0 = P0;
        dbg_lines.back().P1 = P1;
        dbg_lines.back().color = c;
    }
    IC void dbg_addbox(const Fbox& box, const u32& color)
    {
        Fvector c, r;
        box.getcenter(c);
        box.getradius(r);
        dbg_addbox(c, r.x, r.y, r.z, color);
    }
    IC void dbg_addbox(const Fvector& c, float rx, float ry, float rz, u32 color)
    {
        Fvector p1, p2, p3, p4, p5, p6, p7, p8;

        p1.set(c.x + rx, c.y + ry, c.z + rz);
        p2.set(c.x + rx, c.y - ry, c.z + rz);
        p3.set(c.x - rx, c.y - ry, c.z + rz);
        p4.set(c.x - rx, c.y + ry, c.z + rz);

        p5.set(c.x + rx, c.y + ry, c.z - rz);
        p6.set(c.x + rx, c.y - ry, c.z - rz);
        p7.set(c.x - rx, c.y - ry, c.z - rz);
        p8.set(c.x - rx, c.y + ry, c.z - rz);

        dbg_addline(p1, p2, color);
        dbg_addline(p2, p3, color);
        dbg_addline(p3, p4, color);
        dbg_addline(p4, p1, color);

        dbg_addline(p5, p6, color);
        dbg_addline(p6, p7, color);
        dbg_addline(p7, p8, color);
        dbg_addline(p8, p5, color);

        dbg_addline(p1, p5, color);
        dbg_addline(p2, p6, color);
        dbg_addline(p3, p7, color);
        dbg_addline(p4, p8, color);
    }
    IC void dbg_addplane(Fplane& P0, u32 /*c*/) { dbg_planes.push_back(P0); }
#else
    IC void dbg_addline(Fvector& P0, Fvector& P1, u32 c) {}
    IC void dbg_addplane(Fplane& P0, u32 c) {}
#endif
};
