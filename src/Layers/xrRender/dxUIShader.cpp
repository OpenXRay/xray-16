#include "stdafx.h"
#include "dxUIShader.h"

// FrameGraph includes - only available in R4 renderer
#if defined(USE_DX11)
#include "xrRender_console.h"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "ResourceManager/TextureManager.h"

extern ENGINE_API int ps_r4_use_framegraph;
#endif

namespace xray::render::RENDER_NAMESPACE
{
void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }

void dxUIShader::create(LPCSTR sh, LPCSTR tex)
{
    hShader.create(sh, tex);

    // CRITICAL: Force UI textures to load immediately to avoid lazy loading during rendering
    // This ensures dimensions are available when UI calculates UVs
    CTexture* baseTexture = GetBaseTexture();
    if (baseTexture && baseTexture->get_Width() == 0)
    {
        baseTexture->Load();
    }
}

void dxUIShader::destroy() { hShader.destroy(); }

CTexture* dxUIShader::GetBaseTexture() const
{
    if (!hShader)
        return nullptr;

    const SPass& pass = *hShader->E[0]->passes[0];
    if (!pass.T)
        return nullptr;

    const STextureList& textures = *pass.T;
    if (textures.empty())
        return nullptr;

    const R_constant* sbase = pass.constants->get(baseTexture)._get();

    return textures[sbase ? sbase->samp.index : 0].second._get();
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

#if defined(USE_DX11)
    // ═══════════════════════════════════════════════════
    //  FRAMEGRAPH MODE: Read dimensions from TextureManager
    // ═══════════════════════════════════════════════════
    // When using FrameGraph, textures are loaded through TextureManager and
    // CTexture::get_Width() may return 0. We need to read from NVRHI instead.

    using namespace xray::render::ng;
    using namespace xray::render::resources;

    if (ps_r4_use_framegraph && RImplementation.m_renderDevice)
    {
        FGResourceManager* resourceMgr = RImplementation.m_renderDevice->GetFGResourceManager();
        if (resourceMgr)
        {
            TextureManager* texManager = resourceMgr->GetTextureManager();

            // Load texture to ensure it exists (deduplicates automatically if already loaded)
            TextureHandle handle = texManager->LoadTexture(texture->cName.c_str());
            if (handle.IsValid())
            {
                // Get NVRHI texture and query its dimensions
                nvrhi::ITexture* nvrhiTexture = texManager->GetNVRHITexture(handle);
                if (nvrhiTexture)
                {
                    nvrhi::TextureDesc desc = nvrhiTexture->getDesc();
                    res = { float(desc.width), float(desc.height) };
                    return true;
                }
            }
            else
            {
                Msg("! [dxUIShader] FindTexture failed for: '%s'", texture->cName.c_str());
            }
        }
    }
#endif

    // ═══════════════════════════════════════════════════
    //  LEGACY MODE: Read from CTexture
    // ═══════════════════════════════════════════════════

    u32 w = texture->get_Width();
    u32 h = texture->get_Height();

    if (w == 0 || h == 0)
    {
        res = { 1.0f, 1.0f };  // Safety fallback to avoid division by zero
        return false;
    }

    res = { float(w), float(h) };
    return true;
}
} // namespace xray::render::RENDER_NAMESPACE
