#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_CLSID.h"

namespace xray::render::fg
{
void CResourceManager::OnDeviceDestroy(BOOL)
{
    if (Device.b_is_Ready)
        return;
    TextureDescr.UnLoad();

    // Release blenders
    for (auto b = m_blenders.begin(); b != m_blenders.end(); ++b)
    {
        xr_free((char*&)b->first);
        IBlender::Destroy(b->second);
    }
    m_blenders.clear();

    // destroy TD
    for (auto _t = m_td.begin(); _t != m_td.end(); ++_t)
    {
        xr_free((char*&)_t->first);
        xr_free((char*&)_t->second.T);
    }
    m_td.clear();
}

void CResourceManager::OnDeviceCreate(IReader* F)
{
    if (!Device.b_is_Ready)
        return;

    ZoneScoped;
    string256 name;

    IReader* fs = nullptr;
    // Load blenders
    fs = F->open_chunk(2);
    if (fs)
    {
        ZoneScopedN("Load blenders");
        IReader* chunk = nullptr;
        int chunk_id = 0;

        while ((chunk = fs->open_chunk(chunk_id)) != nullptr)
        {
            CBlender_DESC desc;
            chunk->r(&desc, sizeof(desc));
            if (IBlender* B = IBlender::Create(desc.CLS))
            {
#ifndef MASTER_GOLD
                if (B->getDescription().version != desc.version)
                {
                    Msg("! Version conflict in shader '%s'", desc.cName);
                }
#endif
                chunk->seek(0);
                B->Load(*chunk, desc.version);

                // XXX: SDK must prevent the duplication,
                // the engine should just work
                auto I = m_blenders.emplace(xr_strdup(desc.cName), B);
                R_ASSERT2(I.second, "shader.xr - found duplicate name!!!");
            }
#ifndef MASTER_GOLD
            else
            {
                Msg("! Renderer doesn't support blender '%s'", desc.cName);
            }
#endif
            chunk->close();
            chunk_id += 1;
        }
        fs->close();
    }

    TextureDescr.Load();
}

void CResourceManager::OnDeviceCreate(LPCSTR shName)
{
    ZoneScoped;

#ifdef _EDITOR
    if (!FS.exist(shName))
        return;
#endif

    // Check if file is compressed already
    string32 ID = "shENGINE";
    string32 id;
    IReader* F = FS.r_open(shName);
    R_ASSERT2(F, shName);
    F->r(&id, 8);
    if (0 == strncmp(id, ID, 8))
    {
        FATAL("Unsupported blender library. Compressed?");
    }
    OnDeviceCreate(F);
    FS.r_close(F);
}

void CResourceManager::StoreNecessaryTextures()
{
    if (!m_necessary.empty())
        return;

    m_necessary.reserve(m_textures.size());
    for (auto& mtex : m_textures)
    {
        LPCSTR texture_name = mtex.first;
        if (strstr(texture_name, DELIMITER "levels" DELIMITER))
            continue;
        if (!strchr(texture_name, _DELIMITER))
            continue;

        ref_texture T;
        T.create(texture_name);
        m_necessary.push_back(T);
    }
}

void CResourceManager::DestroyNecessaryTextures() { m_necessary.clear(); }
} // namespace xray::render::fg
