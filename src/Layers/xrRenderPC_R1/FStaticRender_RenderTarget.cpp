#include "stdafx.h"
#include "FStaticRender_RenderTarget.h"

#include "Layers/xrRender/blenders/Blender_Blur.h"
#include "Layers/xrRender/blenders/Blender_Screen_GRAY.h"

#include "xrEngine/IGame_Persistent.h"

CRenderTarget::CRenderTarget()
    : im_noise_time(1.f / 100.0f),
      param_noise_scale(1.f),
      param_noise_fps(25.f),
      param_color_base(color_rgba(127, 127, 127, 0)),
      param_color_gray(color_rgba(85, 85, 85, 0))
{
    curWidth = Device.dwWidth;
    curHeight = Device.dwHeight;

    // Select mode to operate in
    const float amount = ps_r__Supersample ? float(ps_r__Supersample) : 1;
    const float scale = _sqrt(amount);
    rtWidth = clampr(iFloor(scale * Device.dwWidth + .5f), 128, 2048);
    rtHeight = clampr(iFloor(scale * Device.dwHeight + .5f), 128, 2048);
    while (rtWidth % 2)
        rtWidth--;
    while (rtHeight % 2)
        rtHeight--;
    Msg("* SSample: %dx%d", rtWidth, rtHeight);

    // Bufferts
    rt_Base.resize(HW.BackBufferCount);
    for (u32 i = 0; i < HW.BackBufferCount; i++)
    {
        string32 temp;
        xr_sprintf(temp, "%s%d", r1_RT_base, i);
        rt_Base[i].create(temp, curWidth, curHeight, HW.Caps.fTarget, 1, { CRT::CreateBase });
    }
    rt_Base_Depth.create(r1_RT_base_depth, curWidth, curHeight, HW.Caps.fDepth, 1, { CRT::CreateBase });

    rt_Generic.create(r1_RT_generic, rtWidth, rtHeight, HW.Caps.fTarget);
    rt_distort.create(rt_RT_distort, rtWidth, rtHeight, HW.Caps.fTarget);
    if (RImplementation.o.color_mapping)
    {
        //rt_color_map.create(rt_RT_color_map, rtWidth, rtHeight, HW.Caps.fTarget);
        rt_color_map.create(rt_RT_color_map, curWidth, curHeight, HW.Caps.fTarget);
    }
    // RImplementation.o.color_mapping = RT_color_map->valid();

    if (rtHeight != Device.dwHeight || rtWidth != Device.dwWidth)
    {
        rt_Depth.create(r1_RT_depth, rtWidth, rtHeight, HW.Caps.fDepth, 0, { CRT::CreateSurface });
    }
    else
    {
        rt_Depth = rt_Base_Depth;
    }

    // Temp ZB, used by some of the shadowing code
    rt_temp_zb.create(rt_RT_temp_zb, 512, 512, HW.Caps.fDepth, 0, { CRT::CreateSurface });

    // Igor: TMP
    // Create an RT for online screenshot makining
    rt_async_ss.create(r1_RT_async_ss, rtWidth, rtHeight, HW.Caps.fTarget, 0, { CRT::CreateSurface });

    // Shaders and stream
    if (RImplementation.o.ffp)
    {
        g_postprocess[0].create(FVF::F_TL, RImplementation.Vertex.Buffer(), RImplementation.QuadIB);
        g_postprocess[1].create(FVF::F_TL2uv, RImplementation.Vertex.Buffer(), RImplementation.QuadIB);
        s_set.create("effects\\screen_set", r1_RT_generic);
        s_gray.create("effects\\screen_gray", r1_RT_generic);
        if (!s_gray)
        {
            CBlender_Screen_GRAY b;
            s_gray.create(&b, "effects\\screen_gray", r1_RT_generic);
        }
        s_blend.create("effects\\screen_blend", r1_RT_generic);
        s_duality.create("effects\\blur", r1_RT_generic "," r1_RT_generic);
        if (!s_duality)
        {
            CBlender_Blur b;
            s_duality.create(&b, "effects\\blur", r1_RT_generic "," r1_RT_generic);
        }
        s_noise.create("effects\\screen_noise", "fx\\fx_noise2");
    }
    else
    {
        s_postprocess[0].create("postprocess");
        if (RImplementation.o.distortion)
            s_postprocess_D[0].create("postprocess_d");

        if (RImplementation.o.color_mapping)
        {
            s_postprocess[1].create("postprocess_cm");
            if (RImplementation.o.distortion)
                s_postprocess_D[1].create("postprocess_dcm");
            if (!s_postprocess[1] || !s_postprocess_D[1])
            {
                Log("~ Color mapping disabled due to lack of one shader or both shaders");
                s_postprocess[1].destroy();
                s_postprocess_D[1].destroy();
                rt_color_map->destroy();
                RImplementation.o.color_mapping = FALSE;
            }
        }
        g_postprocess[0].create(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX3,
            RImplementation.Vertex.Buffer(), RImplementation.QuadIB);
    }

    bAvailable = rt_Generic->valid() && rt_distort->valid();
    Msg("* SSample: %s", bAvailable ? "enabled" : "disabled");
}

void CRenderTarget::calc_tc_noise(Fvector2& p0, Fvector2& p1)
{
    //. CTexture*   T                   = RCache.get_ActiveTexture  (2);
    //. VERIFY2     (T, "Texture #3 in noise shader should be setted up");
    //. u32         tw                  = iCeil(float(T->get_Width  ())*param_noise_scale+EPS_S);
    //. u32         th                  = iCeil(float(T->get_Height ())*param_noise_scale+EPS_S);
    const u32 tw = iCeil(256 * param_noise_scale + EPS_S);
    const u32 th = iCeil(256 * param_noise_scale + EPS_S);
    VERIFY2(tw && th, "Noise scale can't be zero in any way");
    //. if (bDebug) Msg         ("%d,%d,%f",tw,th,param_noise_scale);

    // calculate shift from FPSes
    im_noise_time -= Device.fTimeDelta;
    if (im_noise_time < 0)
    {
        im_noise_shift_w = ::Random.randI(tw ? tw : 1);
        im_noise_shift_h = ::Random.randI(th ? th : 1);
        const float fps_time = 1 / param_noise_fps;
        while (im_noise_time < 0)
            im_noise_time += fps_time;
    }

    const u32 shift_w = im_noise_shift_w;
    const u32 shift_h = im_noise_shift_h;
    const float start_u = (float(shift_w) + .5f) / (tw);
    const float start_v = (float(shift_h) + .5f) / (th);
    const u32 _w = Device.dwWidth;
    const u32 _h = Device.dwHeight;
    const u32 cnt_w = _w / tw;
    const u32 cnt_h = _h / th;
    const float end_u = start_u + float(cnt_w) + 1;
    const float end_v = start_v + float(cnt_h) + 1;

    p0.set(start_u, start_v);
    p1.set(end_u, end_v);
}

void CRenderTarget::calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1)
{
    // Calculate ordinaty TCs from blur and SS
    const float tw = float(rtWidth);
    const float th = float(rtHeight);
    if (rtHeight != Device.dwHeight)
        param_blur = 1.f;
    Fvector2 shift, p0, p1;
    shift.set(.5f / tw, .5f / th);
    shift.mul(param_blur);
    p0.set(.5f / tw, .5f / th).add(shift);
    p1.set((tw + .5f) / tw, (th + .5f) / th).add(shift);

    // Calculate Duality TC
    const float shift_u = param_duality_h * .5f;
    const float shift_v = param_duality_v * .5f;

    r0.set(p0.x, p0.y);
    r1.set(p1.x - shift_u, p1.y - shift_v);
    l0.set(p0.x + shift_u, p0.y + shift_v);
    l1.set(p1.x, p1.y);
}

bool CRenderTarget::NeedColorMapping() const
{
    return RImplementation.o.color_mapping && param_color_map_influence > 0.001f;
}

bool CRenderTarget::NeedPostProcess() const
{
    const bool _blur  = param_blur > 0.001f;
    const bool _gray  = param_gray > 0.001f;
    const bool _noise = param_noise > 0.001f;
    const bool _dual  = param_duality_h > 0.001f || param_duality_v > 0.001f;

    const bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

    const bool _cmap = NeedColorMapping();

    bool _cbase = false;
    {
        int _r = color_get_R(param_color_base);
        _r = _abs(_r - int(0x7f));
        int _g = color_get_G(param_color_base);
        _g = _abs(_g - int(0x7f));
        int _b = color_get_B(param_color_base);
        _b = _abs(_b - int(0x7f));
        if (_r > 2 || _g > 2 || _b > 2)
            _cbase = true;
    }
    bool _cadd = false;
    {
        const int _r = _abs((int)(param_color_add.x * 255));
        const int _g = _abs((int)(param_color_add.y * 255));
        const int _b = _abs((int)(param_color_add.z * 255));
        if (_r > 2 || _g > 2 || _b > 2)
            _cadd = true;
    }
    return _blur || _gray || _noise || _dual || _cbase || _cadd || _cmap || _menu_pp;
}

bool CRenderTarget::Perform() const
{
    return Available() && ((RImplementation.m_bMakeAsyncSS) || NeedPostProcess() || (ps_r__Supersample > 1) ||
                              (frame_distort == (Device.dwFrame - 1)));
}

void CRenderTarget::Begin()
{
    if (!Perform())
    {
        // Base RT
        RCache.set_RT(get_base_rt());
        RCache.set_ZB(get_base_zb());
        curWidth = Device.dwWidth;
        curHeight = Device.dwHeight;
    }
    else
    {
        // Our
        RCache.set_RT(rt_Generic->pRT);
        RCache.set_ZB(rt_Depth->pRT);
        curWidth = rtWidth;
        curHeight = rtHeight;
    }
    Device.Clear();
}

struct TL_2c3uv
{
    Fvector4 p;
    u32 color0;
    u32 color1;
    Fvector2 uv[3];

    void set(float x, float y, u32 c0, u32 c1, float u0, float v0, float u1, float v1, float u2, float v2)
    {
        p.set(x, y, EPS_S, 1.f);
        color0 = c0;
        color1 = c1;
        uv[0].set(u0, v0);
        uv[1].set(u1, v1);
        uv[2].set(u2, v2);
    }
};

void CRenderTarget::DoAsyncScreenshot() const
{
    //  Igor: screenshot will not have postprocess applied.
    //  TODO: fox that later
    if (RImplementation.m_bMakeAsyncSS)
    {
        HRESULT hr;

        IDirect3DSurface9* pFBSrc = get_base_rt();
        //  Don't addref, no need to release.
        // ID3DTexture2D *pTex = RT->pSurface;

        // hr = pTex->GetSurfaceLevel(0, &pFBSrc);

        //  SHould be async function
        hr = HW.pDevice->GetRenderTargetData(pFBSrc, rt_async_ss->pRT);

        // pFBSrc->Release();

        RImplementation.m_bMakeAsyncSS = false;
    }
}

void CRenderTarget::End()
{
    auto& dsgraph = RImplementation.get_imm_context();

    // find if distortion is needed at all
    const bool bPerform = Perform();
    bool bDistort = RImplementation.o.distortion;
    const bool bCMap = NeedColorMapping();

    if (dsgraph.mapDistort.empty())
        bDistort = FALSE;
    if (bDistort)
    {
        phase_distortion();

        dsgraph.render_distort();
        dsgraph.mapDistort.clear();
    }

    // combination/postprocess
    RCache.set_RT(get_base_rt());
    RCache.set_ZB(get_base_zb());
    curWidth = Device.dwWidth;
    curHeight = Device.dwHeight;

    if (!bPerform)
        return;

    phase_combine(bDistort, bCMap);
}

void CRenderTarget::phase_combine(bool bDistort, bool bCMap)
{
    RCache.set_RT(get_base_rt());
    RCache.set_ZB(get_base_zb());
    curWidth = Device.dwWidth;
    curHeight = Device.dwHeight;

    const int gblend = clampr(iFloor((1 - param_gray) * 255.f), 0, 255);
    const int nblend = clampr(iFloor((1 - param_noise) * 255.f), 0, 255);
    const u32 p_color = subst_alpha(param_color_base, nblend);
    const u32 p_gray = subst_alpha(param_color_gray, gblend);
    Fvector p_brightness = param_color_add;

    Fvector2 n0, n1, r0, r1, l0, l1;
    calc_tc_duality_ss(r0, r1, l0, l1);
    calc_tc_noise(n0, n1);

    if (RImplementation.o.ffp)
    {
        const u32 p_alpha = color_rgba(255, 255, 255, gblend);
        phase_combine_fpp(p_color, p_gray, p_alpha, n0, n1, r0, r1, l0, l1);
        return;
    }

    // Draw full-screen quad textured with our scene image
    u32 Offset;
    const float _w = float(Device.dwWidth);
    const float _h = float(Device.dwHeight);

    // Fill vertex buffer
    const float du = ps_r1_pps_u, dv = ps_r1_pps_v;
    TL_2c3uv* pv = (TL_2c3uv*)RImplementation.Vertex.Lock(4, g_postprocess[0].stride(), Offset);
    pv->set(du + 0, dv + float(_h), p_color, p_gray, r0.x, r1.y, l0.x, l1.y, n0.x, n1.y);
    pv++;
    pv->set(du + 0, dv + 0, p_color, p_gray, r0.x, r0.y, l0.x, l0.y, n0.x, n0.y);
    pv++;
    pv->set(du + float(_w), dv + float(_h), p_color, p_gray, r1.x, r1.y, l1.x, l1.y, n1.x, n1.y);
    pv++;
    pv->set(du + float(_w), dv + 0, p_color, p_gray, r1.x, r0.y, l1.x, l0.y, n1.x, n0.y);
    pv++;
    RImplementation.Vertex.Unlock(4, g_postprocess[0].stride());

    static shared_str s_colormap = "c_colormap";
    if (bCMap)
    {
        RCache.set_RT(rt_color_map->pRT);

        //  Prepare colormapped buffer
        RCache.set_Element(bDistort ? s_postprocess_D[1]->E[4] : s_postprocess[1]->E[4]);
        RCache.set_Geometry(g_postprocess[0]);
        RCache.set_c(s_colormap, param_color_map_influence, param_color_map_interpolate, 0.0f, 0.0f);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

        RCache.set_RT(get_base_rt());
        // return;
    }

    //  Element 0 for for normal post-process
    //  Element 4 for color map post-process
    // int iShaderElement   = bCMap ? 4 : 0;
    // RCache.set_Element   (bDistort ? s_postprocess_D->E[iShaderElement] : s_postprocess->E[iShaderElement]);

    RCache.set_Shader(bDistort ? s_postprocess_D[bCMap] : s_postprocess[bCMap]);

    // RCache.set_Shader    (bDistort ? s_postprocess_D : s_postprocess);

    // Actual rendering
    static shared_str s_brightness = "c_brightness";
    RCache.set_c(s_brightness, p_brightness.x, p_brightness.y, p_brightness.z, 0.0f);
    RCache.set_c(s_colormap, param_color_map_influence, param_color_map_interpolate, 0.0f, 0.0f);
    RCache.set_Geometry(g_postprocess[0]);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_combine_fpp(u32 p_color, u32 p_gray, u32 p_alpha,
    Fvector2 n0, Fvector2 n1, Fvector2 r0, Fvector2 r1, Fvector2 l0, Fvector2 l1)
{
    u32 Offset;
    const float _w = float(Device.dwWidth);
    const float _h = float(Device.dwHeight);

    // Fill vertex buffer
    auto pv = static_cast<FVF::TL*>(RImplementation.Vertex.Lock(12, g_postprocess[0].stride(), Offset));
    pv->set(0,         float(_h), EPS_S, 1.f, p_gray, r0.x, l1.y);
    pv++;
    pv->set(0,         0,         EPS_S, 1.f, p_gray, r0.x, r0.y);
    pv++;
    pv->set(float(_w), float(_h), EPS_S, 1.f, p_gray, l1.x, l1.y);
    pv++;
    pv->set(float(_w), 0,         EPS_S, 1.f, p_gray, l1.x, r0.y);
    pv++;

    pv->set(0,         float(_h), EPS_S, 1.f, p_color, r0.x, l1.y);
    pv++;
    pv->set(0,         0,         EPS_S, 1.f, p_color, r0.x, r0.y);
    pv++;
    pv->set(float(_w), float(_h), EPS_S, 1.f, p_color, l1.x, l1.y);
    pv++;
    pv->set(float(_w), 0,         EPS_S, 1.f, p_color, l1.x, r0.y);
    pv++;

    pv->set(0,         float(_h), EPS_S, 1.f, p_alpha, r0.x, l1.y);
    pv++;
    pv->set(0,         0,         EPS_S, 1.f, p_alpha, r0.x, r0.y);
    pv++;
    pv->set(float(_w), float(_h), EPS_S, 1.f, p_alpha, l1.x, l1.y);
    pv++;
    pv->set(float(_w), 0,         EPS_S, 1.f, p_alpha, l1.x, r0.y);
    pv++;
    RImplementation.Vertex.Unlock(12, g_postprocess[0].stride());

    // Actual rendering
    if (param_duality_h > 0.001f || param_duality_v > 0.001f)
    {
        constexpr u32 C = 0xffffffff;

        auto pv2 = static_cast<FVF::TL2uv*>(RImplementation.Vertex.Lock(4, g_postprocess[1].stride(), Offset));
        pv2->set(0,         float(_h), EPS_S, 1.f, C, r0.x, r1.y, l0.x, l1.y);
        pv2++;
        pv2->set(0,         0,         EPS_S, 1.f, C, r0.x, r0.y, l0.x, l0.y);
        pv2++;
        pv2->set(float(_w), float(_h), EPS_S, 1.f, C, r1.x, r1.y, l1.x, l1.y);
        pv2++;
        pv2->set(float(_w), 0,         EPS_S, 1.f, C, r1.x, r0.y, l1.x, l0.y);
        pv2++;
        RImplementation.Vertex.Unlock(4, g_postprocess[1].stride());

        // Draw Noise
        RCache.set_Shader(s_duality);
        RCache.set_Geometry(g_postprocess[1]);
        RCache.Render(D3DPT_TRIANGLELIST, Offset + 0, 0, 4, 0, 2);
    }
    else
    {
        if (param_gray > 0.001f)
        {
            // Draw GRAY
            RCache.set_Shader(s_gray);
            RCache.set_Geometry(g_postprocess[0]);
            RCache.Render(D3DPT_TRIANGLELIST, Offset + 0, 0, 4, 0, 2);

            if (param_gray < 0.999f)
            {
                // Blend COLOR
                RCache.set_Shader(s_blend);
                RCache.set_Geometry(g_postprocess[0]);
                RCache.Render(D3DPT_TRIANGLELIST, Offset + 4, 0, 4, 0, 2);
            }
        }
        else
        {
            // Draw COLOR
            RCache.set_Shader(s_set);
            RCache.set_Geometry(g_postprocess[0]);
            RCache.Render(D3DPT_TRIANGLELIST, Offset + 8, 0, 4, 0, 2);
        }
    }

    if (param_noise > 0.001f)
    {
        auto* pv2 = static_cast<FVF::TL*>(RImplementation.Vertex.Lock(4, g_postprocess[0].stride(), Offset));
        pv2->set(0,         float(_h), EPS_S, 1.f, p_color, n0.x, n1.y);
        pv2++;
        pv2->set(0,         0,         EPS_S, 1.f, p_color, n0.x, n0.y);
        pv2++;
        pv2->set(float(_w), float(_h), EPS_S, 1.f, p_color, n1.x, n1.y);
        pv2++;
        pv2->set(float(_w), 0,         EPS_S, 1.f, p_color, n1.x, n0.y);
        pv2++;
        RImplementation.Vertex.Unlock(4, g_postprocess[0].stride());

        // Draw Noise
        RCache.set_Shader(s_noise);
        RCache.set_Geometry(g_postprocess[0]);
        RCache.Render(D3DPT_TRIANGLELIST, Offset + 0, 0, 4, 0, 2);
    }
}

void CRenderTarget::phase_distortion()
{
    frame_distort = Device.dwFrame;
    RCache.set_RT(rt_distort->pRT);
    RCache.set_ZB(rt_Depth->pRT);
    RCache.set_CullMode(CULL_CCW);
    RCache.set_ColorWriteEnable();
    RCache.ClearRT(rt_distort, color_rgba(127, 127, 127, 127));
}
