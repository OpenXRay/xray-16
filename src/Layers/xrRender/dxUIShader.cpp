#include "stdafx.h"
#include "dxUIShader.h"
#include "xrRender_console.h"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "ResourceManager/TextureManager.h"
#include "FrameGraph/ShaderCache.h"

extern ENGINE_API int ps_r4_use_framegraph;

using namespace xray::render::resources;

namespace xray::render::RENDER_NAMESPACE
{
void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }

void dxUIShader::create(LPCSTR sh, LPCSTR tex)
{
    hShader.create(sh, tex);

    CTexture* baseTexture = GetBaseTexture();
    if (baseTexture && baseTexture->get_Width() == 0)
    {
        baseTexture->Load();
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

    FGResourceManager* resourceMgr = RImplementation.m_renderDevice->GetFGResourceManager();
    if (!resourceMgr)
        return false;

    TextureManager* texManager = resourceMgr->GetTextureManager();
    TextureHandle handle = texManager->LoadTexture(texture->cName.c_str());
    if (!handle.IsValid())
        return false;

    nvrhi::ITexture* nvrhiTexture = texManager->GetNVRHITexture(handle);
    if (!nvrhiTexture)
        return false;

    nvrhi::TextureDesc desc = nvrhiTexture->getDesc();
    res = { float(desc.width), float(desc.height) };
    return true;
}
} // namespace xray::render::RENDER_NAMESPACE
