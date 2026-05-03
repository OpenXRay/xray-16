#include "stdafx.h"
#include "Layers/xrRender/fgUIShader.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "xrEngine/IRenderBackend.h"

using namespace xray::render::resources;

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

    if (!m_baseTexture)
        return UINT32_MAX;

    auto* texManager = RImplementation.GetRenderDevice()->GetFGResourceManager()->GetTextureManager();
    auto handle = texManager->LoadTexture(m_baseTexture->cName.c_str());

    if (!handle.IsValid())
        return UINT32_MAX;

    nvrhi::ITexture* nvTex = texManager->GetNVRHITexture(handle);
    if (!nvTex)
        return UINT32_MAX;

    if (!GEnv.Backend)
        return UINT32_MAX;

    m_bindlessTextureIndex = GEnv.Backend->RegisterBindlessTexture(nvTex);
    return m_bindlessTextureIndex;
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
    const auto texture = GetBaseTexture();
    if (!texture)
        return false;

    FGResourceManager* resourceMgr = RImplementation.GetRenderDevice()->GetFGResourceManager();
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
}
