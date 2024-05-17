#pragma once

#include "ResourceManager.h"

#ifdef USE_OGL
static void show_compile_errors(cpcstr filename, GLuint program, GLuint shader)
{
    GLint length;
    GLchar *errors = nullptr, *sources = nullptr;

    if (program)
    {
        CHK_GL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
        errors = xr_alloc<GLchar>(length);
        CHK_GL(glGetProgramInfoLog(program, length, nullptr, errors));
    }
    else if (shader)
    {
        CHK_GL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
        errors = xr_alloc<GLchar>(length);
        CHK_GL(glGetShaderInfoLog(shader, length, nullptr, errors));

        CHK_GL(glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &length));
        sources = xr_alloc<GLchar>(length);
        CHK_GL(glGetShaderSource(shader, length, nullptr, sources));
    }

    Log("! shader compilation failed:", filename);
    if (errors)
        Log("! error: ", errors);

    if (sources)
    {
        Log("Shader source:");
        Log(sources);
        Log("Shader source end.");
    }
    xr_free(errors);
    xr_free(sources);
}

template<GLenum type>
inline std::pair<char, GLuint> GLCompileShader(pcstr* buffer, size_t size, pcstr name)
{
    const GLuint shader = glCreateShader(type);
    R_ASSERT(shader);
    CHK_GL(glShaderSource(shader, size, buffer, nullptr));
    CHK_GL(glCompileShader(shader));

    GLint status{};
    CHK_GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (GLboolean(status) == GL_FALSE)
    {
        show_compile_errors(name, 0, shader);
        CHK_GL(glDeleteShader(shader));
        return { 's', 0 }; // 's' means "shader", 0 means error
    }

    if (!HW.SeparateShaderObjectsSupported)
        return { 's', shader };

    const GLuint program = glCreateProgram();
    R_ASSERT(program);
    if (epoxy_gl_version() >= 43)
        CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    CHK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, (GLint)GL_TRUE));
    if (HW.ShaderBinarySupported)
        CHK_GL(glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, (GLint)GL_TRUE));

    CHK_GL(glAttachShader(program, shader));
    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target"));
    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target0"));
    CHK_GL(glBindFragDataLocation(program, 1, "SV_Target1"));
    CHK_GL(glBindFragDataLocation(program, 2, "SV_Target2"));
    CHK_GL(glLinkProgram(program));
    CHK_GL(glDetachShader(program, shader));
    CHK_GL(glDeleteShader(shader));

    CHK_GL(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (GLboolean(status) == GL_FALSE)
    {
        show_compile_errors(name, program, 0);
        CHK_GL(glDeleteProgram(program));
        return { 'p', 0 }; // 'p' means "program", 0 means error
    }

    return { 'p', program };
}

inline std::pair<char, GLuint> GLUseBinary(pcstr* buffer, size_t size, const GLenum* format, pcstr name)
{
    GLint status{};

    const GLuint program = glCreateProgram();
    R_ASSERT(program);
    if (epoxy_gl_version() >= 43)
        CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    CHK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, (GLint)GL_TRUE));

    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target"));
    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target0"));
    CHK_GL(glBindFragDataLocation(program, 1, "SV_Target1"));
    CHK_GL(glBindFragDataLocation(program, 2, "SV_Target2"));

    CHK_GL(glProgramBinary(program, *format, buffer, size));
    CHK_GL(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (GLboolean(status) == GL_FALSE)
    {
        show_compile_errors(name, program, 0);
        CHK_GL(glDeleteProgram(program));
        return { 'p', 0 }; // 'p' means "program", 0 means error
    }

    return { 'p', program };
}

static GLuint GLLinkMonolithicProgram(pcstr name, GLuint ps, GLuint vs, GLuint gs)
{
    const GLuint program = glCreateProgram();
    R_ASSERT(program);
    if (epoxy_gl_version() >= 43)
        CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    // XXX: support caching for monolithic programs
    //if (HW.ShaderBinarySupported)
    //    CHK_GL(glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, (GLint)GL_TRUE));

    CHK_GL(glAttachShader(program, ps));
    CHK_GL(glAttachShader(program, vs));
    if (gs)
        CHK_GL(glAttachShader(program, gs));
    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target"));
    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target0"));
    CHK_GL(glBindFragDataLocation(program, 1, "SV_Target1"));
    CHK_GL(glBindFragDataLocation(program, 2, "SV_Target2"));
    CHK_GL(glLinkProgram(program));
    CHK_GL(glDetachShader(program, ps));
    CHK_GL(glDetachShader(program, vs));
    if (gs)
        CHK_GL(glDetachShader(program, gs));

    GLint status{};
    CHK_GL(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (GLboolean(status) == GL_FALSE)
    {
        show_compile_errors(name, program, 0);
        CHK_GL(glDeleteProgram(program));
        return 0; // 0 means error
    }
    return program;
}

static GLuint GLGeneratePipeline(pcstr name, GLuint ps, GLuint vs, GLuint gs)
{
    GLuint pp;
    CHK_GL(glGenProgramPipelines(1, &pp));
    R_ASSERT(pp);
    CHK_GL(glUseProgramStages(pp, GL_FRAGMENT_SHADER_BIT, ps));
    CHK_GL(glUseProgramStages(pp, GL_VERTEX_SHADER_BIT,   vs));
    CHK_GL(glUseProgramStages(pp, GL_GEOMETRY_SHADER_BIT, gs));
    CHK_GL(glValidateProgramPipeline(pp));
    return pp;
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
    using ResultType = std::pair<char, GLuint>;
#elif defined(USE_DX11)
    using LinkageType = ID3D11ClassLinkage*;
    using HWShaderType = ID3DVertexShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#else
#   error No graphics API selected or enabled!
#endif

    static inline const char* GetShaderExt() { return ".vs"; }

    static inline const char* GetCompilationTarget()
    {
        return HW.Caps.geometry_profile;
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

#if defined(USE_DX11)
        res = HW.pDevice->CreateVertexShader(buffer, size, linkage, &sh);
        UNUSED(name);
#elif defined(USE_OGL)
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_VERTEX_SHADER>(buffer, size, name);
#else
#   error No graphics API selected or enabled!
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_vertex; }
};

template <>
struct ShaderTypeTraits<SPS>
{
    using MapType = CResourceManager::map_PS;

#if defined(USE_OGL)
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<char, GLuint>;
#elif defined(USE_DX11)
    using LinkageType = ID3D11ClassLinkage*;
    using HWShaderType = ID3DPixelShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#else
#   error No graphics API selected or enabled!
#endif

    static inline const char* GetShaderExt() { return ".ps"; }

    static inline const char* GetCompilationTarget()
    {
        return HW.Caps.raster_profile;
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

#if defined(USE_DX11)
        res = HW.pDevice->CreatePixelShader(buffer, size, linkage, &sh);
        UNUSED(name);
#elif defined(USE_OGL)
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_FRAGMENT_SHADER>(buffer, size, name);
#else
#       error No graphics API selected or enabled!
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_pixel; }
};

template <>
struct ShaderTypeTraits<SGS>
{
    using MapType = CResourceManager::map_GS;

#   if defined(USE_DX11)
    using LinkageType = ID3D11ClassLinkage*;
    using HWShaderType = ID3DGeometryShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#   elif defined(USE_OGL)
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<char, GLuint>;
#   else
#       error No graphics API selected or enabled!
#   endif

    static inline const char* GetShaderExt() { return ".gs"; }

    static inline const char* GetCompilationTarget()
    {
#   ifdef USE_DX11
        switch (HW.FeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
            return "gs_4_0";
        case D3D_FEATURE_LEVEL_10_1:
            return "gs_4_1";
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
#       ifdef HAS_DX11_3
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
#       endif
            return "gs_5_0";
        }
#   endif // USE_DX11
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

#   if defined(USE_DX11)
        res = HW.pDevice->CreateGeometryShader(buffer, size, linkage, &sh);
        UNUSED(name);
#   elif defined(USE_OGL)
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_GEOMETRY_SHADER>(buffer, size, name);
#   else
#       error No graphics API selected or enabled!
#   endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_geometry; }
};

template <>
struct ShaderTypeTraits<SHS>
{
    using MapType = CResourceManager::map_HS;

#   if defined(USE_DX11)
    using LinkageType = ID3D11ClassLinkage*;
    using HWShaderType = ID3D11HullShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#   elif defined(USE_OGL)
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<char, GLuint>;
#   else
#       error No graphics API selected or enabled!
#   endif

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

#   if defined(USE_DX11)
        res = HW.pDevice->CreateHullShader(buffer, size, linkage, &sh);
        UNUSED(name);
#   elif defined(USE_OGL)
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_TESS_CONTROL_SHADER>(buffer, size, name);
#   else
#       error No graphics API selected or enabled!
#   endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_hull; }
};

template <>
struct ShaderTypeTraits<SDS>
{
    using MapType = CResourceManager::map_DS;

#if defined(USE_DX11)
    using LinkageType = ID3D11ClassLinkage*;
    using HWShaderType = ID3D11DomainShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#elif defined(USE_OGL)
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<char, GLuint>;
#   else
#       error No graphics API selected or enabled!
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

#   if defined(USE_DX11)
        res = HW.pDevice->CreateDomainShader(buffer, size, linkage, &sh);
        UNUSED(name);
#   elif defined(USE_OGL)
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_TESS_EVALUATION_SHADER>(buffer, size, name);
#   else
#       error No graphics API selected or enabled!
#   endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_domain; }
};

template <>
struct ShaderTypeTraits<SCS>
{
    using MapType = CResourceManager::map_CS;

#   if defined(USE_DX11)
    using LinkageType = ID3D11ClassLinkage*;
    using HWShaderType = ID3D11ComputeShader*;
    using BufferType = DWORD const*;
    using ResultType = HRESULT;
#   elif defined(USE_OGL)
    using LinkageType = const GLenum*;
    using HWShaderType = GLuint;
    using BufferType = pcstr*;
    using ResultType = std::pair<char, GLuint>;
#   else
#       error No graphics API selected or enabled!
#   endif

    static inline const char* GetShaderExt() { return ".cs"; }

    static inline const char* GetCompilationTarget()
    {
#   ifdef USE_DX11
        switch (HW.FeatureLevel)
        {
        case D3D_FEATURE_LEVEL_10_0:
            return "cs_4_0";
        case D3D_FEATURE_LEVEL_10_1:
            return "cs_4_1";
        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
#       ifdef HAS_DX11_3
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
#       endif
            return "cs_5_0";
        }
#   endif // USE_DX11

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

#if defined(USE_DX11)
        res = HW.pDevice->CreateComputeShader(buffer, size, linkage, &sh);
        UNUSED(name);
#elif defined(USE_OGL)
        if (linkage)
            res = GLUseBinary(buffer, size, linkage, name);
        else
            res = GLCompileShader<GL_COMPUTE_SHADER>(buffer, size, name);
#else
#   error No graphics API selected or enabled!
#endif

        return res;
    }

    static inline u32 GetShaderDest() { return RC_dest_compute; }
};

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

template <>
inline CResourceManager::map_GS& CResourceManager::GetShaderMap()
{
    return m_gs;
}

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
        sh_map.emplace(sh->set_name(name), sh);
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
        strconcat(cname, RImplementation.getShaderPath(), shName,
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
            strconcat(tmp, "stub_default", ShaderTypeTraits<T>::GetShaderExt());

            Msg("CreateShader: %s is missing. Replacing it with %s", cname, tmp);
            strconcat(cname, RImplementation.getShaderPath(), tmp);
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

#if defined(USE_D3DX)
#   ifdef NDEBUG
        flags |= D3DXSHADER_PACKMATRIX_ROWMAJOR;
#   else
        flags |= D3DXSHADER_PACKMATRIX_ROWMAJOR | (xrDebug::DebuggerIsPresent() ? D3DXSHADER_DEBUG : 0);
#   endif
#elif !defined(USE_OGL)
#   ifdef NDEBUG
        flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#   else
        flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_DEBUG;
#   endif
#endif

        // Compile
        HRESULT const _hr = RImplementation.shader_compile(name, file, c_entry, c_target, flags, (void*&)sh);

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
