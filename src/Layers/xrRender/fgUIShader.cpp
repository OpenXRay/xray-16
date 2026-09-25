#include "stdafx.h"
#include "Layers/xrRender/fgUIShader.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg
{
void fgUIShader::Copy(IUIShader& _in) { *this = *((fgUIShader*)&_in); }

void fgUIShader::create(LPCSTR sh, LPCSTR tex)
{
    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
    {
        Msg("! [fgUIShader] ShaderLoader is NULL for shader: %s", sh);
        return;
    }

    if (tex)
    {
        bool prevDeferredLoad = RImplementation.Resources->bDeferredLoad;
        RImplementation.Resources->bDeferredLoad = true;
        m_baseTexture = RImplementation.Resources->_CreateTexture(tex);
        RImplementation.Resources->bDeferredLoad = prevDeferredLoad;
    }

    auto vsResult = shaderLoader->LoadVertexShader(sh, "main");
    auto psResult = shaderLoader->LoadPixelShader(sh, "main");

    if (!vsResult.handle || !psResult.handle)
    {
        Msg("* [fgUIShader] Shader '%s' not found, falling back to stub_notransform_t", sh);
        vsResult = shaderLoader->LoadVertexShader("stub_notransform_t", "main");
        psResult = shaderLoader->LoadPixelShader("stub_default", "main");
    }

    if (vsResult.handle && psResult.handle)
    {
        m_vsHandle = vsResult.handle;
        m_psHandle = psResult.handle;
        m_vsReflection = vsResult.reflection;
        m_psReflection = psResult.reflection;
        vsResult.reflection = nullptr;
        psResult.reflection = nullptr;
    }
    else
    {
        Msg("! [fgUIShader] Failed to compile UI shader: %s (VS=%s, PS=%s)",
            sh,
            vsResult.handle ? "OK" : "FAILED",
            psResult.handle ? "OK" : "FAILED");
    }
}

u32 fgUIShader::GetBindlessIndex()
{
    if (m_bindlessTextureIndex != UINT32_MAX)
        return m_bindlessTextureIndex;

    nvrhi::ITexture* texture = LoadBaseTexture();
    if (!texture || !GEnv.Backend)
        return UINT32_MAX;

    m_bindlessTextureIndex = GEnv.Backend->RegisterBindlessTexture(texture);
    return m_bindlessTextureIndex;
}

nvrhi::ITexture* fgUIShader::LoadBaseTexture()
{
    if (!m_baseTexture)
        return nullptr;
    if (!m_baseTexture->flags.bLoaded)
        m_baseTexture->Load();
    return m_baseTexture->surface_get_native();
}

void fgUIShader::destroy()
{
    if (m_bindlessTextureIndex != UINT32_MAX && GEnv.Backend) {
        GEnv.Backend->UnregisterBindlessTexture(m_bindlessTextureIndex);
        m_bindlessTextureIndex = UINT32_MAX;
    }
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

CTexture* fgUIShader::GetBaseTexture() const
{
    return m_baseTexture;
}

xrImTextureData fgUIShader::GetImGuiTextureId()
{
    const auto texture = GetBaseTexture();
    if (!texture)
        return {};

    return
    {
        texture->GetImTextureID(),
        {
            (float)texture->get_Width(),
            (float)texture->get_Height()
        }
    };
}

bool fgUIShader::GetBaseTextureResolution(Fvector2& res)
{
    res = { 1.0f, 1.0f };
    nvrhi::ITexture* texture = LoadBaseTexture();
    if (!texture)
        return false;

    const auto& desc = texture->getDesc();
    res = { float(desc.width), float(desc.height) };
    return true;
}
}
