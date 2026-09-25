#include "stdafx.h"

#include "SH_Texture.h"
#include "ColorSpace.h"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "ResourceManager/TextureManager.h"
#include "r_FrameGraphRenderer.h"

namespace xray::render::fg
{
void fix_texture_name(pstr fn)
{
    pstr _ext = strext(fn);
    if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".dds") || !xr_stricmp(_ext, ".bmp") ||
                 !xr_stricmp(_ext, ".ogm")))
    {
        *_ext = 0;
    }
}

namespace
{
constexpr u64 kMaxReportedTextureMemory = (1u << 28) - 1;

resources::TextureManager* ManagedTextures()
{
    auto* renderDevice = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    auto* resourceManager = renderDevice ? renderDevice->GetFGResourceManager() : nullptr;
    return resourceManager ? resourceManager->GetTextureManager() : nullptr;
}
}

void resptrcode_texture::create(LPCSTR _name) { _set(RImplementation.Resources->_CreateTexture(_name)); }

CTexture::CTexture()
    : m_material(1.0f)
{
    flags.bLoaded = false;
    flags.bUser = false;
    flags.MemoryUsage = 0;
}

CTexture::~CTexture()
{
    Unload();
    if (RImplementation.Resources)
        RImplementation.Resources->_DeleteTexture(this);
}

void CTexture::desc_update()
{
    desc_cache = nvrhiTexture.Get();
    if (nvrhiTexture)
    {
        const auto& d = nvrhiTexture->getDesc();
        m_width = d.width;
        m_height = d.height;
    }
    else
    {
        m_width = 0;
        m_height = 0;
    }
}

void CTexture::PostLoad() {}

void CTexture::Preload()
{
    m_bumpmap = TextureDescr.GetBumpName(cName);
    m_material = TextureDescr.GetMaterial(cName);
    m_metallic = TextureDescr.GetMetallicName(cName);
    m_roughness = TextureDescr.GetRoughnessName(cName);
    m_ao = TextureDescr.GetAOName(cName);
    m_parallax = TextureDescr.GetParallaxName(cName);
}

void CTexture::Load()
{
    flags.bLoaded = true;
    desc_cache = nullptr;
    if (nvrhiTexture)
        return;

    flags.bUser = false;
    flags.MemoryUsage = 0;
    if (0 == xr_stricmp(cName.c_str(), "$null"))
        return;
    if (0 == strncmp(cName.c_str(), "$user$", sizeof("$user$") - 1))
    {
        flags.bUser = true;
        return;
    }

    ZoneScoped;

    Preload();

    resources::TextureManager* textures = ManagedTextures();
    if (!textures)
    {
        flags.bLoaded = false;
        return;
    }

    ReleaseManagedTexture();
    m_managedTexture = textures->LoadTexture(cName.c_str(), TextureColorSpace::Linear);
    nvrhiTexture = textures->GetNVRHITexture(m_managedTexture);
    if (const resources::TextureMetadata* metadata = textures->GetMetadata(m_managedTexture))
        flags.MemoryUsage = u32(std::min(metadata->memoryUsed, kMaxReportedTextureMemory));
    desc_update();

    PostLoad();
}

void CTexture::Unload()
{
    ZoneScoped;
    flags.bLoaded = FALSE;
    nvrhiTexture = nullptr;
    desc_cache = nullptr;
    ReleaseManagedTexture();
}

void CTexture::ReleaseManagedTexture()
{
    if (!m_managedTexture.IsValid())
        return;
    if (resources::TextureManager* textures = ManagedTextures())
        textures->Release(m_managedTexture);
    m_managedTexture = TextureHandle();
}

ImTextureID CTexture::GetImTextureID()
{
    if (!flags.bLoaded)
        Load();
    return reinterpret_cast<ImTextureID>(nvrhiTexture.Get());
}
}
