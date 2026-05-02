#include "stdafx.h"
#include "Layers/xrRenderDX11/ResourceManager.h"

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
    return;
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
