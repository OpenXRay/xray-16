#include "stdafx.h"
#pragma hdrstop

#include "xrCore/Threading/ParallelForEach.hpp"

#include "ResourceManager.h"
#include "Layers/xrRender/tss.h"
#include "Layers/xrRenderDX11/Blender.h"
#include "Layers/xrRenderDX11/Blender_Recorder.h"

#include "Layers/xrRenderDX11/Blender_CLSID.h"
#include "Layers/xrRenderDX11/blenders/blender_deffer_aref.h"
#include "Layers/xrRenderDX11/blenders/blender_deffer_model.h"
#include "Layers/xrRenderDX11/blenders/Blender_Vertex_aref.h"
#include "Layers/xrRenderDX11/blenders/Blender_default_aref.h"
#include "Layers/xrRenderDX11/blenders/Blender_tree.h"
#include "Layers/xrRenderDX11/blenders/Blender_detail_still.h"
#include "Layers/xrRenderDX11/blenders/Blender_Model.h"
#include "Layers/xrRenderDX11/blenders/Blender_Model_EbB.h"
#include "Layers/xrRenderDX11/blenders/Blender_Screen_SET.h"
#include "Layers/xrRenderDX11/blenders/Blender_Particle.h"

namespace xray::render::fg
{
void fix_texture_name(pstr fn);


IBlender* CResourceManager::_FindBlender(LPCSTR Name)
{
    if (!(Name && Name[0]))
        return nullptr;

    pstr N = pstr(Name);
    map_Blender::iterator I = m_blenders.find(N);
    if (I == m_blenders.end())
        return nullptr;
    else
        return I->second;
}

static CResourceManager::BlendMode TokenToBlendMode(u32 tokenID)
{
    using BlendMode = CResourceManager::BlendMode;
    switch (tokenID)
    {
    case 0: return BlendMode::Opaque;
    case 1: return BlendMode::AlphaBlend;
    case 2: return BlendMode::Additive;
    case 3: return BlendMode::Multiply;
    case 4: return BlendMode::Multiply2X;
    case 5: return BlendMode::Additive;
    default: return BlendMode::AlphaBlend;
    }
}

bool CResourceManager::GetBlenderProperties(LPCSTR shaderName, BlenderProperties& outProps)
{
    IBlender* B = _FindBlender(shaderName);
    if (!B)
        return false;

    const CLASS_ID cls = B->getDescription().CLS;
    outProps = {};
    outProps.strictB2F = B->IsStrictSorting();

    if (cls == B_DEFAULT_AREF || cls == B_VERT_AREF)
    {
        auto* b = (cls == B_DEFAULT_AREF)
            ? static_cast<CBlender_deffer_aref*>(B)
            : reinterpret_cast<CBlender_deffer_aref*>(static_cast<CBlender_Vertex_aref*>(B));
        outProps.alphaRef = b->oAREF.value;
        if (b->oBlend.value)
            outProps.blendMode = BlendMode::AlphaBlend;
        else if (b->oAREF.value > 0)
            outProps.blendMode = BlendMode::AlphaTest;
        else
            outProps.blendMode = BlendMode::Opaque;
        outProps.writesDepth = (outProps.blendMode != BlendMode::AlphaBlend);
        return true;
    }
    if (cls == B_TREE)
    {
        auto* b = static_cast<CBlender_Tree*>(B);
        outProps.blendMode = b->oBlend.value ? BlendMode::AlphaTest : BlendMode::Opaque;
        outProps.alphaRef = b->oBlend.value ? 200 : 0;
        outProps.writesDepth = true;
        return true;
    }
    if (cls == B_DETAIL)
    {
        auto* b = static_cast<CBlender_Detail_Still*>(B);
        outProps.blendMode = b->oBlend.value ? BlendMode::AlphaBlend : BlendMode::AlphaTest;
        outProps.alphaRef = 200;
        outProps.writesDepth = !b->oBlend.value;
        return true;
    }
    if (cls == B_MODEL)
    {
        auto* b = static_cast<CBlender_Model*>(B);
        outProps.alphaRef = b->oAREF.value;
        if (b->oBlend.value)
            outProps.blendMode = BlendMode::AlphaBlend;
        else if (b->oAREF.value > 0)
            outProps.blendMode = BlendMode::AlphaTest;
        else
            outProps.blendMode = BlendMode::Opaque;
        outProps.writesDepth = (outProps.blendMode != BlendMode::AlphaBlend);
        return true;
    }
    if (cls == B_MODEL_EbB)
    {
        auto* b = static_cast<CBlender_Model_EbB*>(B);
        outProps.blendMode = b->oBlend.value ? BlendMode::AlphaBlend : BlendMode::Opaque;
        outProps.writesDepth = !b->oBlend.value;
        return true;
    }
    if (cls == B_SCREEN_SET)
    {
        auto* b = static_cast<CBlender_Screen_SET*>(B);
        outProps.blendMode = TokenToBlendMode(b->oBlend.IDselected);
        outProps.alphaRef = b->oAREF.value;
        outProps.writesDepth = (outProps.blendMode == BlendMode::Opaque);
        return true;
    }
    if (cls == B_PARTICLE)
    {
        auto* b = static_cast<CBlender_Particle*>(B);
        outProps.blendMode = TokenToBlendMode(b->oBlend.IDselected);
        outProps.alphaRef = b->oAREF.value;
        outProps.writesDepth = false;
        return true;
    }
    if (cls == B_DEFAULT || cls == B_VERT || cls == B_LmBmmD || cls == B_LaEmB || cls == B_LmEbB ||
        cls == B_B || cls == B_BmmD || cls == B_SCREEN_GRAY || cls == B_LIGHT || cls == B_BLUR ||
        cls == B_SHADOW_TEX || cls == B_SHADOW_WORLD || cls == B_EDITOR_WIRE || cls == B_EDITOR_SEL)
    {
        outProps.blendMode = BlendMode::Opaque;
        outProps.writesDepth = true;
        return true;
    }

    return false;
}

void CResourceManager::ED_UpdateBlender(LPCSTR Name, IBlender* data)
{
    pstr N = pstr(Name);
    map_Blender::iterator I = m_blenders.find(N);
    if (I != m_blenders.end())
    {
        R_ASSERT(data->getDescription().CLS == I->second->getDescription().CLS);
        xr_delete(I->second);
        I->second = data;
    }
    else
    {
        m_blenders.emplace(xr_strdup(Name), data);
    }
}

void CResourceManager::_ParseList(sh_list& dest, LPCSTR names)
{
    if (nullptr == names || 0 == names[0])
        names = "$null";

    dest.clear();
    char* P = (char*)names;
    svector<char, 128> N;

    while (*P)
    {
        if (*P == ',')
        {
            N.push_back(0);
            xr_strlwr(N.begin());

            fix_texture_name(N.begin());
            dest.push_back(N.begin());
            N.clear();
        }
        else
        {
            N.push_back(*P);
        }
        P++;
    }
    if (N.size())
    {
        N.push_back(0);
        xr_strlwr(N.begin());

        fix_texture_name(N.begin());
        dest.push_back(N.begin());
    }
}

ShaderElement* CResourceManager::_CreateElement(ShaderElement&& S)
{
    if (S.passes.empty())
        return nullptr;

    for (ShaderElement* elem : v_elements)
        if (S.equal(*elem))
            return elem;

    ShaderElement* N = v_elements.emplace_back(xr_new<ShaderElement>(std::move(S)));
    N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    return N;
}

void CResourceManager::_DeleteElement(const ShaderElement* S)
{
    if (0 == (S->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_elements, S))
        return;
    Msg("! ERROR: Failed to find compiled 'shader-element'");
}



IReader* open_shader(pcstr shader)
{
    string_path shaderPath;

    FS.update_path(shaderPath, "$game_shaders$", RImplementation.getShaderPath());
    xr_strcat(shaderPath, shader);

    return FS.r_open(shaderPath);
}

void CResourceManager::CompatibilityCheck()
{
    ZoneScoped;

    {
        IReader* skinh = open_shader("skin.h");
        R_ASSERT3(skinh, "Can't open shader", "skin.h");
        xr_string str(static_cast<pcstr>(skinh->pointer()), skinh->length());

        bool hq_skinning = true;
        const auto check = [&](cpcstr searchBegin, cpcstr searchEnd) -> bool
        {
            cpcstr begin = strstr(str.c_str(), searchBegin);
            if (!begin)
                return false;

            cpcstr end = strstr(begin, searchEnd);
            if (!end)
                return false;

            str.assign(begin, end);
            pcstr ptr = str.data();

            if ((ptr = strstr(ptr, "12.")))
            {
                if ((ptr = strstr(ptr, "/")))
                    if (strstr(ptr, "32768."))
                    {
                        hq_skinning = false;
                        return true;
                    }
            }
            return false;
        };
        if (!check("u_position", "sbones_array"))
        {
            check("skinning_pos", "skinning_0");
        }
        RImplementation.m_hq_skinning = hq_skinning;
        FS.r_close(skinh);
    }
}

Shader* CResourceManager::Create(IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    return nullptr;
}

Shader* CResourceManager::Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    return nullptr;
}

void CResourceManager::Delete(const Shader* S)
{
    if (0 == (S->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_shaders, S))
        return;
    Msg("! ERROR: Failed to find complete shader");
}

void CResourceManager::DeferredUpload()
{
    if (!Device.b_is_Ready)
        return;

    ZoneScoped;

#if defined(USE_DX11)
    for (auto& texture : m_textures)
        texture.second->Load();
#elif defined(USE_OGL)
    for (auto& texture : m_textures)
        texture.second->Load();
#else
#   error No graphics API selected or enabled!
#endif
}

void CResourceManager::DeferredUnload()
{
    if (!Device.b_is_Ready)
        return;

    ZoneScoped;

#if defined(USE_DX11)
    xr_parallel_for_each(m_textures, [&](auto m_tex) { m_tex.second->Unload(); });
#elif defined(USE_OGL)
    for (auto& texture : m_textures)
        texture.second->Unload();
#else
#   error No graphics API selected or enabled!
#endif
}

#ifdef _EDITOR
void CResourceManager::ED_UpdateTextures(AStringVec* names)
{
    if (names)
    {
        for (u32 nid = 0; nid < names->size(); nid++)
        {
            map_TextureIt I = m_textures.find((*names)[nid].c_str());
            if (I != m_textures.end())
                I->second->Unload();
        }
    }
    else
    {
        for (map_TextureIt t = m_textures.begin(); t != m_textures.end(); t++)
            t->second->Unload();
    }

}
#endif

void CResourceManager::_GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
    m_base = c_base = m_lmaps = c_lmaps = 0;

    map_Texture::iterator I = m_textures.begin();
    map_Texture::iterator E = m_textures.end();
    for (; I != E; ++I)
    {
        u32 m = I->second->flags.MemoryUsage;
        if (strstr(I->first, "lmap"))
        {
            c_lmaps++;
            m_lmaps += m;
        }
        else
        {
            c_base++;
            m_base += m;
        }
    }
}
void CResourceManager::_DumpMemoryUsage()
{
    xr_multimap<u32, std::pair<u32, shared_str>> mtex;

    {
        map_Texture::iterator I = m_textures.begin();
        map_Texture::iterator E = m_textures.end();
        for (; I != E; ++I)
        {
            u32 m = I->second->flags.MemoryUsage;
            shared_str n = I->second->cName;
            mtex.emplace(m, std::make_pair(I->second->ref_count.load(), n));
        }
    }

    {
        xr_multimap<u32, std::pair<u32, shared_str>>::iterator I = mtex.begin();
        xr_multimap<u32, std::pair<u32, shared_str>>::iterator E = mtex.end();
        for (; I != E; ++I)
            Msg("* %4.1f : [%4d] %s", float(I->first) / 1024.f, I->second.first, I->second.second.c_str());
    }
}

void CResourceManager::Evict()
{
}
}
