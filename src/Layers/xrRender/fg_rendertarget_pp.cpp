#include "stdafx.h"
#include "r4_rendertarget.h"

namespace xray::render::fg
{
void CRenderTarget::u_calc_tc_noise(Fvector2& p0, Fvector2& p1)
{
    u32 tw = iCeil(512.f * param_noise_scale + EPS_S);
    u32 th = iCeil(512.f * param_noise_scale + EPS_S);
    if (!tw) tw = 1;
    if (!th) th = 1;

    im_noise_time -= Device.fTimeDelta;
    if (im_noise_time < 0)
    {
        im_noise_shift_w = ::Random.randI(tw);
        im_noise_shift_h = ::Random.randI(th);
        float fps_time = 1.f / param_noise_fps;
        while (im_noise_time < 0)
            im_noise_time += fps_time;
    }

    float start_u = (float(im_noise_shift_w) + .5f) / float(tw);
    float start_v = (float(im_noise_shift_h) + .5f) / float(th);
    u32 _w = Device.dwWidth;
    u32 _h = Device.dwHeight;
    u32 cnt_w = _w / tw;
    u32 cnt_h = _h / th;
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
    p0.set(.5f / tw, .5f / th).add(shift);
    p1.set((tw + .5f) / tw, (th + .5f) / th).add(shift);

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
        if (_r > 2 || _g > 2 || _b > 2)
            _cadd = true;
    }
    return _blur || _gray || _noise || _dual || _cbase || _cadd || u_need_CM();
}

void CRenderTarget::phase_pp() {}
} // namespace xray::render::fg
