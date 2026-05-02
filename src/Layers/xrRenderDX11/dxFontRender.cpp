#include "stdafx.h"

#include "Layers/xrRender/dxFontRender.h"

#include "xrEngine/GameFont.h"
#include "xrCore/Text/StringConversion.hpp"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

extern ENGINE_API bool g_bRendering;
extern ENGINE_API Fvector2 g_current_font_scale;

namespace xray::render::fg
{
dxFontRender::~dxFontRender()
{
    m_vsHandle = nullptr;
    m_psHandle = nullptr;
    m_baseTexture = nullptr;

    if (m_vsReflection)
    {
        xr_delete(m_vsReflection);
        m_vsReflection = nullptr;
    }
    if (m_psReflection)
    {
        xr_delete(m_psReflection);
        m_psReflection = nullptr;
    }
}

void dxFontRender::Initialize(cpcstr cShader, cpcstr cTexture)
{
    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
    {
        Msg("! [dxFontRender] ShaderLoader is NULL for shader: %s", cShader);
        return;
    }

    m_shaderName = cShader;
    m_textureName = cTexture;

    if (cTexture)
    {
        bool prevDeferredLoad = RImplementation.Resources->bDeferredLoad;
        RImplementation.Resources->bDeferredLoad = true;
        m_baseTexture = RImplementation.Resources->_CreateTexture(cTexture);
        RImplementation.Resources->bDeferredLoad = prevDeferredLoad;
    }

    auto vsResult = shaderLoader->LoadVertexShader("stub_notransform_t", "main");
    auto psResult = shaderLoader->LoadPixelShader(cShader, "main");

    if (!psResult.handle)
    {
        Msg("* [dxFontRender] Pixel shader '%s' not found, falling back to hud_font", cShader);
        psResult = shaderLoader->LoadPixelShader("hud_font", "main");
    }

    if (vsResult.handle && psResult.handle)
    {
        m_vsHandle = vsResult.handle;
        m_psHandle = psResult.handle;
        m_vsReflection = vsResult.reflection;
        m_psReflection = psResult.reflection;
        vsResult.reflection = nullptr;
        psResult.reflection = nullptr;
        Msg("* [dxFontRender] Compiled font shader: %s (tex: %s)", cShader, cTexture ? cTexture : "none");
    }
    else
    {
        Msg("! [dxFontRender] Failed to compile font shader: %s (VS=%s, PS=%s)",
            cShader,
            vsResult.handle ? "OK" : "FAILED",
            psResult.handle ? "OK" : "FAILED");
    }
}

void dxFontRender::OnRender(CGameFont&)
{
}
} // namespace xray::render::fg
