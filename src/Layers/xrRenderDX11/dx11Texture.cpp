#include "stdafx.h"
#pragma hdrstop

#include <DirectXTex.h>

// Week 6: FrameGraph texture loading integration
#include "Layers/xrRender/xrRender_console.h"                    // ps_r4_use_framegraph
#include "Layers/xrRender/RenderContext/RenderDevice.h"          // RenderDevice
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"   // FGResourceManager (renamed from ModernResourceManager)
#include "Layers/xrRender/ResourceManager/TextureManager.h"      // TextureManager
#include <nvrhi/nvrhi.h>                                         // getNativeObject

extern ENGINE_API int ps_r4_use_framegraph;

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

    ENGINE_API bool is_enough_address_space_available();
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

    // ═══════════════════════════════════════════════════
    //  WEEK 6: FRAMEGRAPH MODE - ROUTE THROUGH TEXTUREMANAGER
    // ═══════════════════════════════════════════════════
    if (ps_r4_use_framegraph && m_renderDevice && m_renderDevice->GetFGResourceManager())
    {
        using namespace xray::render::ng;
        using namespace xray::render::resources;

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
            // Fall back to legacy path
            goto _LEGACY_PATH;
        }

        // Get NVRHI texture
        nvrhi::ITexture* nvrhiTexture = texManager->GetNVRHITexture(handle);
        if (!nvrhiTexture)
        {
            Msg("! [FrameGraph] Failed to get NVRHI texture: %s", fname);
            // Fall back to legacy path
            goto _LEGACY_PATH;
        }

        // Extract D3D11 texture from NVRHI (for compatibility with legacy code)
        ID3D11Texture2D* d3d11Texture = static_cast<ID3D11Texture2D*>(
            nvrhiTexture->getNativeObject(nvrhi::ObjectTypes::D3D11_Resource).pointer
        );

        if (!d3d11Texture)
        {
            Msg("! [FrameGraph] Failed to get D3D11 texture from NVRHI: %s", fname);
            // Fall back to legacy path
            goto _LEGACY_PATH;
        }

        // Get texture metadata for memory size
        const TextureMetadata* meta = texManager->GetMetadata(handle);
        if (meta)
        {
            ret_msize = (u32)meta->memoryUsed;
        }

        Msg("* [FrameGraph] Successfully loaded texture: %s (NVRHI-owned)", fname);

        // Return D3D11 texture (NVRHI still owns it, but we expose D3D11 interface for legacy code)
        return d3d11Texture;
    }

_LEGACY_PATH:
    // ═══════════════════════════════════════════════════
    //  LEGACY PATH: Original D3D11 texture loading
    // ═══════════════════════════════════════════════════

    ID3DBaseTexture* pTexture2D{};
    string_path fn;
    {
        // make file name
        string_path fname;
        xr_strcpy(fname, fRName);
        fix_texture_name(fname);

        // Call to FS.exist WRITES to fn !

        if (!FS.exist(fn, "$game_textures$", fname, ".dds") && strstr(fname, "_bump"))
        {
            Msg("! Fallback to default bump map: %s", fname);
            if (strstr(fname, "_bump#"))
                R_ASSERT1_CURE(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump#", ".dds"), return nullptr);
            else
                R_ASSERT1_CURE(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump", ".dds"), return nullptr);
        }
        else
        {
            bool exist = false;

            for (cpcstr folder : { "$level$", "$game_saves$", "$game_textures$" })
            {
                exist = FS.exist(fn, folder, fname, ".dds");
                if (exist)
                    break;
            }

            if (!exist)
            {
                Msg("! Can't find texture '%s'", fname);
                R_ASSERT1_CURE(FS.exist(fn, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"), return nullptr);
            }
        }
    }

    // Load and get header
    IReader* S = FS.r_open(fn);
    R_ASSERT3_CURE(S, "Can't open texture", fn, { return nullptr; });

    size_t img_size = S->length();
#ifdef DEBUG
    Msg("* Loaded: %s[%zu]", fn, img_size);
#endif // DEBUG

    DirectX::DDS_FLAGS dds_flags{ DirectX::DDS_FLAGS_PERMISSIVE };

    for (int i = 1; i <= 3; ++i) // 3 attempts
    {
        DirectX::TexMetadata IMG;
        DirectX::ScratchImage texture;
        auto hresult = LoadFromDDSMemory(static_cast<u8*>(S->pointer()), img_size, dds_flags, &IMG, texture);

        R_ASSERT3_CURE(SUCCEEDED(hresult), "Failed to load texture from memory", fn,
        {
            FS.r_close(S);
            return nullptr;
        });

        // Check for LMAP and compress if needed
        xr_strlwr(fn);

        const int img_loaded_lod = get_texture_load_lod(fn);

        size_t mip_lod = 0;
        if (img_loaded_lod && !IMG.IsCubemap())
        {
            const auto old_mipmap_cnt = IMG.mipLevels;
            Reduce(IMG.width, IMG.height, IMG.mipLevels, img_loaded_lod);
            mip_lod = old_mipmap_cnt - IMG.mipLevels;
        }

        // DirectX requires compressed texture size to be
        // a multiple of 4. Make sure to meet this requirement.
        if (DirectX::IsCompressed(IMG.format))
        {
            IMG.width = (IMG.width + 3u) & ~0x3u;
            IMG.height = (IMG.height + 3u) & ~0x3u;
        }

        hresult = CreateTextureEx(HW.pDevice, texture.GetImages() + mip_lod, texture.GetImageCount(), IMG,
            D3D_USAGE_IMMUTABLE, D3D_BIND_SHADER_RESOURCE, 0, IMG.miscFlags, DirectX::CREATETEX_DEFAULT,
            &pTexture2D);

        if (SUCCEEDED(hresult))
        {
            // OK
            ret_msize = calc_texture_size(img_loaded_lod, IMG.mipLevels, img_size);
            break;
        }

        if (i == 1)
            dds_flags |= DirectX::DDS_FLAGS::DDS_FLAGS_NO_16BPP; // System isn't WDDM 1.2 compliant
        else if (i == 2)
            dds_flags |= DirectX::DDS_FLAGS::DDS_FLAGS_FORCE_RGB; // Not even WDDM 1.1 compliant
        else if (i == 3)
            Msg("! Could not load texture [%s] after %d attempts", fn, i);
        else
        {
            NODEFAULT;
        }
    }

    FS.r_close(S);
    return pTexture2D;
}
} // namespace xray::render::RENDER_NAMESPACE
