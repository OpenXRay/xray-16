#include "stdafx.h"
#include "dxUIShader.h"
#include "xrRender_console.h"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "ResourceManager/TextureManager.h"
#include "FrameGraph/ShaderCache.h"
#include "FrameGraph/ShaderLoader.h"

using namespace xray::render::resources;

namespace xray::render::RENDER_NAMESPACE
{
void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }

void dxUIShader::create(LPCSTR sh, LPCSTR tex)
{
    // DX12/FrameGraph: Compile shaders using NVRHI ShaderLoader
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
    {
        auto* shaderLoader = RImplementation.GetShaderLoader();
        if (!shaderLoader)
        {
            Msg("! [dxUIShader] ShaderLoader is NULL for shader: %s", sh);
            return;
        }

        // Cache the base texture for GetBaseTexture()
        // For DX12, we defer texture loading - it will be loaded via TextureManager when creating the PSO
        if (tex) {
            // Set deferred load flag to prevent _CreateTexture from calling Load()
            bool prevDeferredLoad = RImplementation.Resources->bDeferredLoad;
            RImplementation.Resources->bDeferredLoad = true;

            m_baseTexture = RImplementation.Resources->_CreateTexture(tex);

            RImplementation.Resources->bDeferredLoad = prevDeferredLoad;
        }

        // Try to load the requested shader
        auto vsResult = shaderLoader->LoadVertexShader(sh, "main");
        auto psResult = shaderLoader->LoadPixelShader(sh, "main");

        // If shader not found, fall back to stub_notransform_t (UI shaders use screen-space coordinates)
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

            // Store reflection data (transfer ownership from ShaderResult)
            m_vsReflection = vsResult.reflection;
            m_psReflection = psResult.reflection;
            vsResult.reflection = nullptr;  // Transfer ownership
            psResult.reflection = nullptr;  // Transfer ownership

            Msg("* [dxUIShader] Compiled UI shader: %s (tex: %s)", sh, tex ? tex : "none");
        }
        else
        {
            Msg("! [dxUIShader] Failed to compile UI shader: %s (VS=%s, PS=%s)",
                sh,
                vsResult.handle ? "OK" : "FAILED",
                psResult.handle ? "OK" : "FAILED");
        }

        // Texture loading is deferred to PSO creation time (via TextureManager)
        // Don't call Load() here as it would use legacy D3D11 HW.pDevice

        return;
    }

    // Legacy D3D11 path
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

void dxUIShader::destroy()
{
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
    {
        m_vsHandle = nullptr;
        m_psHandle = nullptr;
        m_baseTexture = nullptr;  // Release texture reference

        // Clean up reflection data
        if (m_vsReflection) {
            xr_delete(m_vsReflection);
            m_vsReflection = nullptr;
        }
        if (m_psReflection) {
            xr_delete(m_psReflection);
            m_psReflection = nullptr;
        }
        return;
    }

    hShader.destroy();
}

CTexture* dxUIShader::GetBaseTexture() const
{
    // DX12: Return cached texture pointer
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
    {
        return m_baseTexture;
    }

    // Legacy D3D11 path
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
