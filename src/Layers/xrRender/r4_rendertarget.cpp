#include "stdafx.h"
#include "Layers/xrRender/ResourceManager.h"

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
