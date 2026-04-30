#include "stdafx.h"
#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRenderDX11/blenders/blender_light_occq.h"
#include "Layers/xrRenderDX11/blenders/blender_light_mask.h"
#include "Layers/xrRenderDX11/blenders/blender_light_direct.h"
#include "Layers/xrRenderDX11/blenders/blender_light_point.h"
#include "Layers/xrRenderDX11/blenders/blender_light_spot.h"
#include "Layers/xrRenderDX11/blenders/blender_light_reflected.h"
#include "Layers/xrRenderDX11/blenders/blender_combine.h"
#include "Layers/xrRenderDX11/blenders/blender_bloom_build.h"
#include "Layers/xrRenderDX11/blenders/blender_luminance.h"
#include "Layers/xrRenderDX11/blenders/blender_ssao.h"

#include "Layers/xrRenderDX11/blenders/dx11MSAABlender.h"
#include "Layers/xrRenderDX11/blenders/dx11RainBlender.h"

#include "Layers/xrRenderDX11/blenders/dx11MinMaxSMBlender.h"
#if defined(USE_DX11)
#    include "Layers/xrRenderDX11/blenders/dx11HDAOCSBlender.h"
#endif

namespace xray::render::fg
{
void CRenderTarget::u_stencil_optimize(CBackend& cmd_list, eStencilOptimizeMode eSOM)
{
    UNUSED(cmd_list);
    UNUSED(eSOM);
}

// 2D texgen (texture adjustment matrix)
void CRenderTarget::u_compute_texgen_screen(CBackend& cmd_list, Fmatrix& m_Texgen)
{
#if defined(USE_DX11)
    Fmatrix m_TexelAdjust =
    {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
};
#elif defined(USE_OGL)
    Fmatrix m_TexelAdjust =
    {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    };
#else
#   error No graphics API selected or enabled!
#endif

    m_Texgen.mul(m_TexelAdjust, cmd_list.xforms.m_wvp);
}

// 2D texgen for jitter (texture adjustment matrix)
void CRenderTarget::u_compute_texgen_jitter(CBackend& cmd_list, Fmatrix& m_Texgen_J)
{
    // place into 0..1 space
    Fmatrix m_TexelAdjust =
    {
        0.5f, 0.0f, 0.0f, 0.0f,
#if defined(USE_DX11)
        0.0f, -0.5f, 0.0f, 0.0f,
#elif defined(USE_OGL)
        0.0f, 0.5f, 0.0f, 0.0f,
#else
#   error No graphics API selected or enabled!
#endif
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    };
    m_Texgen_J.mul(m_TexelAdjust, cmd_list.xforms.m_wvp);

    // rescale - tile it
    float scale_X = float(Device.dwWidth) / float(TEX_jitter);
    float scale_Y = float(Device.dwHeight) / float(TEX_jitter);
    m_TexelAdjust.scale(scale_X, scale_Y, 1.f);
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
    return;
}

CRenderTarget::~CRenderTarget()
{
#if defined(USE_DX11)
    _RELEASE(t_ss_async);
#elif defined(USE_OGL)
    // Textures
    t_material->surface_set(GL_TEXTURE_3D, 0);
    glDeleteTextures(1, &t_material_surf);
    t_material.destroy();

    t_LUM_src->surface_set(GL_TEXTURE_2D, 0);
    t_LUM_dest->surface_set(GL_TEXTURE_2D, 0);
    t_LUM_src.destroy();
    t_LUM_dest.destroy();

    // Jitter
    for (u32 it = 0; it < TEX_jitter_count; it++)
    {
        t_noise[it]->surface_set(GL_TEXTURE_2D, 0);
    }
    glDeleteTextures(TEX_jitter_count, t_noise_surf);

    t_noise_mipped->surface_set(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &t_noise_surf_mipped);
#else
#   error No graphics API selected or enabled!
#endif
}

void CRenderTarget::reset_light_marker(CBackend& cmd_list, bool bResetStencil)
{
    UNUSED(cmd_list);
    UNUSED(bResetStencil);
    dwLightMarkerID = 5;
}

void CRenderTarget::increment_light_marker(CBackend& cmd_list)
{
    dwLightMarkerID += 2;

    const u32 iMaxMarkerValue = RImplementation.o.msaa ? 127 : 255;

    if (dwLightMarkerID > iMaxMarkerValue)
        reset_light_marker(cmd_list, true);
}

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
