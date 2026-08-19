#include "stdafx.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/r4_rendertarget.h"
#include "xrEngine/device.h"

namespace xray::render::fg
{
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

CRenderTarget::CRenderTarget()
{
    im_noise_time = 1.f;
    im_noise_shift_w = 0;
    im_noise_shift_h = 0;
    param_blur = 0.f;
    param_gray = 0.f;
    param_duality_h = 0.f;
    param_duality_v = 0.f;
    param_noise = 0.f;
    param_noise_scale = 1.f;
    param_noise_fps = 25.f;
    param_color_base = color_rgba(127, 127, 127, 0);
    param_color_gray = color_rgba(85, 85, 85, 0);
    param_color_add.set(0.f, 0.f, 0.f);
    param_color_map_influence = 0.f;
    param_color_map_interpolate = 0.f;
    m_bHasActiveVolumetric = false;
}

CRenderTarget::~CRenderTarget() {}

void CRenderTarget::u_calc_tc_noise(Fvector2& p0, Fvector2& p1)
{
    u32 tw = iCeil(512.f * param_noise_scale + EPS_S);
    u32 th = iCeil(512.f * param_noise_scale + EPS_S);
    if (!tw)
        tw = 1;
    if (!th)
        th = 1;

    im_noise_time -= Device.fTimeDelta;
    if (im_noise_time < 0)
    {
        im_noise_shift_w = ::Random.randI(tw);
        im_noise_shift_h = ::Random.randI(th);
        float fps_time = 1.f / std::max(param_noise_fps, 1.f);
        while (im_noise_time < 0)
            im_noise_time += fps_time;
    }

    float start_u = (float(im_noise_shift_w) + .5f) / float(tw);
    float start_v = (float(im_noise_shift_h) + .5f) / float(th);
    u32 cnt_w = Device.dwWidth / tw;
    u32 cnt_h = Device.dwHeight / th;
    p0.set(start_u, start_v);
    p1.set(start_u + float(cnt_w) + 1.f, start_v + float(cnt_h) + 1.f);
}

void CRenderTarget::u_calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1)
{
    float tw = float(Device.dwWidth);
    float th = float(Device.dwHeight);
    Fvector2 shift, p0, p1;
    shift.set(.5f / tw, .5f / th);
    shift.mul(param_blur);
    p0.set(.5f / tw, .5f / th);
    p0.add(shift);
    p1.set((tw + .5f) / tw, (th + .5f) / th);
    p1.add(shift);

    float shift_u = param_duality_h * .5f;
    float shift_v = param_duality_v * .5f;

    r0.set(p0.x, p0.y);
    r1.set(p1.x - shift_u, p1.y - shift_v);
    l0.set(p0.x + shift_u, p0.y + shift_v);
    l1.set(p1.x, p1.y);
}

bool CRenderTarget::u_need_CM()
{
    return param_color_map_influence > 0.001f;
}

bool CRenderTarget::u_need_PP()
{
    bool _blur = (param_blur > 0.001f);
    bool _gray = (param_gray > 0.001f);
    bool _noise = (param_noise > 0.001f);
    bool _dual = (param_duality_h > 0.001f) || (param_duality_v > 0.001f);

    bool _cbase = false;
    {
        int _r = _abs(int(color_get_R(param_color_base)) - int(0x7f));
        int _g = _abs(int(color_get_G(param_color_base)) - int(0x7f));
        int _b = _abs(int(color_get_B(param_color_base)) - int(0x7f));
        if (_r > 2 || _g > 2 || _b > 2)
            _cbase = true;
    }
    bool _cadd = false;
    {
        int _r = _abs((int)(param_color_add.x * 255));
        int _g = _abs((int)(param_color_add.y * 255));
        int _b = _abs((int)(param_color_add.z * 255));
        if (_r > 0 || _g > 0 || _b > 0)
            _cadd = true;
    }
    return _blur || _gray || _noise || _dual || _cbase || _cadd || u_need_CM();
}

void CRenderTarget::phase_pp() {}

bool CRenderTarget::need_to_render_sunshafts()
{
    if (!(RImplementation.o.advancedpp && ps_r_sun_shafts))
        return false;

    {
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        const float fValue = env.m_fSunShaftsIntensity;
        // TODO: add multiplication by sun color here
        if (fValue < 0.0001)
            return false;
    }

    return true;
}

bool CRenderTarget::use_minmax_sm_this_frame()
{
    switch (RImplementation.o.minmax_sm)
    {
    case FrameGraphRenderer::MMSM_ON: return true;
    case FrameGraphRenderer::MMSM_AUTO: return need_to_render_sunshafts();
    case FrameGraphRenderer::MMSM_AUTODETECT:
    {
        const auto& [width, height] = GEnv.Backend->GetBackBufferSize();
        u32 dwScreenArea = width * height;

        if (dwScreenArea >= RImplementation.o.minmax_sm_screenarea_threshold)
            return need_to_render_sunshafts();
        return false;
    }

    default: return false;
    }
}
} // namespace xray::render::fg
