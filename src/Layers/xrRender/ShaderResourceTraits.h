#pragma once

#if defined(USE_DX10) || defined(USE_DX11)

#include "ResourceManager.h"

template <typename T>
struct ShaderTypeTraits;

#if defined(USE_DX10) || defined(USE_DX11)
template <>
struct ShaderTypeTraits<SGS>
{
    typedef CResourceManager::map_GS MapType;
    typedef ID3DGeometryShader DXIface;

    static inline const char* GetShaderExt() { return ".gs"; }
    static inline const char* GetCompilationTarget()
    {
#ifdef USE_DX10
        if (HW.pDevice1 == 0)
            return D3D10GetGeometryShaderProfile(HW.pDevice);
        else
            return "gs_4_1";
#endif
#ifdef USE_DX11
        if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
            return "gs_4_0";
        else if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_1)
            return "gs_4_1";
        else if (HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0)
            return "gs_5_0";
#endif
        NODEFAULT;
        return "gs_4_0";
    }
    static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
    {
        DXIface* gs = 0;
#ifdef USE_DX11
        R_CHK(HW.pDevice->CreateGeometryShader(buffer, size, 0, &gs));
#else
        R_CHK(HW.pDevice->CreateGeometryShader(buffer, size, &gs));
#endif       
        return gs;
    }

    static inline u32 GetShaderDest() { return RC_dest_geometry; }
};
#endif

#ifdef USE_DX11
template <>
struct ShaderTypeTraits<SHS>
{
    typedef CResourceManager::map_HS MapType;
    typedef ID3D11HullShader DXIface;

    static inline const char* GetShaderExt() { return ".hs"; }
    static inline const char* GetCompilationTarget() { return "hs_5_0"; }
    static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
    {
        DXIface* hs = 0;
        R_CHK(HW.pDevice->CreateHullShader(buffer, size, NULL, &hs));
        return hs;
    }

    static inline u32 GetShaderDest() { return RC_dest_hull; }
};

template <>
struct ShaderTypeTraits<SDS>
{
    typedef CResourceManager::map_DS MapType;
    typedef ID3D11DomainShader DXIface;

    static inline const char* GetShaderExt() { return ".ds"; }
    static inline const char* GetCompilationTarget() { return "ds_5_0"; }
    static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
    {
        DXIface* hs = 0;
        R_CHK(HW.pDevice->CreateDomainShader(buffer, size, NULL, &hs));
        return hs;
    }

    static inline u32 GetShaderDest() { return RC_dest_domain; }
};

template <>
struct ShaderTypeTraits<SCS>
{
    typedef CResourceManager::map_CS MapType;
    typedef ID3D11ComputeShader DXIface;

    static inline const char* GetShaderExt() { return ".cs"; }
    static inline const char* GetCompilationTarget() { return "cs_5_0"; }
    static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
    {
        DXIface* cs = 0;
        R_CHK(HW.pDevice->CreateComputeShader(buffer, size, NULL, &cs));
        return cs;
    }

    static inline u32 GetShaderDest() { return RC_dest_compute; }
};
#endif

#if defined(USE_DX10) || defined(USE_DX11)
template <>
inline CResourceManager::map_GS& CResourceManager::GetShaderMap()
{
    return m_gs;
}
#endif

#if defined(USE_DX11)
template <>
inline CResourceManager::map_DS& CResourceManager::GetShaderMap()
{
    return m_ds;
}

template <>
inline CResourceManager::map_HS& CResourceManager::GetShaderMap()
{
    return m_hs;
}

template <>
inline CResourceManager::map_CS& CResourceManager::GetShaderMap()
{
    return m_cs;
}
#endif

template <typename T>
inline T* CResourceManager::CreateShader(const char* name)
{
    ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<ShaderTypeTraits<T>::MapType>();
    LPSTR N = LPSTR(name);
    auto iterator = sh_map.find(N);

    if (iterator != sh_map.end())
        return iterator->second;
    else
    {
        T* sh = new T();

        sh->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        sh_map.insert(std::make_pair(sh->set_name(name), sh));
        if (0 == xr_stricmp(name, "null"))
        {
            sh->sh = NULL;
            return sh;
        }

        // Remove ( and everything after it
        string_path shName;
        const char* pchr = strchr(name, '(');
        ptrdiff_t strSize = pchr ? pchr - name : xr_strlen(name);
        strncpy(shName, name, strSize);
        shName[strSize] = 0;

        // Open file
        string_path cname;
        strconcat(sizeof(cname), cname, GEnv.Render->getShaderPath(), /*name*/ shName,
            ShaderTypeTraits<T>::GetShaderExt());
        FS.update_path(cname, "$game_shaders$", cname);

        // duplicate and zero-terminate
        IReader* file = FS.r_open(cname);
        if (!file && strstr(Core.Params, "-lack_of_shaders"))
        {
            string1024 tmp;
            xr_sprintf(tmp, "CreateShader: %s is missing. Replacing it with stub_default%s", cname, ShaderTypeTraits<T>::GetShaderExt());
            Msg(tmp);
            strconcat(sizeof(cname), cname, GEnv.Render->getShaderPath(), "stub_default", ShaderTypeTraits<T>::GetShaderExt());
            FS.update_path(cname, "$game_shaders$", cname);
            file = FS.r_open(cname);
        }
        R_ASSERT2(file, cname);

        // Select target
        LPCSTR c_target = ShaderTypeTraits<T>::GetCompilationTarget();
        LPCSTR c_entry = "main";

        // Compile
        HRESULT const _hr = GEnv.Render->shader_compile(name, (DWORD const*)file->pointer(), file->length(),
            c_entry, c_target, D3D10_SHADER_PACK_MATRIX_ROW_MAJOR, (void*&)sh);

        FS.r_close(file);

        VERIFY(SUCCEEDED(_hr));

        CHECK_OR_EXIT(!FAILED(_hr), "Your video card doesn't meet game requirements.\n\nTry to lower game settings.");

        return sh;
    }
}

template <typename T>
inline void CResourceManager::DestroyShader(const T* sh)
{
    if (0 == (sh->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;

    ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<ShaderTypeTraits<T>::MapType>();

    LPSTR N = LPSTR(*sh->cName);
    auto iterator = sh_map.find(N);

    if (iterator != sh_map.end())
    {
        sh_map.erase(iterator);
        return;
    }
    Msg("! ERROR: Failed to find compiled geometry shader '%s'", *sh->cName);
}

#endif
