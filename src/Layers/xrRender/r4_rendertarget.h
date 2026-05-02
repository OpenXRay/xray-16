#pragma once

#include "Layers/xrRender/ColorMapManager.h"
#include "Layers/xrRender/r2_types.h"

namespace xray::render::fg
{
class light;

#define VOLUMETRIC_SLICES 100

class CRenderTarget
{
    u32 dwWidth[R__NUM_CONTEXTS];
    u32 dwHeight[R__NUM_CONTEXTS];
    u32 dwAccumulatorClearMark;

public:
    enum eStencilOptimizeMode
    {
        SO_Light = 0,
        SO_Combine,
    };

    IBlender* b_accum_spot{};
    IBlender* b_accum_spot_msaa[8]{};
    IBlender* b_accum_volumetric_msaa[8]{};

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

    xr_vector<ref_rt> rt_Base;
    ref_rt rt_Base_Depth;

    ref_rt rt_Depth;
    ref_rt rt_MSAADepth;
    ref_rt rt_Generic_0_r;
    ref_rt rt_Generic_1_r;
    ref_rt rt_Generic;
    ref_rt rt_Position;
    ref_rt rt_Normal;
    ref_rt rt_Color;

    ref_rt rt_Accumulator;
    ref_rt rt_Accumulator_temp;
    ref_rt rt_Generic_0;
    ref_rt rt_Generic_1;
    ref_rt rt_Generic_2;
    ref_rt rt_Bloom_1;
    ref_rt rt_Bloom_2;
    ref_rt rt_LUM_64;
    ref_rt rt_LUM_8;

    ref_rt rt_LUM_pool[CHWCaps::MAX_GPUS * 2];
    ref_texture t_LUM_src;
    ref_texture t_LUM_dest;

    ref_rt rt_smap_surf;
    ref_rt rt_smap_depth;
    ref_rt rt_smap_rain;
    ref_rt rt_smap_depth_minmax;

private:
    ref_shader s_occq;

    ref_shader s_accum_mask;
    ref_shader s_accum_mask_msaa[8];
    ref_shader s_accum_direct;
    ref_shader s_accum_direct_msaa[8];
    ref_shader s_accum_direct_volumetric;
    ref_shader s_accum_direct_volumetric_msaa[8];
    ref_shader s_accum_direct_volumetric_minmax;
    ref_shader s_accum_point;
    ref_shader s_accum_point_msaa[8];
    ref_shader s_accum_spot;
    ref_shader s_accum_spot_msaa[8];
    ref_shader s_accum_reflected;
    ref_shader s_accum_reflected_msaa[8];
    ref_shader s_accum_volume;
    ref_shader s_accum_volume_msaa[8];

    ref_shader s_create_minmax_sm;

    ref_shader s_rain;
    ref_shader s_rain_msaa[8];

    ref_shader s_mark_msaa_edges;

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

    ref_rt rt_ssao_temp;
    ref_rt rt_half_depth;
    ref_shader s_ssao;
    ref_shader s_ssao_msaa[8];
    ref_shader s_hdao_cs;

    ref_geom g_bloom_build;
    ref_geom g_bloom_filter;
    ref_shader s_bloom_dbg_1;
    ref_shader s_bloom_dbg_2;
    ref_shader s_bloom;
    ref_shader s_bloom_msaa;
    float f_bloom_factor;

    ref_shader s_luminance;
    float f_luminance_adapt;

    ref_geom g_combine;
    ref_geom g_combine_VP;
    ref_geom g_combine_2UV;
    ref_geom g_combine_cuboid;
    ref_geom g_aa_blur;
    ref_geom g_aa_AA;
    ref_shader s_combine_dbg_0;
    ref_shader s_combine_dbg_1;
    ref_shader s_combine_dbg_Accumulator;
    ref_shader s_combine;
    ref_shader s_combine_msaa[8];
    ref_shader s_combine_volumetric;

public:
    ref_shader s_postprocess;
    ref_shader s_postprocess_msaa;
    ref_geom g_postprocess;
    ref_shader s_menu;
    ref_geom g_menu;
#if 0
    ref_shader s_flip;
    ref_geom g_flip;
#endif
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

    float param_color_map_influence;
    float param_color_map_interpolate;
    ColorMapManager color_map_manager;

    bool m_bHasActiveVolumetric;

public:
    CRenderTarget();
    ~CRenderTarget();

    void accum_point_geom_create();
    void accum_point_geom_destroy();
    void accum_omnip_geom_create();
    void accum_omnip_geom_destroy();
    void accum_spot_geom_create();
    void accum_spot_geom_destroy();
    void accum_volumetric_geom_create();
    void accum_volumetric_geom_destroy();

    ID3DRenderTargetView* get_base_rt() { return rt_Base[HW.CurrentBackBuffer]->pRT; }
    ID3DDepthStencilView* get_base_zb() { return rt_Base_Depth->pZRT[CHW::IMM_CTX_ID]; }

    void u_calc_tc_noise(Fvector2& p0, Fvector2& p1);
    void u_calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);
    bool u_need_PP();
    bool u_need_CM();
    bool u_DBT_enable(float zMin, float zMax);
    void u_DBT_disable();

    void phase_scene_prepare();
    void phase_scene_begin();
    void phase_scene_end();
    void phase_occq();
    void phase_ssao();
    void phase_hdao();
    void phase_downsamp();
    void phase_wallmarks();

    void mark_msaa_edges();

    bool need_to_render_sunshafts();
    bool use_minmax_sm_this_frame();

    bool enable_scissor(light* L);
    void enable_dbt_bounds(light* L);

    void disable_aniso();

    void phase_bloom();
    void phase_luminance();
    void phase_combine();
    void phase_combine_volumetric();
    void phase_pp();
#if 0
    void phase_flip();
#endif

    void set_blur(float f) { param_blur = f; }
    void set_gray(float f) { param_gray = f; }
    void set_duality_h(float f) { param_duality_h = _abs(f); }
    void set_duality_v(float f) { param_duality_v = _abs(f); }
    void set_noise(float f) { param_noise = f; }
    void set_noise_scale(float f) { param_noise_scale = f; }
    void set_noise_fps(float f) { param_noise_fps = _abs(f) + EPS_S; }
    void set_color_base(u32 f) { param_color_base = f; }
    void set_color_gray(u32 f) { param_color_gray = f; }
    void set_color_add(const Fvector& f) { param_color_add = f; }
    void set_cm_imfluence(float f) { param_color_map_influence = f; }
    void set_cm_interpolate(float f) { param_color_map_interpolate = f; }
    void set_cm_textures(const shared_str& tex0, const shared_str& tex1)
    {
        color_map_manager.SetTextures(tex0, tex1);
    }

#ifdef DEBUG
    void dbg_addline(const Fvector& P0, const Fvector& P1, u32 c)
    {
        dbg_lines.emplace_back(dbg_line_t{ P0, P1, c });
    }

    void dbg_addbox(const Fbox& box, const u32& color)
    {
        Fvector c, r;
        box.getcenter(c);
        box.getradius(r);
        dbg_addbox(c, r.x, r.y, r.z, color);
    }

    void dbg_addbox(const Fvector& c, float rx, float ry, float rz, u32 color)
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
    void dbg_addplane(Fplane& P0, u32 ) { dbg_planes.emplace_back(P0); }
#else
    void dbg_addline(Fvector& , Fvector& , u32 ) {}
    void dbg_addplane(Fplane& , u32 ) {}
#endif
};
}
