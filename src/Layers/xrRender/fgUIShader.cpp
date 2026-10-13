#include "stdafx.h"
#include "Layers/xrRender/fgUIShader.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg
{
namespace
{
xr_shared_ptr<const framegraph::ExtractedReflection> ShareReflection(framegraph::ExtractedReflection*& reflection)
{
    if (!reflection)
        return {};
    xr_shared_ptr<const framegraph::ExtractedReflection> shared(reflection, xr_custom_deleter<framegraph::ExtractedReflection>());
    reflection = nullptr;
    return shared;
}
}

fgUIShader::~fgUIShader()
{
    destroy();
    m_aliveSentinel = DEAD_SENTINEL;
}

void fgUIShader::Copy(IUIShader& _in)
{
    auto& source = static_cast<fgUIShader&>(_in);
    if (&source == this)
        return;

    destroy();
    m_vsHandle = source.m_vsHandle;
    m_psHandle = source.m_psHandle;
    m_vsReflection = source.m_vsReflection;
    m_psReflection = source.m_psReflection;
    m_baseTexture = source.m_baseTexture;
}

void fgUIShader::create(LPCSTR sh, LPCSTR tex)
{
    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
    {
        Msg("! [fgUIShader] ShaderLoader is NULL for shader: %s", sh);
        destroy();
        return;
    }

    ref_texture texture;
    if (tex)
    {
        bool prevDeferredLoad = RImplementation.Resources->bDeferredLoad;
        RImplementation.Resources->bDeferredLoad = true;
        texture.create(tex);
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

    destroy();
    m_baseTexture = texture;

    if (vsResult.handle && psResult.handle)
    {
        m_vsHandle = vsResult.handle;
        m_psHandle = psResult.handle;
        m_vsReflection = ShareReflection(vsResult.reflection);
        m_psReflection = ShareReflection(psResult.reflection);
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
    nvrhi::ITexture* texture = LoadBaseTexture();
    if (texture && texture == m_bindlessTexture)
        return m_bindlessTextureIndex;

    ReleaseBindlessIndex();
    if (!texture || !GEnv.Backend)
        return UINT32_MAX;

    const u32 index = GEnv.Backend->RegisterBindlessTexture(texture);
    if (index == UINT32_MAX)
        return UINT32_MAX;

    m_bindlessTexture = texture;
    m_bindlessTextureIndex = index;
    return index;
}

void fgUIShader::ReleaseBindlessIndex()
{
    if (m_bindlessTextureIndex != UINT32_MAX && GEnv.Backend)
        GEnv.Backend->UnregisterBindlessTexture(m_bindlessTextureIndex);
    m_bindlessTextureIndex = UINT32_MAX;
    m_bindlessTexture = nullptr;
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
    ReleaseBindlessIndex();
    m_vsHandle = nullptr;
    m_psHandle = nullptr;
    m_vsReflection.reset();
    m_psReflection.reset();
    m_baseTexture.destroy();
}

CTexture* fgUIShader::GetBaseTexture() const
{
    return m_baseTexture._get();
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
