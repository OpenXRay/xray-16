#pragma once

#include "ResourceManager.h"

#ifdef USE_OGL
template<GLenum type>
inline std::pair<GLuint, GLuint> GLCompileShader(pcstr* buffer, size_t size, pcstr name)
{
    GLint status{};

    GLuint shader = glCreateShader(type);
    R_ASSERT(shader);
    glShaderSource(shader, size, buffer, nullptr);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (GLboolean(status) == GL_FALSE)
        return { shader, -1 };

    GLuint program = glCreateProgram();
    R_ASSERT(program);
    CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    CHK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, (GLint)GL_TRUE));
    if (HW.ShaderBinarySupported)
        CHK_GL(glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, (GLint)GL_TRUE));

    glAttachShader(program, shader);
    glBindFragDataLocation(program, 0, "SV_Target");
    glBindFragDataLocation(program, 0, "SV_Target0");
    glBindFragDataLocation(program, 1, "SV_Target1");
    glBindFragDataLocation(program, 2, "SV_Target2");
    glLinkProgram(program);
    glDetachShader(program, shader);
    glDeleteShader(shader);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (GLboolean(status) == GL_FALSE)
        return { -1, program };

    return { 0, program };
}

inline std::pair<GLuint, GLuint> GLUseBinary(pcstr* buffer, size_t size, const GLenum* format, pcstr name)
{
    GLint status{};

    GLuint program = glCreateProgram();
    R_ASSERT(program);
    CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    CHK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, (GLint)GL_TRUE));

    glBindFragDataLocation(program, 0, "SV_Target");
    glBindFragDataLocation(program, 0, "SV_Target0");
    glBindFragDataLocation(program, 1, "SV_Target1");
    glBindFragDataLocation(program, 2, "SV_Target2");

    glProgramBinary(program, *format, buffer, size);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if ((GLboolean)status == GL_FALSE)
        return { -1, program };

    return { 0, program };
}
#endif

template <typename T>
struct ShaderTypeTraits;

template <>
struct ShaderTypeTraits<SVS>
{
    using MapType = CResourceManager::map_VS;

#ifdef USE_OGL
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<GLuint, GLuint>;
#else
#ifdef USE_DX11
    using LinkageType = ID3D11ClassLinkage*;
#else
    using LinkageType = void*;
#endif
    using HWShaderType = ID3DVertexShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#endif

    static inline const char* GetShaderExt() { return ".vs"; }

    static inline const char* GetCompilationTarget()
    {
#ifdef USE_DX9
        return D3DXGetVertexShaderProfile(HW.pDevice); // vertex "vs_2_a";
#elif !defined(USE_DX9) && !defined(USE_OGL)
        switch (HW.FeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
            return "vs_4_0";
        case D3D_FEATURE_LEVEL_10_1:
            return "vs_4_1";
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
#ifdef HAS_DX11_3
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
#endif
            return "vs_5_0";
        }
#endif
        if (HW.Caps.geometry_major >= 2)
            return "vs_2_0";

        return "vs_1_1";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* data)
    {
        entry = "main";

        if (strstr(data, "main_vs_1_1"))
        {
            target = "vs_1_1";
            entry = "main_vs_1_1";
        }
        else if (strstr(data, "main_vs_2_0"))
        {
            target = "vs_2_0";
            entry = "main_vs_2_0";
        }
#ifdef USE_DX9 // For DX10+ we always should use SM4.0 or higher
        else
#endif
        {
            target = GetCompilationTarget();
        }
    }

    static inline ResultType CreateHWShader(BufferType buffer, size_t size, HWShaderType& sh,
        LinkageType linkage = nullptr, pcstr name = nullptr)
    {
        ResultType res{};

#ifdef USE_OGL
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_VERTEX_SHADER>(buffer, size, name);
#elif defined(USE_DX11)
        res = HW.pDevice->CreateVertexShader(buffer, size, linkage, &sh);
#else
        res = HW.pDevice->CreateVertexShader(buffer, &sh);
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_vertex; }
};

template <>
struct ShaderTypeTraits<SPS>
{
    using MapType = CResourceManager::map_PS;

#ifdef USE_OGL
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<GLuint, GLuint>;
#else
#ifdef USE_DX11
    using LinkageType = ID3D11ClassLinkage*;
#else
    using LinkageType = void*;
#endif
    using HWShaderType = ID3DPixelShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#endif

    static inline const char* GetShaderExt() { return ".ps"; }

    static inline const char* GetCompilationTarget()
    {
#ifdef USE_DX9
        return D3DXGetPixelShaderProfile(HW.pDevice); // pixel "ps_2_a";
#elif !defined(USE_DX9) && !defined(USE_OGL)
        switch (HW.FeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
            return "ps_4_0";
        case D3D_FEATURE_LEVEL_10_1:
            return "ps_4_1";
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
#ifdef HAS_DX11_3
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
#endif
            return "ps_5_0";
        }
#endif

        return "ps_2_0";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* data)
    {
        entry = "main";
        if (strstr(data, "main_ps_1_1"))
        {
            target = "ps_1_1";
            entry = "main_ps_1_1";
        }
        else if (strstr(data, "main_ps_1_2"))
        {
            target = "ps_1_2";
            entry = "main_ps_1_2";
        }
        else if (strstr(data, "main_ps_1_3"))
        {
            target = "ps_1_3";
            entry = "main_ps_1_3";
        }
        else if (strstr(data, "main_ps_1_4"))
        {
            target = "ps_1_4";
            entry = "main_ps_1_4";
        }
        else if (strstr(data, "main_ps_2_0"))
        {
            target = "ps_2_0";
            entry = "main_ps_2_0";
        }
#ifdef USE_DX9 // For DX10+ we always should use SM4.0 or higher
        else
#endif
        {
            target = GetCompilationTarget();
        }
    }

    static inline ResultType CreateHWShader(BufferType buffer, size_t size, HWShaderType& sh,
        LinkageType linkage = nullptr, pcstr name = nullptr)
    {
        ResultType res{};

#ifdef USE_OGL
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_FRAGMENT_SHADER>(buffer, size, name);
#elif defined(USE_DX11)
        res = HW.pDevice->CreatePixelShader(buffer, size, linkage, &sh);
#else
        res = HW.pDevice->CreatePixelShader(buffer, &sh);
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_pixel; }
};

#ifndef USE_DX9
template <>
struct ShaderTypeTraits<SGS>
{
    using MapType = CResourceManager::map_GS;

#ifdef USE_OGL
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<GLuint, GLuint>;
#else
#ifdef USE_DX11
    using LinkageType = ID3D11ClassLinkage*;
#else
    using LinkageType = void*;
#endif
    using HWShaderType = ID3DGeometryShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#endif

    static inline const char* GetShaderExt() { return ".gs"; }

    static inline const char* GetCompilationTarget()
    {
#ifdef USE_DX11
        switch (HW.FeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
            return "gs_4_0";
        case D3D_FEATURE_LEVEL_10_1:
            return "gs_4_1";
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
#ifdef HAS_DX11_3
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
#endif
            return "gs_5_0";
        }
#endif // USE_DX11
        NODEFAULT;
        return "gs_4_0";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline ResultType CreateHWShader(BufferType buffer, size_t size, HWShaderType& sh,
        LinkageType linkage = nullptr, pcstr name = nullptr)
    {
        ResultType res{};

#ifdef USE_OGL
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_GEOMETRY_SHADER>(buffer, size, name);
#elif defined(USE_DX11)
        res = HW.pDevice->CreateGeometryShader(buffer, size, linkage, &sh);
#else
        res = HW.pDevice->CreateGeometryShader(buffer, size, &sh);
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_geometry; }
};
#endif

#ifndef USE_DX9
template <>
struct ShaderTypeTraits<SHS>
{
    using MapType = CResourceManager::map_HS;

#ifdef USE_OGL
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<GLuint, GLuint>;
#else
#ifdef USE_DX11
    using LinkageType = ID3D11ClassLinkage*;
#else
    using LinkageType = void*;
#endif
    using HWShaderType = ID3D11HullShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#endif

    static inline const char* GetShaderExt() { return ".hs"; }

    static inline const char* GetCompilationTarget()
    {
        return "hs_5_0";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline ResultType CreateHWShader(BufferType buffer, size_t size, HWShaderType& sh,
        LinkageType linkage = nullptr, pcstr name = nullptr)
    {
        ResultType res{};

#ifdef USE_OGL
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_TESS_CONTROL_SHADER>(buffer, size, name);
#else
        res = HW.pDevice->CreateHullShader(buffer, size, linkage, &sh);
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_hull; }
};

template <>
struct ShaderTypeTraits<SDS>
{
    using MapType = CResourceManager::map_DS;

#ifdef USE_OGL
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<GLuint, GLuint>;
#else
#ifdef USE_DX11
    using LinkageType = ID3D11ClassLinkage*;
#else
    using LinkageType = void*;
#endif
    using HWShaderType = ID3D11DomainShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#endif

    static inline const char* GetShaderExt() { return ".ds"; }

    static inline const char* GetCompilationTarget()
    {
        return "ds_5_0";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline ResultType CreateHWShader(BufferType buffer, size_t size, HWShaderType& sh,
        LinkageType linkage = nullptr, pcstr name = nullptr)
    {
        ResultType res{};

#ifdef USE_OGL
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_TESS_EVALUATION_SHADER>(buffer, size, name);
#else
        res = HW.pDevice->CreateDomainShader(buffer, size, linkage, &sh);
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_domain; }
};

template <>
struct ShaderTypeTraits<SCS>
{
    using MapType = CResourceManager::map_CS;

#ifdef USE_OGL
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<GLuint, GLuint>;
#else
#ifdef USE_DX11
    using LinkageType = ID3D11ClassLinkage*;
#else
    using LinkageType = void*;
#endif
    using HWShaderType = ID3D11ComputeShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#endif

    static inline const char* GetShaderExt() { return ".cs"; }

    static inline const char* GetCompilationTarget()
    {
#ifdef USE_DX11
        switch (HW.FeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
            return "cs_4_0";
        case D3D_FEATURE_LEVEL_10_1:
            return "cs_4_1";
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
#ifdef HAS_DX11_3
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
#endif
            return "cs_5_0";
        }
#endif // USE_DX11

        return "cs_5_0";
    }

    static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
    {
        target = GetCompilationTarget();
        entry = "main";
    }

    static inline ResultType CreateHWShader(BufferType buffer, size_t size, HWShaderType& sh,
        LinkageType linkage = nullptr, pcstr name = nullptr)
    {
        ResultType res{};
        
#ifdef USE_OGL
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_COMPUTE_SHADER>(buffer, size, name);
#else
        res = HW.pDevice->CreateComputeShader(buffer, size, linkage, &sh);
#endif

        return res;
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

#ifndef USE_DX9
template <>
inline CResourceManager::map_GS& CResourceManager::GetShaderMap()
{
    return m_gs;
}
#endif

#ifndef USE_DX9
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
T* CResourceManager::CreateShader(cpcstr name, pcstr filename /*= nullptr*/, u32 flags /*= 0*/)
{
    typename ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<typename ShaderTypeTraits<T>::MapType>();
    pstr N = pstr(name);
    auto iterator = sh_map.find(N);

    if (iterator != sh_map.end())
        return iterator->second;
    else
    {
        T* sh = xr_new<T>();

        sh->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        sh_map.insert(std::make_pair(sh->set_name(name), sh));
        if (0 == xr_stricmp(name, "null"))
        {
            sh->sh = 0;
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

        // Here we can fallback to "stub_default"
        bool fallback = m_shader_fallback_allowed;
        if (!file && fallback)
        {
        fallback:
            fallback = false;

            string_path tmp;
            strconcat(sizeof(tmp), tmp, "stub_default", ShaderTypeTraits<T>::GetShaderExt());

            Msg("CreateShader: %s is missing. Replacing it with %s", cname, tmp);
            strconcat(sizeof(cname), cname, GEnv.Render->getShaderPath(), tmp);
            FS.update_path(cname, "$game_shaders$", cname);
            file = FS.r_open(cname);
        }
        R_ASSERT3(file, "Shader file doesnt exist", cname);

        // Duplicate and zero-terminate
        const auto size = file->length();
        char* const data = (pstr)xr_alloca(size + 1);
        CopyMemory(data, file->pointer(), size);
        data[size] = 0;

        // Select target
        pcstr c_target, c_entry;
        ShaderTypeTraits<T>::GetCompilationTarget(c_target, c_entry, data);

#if !defined(USE_DX9) && !defined(USE_OGL)
#   ifdef NDEBUG
        flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#   else
        flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | (xrDebug::DebuggerIsPresent() ? D3DCOMPILE_DEBUG : 0);
#   endif
#elif defined(USE_DX9)
#   ifdef NDEBUG
        flags |= D3DXSHADER_PACKMATRIX_ROWMAJOR;
#   else
        flags |= D3DXSHADER_PACKMATRIX_ROWMAJOR | (xrDebug::DebuggerIsPresent() ? D3DXSHADER_DEBUG : 0);
#   endif
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

    pstr N = pstr(*sh->cName);
    auto iterator = sh_map.find(N);

    if (iterator != sh_map.end())
    {
        sh_map.erase(iterator);
        return true;
    }

    Msg("! ERROR: Failed to find compiled shader '%s'", sh->cName.c_str());
    return false;
}
