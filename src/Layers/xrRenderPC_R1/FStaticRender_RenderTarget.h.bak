#pragma once

#include "Layers/xrRender/ColorMapManager.h"

class CRenderTarget : public IRender_Target
{
private:
    bool bAvailable;
    u32 rtWidth;
    u32 rtHeight;

    u32 curWidth;
    u32 curHeight;

public:
    // Base targets
    xr_vector<ref_rt> rt_Base;
    ref_rt rt_Base_Depth;

private:
    ref_rt rt_Generic;
    ref_rt rt_Depth;

    ref_rt rt_color_map;
    ref_rt rt_distort;

    // FFP postprocessing
    ref_shader s_set;
    ref_shader s_gray;
    ref_shader s_blend;
    ref_shader s_duality;
    ref_shader s_noise;

    // Can't implement in a single pass of a shader since
    // should be compiled only for the hardware that supports it.
    ref_shader s_postprocess[2]{};   // Igor: 0 - plain, 1 - colormapped
    ref_shader s_postprocess_D[2]{}; // Igor: 0 - plain, 1 - colormapped
    ref_geom g_postprocess[2]{};

    float im_noise_time;
    u32 im_noise_shift_w{};
    u32 im_noise_shift_h{};

    float param_blur{};
    float param_gray{};
    float param_duality_h{};
    float param_duality_v{};
    float param_noise{};
    float param_noise_scale;
    float param_noise_fps;

    //	Color mapping
    float param_color_map_influence{};
    float param_color_map_interpolate{};
    ColorMapManager color_map_manager;

    u32 param_color_base;
    u32 param_color_gray;
    Fvector param_color_add{};

    u32 frame_distort;

public:
    ref_rt rt_temp_zb;

    //	Igor: for async screenshots
    ref_rt rt_async_ss; // 32bit		(r,g,b,a) is situated in the system memory

private:
    [[nodiscard]]
    bool NeedColorMapping() const;

    [[nodiscard]]
    bool NeedPostProcess() const;

    [[nodiscard]]
    bool Available() const { return bAvailable; }

    void calc_tc_noise(Fvector2& p0, Fvector2& p1);
    void calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);

public:
    CRenderTarget();

    void Begin();
    void End();

    void DoAsyncScreenshot() const;
    [[nodiscard]]
    bool Perform() const;

    [[nodiscard]]
    ID3DRenderTargetView* get_base_rt() const { return rt_Base[HW.CurrentBackBuffer]->pRT; }
    [[nodiscard]]
    ID3DDepthStencilView* get_base_zb() const { return rt_Base_Depth->pRT; }

    void set_blur(float f) override { param_blur = f; }
    void set_gray(float f) override { param_gray = f; }
    void set_duality_h(float f) override { param_duality_h = _abs(f); }
    void set_duality_v(float f) override { param_duality_v = _abs(f); }
    void set_noise(float f) override { param_noise = f; }
    void set_noise_scale(float f) override { param_noise_scale = f; }
    void set_noise_fps(float f) override { param_noise_fps = _abs(f) + EPS_S; }
    void set_color_base(u32 f) override { param_color_base = f; }
    void set_color_gray(u32 f) override { param_color_gray = f; }
    void set_color_add(const Fvector& f) override { param_color_add = f; }
    void set_cm_imfluence(float f) override { param_color_map_influence = f; }
    void set_cm_interpolate(float f) override { param_color_map_interpolate = f; }

    void set_cm_textures(const shared_str& tex0, const shared_str& tex1) override
    {
        color_map_manager.SetTextures(tex0, tex1);
    }

    u32 get_width(CBackend& cmd_list) override { return curWidth; }
    u32 get_height(CBackend& cmd_list) override { return curHeight; }
    u32 get_rtwidth() const { return rtWidth; }
    u32 get_rtheight() const { return rtHeight; }

    void phase_distortion();
    void phase_combine(bool bDistort, bool bCMap);

private:
    void phase_combine_fpp(u32 p_color, u32 p_gray, u32 p_alpha,
        Fvector2 n0, Fvector2 n1, Fvector2 r0, Fvector2 r1, Fvector2 l0, Fvector2 l1);
};
