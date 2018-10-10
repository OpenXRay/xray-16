#pragma once

#include "ResourceManager.h"

template <typename T>
struct ShaderTypeTraits;

template <>
struct ShaderTypeTraits<SVS>
{
    using MapType = CResourceManager::map_VS;

#ifdef USE_OGL
    using HWShaderType = GLuint;
#else
    using HWShaderType = ID3DVertexShader*;
#endif

    static inline const char* GetShaderExt() { return ".vs"; }
    static inline const char* GetCompilationTarget()
    {
        return "vs_2_0";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* data)
    {
        if (HW.Caps.geometry_major >= 2)
            target = "vs_2_0";
        else
            target = "vs_1_1";

        if (strstr(data, "main_vs_1_1"))
        {
            target = "vs_1_1";
            entry = "main_vs_1_1";
        }

        if (strstr(data, "main_vs_2_0"))
        {
            target = "vs_2_0";
            entry = "main_vs_2_0";
        }
    }

    static inline HWShaderType CreateHWShader(DWORD const* buffer, size_t size)
    {
        HWShaderType sh = 0;

#ifdef USE_OGL
        sh = glCreateShader(GL_VERTEX_SHADER);
#elif defined(USE_DX11)
        R_CHK(HW.pDevice->CreateVertexShader(buffer, size, 0, &sh));
#elif defined(USE_DX10)
        R_CHK(HW.pDevice->CreateVertexShader(buffer, size, &sh));
#else
        R_CHK(HW.pDevice->CreateVertexShader(buffer, &sh));
#endif

        return sh;
    }

    static inline u32 GetShaderDest() { return RC_dest_vertex; }
};

template <>
struct ShaderTypeTraits<SPS>
{
    using MapType = CResourceManager::map_PS;

#ifdef USE_OGL
    using HWShaderType = GLuint;
#else
    using HWShaderType = ID3DPixelShader*;
#endif

    static inline const char* GetShaderExt() { return ".ps"; }
    static inline const char* GetCompilationTarget()
    {
        return "ps_2_0";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* data)
    {
        if (strstr(data, "main_ps_1_1"))
        {
            target = "ps_1_1";
            entry = "main_ps_1_1";
        }
        if (strstr(data, "main_ps_1_2"))
        {
            target = "ps_1_2";
            entry = "main_ps_1_2";
        }
        if (strstr(data, "main_ps_1_3"))
        {
            target = "ps_1_3";
            entry = "main_ps_1_3";
        }
        if (strstr(data, "main_ps_1_4"))
        {
            target = "ps_1_4";
            entry = "main_ps_1_4";
        }
        if (strstr(data, "main_ps_2_0"))
        {
            target = "ps_2_0";
            entry = "main_ps_2_0";
        }
    }

    static inline HWShaderType CreateHWShader(DWORD const* buffer, size_t size)
    {
        HWShaderType sh = 0;

#ifdef USE_OGL
        sh = glCreateShader(GL_FRAGMENT_SHADER);
#elif defined(USE_DX11)
        R_CHK(HW.pDevice->CreatePixelShader(buffer, size, 0, &sh));
#elif defined(USE_DX10)
        R_CHK(HW.pDevice->CreatePixelShader(buffer, size, &sh));
#else
        R_CHK(HW.pDevice->CreatePixelShader(buffer, &sh));
#endif

        return sh;
    }

    static inline u32 GetShaderDest() { return RC_dest_pixel; }
};

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
template <>
struct ShaderTypeTraits<SGS>
{
    using MapType = CResourceManager::map_GS;

#ifdef USE_OGL
    using HWShaderType = GLuint;
#else
    using HWShaderType = ID3DGeometryShader*;
#endif


    static inline const char* GetShaderExt() { return ".gs"; }
    static inline const char* GetCompilationTarget()
    {
#ifdef USE_DX10
        if (HW.pDevice1 == nullptr)
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

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline HWShaderType CreateHWShader(DWORD const* buffer, size_t size)
    {
        HWShaderType sh = 0;

#ifdef USE_OGL
        sh = glCreateShader(GL_GEOMETRY_SHADER);
#elif defined(USE_DX11)
        R_CHK(HW.pDevice->CreateGeometryShader(buffer, size, 0, &sh));
#else
        R_CHK(HW.pDevice->CreateGeometryShader(buffer, size, &sh));
#endif

        return sh;
    }

    static inline u32 GetShaderDest() { return RC_dest_geometry; }
};
#endif

#if defined(USE_DX11) || defined(USE_OGL)
template <>
struct ShaderTypeTraits<SHS>
{
    using MapType = CResourceManager::map_HS;

#ifdef USE_OGL
    using HWShaderType = GLuint;
#else
    using HWShaderType = ID3D11HullShader*;
#endif

    static inline const char* GetShaderExt() { return ".hs"; }
    static inline const char* GetCompilationTarget() { return "hs_5_0"; }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline HWShaderType CreateHWShader(DWORD const* buffer, size_t size)
    {
        HWShaderType sh = 0;

#ifdef USE_OGL
        sh = glCreateShader(GL_TESS_CONTROL_SHADER);
#else
        R_CHK(HW.pDevice->CreateHullShader(buffer, size, NULL, &sh));
#endif

        return sh;
    }

    static inline u32 GetShaderDest() { return RC_dest_hull; }
};

template <>
struct ShaderTypeTraits<SDS>
{
    using MapType = CResourceManager::map_DS;

#ifdef USE_OGL
    using HWShaderType = GLuint;
#else
    using HWShaderType = ID3D11DomainShader*;
#endif

    static inline const char* GetShaderExt() { return ".ds"; }
    static inline const char* GetCompilationTarget() { return "ds_5_0"; }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline HWShaderType CreateHWShader(DWORD const* buffer, size_t size)
    {
        HWShaderType sh = 0;

#ifdef USE_OGL
        sh = glCreateShader(GL_TESS_EVALUATION_SHADER);
#else
        R_CHK(HW.pDevice->CreateDomainShader(buffer, size, NULL, &sh));
#endif

        return sh;
    }

    static inline u32 GetShaderDest() { return RC_dest_domain; }
};

template <>
struct ShaderTypeTraits<SCS>
{
    using MapType = CResourceManager::map_CS;

#ifdef USE_OGL
    using HWShaderType = GLuint;
#else
    using HWShaderType = ID3D11ComputeShader*;
#endif

    static inline const char* GetShaderExt() { return ".cs"; }
    static inline const char* GetCompilationTarget() { return "cs_5_0"; }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline HWShaderType CreateHWShader(DWORD const* buffer, size_t size)
    {
        HWShaderType sh = 0;
        
#ifdef USE_OGL
        sh = glCreateShader(GL_COMPUTE_SHADER);
#else
        R_CHK(HW.pDevice->CreateComputeShader(buffer, size, NULL, &sh));
#endif

        return sh;
    }

    static inline u32 GetShaderDest() { return RC_dest_compute; }
};
#endif

template <>
inline CResourceManager::map_PS& CResourceManager::GetShaderMap()
{
    return m_ps;
}

template <>
inline CResourceManager::map_VS& CResourceManager::GetShaderMap()
{
    return m_vs;
}

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
template <>
inline CResourceManager::map_GS& CResourceManager::GetShaderMap()
{
    return m_gs;
}
#endif

#if defined(USE_DX11) || defined(USE_OGL)
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
inline T* CResourceManager::CreateShader(const char* name, const char* filename /*= nullptr*/, const bool searchForEntryAndTarget /*= false*/)
{
    typename ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<typename ShaderTypeTraits<T>::MapType>();
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
        {
            if (filename == nullptr)
                filename = name;

            pcstr pchr = strchr(filename, '(');
            ptrdiff_t size = pchr ? pchr - filename : xr_strlen(filename);
            strncpy(shName, filename, size);
            shName[size] = 0;
        }

        // Open file
        string_path cname;
        strconcat(sizeof(cname), cname, GEnv.Render->getShaderPath(), shName,
            ShaderTypeTraits<T>::GetShaderExt());
        FS.update_path(cname, "$game_shaders$", cname);

        // Try to open
        IReader* file = FS.r_open(cname);

        bool fallback = strstr(Core.Params, "-lack_of_shaders");
        if (!file && fallback)
        {
        fallback:
            fallback = false;

            string1024 tmp;
            xr_sprintf(tmp, "CreateShader: %s is missing. Replacing it with stub_default%s", cname, ShaderTypeTraits<T>::GetShaderExt());
            Msg(tmp);
            strconcat(sizeof(cname), cname, GEnv.Render->getShaderPath(), "stub_default", ShaderTypeTraits<T>::GetShaderExt());
            FS.update_path(cname, "$game_shaders$", cname);
            file = FS.r_open(cname);
        }
        R_ASSERT3(file, "Shader file doesnt exist:", cname);

        // Duplicate and zero-terminate
        const auto size = file->length();
        char* const data = (LPSTR)_alloca(size + 1);
        CopyMemory(data, file->pointer(), size);
        data[size] = 0;

        // Select target
        LPCSTR c_target = ShaderTypeTraits<T>::GetCompilationTarget();
        LPCSTR c_entry = "main";
        
        if (searchForEntryAndTarget)
            ShaderTypeTraits<T>::GetCompilationTarget(c_target, c_entry, data);

#ifdef USE_OGL
        DWORD flags = NULL;
#elif defined(USE_DX10) || defined(USE_DX11)
        DWORD flags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;
#else
        DWORD flags = D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR;
#endif

        // Compile
        HRESULT const _hr = GEnv.Render->shader_compile(name, file, c_entry, c_target, flags, (void*&)sh);

        FS.r_close(file);

        VERIFY(SUCCEEDED(_hr));

        if (FAILED(_hr) && fallback)
            goto fallback;

        CHECK_OR_EXIT(!FAILED(_hr), "Your video card doesn't meet game requirements.\n\nTry to lower game settings.");

        return sh;
    }
}

template <typename T>
bool CResourceManager::DestroyShader(const T* sh)
{
    if (0 == (sh->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return false;

   typename ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<typename ShaderTypeTraits<T>::MapType>();

    LPSTR N = LPSTR(*sh->cName);
    auto iterator = sh_map.find(N);

    if (iterator != sh_map.end())
    {
        sh_map.erase(iterator);
        return true;
    }

    Msg("! ERROR: Failed to find compiled shader '%s'", sh->cName.c_str());
    return false;
}
