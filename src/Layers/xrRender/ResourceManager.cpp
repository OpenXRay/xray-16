// TextureManager.cpp: implementation of the CResourceManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "xrCore/Threading/ParallelForEach.hpp"

#include "ResourceManager.h"
#include "tss.h"
#include "Blender.h"
#include "Blender_Recorder.h"

#include "Blender_CLSID.h"
#include "blenders/blender_deffer_aref.h"
#include "blenders/blender_deffer_model.h"
#include "blenders/Blender_Vertex_aref.h"
#include "blenders/Blender_default_aref.h"
#include "blenders/Blender_tree.h"
#include "blenders/Blender_detail_still.h"
#include "blenders/Blender_Model.h"
#include "blenders/Blender_Model_EbB.h"
#include "blenders/Blender_Screen_SET.h"
#include "blenders/Blender_Particle.h"

namespace xray::render::RENDER_NAMESPACE
{
//	Already defined in Texture.cpp
void fix_texture_name(pstr fn);
/*
void fix_texture_name(LPSTR fn)
{
    LPSTR _ext = strext(fn);
    if (_ext &&
        (0==xr_stricmp(_ext, ".tga") ||
        0==xr_stricmp(_ext, ".dds") ||
        0==xr_stricmp(_ext, ".bmp") ||
        0==xr_stricmp(_ext, ".ogm")))
        *_ext = 0;
}
*/

//--------------------------------------------------------------------------------------------------------------
IBlender* CResourceManager::_GetBlender(LPCSTR Name)
{
    R_ASSERT(Name && Name[0]);

    pstr N = pstr(Name);
    map_Blender::iterator I = m_blenders.find(N);

    if (I == m_blenders.end())
    {
        Msg("! Shader '%s' not found in library.", Name);
        return nullptr;
    }

    return I->second;
}

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

// Helper: Convert oBlend token ID to BlendMode
// Token IDs from Blender_Screen_SET/Blender_Particle: 0=SET, 1=BLEND, 2=ADD, 3=MUL, 4=MUL_2X, 5=ALPHA-ADD
static CResourceManager::BlendMode TokenToBlendMode(u32 tokenID)
{
    using BlendMode = CResourceManager::BlendMode;
    switch (tokenID)
    {
    case 0: return BlendMode::Opaque;       // SET
    case 1: return BlendMode::AlphaBlend;   // BLEND
    case 2: return BlendMode::Additive;     // ADD
    case 3: return BlendMode::Multiply;     // MUL
    case 4: return BlendMode::Multiply2X;   // MUL_2X
    case 5: return BlendMode::Additive;     // ALPHA-ADD
    default: return BlendMode::AlphaBlend;  // Unknown - assume blend
    }
}

bool CResourceManager::GetBlenderProperties(LPCSTR shaderName, BlenderProperties& outProps)
{
    IBlender* B = _FindBlender(shaderName);
    if (!B)
        return false;

    const CLASS_ID cls = B->getDescription().CLS;
    outProps = {};  // Reset to defaults

    // B_DEFAULT_AREF / B_VERT_AREF - alpha ref blenders (oAREF, oBlend)
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
    // B_TREE - tree blender (oBlend = leaves use alpha test)
    if (cls == B_TREE)
    {
        auto* b = static_cast<CBlender_Tree*>(B);
        outProps.blendMode = b->oBlend.value ? BlendMode::AlphaTest : BlendMode::Opaque;
        outProps.alphaRef = b->oBlend.value ? 200 : 0;
        outProps.writesDepth = true;
        return true;
    }
    // B_DETAIL - detail (always alpha test, oBlend controls additional blending)
    if (cls == B_DETAIL)
    {
        auto* b = static_cast<CBlender_Detail_Still*>(B);
        outProps.blendMode = b->oBlend.value ? BlendMode::AlphaBlend : BlendMode::AlphaTest;
        outProps.alphaRef = 200;
        outProps.writesDepth = !b->oBlend.value;
        return true;
    }
    // B_MODEL - model blender (oAREF, oBlend)
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
    // B_MODEL_EbB - model with env (oBlend)
    if (cls == B_MODEL_EbB)
    {
        auto* b = static_cast<CBlender_Model_EbB*>(B);
        outProps.blendMode = b->oBlend.value ? BlendMode::AlphaBlend : BlendMode::Opaque;
        outProps.writesDepth = !b->oBlend.value;
        return true;
    }
    // B_SCREEN_SET - screen effect (oBlend token, oAREF)
    if (cls == B_SCREEN_SET)
    {
        auto* b = static_cast<CBlender_Screen_SET*>(B);
        outProps.blendMode = TokenToBlendMode(b->oBlend.IDselected);
        outProps.alphaRef = b->oAREF.value;
        outProps.writesDepth = (outProps.blendMode == BlendMode::Opaque);
        return true;
    }
    // B_PARTICLE - particle (oBlend token, oAREF)
    if (cls == B_PARTICLE)
    {
        auto* b = static_cast<CBlender_Particle*>(B);
        outProps.blendMode = TokenToBlendMode(b->oBlend.IDselected);
        outProps.alphaRef = b->oAREF.value;
        outProps.writesDepth = false;  // Particles never write depth
        return true;
    }
    // Opaque blenders (no configurable alpha)
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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
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
            // flush
            N.push_back(0);
            xr_strlwr(N.begin());

            fix_texture_name(N.begin());
            //. andy			if (strext(N.begin())) *strext(N.begin())=0;
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
        // flush
        N.push_back(0);
        xr_strlwr(N.begin());

        fix_texture_name(N.begin());
        //. andy		if (strext(N.begin())) *strext(N.begin())=0;
        dest.push_back(N.begin());
    }
}

ShaderElement* CResourceManager::_CreateElement(ShaderElement&& S)
{
    if (S.passes.empty())
        return nullptr;

    // Search equal in shaders array
    for (ShaderElement* elem : v_elements)
        if (S.equal(*elem))
            return elem;

    // Create _new_ entry
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

Shader* CResourceManager::_cpp_Create(
    IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    CBlender_Compile C;
    Shader S;

    //.
    // if (strstr(s_shader,"transparent"))	__asm int 3;

    // Access to template
    C.BT = B;
    C.bFFP = RImplementation.o.ffp;
    C.bDetail = FALSE;
#ifdef _EDITOR
    if (!C.BT)
    {
        ELog.Msg(mtError, "Can't find shader '%s'", s_shader);
        return 0;
    }
    C.bFFP = true;
#else
    UNUSED(s_shader);
#endif

    // Parse names
    _ParseList(C.L_textures, s_textures);
    _ParseList(C.L_constants, s_constants);
    _ParseList(C.L_matrices, s_matrices);

    // Compile element	(LOD0 - HQ)
    {
        C.iElement = SE_R1_NORMAL_HQ;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_NORMAL_HQ] = _CreateElement(std::move(E));
    }

    // Compile element	(LOD1)
    {
        C.iElement = SE_R1_NORMAL_LQ;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_NORMAL_LQ] = _CreateElement(std::move(E));
    }

    // Compile element
    {
        C.iElement = SE_R1_LPOINT;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_LPOINT] = _CreateElement(std::move(E));
    }

    // Compile element
    {
        C.iElement = SE_R1_LSPOT;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_LSPOT] = _CreateElement(std::move(E));
    }

    // Compile element
    {
        C.iElement = SE_R1_LMODELS;
        C.bDetail = TRUE; //.$$$ HACK :)
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_LMODELS] = _CreateElement(std::move(E));
    }

    // Compile element
    {
        C.iElement = 5;
        C.bDetail = FALSE;
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[5] = _CreateElement(std::move(E));
    }

    // Search equal in shaders array
    for (u32 it = 0; it < v_shaders.size(); it++)
        if (S.equal(v_shaders[it]))
            return v_shaders[it];

    // Create _new_ entry
    Shader* N = v_shaders.emplace_back(xr_new<Shader>(std::move(S)));
    N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    return N;
}

Shader* CResourceManager::_cpp_Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    if (!GEnv.isDedicatedServer)
    {
        IBlender* pBlender = _GetBlender(s_shader ? s_shader : "null");
        if (!pBlender)
            return nullptr;
        return _cpp_Create(pBlender, s_shader, s_textures, s_constants, s_matrices);
    }
    return nullptr;
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

    // Check Shoker HQ Geometry Fix support
    {
        IReader* skinh = open_shader("skin.h");
        R_ASSERT3(skinh, "Can't open shader", "skin.h");
        xr_string str(static_cast<pcstr>(skinh->pointer()), skinh->length());

        bool hq_skinning = true;
        // search for (12.f / 32768.f)
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

            if ((ptr = strstr(ptr, "12.")))     // 12.f or 12.0
            {
                if ((ptr = strstr(ptr, "/")))   // /
                    if (strstr(ptr, "32768."))  // 32768.f or 32768.0
                    {
                        hq_skinning = false;    // found
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
    if (GEnv.isDedicatedServer)
        return nullptr;

    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
    {
        return nullptr;
    }

    return _cpp_Create(B, s_shader, s_textures, s_constants, s_matrices);
}

Shader* CResourceManager::Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    if (!GEnv.isDedicatedServer)
    {
        // D3D12: Skip legacy shader creation - use NVRHI ShaderLoader instead
        // Individual systems (UI, lensflares, etc.) handle DX12 path in their create() methods
        if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
        {
            return nullptr;  // No legacy shader object for DX12
        }

#if defined(USE_DX9)
        const bool useCppBlender = RImplementation.o.ffp && _GetBlender(s_shader);
        if (!useCppBlender && _lua_HasShader(s_shader))
            return _lua_Create(s_shader, s_textures);
        return _cpp_Create(s_shader, s_textures, s_constants, s_matrices);
#else // TODO: DX11: When all shaders are ready switch to common path
        if (_lua_HasShader(s_shader))
            return _lua_Create(s_shader, s_textures);
        else
        {
            Shader* pShader = _cpp_Create(s_shader, s_textures, s_constants, s_matrices);
            if (pShader)
                return pShader;
            else
            {
                if (_lua_HasShader("stub_default"))
                    return _lua_Create("stub_default", s_textures);
                else
                {
                    FATAL("Can't find stub_default.s");
                    return 0;
                }
            }
        }
#endif
    }
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
#elif defined(USE_OGL) // XXX: OGL: Set additional contexts for all worker threads?
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
#elif defined(USE_OGL) // XXX: OGL: Set additional contexts for all worker threads?
    for (auto& texture : m_textures)
        texture.second->Unload();
#else
#   error No graphics API selected or enabled!
#endif
}

#ifdef _EDITOR
void CResourceManager::ED_UpdateTextures(AStringVec* names)
{
    // 1. Unload
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

    // 2. Load
    // DeferredUpload	();
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

    // sort
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

    // dump
    {
        xr_multimap<u32, std::pair<u32, shared_str>>::iterator I = mtex.begin();
        xr_multimap<u32, std::pair<u32, shared_str>>::iterator E = mtex.end();
        for (; I != E; ++I)
            Msg("* %4.1f : [%4d] %s", float(I->first) / 1024.f, I->second.first, I->second.second.c_str());
    }
}

void CResourceManager::Evict()
{
    // TODO: DX11: check if we really need this method
}
/*
BOOL	CResourceManager::_GetDetailTexture(LPCSTR Name,LPCSTR& T, R_constant_setup* &CS)
{
    LPSTR N = LPSTR(Name);
    map_TD::iterator I = m_td.find	(N);
    if (I!=m_td.end())
    {
        T	= I->second.T;
        CS	= I->second.cs;
        return TRUE;
    } else {
        return FALSE;
    }
}*/
} // namespace xray::render::RENDER_NAMESPACE
