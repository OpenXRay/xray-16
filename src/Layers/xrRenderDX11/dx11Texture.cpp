#include "stdafx.h"
#pragma hdrstop

#include <DirectXTex.h>

// FrameGraph texture loading integration
#include "Layers/xrRender/RenderContext/RenderDevice.h"          // RenderDevice
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"   // FGResourceManager (renamed from ModernResourceManager)
#include "Layers/xrRender/ResourceManager/TextureManager.h"      // TextureManager
#include <nvrhi/nvrhi.h>                                         // getNativeObject

using namespace xray::render::fg;
using namespace xray::render::resources;

ENGINE_API bool is_enough_address_space_available();

namespace xray::render::RENDER_NAMESPACE
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

int get_texture_load_lod(LPCSTR fn)
{
    CInifile::Sect& sect = pSettings->r_section("reduce_lod_texture_list");
    auto it_ = sect.Data.cbegin();
    auto it_e_ = sect.Data.cend();

    static bool enough_address_space_available = is_enough_address_space_available();

    auto it = it_;
    auto it_e = it_e_;

    for (; it != it_e; ++it)
    {
        if (strstr(fn, it->first.c_str()))
        {
            if (psTextureLOD < 1)
            {
                if (enough_address_space_available)
                    return 0;
                else
                    return 1;
            }
            else if (psTextureLOD < 3)
                return 1;
            else
                return 2;
        }
    }

    if (psTextureLOD < 2)
    {
        //if (enough_address_space_available)
        return 0;
        //else
        //    return 1;
    }
    else if (psTextureLOD < 4)
        return 1;
    else
        return 2;
}

u32 calc_texture_size(int lod, u32 mip_cnt, size_t orig_size)
{
    if (1 == mip_cnt)
        return orig_size;

    int _lod = lod;
    float res = float(orig_size);

    while (_lod > 0)
    {
        --_lod;
        res -= res / 1.333f;
    }
    return iFloor(res);
}

//////////////////////////////////////////////////////////////////////
// Utility pack
//////////////////////////////////////////////////////////////////////

IC void Reduce(size_t& w, size_t& h, size_t& l, int skip)
{
    while ((l > 1) && skip)
    {
        w /= 2;
        h /= 2;
        l -= 1;

        skip--;
    }
    if (w < 1)
        w = 1;
    if (h < 1)
        h = 1;
}

ID3DBaseTexture* CRender::texture_load(LPCSTR fRName, u32& ret_msize)
{
    ret_msize = 0;
    R_ASSERT1_CURE(fRName && fRName[0], { return nullptr; });

    if (!m_renderDevice || !m_renderDevice->GetFGResourceManager())
        return nullptr;


    // Fix texture name (remove extension)
    string_path fname;
    xr_strcpy(fname, fRName);
    fix_texture_name(fname);

    // Build VFS path for DDS texture
    string_path vfsPath;
    xr_sprintf(vfsPath, "$game_textures$\\%s", fname);

    Msg("* [FrameGraph] Loading texture through TextureManager: %s", fname);

    // Load through FGResourceManager
    FGResourceManager* resourceMgr = m_renderDevice->GetFGResourceManager();
    TextureManager* texManager = resourceMgr->GetTextureManager();

    // Load texture and get handle
    TextureHandle handle = texManager->LoadTexture(fRName, TexturePriority::High);
    if (!handle.IsValid())
    {
        Msg("! [FrameGraph] Failed to load texture through TextureManager: %s", fname);
        return nullptr; // Modern path failed, return null (don't fallback to legacy if FrameGraph is on)
    }

    // Get NVRHI texture
    nvrhi::ITexture* nvrhiTexture = texManager->GetNVRHITexture(handle);
    if (!nvrhiTexture)
    {
        Msg("! [FrameGraph] Failed to get NVRHI texture: %s", fname);
        return nullptr;
    }

    ID3D11Texture2D* d3d11Texture = static_cast<ID3D11Texture2D*>(
        nvrhiTexture->getNativeObject(nvrhi::ObjectTypes::D3D11_Resource).pointer
    );

    if (!d3d11Texture)
    {
        return nullptr;
    }

    const TextureMetadata* meta = texManager->GetMetadata(handle);
    if (meta)
    {
        ret_msize = (u32)meta->memoryUsed;
    }

    Msg("* [FrameGraph] Successfully loaded texture: %s (NVRHI-owned)", fname);

    // CRITICAL: AddRef() to give CTexture its own COM reference
    d3d11Texture->AddRef();

    return d3d11Texture;
}
} // namespace xray::render::RENDER_NAMESPACE
