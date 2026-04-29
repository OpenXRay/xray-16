#include "stdafx.h"
#include "r2.h"
#include "Layers/xrRenderDX11/ShaderResourceTraits.h"
#include "xrCore/FileCRC32.h"
#include "Layers/xrRender/Shaders/SlangCompiler.h"
#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::RENDER_NAMESPACE
{
// Extern reference to global failed shaders list (defined in r2.cpp)
extern xr_vector<xr_string> g_failedShaders;

void CRender::addShaderOption(const char* name, const char* value)
{
    D3D_SHADER_MACRO macro = {name, value};
    m_ShaderOptions.push_back(macro);
}

template <typename T>
static HRESULT create_shader(DWORD const* buffer, size_t const buffer_size, LPCSTR const file_name,
    T*& result, xray::render::framegraph::ExtractedReflection* extractedReflection)
{
    HRESULT _hr = S_OK;

    // Create native NVRHI shader (Slang path)
    if (extractedReflection)
    {
        nvrhi::ShaderDesc shaderDesc{};
        shaderDesc.shaderType = T::GetShaderType();
        shaderDesc.debugName = file_name;
        shaderDesc.entryName = "main";

        result->nvrhiShader = RImplementation.m_renderDevice->GetNativeDevice()->createShader(
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

        // Parse constant buffers for legacy constant table from ExtractedReflection
        result->constants.dx9compatibility = false;  // Slang shaders are never DX9 compatible

        // ═══════════════════════════════════════════════════
        //  STORE EXTRACTED REFLECTION IN SHADER STRUCT
        // ═══════════════════════════════════════════════════
        // This reflection data is extracted once (from live Slang OR deserialized from cache)
        // and reused everywhere (MaterialCache, ShaderReflector, etc.)
#if defined(USE_DX11)
        // Allocate and copy extracted reflection
        result->reflection = xr_new<xray::render::framegraph::ExtractedReflection>(*extractedReflection);

        // Debug: Verify reflection was stored
        Msg("  [create_shader] Stored reflection for %s: vsInputs=%u, constantBuffers=%u, rtBindings=%u",
            file_name,
            result->reflection->vertexInputSignature.elements.size(),
            result->reflection->constantLayout.constantBuffers.buffers.size(),
            result->reflection->rtBindings.outputRTs.size());
#endif

        // Bytecode is now only stored internally by NVRHI, not in shader struct
    }
    else
    {
        // Should not happen - all shaders must have reflection in FrameGraph mode
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
        // Input signature is now handled via reflection data, no need for legacy signature
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
            // possibly in shared directory or somewhere else - open directly
            R = FS.r_open("$game_shaders$", pFileName);
            if (nullptr == R)
                return E_FAIL;
        }

        // duplicate and zero-terminate
        const size_t size = R->length();
        u8* data = xr_alloc<u8>(size + 1);
        CopyMemory(data, R->pointer(), size);
        data[size] = 0;
        FS.r_close(R);

        *ppData = data;
        *pBytes = size;
        return D3D_OK;
    }
    HRESULT __stdcall Close(LPCVOID pData)
    {
        auto mutableData = const_cast<LPVOID>(pData);
        xr_free(mutableData);
        return D3D_OK;
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
    D3D_SHADER_MACRO m_options[128];

public:
    void add(const xr_vector<D3D_SHADER_MACRO>& macros)
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

    D3D_SHADER_MACRO* data() { return m_options; }
};

HRESULT CRender::shader_compile(pcstr name, IReader* fs, pcstr pFunctionName,
    pcstr pTarget, u32 Flags, void*& result)
{
    shader_options_holder options;
    shader_name_holder sh_name;

    // Don't move these variables to lower scope!
    string32 c_smap;
    string32 c_gloss;
    string32 c_sun_shafts;
    string32 c_ssao;
    string32 c_sun_quality;
    char c_msaa_samples[2];
    char c_msaa_current_sample[2];

    // options:
    const auto appendShaderOption = [&](u32 option, cpcstr macro, cpcstr value)
    {
        if (option)
            options.add(macro, value);

        sh_name.append(option);
    };

    // External defines
    options.add(m_ShaderOptions);

    // Shadow map size
    {
        xr_itoa(m_SMAPSize, c_smap, 10);
        options.add("SMAP_size", c_smap);
        sh_name.append(c_smap);
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
    const auto& caps = GEnv.Backend->GetCapabilities();
    appendShaderOption(caps.raster_major >= 3, "USE_BRANCHING", "1");

    // Vertex texture fetch
    appendShaderOption(caps.geometry.bVTF, "USE_VTF", "1");

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
        sh_name.append(static_cast<u32>(0)); // Half data off
    }
    else // SSAO HBAO
    {
        sh_name.append(static_cast<u32>(0)); // HDAO off
        sh_name.append(o.ssao_hbao);         // HBAO on/off
        sh_name.append(o.ssao_half_data);    // Half data on/off

        if (o.ssao_hbao)
        {
            if (o.ssao_half_data)
                options.add("SSAO_OPT_DATA", "2");
            else
                options.add("SSAO_OPT_DATA", "1");

            if (o.hbao_vectorized)
                options.add("VECTORIZED_CODE", "1");

            options.add("USE_HBAO", "1");
        }
    }

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
    appendShaderOption(o.gbuffer_opt, "GBUFFER_OPTIMIZATION", "1");

    // Shader Model 4.1
    appendShaderOption(o.dx11_sm4_1, "SM_4_1", "1");

    // Shader Model 5.0
    appendShaderOption(HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0, "SM_5", "1");

    // Double precision
    appendShaderOption(HW.DoublePrecisionFloatShaderOps, "DOUBLE_PRECISION", "1");

    // Extended doubles instructions
    appendShaderOption(HW.ExtendedDoublesShaderInstructions, "EXTENDED_DOUBLES", "1");

    // SAD4 intrinsic support
    appendShaderOption(HW.SAD4ShaderInstructions, "SAD4_SUPPORTED", "1");

    // Minmax SM
    appendShaderOption(o.minmax_sm, "USE_MINMAX_SM", "1");

    // Be carefull!!!!! this should be at the end to correctly generate
    // compiled shader name;
    // add a #define for DX10_1 MSAA support
    if (o.msaa)
    {
        appendShaderOption(o.msaa, "USE_MSAA", "1");

        // Number of samples
        {
            c_msaa_samples[0] = char(o.msaa_samples) + '0';
            c_msaa_samples[1] = 0;
            appendShaderOption(o.msaa_samples, "MSAA_SAMPLES", c_msaa_samples);
        }
        // Current sample
        {
            if (m_MSAASample < 0 || o.msaa_opt)
                c_msaa_current_sample[0] = '0';
            else
                c_msaa_current_sample[0] = '0' + char(m_MSAASample);
            c_msaa_current_sample[1] = 0;

            appendShaderOption(m_MSAASample >= 0 ? m_MSAASample : 0,
                "ISAMPLE", c_msaa_current_sample);
        }

        appendShaderOption(o.msaa_opt, "MSAA_OPTIMIZATION", "1");

        switch (o.msaa_alphatest)
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
        sh_name.append(static_cast<u32>(0)); // No MSAA optimization
        sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
        sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
        sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
    }

    // finish
    options.finish();
    sh_name.finish();

    if (!m_shaderLoader)
    {
        Msg("! [shader_compile] ShaderLoader not initialized!");
        R_ASSERT2(false, "ShaderLoader must be initialized");
        return E_FAIL;
    }

    // Determine shader stage from target
    char extension[3];
    strncpy_s(extension, pTarget, 2);

    bool isVertexShader = (strcmp(extension, "vs") == 0);
    bool isPixelShader = (strcmp(extension, "ps") == 0);

    // Build defines string for logging
    xr_string definesStr;
    for (const D3D_SHADER_MACRO* macro = options.data(); macro->Name != nullptr; ++macro)
    {
        definesStr.append(macro->Name);
        definesStr.append("=");
        if (macro->Definition)
            definesStr.append(macro->Definition);
        definesStr.append(";");
    }

    Msg("~ [shader_compile] Compiling '%s' (target:%s, defines:%s)",
        name, pTarget, definesStr.c_str());

    // Use ShaderLoader's CompileShaderWithDefines (which handles caching internally!)
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

    // Convert D3D_SHADER_MACRO to SlangCompiler::Define
    size_t defineCount = 0;
    for (const D3D_SHADER_MACRO* macro = options.data(); macro->Name != nullptr; ++macro)
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
    bool success = m_shaderLoader->CompileShaderWithDefines(
        name,           // Shader name
        extWithDot,     // Extension (.vs, .ps, etc.)
        pFunctionName,  // Entry point
        stage,          // Shader stage
        slangDefines,   // Preprocessor defines
        bytecode        // Output bytecode
    );

    if (!success)
    {
        Msg("! [shader_compile] ShaderLoader failed to compile '%s'", name);

        // Store failed shader info for batch reporting
        xr_string failedInfo;
        failedInfo.append("║   - ");
        failedInfo.append(name);
        failedInfo.append(" (");
        failedInfo.append(pTarget);
        failedInfo.append(")");
        g_failedShaders.push_back(failedInfo);

        return E_FAIL;
    }

    // Get reflection data from ShaderLoader's cache
    auto* reflection = m_shaderLoader->GetCachedReflection(name, extWithDot);

    // Create the hardware shader object
    HRESULT _result = create_shader(
        pTarget,                          // Shader target
        (DWORD*)bytecode.data(),         // Bytecode
        bytecode.size(),                  // Size
        name,                             // Name
        result,                           // Output shader object
        o.disasm,                         // Disassembly flag
        reflection                        // Reflection data
    );

    if (SUCCEEDED(_result))
    {
        Msg("* [shader_compile] SUCCESS via ShaderLoader: '%s' (%zu bytes)",
            name, bytecode.size());
    }

    return _result;
}
} // namespace xray::render::RENDER_NAMESPACE
