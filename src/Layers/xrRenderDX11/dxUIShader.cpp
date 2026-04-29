#include "stdafx.h"
#include "Layers/xrRender/dxUIShader.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

using namespace xray::render::resources;

namespace xray::render::fg
{
void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }

void dxUIShader::create(LPCSTR sh, LPCSTR tex)
{
    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
    {
        Msg("! [dxUIShader] ShaderLoader is NULL for shader: %s", sh);
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
        Msg("* [dxUIShader] Shader '%s' not found, falling back to stub_notransform_t", sh);
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
        Msg("* [dxUIShader] Compiled UI shader: %s (tex: %s)", sh, tex ? tex : "none");
    }
    else
    {
        Msg("! [dxUIShader] Failed to compile UI shader: %s (VS=%s, PS=%s)",
            sh,
            vsResult.handle ? "OK" : "FAILED",
            psResult.handle ? "OK" : "FAILED");
    }
}

void dxUIShader::destroy()
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

CTexture* dxUIShader::GetBaseTexture() const
{
    return m_baseTexture;
}

xrImTextureData dxUIShader::GetImGuiTextureId()
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

bool dxUIShader::GetBaseTextureResolution(Fvector2& res)
{
    const auto texture = GetBaseTexture();
    if (!texture)
    {
        res = { 1.0f, 1.0f };  // Safety fallback to avoid division by zero
        return false;
    }

    FGResourceManager* resourceMgr = RImplementation.m_renderDevice->GetFGResourceManager();
    if (!resourceMgr)
        return false;

    TextureManager* texManager = resourceMgr->GetTextureManager();
    TextureHandle handle = texManager->LoadTexture(texture->cName.c_str());
    if (!handle.IsValid())
        return false;

    nvrhi::ITexture* nvrhiTexture = texManager->GetNVRHITexture(handle);
    bool ok = (nvrhiTexture != nullptr);
    if (ok)
    {
        nvrhi::TextureDesc desc = nvrhiTexture->getDesc();
        res = { float(desc.width), float(desc.height) };
    }
    texManager->Release(handle);
    return ok;
}
} // namespace xray::render::fg
