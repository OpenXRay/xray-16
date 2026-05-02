#include "stdafx.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrCore/FileCRC32.h"
#include "Layers/xrRender/Shaders/SlangCompiler.h"
#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render
{
using namespace fg;

xr_vector<xr_string> g_failedShaders;

void FrameGraphRenderer::addShaderOption(const char* name, const char* value)
{
    m_ShaderOptions.push_back(ShaderMacro{name, value});
}

template <typename T>
static HRESULT create_shader(DWORD const* buffer, size_t const buffer_size, LPCSTR const file_name,
    T*& result, xray::render::framegraph::ExtractedReflection* extractedReflection)
{
    HRESULT _hr = S_OK;

    if (extractedReflection)
    {
        nvrhi::ShaderDesc shaderDesc{};
        shaderDesc.shaderType = T::GetShaderType();
        shaderDesc.debugName = file_name;
        shaderDesc.entryName = "main";

        result->nvrhiShader = RImplementation.GetRenderDevice()->GetNativeDevice()->createShader(
            shaderDesc,
            buffer,
            buffer_size
        );

        _hr = result->nvrhiShader ? S_OK : E_FAIL;
        if (!SUCCEEDED(_hr))
        {
            Log("! Shader: ", file_name);
            Msg("! NVRHI createShader failed");
            return E_FAIL;
        }

        result->constants.dx9compatibility = false;

#if defined(USE_DX11)
        result->reflection = xr_new<xray::render::framegraph::ExtractedReflection>(*extractedReflection);

        Msg("  [create_shader] Stored reflection for %s: vsInputs=%u, constantBuffers=%u, rtBindings=%u",
            file_name,
            result->reflection->vertexInputSignature.elements.size(),
            result->reflection->constantLayout.constantBuffers.buffers.size(),
            result->reflection->rtBindings.outputRTs.size());
#endif

    }
    else
    {
        Msg("! ERROR: Shader %s created without reflection data", file_name);
        R_ASSERT2(false, "All shaders must have reflection data in FrameGraph mode");
        return E_FAIL;
    }

    return _hr;
}

static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, size_t const buffer_size, LPCSTR const file_name,
    void*& result, bool const disasm, xray::render::framegraph::ExtractedReflection* extractedReflection)
{
    HRESULT _result = E_FAIL;
    pcstr extension = ".hlsl";
    if (pTarget[0] == 'p')
    {
        extension = ".ps";
        _result = create_shader(buffer, buffer_size, file_name, (SPS*&)result, extractedReflection);
    }
    else if (pTarget[0] == 'v')
    {
        extension = ".vs";
        SVS* svs_result = (SVS*)result;
        _result = create_shader(buffer, buffer_size, file_name, svs_result, extractedReflection);
    }
    else if (pTarget[0] == 'g')
    {
        extension = ".gs";
        _result = create_shader(buffer, buffer_size, file_name, (SGS*&)result, extractedReflection);
    }
    else if (pTarget[0] == 'c')
    {
        extension = ".cs";
        _result = create_shader(buffer, buffer_size, file_name, (SCS*&)result, extractedReflection);
    }
    else if (pTarget[0] == 'h')
    {
        extension = ".hs";
        _result = create_shader(buffer, buffer_size, file_name, (SHS*&)result, extractedReflection);
    }
    else if (pTarget[0] == 'd')
    {
        extension = ".ds";
        _result = create_shader(buffer, buffer_size, file_name, (SDS*&)result, extractedReflection);
    }
    else
    {
        NODEFAULT;
    }

    if (disasm)
    {
        ID3DBlob* disasm = 0;
        D3DDisassemble(buffer, buffer_size, FALSE, 0, &disasm);
        if (!disasm)
            return _result;

        string_path dname;
        strconcat(sizeof(dname), dname, "disasm" DELIMITER, file_name, extension);
        IWriter* W = FS.w_open("$app_data_root$", dname);
        W->w(disasm->GetBufferPointer(), disasm->GetBufferSize());
        FS.w_close(W);
        _RELEASE(disasm);
    }

    return _result;
}

class includer : public ID3DInclude
{
public:
    HRESULT __stdcall Open(
        D3D10_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        string_path pname;
        strconcat(pname, RImplementation.getShaderPath(), pFileName);
        IReader* R = FS.r_open("$game_shaders$", pname);
        if (nullptr == R)
        {
            R = FS.r_open("$game_shaders$", pFileName);
            if (nullptr == R)
                return E_FAIL;
        }

        const size_t size = R->length();
        u8* data = xr_alloc<u8>(size + 1);
        CopyMemory(data, R->pointer(), size);
        data[size] = 0;
        FS.r_close(R);

        *ppData = data;
        *pBytes = size;
        return S_OK;
    }
    HRESULT __stdcall Close(LPCVOID pData)
    {
        auto mutableData = const_cast<LPVOID>(pData);
        xr_free(mutableData);
        return S_OK;
    }
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
        name[pos] = '0' + char(value);
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
    ShaderMacro m_options[128];

public:
    void add(const xr_vector<ShaderMacro>& macros)
    {
        for (auto macro : macros)
        {
            m_options[pos] = std::move(macro);
            ++pos;
        }
    }

    void add(cpcstr name, cpcstr value)
    {
        m_options[pos] = { name, value };
        ++pos;
    }

    void finish()
    {
        m_options[pos] = { nullptr, nullptr };
    }

    ShaderMacro* data() { return m_options; }
};

HRESULT FrameGraphRenderer::shader_compile(pcstr name, IReader* fs, pcstr pFunctionName,
    pcstr pTarget, u32 Flags, void*& result)
{
    shader_options_holder options;
    shader_name_holder sh_name;

    string32 c_smap;
    string32 c_gloss;
    string32 c_sun_shafts;
    string32 c_ssao;
    string32 c_sun_quality;
    char c_msaa_samples[2];
    char c_msaa_current_sample[2];

    const auto appendShaderOption = [&](u32 option, cpcstr macro, cpcstr value)
    {
        if (option)
            options.add(macro, value);

        sh_name.append(option);
    };

    options.add(m_ShaderOptions);

    {
        xr_itoa(m_SMAPSize, c_smap, 10);
        options.add("SMAP_size", c_smap);
        sh_name.append(c_smap);
    }

    appendShaderOption(RImplementation.o.fp16_filter, "FP16_FILTER", "1");

    appendShaderOption(RImplementation.o.fp16_blend, "FP16_BLEND", "1");

    appendShaderOption(RImplementation.o.HW_smap, "USE_HWSMAP", "1");

    appendShaderOption(RImplementation.o.HW_smap_PCF, "USE_HWSMAP_PCF", "1");

    appendShaderOption(RImplementation.o.HW_smap_FETCH4, "USE_FETCH4", "1");

    appendShaderOption(RImplementation.o.sjitter, "USE_SJITTER", "1");

    const auto& caps = GEnv.Backend->GetCapabilities();
    appendShaderOption(caps.raster_major >= 3, "USE_BRANCHING", "1");

    appendShaderOption(caps.geometry.bVTF, "USE_VTF", "1");

    appendShaderOption(RImplementation.o.Tshadows, "USE_TSHADOWS", "1");

    appendShaderOption(RImplementation.o.mblur, "USE_MBLUR", "1");

    appendShaderOption(RImplementation.o.sunfilter, "USE_SUNFILTER", "1");

    appendShaderOption(RImplementation.o.sunstatic, "USE_R2_STATIC_SUN", "1");

    {
        xr_sprintf(c_gloss, "%f", RImplementation.o.forcegloss_v);
        appendShaderOption(RImplementation.o.forcegloss, "FORCE_GLOSS", c_gloss);
    }

    appendShaderOption(RImplementation.o.forceskinw, "SKIN_COLOR", "1");

    appendShaderOption(RImplementation.o.ssao_blur_on, "USE_SSAO_BLUR", "1");

    if (RImplementation.o.ssao_hdao)
    {
        options.add("HDAO", "1");
        sh_name.append(static_cast<u32>(1));
        sh_name.append(static_cast<u32>(0));
        sh_name.append(static_cast<u32>(0));
    }
    else
    {
        sh_name.append(static_cast<u32>(0));
        sh_name.append(RImplementation.o.ssao_hbao);
        sh_name.append(RImplementation.o.ssao_half_data);

        if (RImplementation.o.ssao_hbao)
        {
            if (RImplementation.o.ssao_half_data)
                options.add("SSAO_OPT_DATA", "2");
            else
                options.add("SSAO_OPT_DATA", "1");

            if (RImplementation.o.hbao_vectorized)
                options.add("VECTORIZED_CODE", "1");

            options.add("USE_HBAO", "1");
        }
    }

    appendShaderOption(m_skinning < 0, "SKIN_NONE", "1");

    appendShaderOption(0 == m_skinning, "SKIN_0", "1");

    appendShaderOption(1 == m_skinning, "SKIN_1", "1");

    appendShaderOption(2 == m_skinning, "SKIN_2", "1");

    appendShaderOption(3 == m_skinning, "SKIN_3", "1");

    appendShaderOption(4 == m_skinning, "SKIN_4", "1");

    {
        const bool softWater = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_WATER);
        appendShaderOption(softWater, "USE_SOFT_WATER", "1");
    }

    {
        const bool useSoftParticles = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES);
        appendShaderOption(useSoftParticles, "USE_SOFT_PARTICLES", "1");
    }

    {
        const bool dof = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF);
        appendShaderOption(dof, "USE_DOF", "1");
    }

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

    if (RImplementation.o.advancedpp && ps_r_sun_quality)
    {
        xr_sprintf(c_sun_quality, "%d", ps_r_sun_quality);
        options.add("SUN_QUALITY", c_sun_quality);
        sh_name.append(ps_r_sun_quality);
    }
    else
        sh_name.append(static_cast<u32>(0));

    {
        const bool steepParallax = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX);
        appendShaderOption(steepParallax, "ALLOW_STEEPPARALLAX", "1");
    }

    appendShaderOption(RImplementation.o.gbuffer_opt, "GBUFFER_OPTIMIZATION", "1");

    appendShaderOption(RImplementation.o.dx11_sm4_1, "SM_4_1", "1");

    appendShaderOption(true, "SM_5", "1");

    appendShaderOption(RImplementation.o.minmax_sm, "USE_MINMAX_SM", "1");

    if (RImplementation.o.msaa)
    {
        appendShaderOption(RImplementation.o.msaa, "USE_MSAA", "1");

        {
            c_msaa_samples[0] = char(RImplementation.o.msaa_samples) + '0';
            c_msaa_samples[1] = 0;
            appendShaderOption(RImplementation.o.msaa_samples, "MSAA_SAMPLES", c_msaa_samples);
        }
        {
            if (m_MSAASample < 0 || RImplementation.o.msaa_opt)
                c_msaa_current_sample[0] = '0';
            else
                c_msaa_current_sample[0] = '0' + char(m_MSAASample);
            c_msaa_current_sample[1] = 0;

            appendShaderOption(m_MSAASample >= 0 ? m_MSAASample : 0,
                "ISAMPLE", c_msaa_current_sample);
        }

        appendShaderOption(RImplementation.o.msaa_opt, "MSAA_OPTIMIZATION", "1");

        switch (RImplementation.o.msaa_alphatest)
        {
        case FrameGraphRenderer::MSAA_ATEST_DX10_0_ATOC:
            options.add("MSAA_ALPHATEST_DX10_0_ATOC", "1");

            sh_name.append(static_cast<u32>(1));
            sh_name.append(static_cast<u32>(0));
            sh_name.append(static_cast<u32>(0));
            break;
        case FrameGraphRenderer::MSAA_ATEST_DX10_1_ATOC:
            options.add("MSAA_ALPHATEST_DX10_1_ATOC", "1");

            sh_name.append(static_cast<u32>(0));
            sh_name.append(static_cast<u32>(1));
            sh_name.append(static_cast<u32>(0));
            break;
        case FrameGraphRenderer::MSAA_ATEST_DX10_1_NATIVE:
            options.add("MSAA_ALPHATEST_DX10_1", "1");

            sh_name.append(static_cast<u32>(0));
            sh_name.append(static_cast<u32>(0));
            sh_name.append(static_cast<u32>(1));
            break;
        default:
            sh_name.append(static_cast<u32>(0));
            sh_name.append(static_cast<u32>(0));
            sh_name.append(static_cast<u32>(0));
        }
    }
    else
    {
        sh_name.append(static_cast<u32>(0));
        sh_name.append(static_cast<u32>(0));
        sh_name.append(static_cast<u32>(0));
        sh_name.append(static_cast<u32>(0));
        sh_name.append(static_cast<u32>(0));
        sh_name.append(static_cast<u32>(0));
    }

    options.finish();
    sh_name.finish();

    if (!GEnv.Render->GetShaderLoader())
    {
        Msg("! [shader_compile] ShaderLoader not initialized!");
        R_ASSERT2(false, "ShaderLoader must be initialized");
        return E_FAIL;
    }

    char extension[3];
    strncpy_s(extension, pTarget, 2);

    bool isVertexShader = (strcmp(extension, "vs") == 0);
    bool isPixelShader = (strcmp(extension, "ps") == 0);

    xr_string definesStr;
    for (const ShaderMacro* macro = options.data(); macro->Name != nullptr; ++macro)
    {
        definesStr.append(macro->Name);
        definesStr.append("=");
        if (macro->Definition)
            definesStr.append(macro->Definition);
        definesStr.append(";");
    }

    Msg("~ [shader_compile] Compiling '%s' (target:%s, defines:%s)",
        name, pTarget, definesStr.c_str());

    xr_vector<u8> bytecode;
    xray::render::SlangCompiler::Stage stage;

    if (isVertexShader)
        stage = xray::render::SlangCompiler::Stage::Vertex;
    else if (isPixelShader)
        stage = xray::render::SlangCompiler::Stage::Pixel;
    else if (strcmp(extension, "gs") == 0)
        stage = xray::render::SlangCompiler::Stage::Geometry;
    else if (strcmp(extension, "hs") == 0)
        stage = xray::render::SlangCompiler::Stage::Hull;
    else if (strcmp(extension, "ds") == 0)
        stage = xray::render::SlangCompiler::Stage::Domain;
    else if (strcmp(extension, "cs") == 0)
        stage = xray::render::SlangCompiler::Stage::Compute;
    else
    {
        Msg("! [shader_compile] Unknown stage: %s", extension);
        return E_FAIL;
    }

    size_t defineCount = 0;
    for (const ShaderMacro* macro = options.data(); macro->Name != nullptr; ++macro)
        defineCount++;

    xr_vector<xray::render::SlangCompiler::Define> slangDefines;
    slangDefines.reserve(defineCount);
    for (size_t i = 0; i < defineCount; ++i)
    {
        xray::render::SlangCompiler::Define define;
        define.name = options.data()[i].Name;
        define.value = options.data()[i].Definition;
        slangDefines.push_back(define);
    }

    char extWithDot[4] = ".";
    xr_strcat(extWithDot, extension);
    bool success = GEnv.Render->GetShaderLoader()->CompileShaderWithDefines(
        name,
        extWithDot,
        pFunctionName,
        stage,
        slangDefines,
        bytecode
    );

    if (!success)
    {
        Msg("! [shader_compile] ShaderLoader failed to compile '%s'", name);

        xr_string failedInfo;
        failedInfo.append("║   - ");
        failedInfo.append(name);
        failedInfo.append(" (");
        failedInfo.append(pTarget);
        failedInfo.append(")");
        g_failedShaders.push_back(failedInfo);

        return E_FAIL;
    }

    auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection(name, extWithDot);

    HRESULT _result = create_shader(
        pTarget,
        (DWORD*)bytecode.data(),
        bytecode.size(),
        name,
        result,
        RImplementation.o.disasm,
        reflection
    );

    if (SUCCEEDED(_result))
    {
        Msg("* [shader_compile] SUCCESS via ShaderLoader: '%s' (%zu bytes)",
            name, bytecode.size());
    }

    return _result;
}
}
