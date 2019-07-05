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
    pcstr Name = nullptr;
    pcstr Definition = nullptr;
    pstr  FullDefine = nullptr;
};

class shader_name_holder
{
    size_t pos{};
    string_path name;

public:
    void append(cpcstr string)
    {
        const size_t size = xr_strlen(string);
        for (size_t i = 0; i < size; ++i)
        {
            name[pos] = string[i];
            ++pos;
        }
    }

    void append(u32 value)
    {
        name[pos] = '0' + char(value); // NOLINT
        ++pos;
    }

    void finish()
    {
        name[pos] = '\0';
    }

    pcstr c_str() const { return name; }
};

class shader_options_holder
{
    size_t pos{};
    SHADER_MACRO m_options[128];

public:
    ~shader_options_holder()
    {
        for (; pos != 0; --pos)
            xr_free(m_options[pos].FullDefine);
    }

    void add(cpcstr name, cpcstr value)
    {
        string512 option_line;
        xr_sprintf(option_line, "#define %s\t%s\n", name, value);
        m_options[pos] = { name, value, xr_strdup(option_line) };
        ++pos;
    }

    void finish()
    {
        m_options[pos] = { nullptr, nullptr, nullptr };
    }

    size_t size() const { return pos; }
    SHADER_MACRO& operator[](size_t idx) { return m_options[idx]; }
    SHADER_MACRO* data() { return m_options; }
};

HRESULT CRender::shader_compile(LPCSTR name, IReader* fs, LPCSTR pFunctionName,
    LPCSTR pTarget, DWORD Flags, void*& result)
{
    shader_options_holder options;
    shader_name_holder sh_name;
    
    xr_vector<pstr> source, includes;

    // Don't move these variables to lower scope!
    string32 c_smapsize;
    string32 c_gloss;
    string32 c_sun_shafts;
    string32 c_ssao;
    string32 c_sun_quality;

    // TODO: OGL: Implement these parameters.
    VERIFY(!pFunctionName);
    VERIFY(!pTarget);
    VERIFY(!Flags);

    // open included files
    load_includes((pcstr)fs->pointer(), fs->length(), source, includes);

    // options:
    const auto appendShaderOption = [&](u32 option, cpcstr macro, cpcstr value)
    {
        if (option)
            options.add(macro, value);

        sh_name.append(option);
    };

    // Shadow map size
    {
        xr_itoa(o.smapsize, c_smapsize, 10);
        options.add("SMAP_size", c_smapsize);
        sh_name.append(c_smapsize);
    }

    // FP16 Filter
    appendShaderOption(o.fp16_filter, "FP16_FILTER", "1");

    // FP16 Blend
    appendShaderOption(o.fp16_blend, "FP16_BLEND", "1");

    // HW smap
    appendShaderOption(o.HW_smap, "USE_HWSMAP", "1");

    // HW smap PCF
    appendShaderOption(o.HW_smap_PCF, "USE_HWSMAP_PCF", "1");

    // Fetch4
    appendShaderOption(o.HW_smap_FETCH4, "USE_FETCH4", "1");

    // SJitter
    appendShaderOption(o.sjitter, "USE_SJITTER", "1");

    // Branching
    appendShaderOption(HW.Caps.raster_major >= 3, "USE_BRANCHING", "1");

    // Vertex texture fetch
    appendShaderOption(HW.Caps.geometry.bVTF, "USE_VTF", "1");

    // Tshadows
    appendShaderOption(o.Tshadows, "USE_TSHADOWS", "1");

    // Motion blur
    appendShaderOption(o.mblur, "USE_MBLUR", "1");

    // Sun filter
    appendShaderOption(o.sunfilter, "USE_SUNFILTER", "1");

    // Static sun on R2 and higher
    appendShaderOption(o.sunstatic, "USE_R2_STATIC_SUN", "1");

    // Force gloss
    {
        xr_sprintf(c_gloss, "%f", o.forcegloss_v);
        appendShaderOption(o.forcegloss, "FORCE_GLOSS", c_gloss);
    }

    // Force skinw
    appendShaderOption(o.forceskinw, "SKIN_COLOR", "1");

    // SSAO Blur
    appendShaderOption(o.ssao_blur_on, "USE_SSAO_BLUR", "1");

    // SSAO HDAO
    if (o.ssao_hdao)
    {
        options.add("HDAO", "1");
        sh_name.append(static_cast<u32>(1)); // HDAO on
        sh_name.append(static_cast<u32>(0)); // HBAO off
        sh_name.append(static_cast<u32>(0)); // HBAO vectorized off
    }
    else // SSAO HBAO
    {
        sh_name.append(static_cast<u32>(0)); // HDAO off
        sh_name.append(o.ssao_hbao);         // HBAO on/off

        appendShaderOption(o.ssao_hbao, "USE_HBAO", "1");
        appendShaderOption(o.hbao_vectorized, "VECTORIZED_CODE", "1");
    }

    if (o.ssao_opt_data)
    {
        if (o.ssao_half_data)
            options.add("SSAO_OPT_DATA", "2");
        else
            options.add("SSAO_OPT_DATA", "1");
    }
    sh_name.append(o.ssao_opt_data ? (o.ssao_half_data ? u32(2) : u32(1)) : u32(0));

    // skinning
    // SKIN_NONE
    appendShaderOption(m_skinning < 0, "SKIN_NONE", "1");

    // SKIN_0
    appendShaderOption(0 == m_skinning, "SKIN_0", "1");

    // SKIN_1
    appendShaderOption(1 == m_skinning, "SKIN_1", "1");

    // SKIN_2
    appendShaderOption(2 == m_skinning, "SKIN_2", "1");

    // SKIN_3
    appendShaderOption(3 == m_skinning, "SKIN_3", "1");

    // SKIN_4
    appendShaderOption(4 == m_skinning, "SKIN_4", "1");

    //	Igor: need restart options
    // Soft water
    {
        const bool softWater = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_WATER);
        appendShaderOption(softWater, "USE_SOFT_WATER", "1");
    }

    // Soft particles
    {
        const bool useSoftParticles = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES);
        appendShaderOption(useSoftParticles, "USE_SOFT_PARTICLES", "1");
    }

    // Depth of field
    {
        const bool dof = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF);
        appendShaderOption(dof, "USE_DOF", "1");
    }

    // FXAA
    // SkyLoader: temporary added
    appendShaderOption(ps_r2_fxaa, "USE_FXAA", "1");
    // end

    // Sun shafts
    if (RImplementation.o.advancedpp && ps_r_sun_shafts)
    {
        xr_sprintf(c_sun_shafts, "%d", ps_r_sun_shafts);
        options.add("SUN_SHAFTS_QUALITY", c_sun_shafts);
        sh_name.append(ps_r_sun_shafts);
    }
    else
        sh_name.append(static_cast<u32>(0));

    if (RImplementation.o.advancedpp && ps_r_ssao)
    {
        xr_sprintf(c_ssao, "%d", ps_r_ssao);
        options.add("SSAO_QUALITY", c_ssao);
        sh_name.append(ps_r_ssao);
    }
    else
        sh_name.append(static_cast<u32>(0));

    // Sun quality
    if (RImplementation.o.advancedpp && ps_r_sun_quality)
    {
        xr_sprintf(c_sun_quality, "%d", ps_r_sun_quality);
        options.add("SUN_QUALITY", c_sun_quality);
        sh_name.append(ps_r_sun_quality);
    }
    else
        sh_name.append(static_cast<u32>(0));

    // Steep parallax
    {
        const bool steepParallax = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX);
        appendShaderOption(steepParallax, "ALLOW_STEEPPARALLAX", "1");
    }

    // Geometry buffer optimization
    appendShaderOption(o.dx10_gbuffer_opt, "GBUFFER_OPTIMIZATION", "1");

    // Shader Model 4.1
    appendShaderOption(o.dx10_sm4_1, "SM_4_1", "1");

    // Minmax SM
    appendShaderOption(o.dx10_minmax_sm, "USE_MINMAX_SM", "1");

    // Shadow of Chernobyl compatibility
    appendShaderOption(ShadowOfChernobylMode, "USE_SHOC_RESOURCES", "1");

    // add a #define for DX10_1 MSAA support
    if (o.dx10_msaa)
    {
        appendShaderOption(o.dx10_msaa, "USE_MSAA", "1");

        {
            static char samples[2];
            samples[0] = char(o.dx10_msaa_samples) + '0';
            samples[1] = 0;
            appendShaderOption(o.dx10_msaa_samples, "MSAA_SAMPLES", samples);
        }

        {
            static char def[2];
            if (m_MSAASample < 0)
                def[0] = '0';
            else
                def[0] = '0' + char(m_MSAASample);

            def[1] = 0;
            options.add("ISAMPLE", def);
            sh_name.append(static_cast<u32>(0));
        }

        appendShaderOption(o.dx10_msaa_opt, "MSAA_OPTIMIZATION", "1");

        switch (o.dx10_msaa_alphatest)
        {
        case MSAA_ATEST_DX10_0_ATOC:
            options.add("MSAA_ALPHATEST_DX10_0_ATOC", "1");

            sh_name.append(static_cast<u32>(1)); // DX10_0_ATOC   on
            sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
            break;
        case MSAA_ATEST_DX10_1_ATOC:
            options.add("MSAA_ALPHATEST_DX10_1_ATOC", "1");

            sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
            sh_name.append(static_cast<u32>(1)); // DX10_1_ATOC   on
            sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
            break;
        case MSAA_ATEST_DX10_1_NATIVE:
            options.add("MSAA_ALPHATEST_DX10_1", "1");

            sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
            sh_name.append(static_cast<u32>(1)); // DX10_1_NATIVE on
            break;
        default:
            sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
        }
    }
    else
    {
        sh_name.append(static_cast<u32>(0)); // MSAA off
        sh_name.append(static_cast<u32>(0)); // No MSAA samples
        sh_name.append(static_cast<u32>(0)); // No MSAA ISAMPLE
        sh_name.append(static_cast<u32>(0)); // No MSAA optimization
        sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
        sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
        sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
    }

    // finish
    options.finish();
    sh_name.finish();

    // Compile sources list
    const size_t head_lines = 2; // "#version" line + name_comment line
    size_t sources_lines = source.size() + options.size() + head_lines;
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
    for (size_t i = 0; i < options.size(); ++i)
    {
        sources[head_lines + i] = options[i].FullDefine;
    }
    CopyMemory(sources + head_lines + options.size(), source.data(), source.size() * sizeof(pstr));

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
