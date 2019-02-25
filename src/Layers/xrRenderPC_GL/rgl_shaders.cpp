#include "stdafx.h"
#include "rgl.h"

void CRender::addShaderOption(const char* name, const char* value)
{
    m_ShaderOptions += "#define ";
    m_ShaderOptions += name;
    m_ShaderOptions += " ";
    m_ShaderOptions += value;
    m_ShaderOptions += "\n";
}

static inline bool match_shader_id(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result);

/////////

// TODO: OGL: make ignore commented includes
static inline void load_includes(pcstr pSrcData, UINT SrcDataLen, xr_vector<pstr>& source, xr_vector<pstr>& includes)
{
    // Copy source file data into a null-terminated buffer
    pstr srcData = xr_alloc<char>(SrcDataLen + 2);
    CopyMemory(srcData, pSrcData, SrcDataLen);
    srcData[SrcDataLen] = '\n';
    srcData[SrcDataLen + 1] = '\0';
    includes.push_back(srcData);
    source.push_back(srcData);

    string_path path;
    pstr str = srcData;
    while (strstr(str, "#include") != nullptr)
    {
        // Get filename of include directive
        str = strstr(str, "#include"); // Find the include directive
        char* fn = strchr(str, '"') + 1; // Get filename, skip quotation
        *str = '\0'; // Terminate previous source
        str = strchr(fn, '"'); // Get end of filename path
        *str = '\0'; // Terminate filename path

        // Create path to included shader
        strconcat(sizeof(path), path, GEnv.Render->getShaderPath(), fn);
        FS.update_path(path, _game_shaders_, path);
        while (pstr sep = strchr(path, '/'))
            *sep = '\\';

        // Open and read file, recursively load includes
        IReader* R = FS.r_open(path);
        R_ASSERT2(R, path);
        load_includes((pstr)R->pointer(), R->length(), source, includes);
        FS.r_close(R);

        // Add next source, skip quotation
        ++str;
        source.push_back(str);
    }
}

struct SHADER_MACRO
{
    pcstr Name          = nullptr;
    pcstr Definition    = nullptr;
    pstr  FullDefine    = nullptr;
};

HRESULT CRender::shader_compile(LPCSTR name, IReader* fs, LPCSTR pFunctionName,
    LPCSTR pTarget, DWORD Flags, void*& result)
{
    xr_vector<pstr> source, includes;
    SHADER_MACRO defines[128];
    int def_it = 0;
    char c_smapsize[32];
    char c_gloss[32];
    char c_sun_shafts[32];
    char c_ssao[32];
    char c_sun_quality[32];

    // TODO: OGL: Implement these parameters.
    VERIFY(!pFunctionName);
    VERIFY(!pTarget);
    VERIFY(!Flags);

    // open included files
    load_includes((pcstr)fs->pointer(), fs->length(), source, includes);

    char sh_name[MAX_PATH] = "";
    u32 len = 0;
    // options
    {
        xr_sprintf(c_smapsize, "%d.0", u32(o.smapsize));
        defines[def_it].Name = "SMAP_size";
        defines[def_it].Definition = c_smapsize;
        def_it++;
        xr_sprintf(sh_name, "%d", u32(o.smapsize));
        len += u32(xr_strlen(sh_name));
    }

    if (o.fp16_filter)
    {
        defines[def_it].Name = "FP16_FILTER";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.fp16_filter);
    ++len;

    if (o.fp16_blend)
    {
        defines[def_it].Name = "FP16_BLEND";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.fp16_blend);
    ++len;

    if (o.HW_smap)
    {
        defines[def_it].Name = "USE_HWSMAP";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.HW_smap);
    ++len;

    if (o.HW_smap_PCF)
    {
        defines[def_it].Name = "USE_HWSMAP_PCF";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.HW_smap_PCF);
    ++len;

    if (o.HW_smap_FETCH4)
    {
        defines[def_it].Name = "USE_FETCH4";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.HW_smap_FETCH4);
    ++len;

    if (o.sjitter)
    {
        defines[def_it].Name = "USE_SJITTER";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.sjitter);
    ++len;

    if (HW.Caps.raster_major >= 3)
    {
        defines[def_it].Name = "USE_BRANCHING";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(HW.Caps.raster_major >= 3);
    ++len;

    if (HW.Caps.geometry.bVTF)
    {
        defines[def_it].Name = "USE_VTF";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(HW.Caps.geometry.bVTF);
    ++len;

    if (o.Tshadows)
    {
        defines[def_it].Name = "USE_TSHADOWS";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.Tshadows);
    ++len;

    if (o.mblur)
    {
        defines[def_it].Name = "USE_MBLUR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.mblur);
    ++len;

    if (o.sunfilter)
    {
        defines[def_it].Name = "USE_SUNFILTER";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.sunfilter);
    ++len;

    if (o.sunstatic)
    {
        defines[def_it].Name = "USE_R2_STATIC_SUN";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.sunstatic);
    ++len;

    if (o.forcegloss)
    {
        xr_sprintf(c_gloss, "%f", o.forcegloss_v);
        defines[def_it].Name = "FORCE_GLOSS";
        defines[def_it].Definition = c_gloss;
        def_it++;
    }
    sh_name[len] = '0' + char(o.forcegloss);
    ++len;

    if (o.forceskinw)
    {
        defines[def_it].Name = "SKIN_COLOR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.forceskinw);
    ++len;

    if (o.ssao_blur_on)
    {
        defines[def_it].Name = "USE_SSAO_BLUR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.ssao_blur_on);
    ++len;

    if (o.ssao_hdao)
    {
        defines[def_it].Name = "HDAO";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.ssao_hdao);
    ++len;

    if (o.ssao_hbao)
    {
        defines[def_it].Name = "USE_HBAO";
        defines[def_it].Definition = "1";
        def_it++;
        if (o.hbao_vectorized)
        {
            defines[def_it].Name = "VECTORIZED_CODE";
            defines[def_it].Definition = "1";
            def_it++;
        }
    }
    sh_name[len] = '0' + char(o.ssao_hbao);
    ++len;
    sh_name[len] = '0' + char(o.ssao_hbao ? o.hbao_vectorized : 0);
    ++len;

    if (o.ssao_opt_data)
    {
        defines[def_it].Name = "SSAO_OPT_DATA";
        if (o.ssao_half_data)
            defines[def_it].Definition = "2";
        else
            defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.ssao_opt_data ? (o.ssao_half_data ? 2 : 1) : 0);
    ++len;

    // skinning
    if (m_skinning < 0)
    {
        defines[def_it].Name = "SKIN_NONE";
        defines[def_it].Definition = "1";
        def_it++;
        sh_name[len] = '1';
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (0 == m_skinning)
    {
        defines[def_it].Name = "SKIN_0";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(0 == m_skinning);
    ++len;

    if (1 == m_skinning)
    {
        defines[def_it].Name = "SKIN_1";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(1 == m_skinning);
    ++len;

    if (2 == m_skinning)
    {
        defines[def_it].Name = "SKIN_2";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(2 == m_skinning);
    ++len;

    if (3 == m_skinning)
    {
        defines[def_it].Name = "SKIN_3";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(3 == m_skinning);
    ++len;

    if (4 == m_skinning)
    {
        defines[def_it].Name = "SKIN_4";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(4 == m_skinning);
    ++len;

    //	Igor: need restart options
    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_WATER))
    {
        defines[def_it].Name = "USE_SOFT_WATER";
        defines[def_it].Definition = "1";
        def_it++;
        sh_name[len] = '1';
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES))
    {
        defines[def_it].Name = "USE_SOFT_PARTICLES";
        defines[def_it].Definition = "1";
        def_it++;
        sh_name[len] = '1';
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF))
    {
        defines[def_it].Name = "USE_DOF";
        defines[def_it].Definition = "1";
        def_it++;
        sh_name[len] = '1';
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    // SkyLoader: temporary added
    if (ps_r2_fxaa)
    {
        defines[def_it].Name = "USE_FXAA";
        defines[def_it].Definition = "1";
        def_it++;
        sh_name[len] = '1';
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }
    // end

    if (RImplementation.o.advancedpp && ps_r_sun_shafts)
    {
        xr_sprintf(c_sun_shafts, "%d", ps_r_sun_shafts);
        defines[def_it].Name = "SUN_SHAFTS_QUALITY";
        defines[def_it].Definition = c_sun_shafts;
        def_it++;
        sh_name[len] = '0' + char(ps_r_sun_shafts);
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r_ssao)
    {
        xr_sprintf(c_ssao, "%d", ps_r_ssao);
        defines[def_it].Name = "SSAO_QUALITY";
        defines[def_it].Definition = c_ssao;
        def_it++;
        sh_name[len] = '0' + char(ps_r_ssao);
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r_sun_quality)
    {
        xr_sprintf(c_sun_quality, "%d", ps_r_sun_quality);
        defines[def_it].Name = "SUN_QUALITY";
        defines[def_it].Definition = c_sun_quality;
        def_it++;
        sh_name[len] = '0' + char(ps_r_sun_quality);
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX))
    {
        defines[def_it].Name = "ALLOW_STEEPPARALLAX";
        defines[def_it].Definition = "1";
        def_it++;
        sh_name[len] = '1';
        ++len;
    }
    else
    {
        sh_name[len] = '0';
        ++len;
    }

    if (o.dx10_gbuffer_opt)
    {
        defines[def_it].Name = "GBUFFER_OPTIMIZATION";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.dx10_gbuffer_opt);
    ++len;

    if (o.dx10_sm4_1)
    {
        defines[def_it].Name = "SM_4_1";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.dx10_sm4_1);
    ++len;

    if (o.dx10_minmax_sm)
    {
        defines[def_it].Name = "USE_MINMAX_SM";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.dx10_minmax_sm != 0);
    ++len;

    // add a #define for DX10_1 MSAA support
    if (o.dx10_msaa)
    {
        static char samples[2];

        defines[def_it].Name = "USE_MSAA";
        defines[def_it].Definition = "1";
        def_it++;

        defines[def_it].Name = "MSAA_SAMPLES";
        samples[0] = char(o.dx10_msaa_samples) + '0';
        samples[1] = 0;
        defines[def_it].Definition = samples;
        def_it++;

        static char def[256];
        if (m_MSAASample < 0)
            def[0] = '0';
        else
            def[0] = '0' + char(m_MSAASample);

        def[1] = 0;
        defines[def_it].Name = "ISAMPLE";
        defines[def_it].Definition = def;
        def_it++;

        if (o.dx10_msaa_opt)
        {
            defines[def_it].Name = "MSAA_OPTIMIZATION";
            defines[def_it].Definition = "1";
            def_it++;
        }

        sh_name[len] = '1';
        ++len;
        sh_name[len] = '0' + char(o.dx10_msaa_samples);
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0' + char(o.dx10_msaa_opt);
        ++len;

        switch (o.dx10_msaa_alphatest)
        {
        case MSAA_ATEST_DX10_0_ATOC:
            defines[def_it].Name = "MSAA_ALPHATEST_DX10_0_ATOC";
            defines[def_it].Definition = "1";
            def_it++;
            sh_name[len] = '1';
            ++len;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
            break;
        case MSAA_ATEST_DX10_1_ATOC:
            defines[def_it].Name = "MSAA_ALPHATEST_DX10_1_ATOC";
            defines[def_it].Definition = "1";
            def_it++;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '1';
            ++len;
            sh_name[len] = '0';
            ++len;
            break;
        case MSAA_ATEST_DX10_1_NATIVE:
            defines[def_it].Name = "MSAA_ALPHATEST_DX10_1";
            defines[def_it].Definition = "1";
            def_it++;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '1';
            ++len;
            break;
        default:
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
            sh_name[len] = '0';
            ++len;
        }
    }
    else
    {
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
        sh_name[len] = '0';
        ++len;
    }

    // Compile sources list
    const size_t head_lines = 2; // "#version" line + name_comment line
    size_t sources_lines = source.size() + def_it + head_lines;
    string256 name_comment;
    xr_sprintf(name_comment, "// %s\n", name);
    pcstr* sources = xr_alloc<pcstr>(sources_lines);
#ifdef DEBUG
    sources[0] = "#version 410\n#pragma optimize (off)\n";
#else
    sources[0] = "#version 410\n";
#endif
    sources[1] = name_comment;

    // Make define lines
    for (u32 i = 0; i < def_it; ++i)
    {
        R_ASSERT2(defines[i].Name && defines[i].Definition, name);

        string256 define_line;
        xr_sprintf(define_line, "#define %s\t%s\n", defines[i].Name, defines[i].Definition);
        const size_t define_len = xr_strlen(define_line);
        defines[i].FullDefine = xr_alloc<char>(define_len + 1);
        CopyMemory(defines[i].FullDefine, define_line, define_len * sizeof(char));
        defines[i].FullDefine[define_len] = '\0';
        sources[head_lines + i] = defines[i].FullDefine;
    }
    CopyMemory(sources + head_lines + def_it, source.data(), source.size() * sizeof(pstr));

    // Compile the shader
    GLuint shader = *(GLuint*)result;
    R_ASSERT(shader);
    CHK_GL(glShaderSource(shader, sources_lines, sources, nullptr));
    CHK_GL(glCompileShader(shader));

    // Create the shader program
    GLuint program = glCreateProgram();
    R_ASSERT(program);
    CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    CHK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, (GLint)GL_TRUE));
    *(GLuint*)result = program;

    // Free string resources
    for (u32 i = 0; i < def_it; ++i)
        xr_free(defines[i].FullDefine);
    xr_free(sources);
    for (auto it = includes.begin(); it != includes.end(); ++it)
        xr_free(*it);

    // Get the compilation result
    GLint status;
    CHK_GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));

    // Link program if compilation succeeded
    GLchar* _pErrorMsgs = nullptr;
    if ((GLboolean)status == GL_TRUE)
    {
        CHK_GL(glAttachShader(program, shader));
        CHK_GL(glBindFragDataLocation(program, 0, "SV_Target"));
        CHK_GL(glBindFragDataLocation(program, 0, "SV_Target0"));
        CHK_GL(glBindFragDataLocation(program, 1, "SV_Target1"));
        CHK_GL(glBindFragDataLocation(program, 2, "SV_Target2"));
        CHK_GL(glLinkProgram(program));
        CHK_GL(glDetachShader(program, shader));
        CHK_GL(glGetProgramiv(program, GL_LINK_STATUS, &status));

        if ((GLboolean)status == GL_FALSE)
        {
            GLint length;
            CHK_GL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
            _pErrorMsgs = xr_alloc<GLchar>(length);
            CHK_GL(glGetProgramInfoLog(program, length, nullptr, _pErrorMsgs));
        }
    }
    else
    {
        GLint length;
        CHK_GL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
        _pErrorMsgs = xr_alloc<GLchar>(length);
        CHK_GL(glGetShaderInfoLog(shader, length, nullptr, _pErrorMsgs));
    }

    if ((GLboolean)status == GL_FALSE)
    {
#ifdef DEBUG
        GLint srcLen;
        CHK_GL(glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &srcLen));
        GLchar* shaderSrc = xr_alloc<GLchar>(srcLen);
        CHK_GL(glGetShaderSource(shader, srcLen, nullptr, shaderSrc));
#endif

        Msg("! shader compilation failed");
        Log("! ", name);
        if (_pErrorMsgs)
            Log("! error: ", _pErrorMsgs);

#ifdef DEBUG
        if (shaderSrc)
        {
            Log("Shader source:");
            Log(shaderSrc);
            Log("Shader source end.");
        }
        xr_free(shaderSrc);
#endif

        xr_free(_pErrorMsgs);
        CHK_GL(glDeleteShader(shader));
        return E_FAIL;
    }

    xr_free(_pErrorMsgs);
    CHK_GL(glDeleteShader(shader));
    return S_OK;
}
