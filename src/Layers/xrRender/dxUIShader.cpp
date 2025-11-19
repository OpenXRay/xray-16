#include "stdafx.h"
#include "dxUIShader.h"

// FrameGraph includes - only available in R4 renderer
#if defined(USE_DX11)
#include "xrRender_console.h"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "ResourceManager/TextureManager.h"
#include "FrameGraph/ShaderCache.h"

extern ENGINE_API int ps_r4_use_framegraph;
#endif

namespace xray::render::RENDER_NAMESPACE
{
void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }

void dxUIShader::create(LPCSTR sh, LPCSTR tex)
{
    Msg("  [dxUIShader::create] Creating UI shader: sh='%s', tex='%s'", sh, tex);
    hShader.create(sh, tex);

    // CRITICAL: Force UI textures to load immediately to avoid lazy loading during rendering
    // This ensures dimensions are available when UI calculates UVs
    CTexture* baseTexture = GetBaseTexture();
    if (baseTexture)
    {
        Msg("  [dxUIShader::create] Got base texture: '%s' (width=%u)",
            baseTexture->cName.c_str(), baseTexture->get_Width());
        if (baseTexture->get_Width() == 0)
        {
            baseTexture->Load();
        }
    }
    else
    {
        Msg("! [dxUIShader::create] GetBaseTexture returned NULL!");
    }
}

void dxUIShader::destroy() { hShader.destroy(); }

CTexture* dxUIShader::GetBaseTexture() const
{
    if (!hShader)
        return nullptr;

    const SPass& pass = *hShader->E[0]->passes[0];
    if (!pass.T)
    {
        return nullptr;
    }

    const STextureList& textures = *pass.T;
    if (textures.empty())
    {
        return nullptr;
    }

    const char* baseTextureName = nullptr;

    // Find s_base texture by querying pixel shader reflection
    SPS* ps = pass.ps._get();
    if (ps && ps->reflection)
    {
        const auto& inputTextures = ps->reflection->rtBindings.inputTextures;
        for (const auto& tex : inputTextures)
        {
            if (tex.name == baseTexture.c_str()) // baseTexture = "s_base"
            {
                // Found s_base, now find the texture with matching slot in pass.T
                u32 expectedSlot = tex.slot + CTexture::rstPixel;

                for (const auto& [slot, textureName] : textures)
                {
                    if (slot == expectedSlot)
                    {
                        baseTextureName = textureName.c_str();
                        break;
                    }
                }

                break;
            }
        }
    }

    // Fallback: use first texture if s_base not found
    if (!baseTextureName || !baseTextureName[0])
    {
        Msg("  [GetBaseTexture] Falling back to first texture");
        baseTextureName = textures[0].second.c_str();
    }

    if (!baseTextureName || !baseTextureName[0])
    {
        Msg("! [GetBaseTexture] No valid texture name found");
        return nullptr;
    }

    // Load the actual CTexture from the texture name
    return RImplementation.Resources->_CreateTexture(baseTextureName);
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
