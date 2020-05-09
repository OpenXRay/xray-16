// TextureManager.cpp: implementation of the CResourceManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include <tbb/parallel_for_each.h>

#include "ResourceManager.h"
#include "tss.h"
#include "blenders/Blender.h"
#include "blenders/Blender_Recorder.h"

//	Already defined in Texture.cpp
void fix_texture_name(LPSTR fn);
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
template <class T>
bool reclaim(xr_vector<T*>& vec, const T* ptr)
{
    auto it = vec.begin();
    auto end = vec.end();
    for (; it != end; ++it)
        if (*it == ptr)
        {
            vec.erase(it);
            return true;
        }
    return false;
}

//--------------------------------------------------------------------------------------------------------------
IBlender* CResourceManager::_GetBlender(LPCSTR Name)
{
    R_ASSERT(Name && Name[0]);

    LPSTR N = LPSTR(Name);
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

    LPSTR N = LPSTR(Name);
    map_Blender::iterator I = m_blenders.find(N);
    if (I == m_blenders.end())
        return nullptr;
    else
        return I->second;
}

void CResourceManager::ED_UpdateBlender(LPCSTR Name, IBlender* data)
{
    LPSTR N = LPSTR(Name);
    map_Blender::iterator I = m_blenders.find(N);
    if (I != m_blenders.end())
    {
        R_ASSERT(data->getDescription().CLS == I->second->getDescription().CLS);
        xr_delete(I->second);
        I->second = data;
    }
    else
    {
        m_blenders.insert(std::make_pair(xr_strdup(Name), data));
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

ShaderElement* CResourceManager::_CreateElement(ShaderElement& S)
{
    if (S.passes.empty())
        return nullptr;

    // Search equal in shaders array
    for (u32 it = 0; it < v_elements.size(); it++)
        if (S.equal(*(v_elements[it])))
            return v_elements[it];

    // Create _new_ entry
    ShaderElement* N = v_elements.emplace_back(new ShaderElement(S));
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
    C.bEditor = FALSE;
    C.bDetail = FALSE;
#ifdef _EDITOR
    if (!C.BT)
    {
        ELog.Msg(mtError, "Can't find shader '%s'", s_shader);
        return 0;
    }
    C.bEditor = TRUE;
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
        S.E[SE_R1_NORMAL_HQ] = _CreateElement(E);
    }

    // Compile element	(LOD1)
    {
        C.iElement = SE_R1_NORMAL_LQ;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_NORMAL_LQ] = _CreateElement(E);
    }

    // Compile element
    {
        C.iElement = SE_R1_LPOINT;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_LPOINT] = _CreateElement(E);
    }

    // Compile element
    {
        C.iElement = SE_R1_LSPOT;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_LSPOT] = _CreateElement(E);
    }

    // Compile element
    {
        C.iElement = SE_R1_LMODELS;
        C.bDetail = TRUE; //.$$$ HACK :)
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[SE_R1_LMODELS] = _CreateElement(E);
    }

    // Compile element
    {
        C.iElement = 5;
        C.bDetail = FALSE;
        ShaderElement E;
        C._cpp_Compile(&E);
        S.E[5] = _CreateElement(E);
    }

    // Search equal in shaders array
    for (u32 it = 0; it < v_shaders.size(); it++)
        if (S.equal(v_shaders[it]))
            return v_shaders[it];

    // Create _new_ entry
    Shader* N = v_shaders.emplace_back(new Shader(S));
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

    FS.update_path(shaderPath, "$game_shaders$", GEnv.Render->getShaderPath());
    xr_strcat(shaderPath, shader);

    return FS.r_open(shaderPath);
}

void CResourceManager::CompatibilityCheck()
{
    // Check Shoker HQ Geometry Fix support
    {
        IReader* skinh = open_shader("skin.h");
        R_ASSERT3(skinh, "Can't open shader", "skin.h");
        // search for (12.f / 32768.f)
        bool hq_skinning = true;
        do
        {
            pcstr begin = strstr((cpcstr)skinh->pointer(), "u_position");
            if (!begin)
                break;

            cpcstr end = strstr(begin, "sbones_array");
            if (!end)
                break;

            xr_string str(begin, end);
            pcstr ptr = str.data();

            if ((ptr = strstr(ptr, "12.f")))    // 12.f
                if ((ptr = strstr(ptr, "/")))   // /
                    if (strstr(ptr, "32768.f")) // 32768.f
                        hq_skinning = false;    // found
            break;
        } while (false);
        RImplementation.m_hq_skinning = hq_skinning;
        FS.r_close(skinh);
    }
#if RENDER != R_R1
    // Check shadow cascades type (old SOC/CS or new COP)
    if (psDeviceFlags.test(rsR2))
    {
        // Check for new cascades support on R2
        IReader* accumSunNearCascade = open_shader("accum_sun_near_cascade.ps");
        RImplementation.o.oldshadowcascades = !accumSunNearCascade;
        ps_r2_ls_flags_ext.set(R2FLAGEXT_SUN_OLD, !accumSunNearCascade);
        FS.r_close(accumSunNearCascade);
    }
    else if (!psDeviceFlags.test(rsR1))
    {
        IReader* accumSunNear = open_shader("accum_sun_near.ps");
        R_ASSERT3(accumSunNear, "Can't open shader", "accum_sun_near.ps");
        bool oldCascades = false;
        do
        {
            pcstr begin = strstr((cpcstr)accumSunNear->pointer(), "float4");
            if (!begin)
                break;

            begin = strstr(begin, "main");
            if (!begin)
                break;

            cpcstr end = strstr(begin, "SV_Target");
            if (!end)
                break;

            xr_string str(begin, end);
            pcstr ptr = str.data();

            if (strstr(ptr, "v2p_TL2uv"))
            {
                oldCascades = true;
            }
            else if (strstr(ptr, "v2p_volume"))
            {
                oldCascades = false;
            }
        } while (false);
        RImplementation.o.oldshadowcascades = oldCascades;
        ps_r2_ls_flags_ext.set(R2FLAGEXT_SUN_OLD, oldCascades);
        FS.r_close(accumSunNear);
    }
#endif
}

Shader* CResourceManager::Create(IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    if (GEnv.isDedicatedServer)
        return nullptr;

    return _cpp_Create(B, s_shader, s_textures, s_constants, s_matrices);
}

Shader* CResourceManager::Create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    if (!GEnv.isDedicatedServer)
    {
//	TODO: DX10: When all shaders are ready switch to common path
#if defined(USE_DX10) || defined(USE_DX11)
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
#else //	USE_DX10
#ifndef _EDITOR
        if (_lua_HasShader(s_shader))
            return _lua_Create(s_shader, s_textures);
        else
#endif
            return _cpp_Create(s_shader, s_textures, s_constants, s_matrices);
#endif //	USE_DX10
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
    if (!RDEVICE.b_is_Ready)
        return;

#ifndef USE_OGL
    tbb::parallel_for_each(m_textures, [&](auto m_tex) { m_tex.second->Load(); });
#else
    for (auto& texture : m_textures)
        texture.second->Load();
#endif
}

void CResourceManager::DeferredUnload()
{
    if (!RDEVICE.b_is_Ready)
        return;

#ifndef USE_OGL
    tbb::parallel_for_each(m_textures, [&](auto m_tex) { m_tex.second->Unload(); });
#else
    for (auto& texture : m_textures)
        texture.second->Unload();
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
            mtex.insert(std::make_pair(m, std::make_pair(I->second->dwReference, n)));
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
//	TODO: DX10: check if we really need this method
#if !defined(USE_DX10) && !defined(USE_DX11) && !defined(USE_OGL)
    CHK_DX(HW.pDevice->EvictManagedResources());
#endif
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
