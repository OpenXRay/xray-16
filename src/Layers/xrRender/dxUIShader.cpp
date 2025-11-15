#include "stdafx.h"
#include "dxUIShader.h"

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

bool dxUIShader::operator==(const IUIShader& other) const
{
    return hShader == static_cast<const dxUIShader&>(other).hShader;
}

CTexture* dxUIShader::GetBaseTexture() const
{
    if (!hShader)
        return nullptr;

    const SPass& pass = *hShader->E[0]->passes[0];
    if (!pass.T)
    {
        Msg("! [GetBaseTexture] pass.T is NULL");
        return nullptr;
    }

    const STextureList& textures = *pass.T;
    if (textures.empty())
    {
        Msg("! [GetBaseTexture] pass.T is empty");
        return nullptr;
    }

    Msg("  [GetBaseTexture] pass.T has %zu textures", textures.size());
    for (size_t i = 0; i < textures.size(); i++)
    {
        const auto& [slot, textureName] = textures[i];
        Msg("    Texture[%zu]: slot=%u, name='%s'", i, slot,
            textureName.c_str() ? textureName.c_str() : "NULL");
    }

    const char* baseTextureName = nullptr;

#ifdef USE_DX11
    SPS* ps = pass.ps._get();
    if (ps && ps->reflection)
    {
        Msg("  [GetBaseTexture] Searching for '%s' in reflection", baseTexture.c_str());
        const auto& inputTextures = ps->reflection->rtBindings.inputTextures;
        for (const auto& tex : inputTextures)
        {
            if (tex.name == baseTexture.c_str())
            {
                u32 expectedSlot = tex.slot + CTexture::rstPixel;
                Msg("  [GetBaseTexture] Found '%s' in reflection at slot %u, expectedSlot (with offset) = %u",
                    baseTexture.c_str(), tex.slot, expectedSlot);

                for (const auto& [slot, textureName] : textures)
                {
                    if (slot == expectedSlot)
                    {
                        Msg("  [GetBaseTexture] ✓ MATCH found at expectedSlot %u", expectedSlot);
                        baseTextureName = textureName.c_str();
                        break;
                    }
                }
                if (!baseTextureName)
                    Msg("! [GetBaseTexture] No texture found at expectedSlot %u", expectedSlot);
                break;
            }
        }
    }
    else
    {
        Msg("! [GetBaseTexture] No pixel shader reflection available");
    }
#endif

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
        res = { 1.0f, 1.0f };
        return false;
    }

#if defined(USE_DX11)

    using namespace xray::render::ng;
    using namespace xray::render::resources;

    if (ps_r4_use_framegraph && RImplementation.m_renderDevice)
    {
        FGResourceManager* resourceMgr = RImplementation.m_renderDevice->GetFGResourceManager();
        if (resourceMgr)
        {
            TextureManager* texManager = resourceMgr->GetTextureManager();

            TextureHandle handle = texManager->LoadTexture(texture->cName.c_str());
            if (handle.IsValid())
            {
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

    u32 w = texture->get_Width();
    u32 h = texture->get_Height();

    if (w == 0 || h == 0)
    {
        res = { 1.0f, 1.0f };
        return false;
    }

    res = { float(w), float(h) };
    return true;
}
} // namespace xray::render::RENDER_NAMESPACE
