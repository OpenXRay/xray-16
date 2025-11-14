#include "stdafx.h"
#include "r2.h"
#include "Layers/xrRender/ShaderResourceTraits.h"
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
        nvrhi::ShaderDesc shaderDesc(T::GetShaderType());
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

        // Populate constant table with texture resources from ExtractedReflection
        // This is needed for r_dx11Texture() calls from script shaders!
        for (const auto& tex : extractedReflection->rtBindings.inputTextures)
        {
            u32 destination = ShaderTypeTraits<T>::GetShaderDest();
            u16 r_index = u16(-1);

            // Apply stage offset based on destination
            if (destination & RC_dest_pixel)
                r_index = u16(tex.slot + CTexture::rstPixel);
            else if (destination & RC_dest_vertex)
                r_index = u16(tex.slot + CTexture::rstVertex);
            else if (destination & RC_dest_geometry)
                r_index = u16(tex.slot + CTexture::rstGeometry);
            else if (destination & RC_dest_hull)
                r_index = u16(tex.slot + CTexture::rstHull);
            else if (destination & RC_dest_domain)
                r_index = u16(tex.slot + CTexture::rstDomain);
            else if (destination & RC_dest_compute)
                r_index = u16(tex.slot + CTexture::rstCompute);
            else
                continue;

            ref_constant C = result->constants.get(tex.name, u16(-1));
            if (!C)
            {
                C = result->constants.table.emplace_back(xr_new<R_constant>());
                C->name = tex.name;
                C->destination = RC_dest_sampler;
                C->type = RC_dx11texture;
                R_constant_load& L = C->samp;
                L.index = r_index;
                L.cls = RC_dx11texture;
                Msg("  [create_shader] Added texture constant '%s' to ctable (index=%u)", tex.name.c_str(), r_index);
            }
        }

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
            result->reflection->constantBuffers.buffers.size(),
            result->reflection->rtBindings.outputRTs.size());
#endif

        // Store bytecode for FrameGraph renderer
#if defined(USE_DX11)
        _hr = D3DCreateBlob(buffer_size, &result->bytecode);
        if (SUCCEEDED(_hr) && result->bytecode)
        {
            CopyMemory(result->bytecode->GetBufferPointer(), buffer, buffer_size);
        }
        else
        {
            Msg("! Failed to create bytecode blob for %s", file_name);
        }
#endif
    }
    else
    {
        // Legacy D3DReflect path (for non-Slang shaders)
        _hr = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size, result->sh);
        if (!SUCCEEDED(_hr))
        {
            Log("! Shader: ", file_name);
            Msg("! CreateHWShader hr == 0x%08x", _hr);
            return E_FAIL;
        }

#ifdef DEBUG
        if (result->sh)
        {
            result->sh->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(file_name), file_name);
        }
#endif

        // Store shader bytecode for NVRHI/FrameGraph renderer
#if defined(USE_DX11)
        _hr = D3DCreateBlob(buffer_size, &result->bytecode);
        if (SUCCEEDED(_hr) && result->bytecode)
        {
            CopyMemory(result->bytecode->GetBufferPointer(), buffer, buffer_size);
        }
        else
        {
            Msg("! Failed to create bytecode blob for %s", file_name);
        }
#endif

        ID3DShaderReflection* pReflection = 0;
        _hr = D3DReflect(buffer, buffer_size, IID_ID3DShaderReflection, (void**)&pReflection);

        if (SUCCEEDED(_hr) && pReflection)
        {
            result->constants.dx9compatibility = false;
            // Parse constant table data
            result->constants.parse(pReflection, ShaderTypeTraits<T>::GetShaderDest());

            _RELEASE(pReflection);
        }
        else
        {
            Msg("! D3DReflectShader %s hr == 0x%08x", file_name, _hr);
        }
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
        if (SUCCEEDED(_result))
        {
            //	Store input signature (need only for VS)
            ID3DBlob* pSignatureBlob;
            CHK_DX(D3DGetInputSignatureBlob(buffer, buffer_size, &pSignatureBlob));
            VERIFY(pSignatureBlob);

            svs_result->signature = RImplementation.Resources->_CreateInputSignature(pSignatureBlob);

            _RELEASE(pSignatureBlob);
        }
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

    HRESULT _result = E_FAIL;

    // Route through Slang if FrameGraph is active
    extern ENGINE_API int ps_r4_use_framegraph;
    if (ps_r4_use_framegraph)
    {
        if (!m_shaderLoader)
        {
            Msg("! [shader_compile] FrameGraph enabled but ShaderLoader not initialized!");
            Msg("! Shader: %s (target: %s)", name, pTarget);
            R_ASSERT2(false, "ShaderLoader must be initialized before compiling shaders with FrameGraph");
            return E_FAIL;
        }
        // Map shader target to stage
        xray::render::SlangCompiler::Stage stage;
        char extension[3];
        strncpy_s(extension, pTarget, 2);

        if (strcmp(extension, "vs") == 0)
            stage = xray::render::SlangCompiler::Stage::Vertex;
        else if (strcmp(extension, "ps") == 0)
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
            Msg("! Unknown shader stage: %s", extension);
            return E_FAIL;
        }

        // Build a string containing all defines for hash computation
        xr_string definesStr;
        for (const D3D_SHADER_MACRO* macro = options.data(); macro->Name != nullptr; ++macro)
        {
            definesStr.append(macro->Name);
            definesStr.append("=");
            if (macro->Definition)
                definesStr.append(macro->Definition);
            definesStr.append(";");
        }

        // Compute hash of shader source + defines for cache lookup
        u32 sourceHash = xray::render::framegraph::ShaderCache::ComputeHash(
            (const char*)fs->pointer(),
            fs->length(),
            definesStr.c_str()
        );

        // Try to load from FrameGraph shader cache
        xr_vector<u8> cachedBytecode;
        static xray::render::framegraph::ShaderCache s_cache;  // Static cache instance for r4_shaders

        // Try to load bytecode + reflection from cache
        xray::render::framegraph::ExtractedReflection deserializedReflection;
        bool cacheHit = s_cache.TryLoad(name, extension, sourceHash, cachedBytecode, &deserializedReflection);

        if (cacheHit && !deserializedReflection.IsEmpty())
        {
            // Cache hit WITH valid reflection! Create shader from cached data
            _result = create_shader(
                pTarget,
                (DWORD*)cachedBytecode.data(),
                cachedBytecode.size(),
                name,
                result,
                o.disasm,
                &deserializedReflection
            );

            if (SUCCEEDED(_result))
            {
                Msg("  ✓ [shader_compile] Cache HIT (with reflection): %s (%zu bytes)", name, cachedBytecode.size());
            }

            return _result;
        }
        else if (cacheHit)
        {
            // Cache hit but NO reflection! This is an old cache file - RECOMPILE
            Msg("! [shader_compile] Cache hit but no reflection for %s - recompiling to extract reflection", name);
        }

        // Cache miss - compile with Slang
        Msg("~ [shader_compile] Compiling with Slang: %s (target: %s)", name, pTarget);

        // Copy shader source to null-terminated string (Slang needs C-string)
        xr_string sourceCode;
        sourceCode.assign((const char*)fs->pointer(), fs->length());

        // Construct full shader path for Slang (needed for include resolution)
        // e.g., "r5/simple_color.ps" so includes like "common.h" resolve to "r5/common.h"
        string_path fullShaderPath;
        strconcat(sizeof(fullShaderPath), fullShaderPath, RImplementation.getShaderPath(), name);

        // Append shader extension based on stage
        if (strcmp(extension, "vs") == 0)
            xr_strcat(fullShaderPath, ".vs");
        else if (strcmp(extension, "ps") == 0)
            xr_strcat(fullShaderPath, ".ps");
        else if (strcmp(extension, "gs") == 0)
            xr_strcat(fullShaderPath, ".gs");
        else if (strcmp(extension, "hs") == 0)
            xr_strcat(fullShaderPath, ".hs");
        else if (strcmp(extension, "ds") == 0)
            xr_strcat(fullShaderPath, ".ds");
        else if (strcmp(extension, "cs") == 0)
            xr_strcat(fullShaderPath, ".cs");

        // Convert D3D_SHADER_MACRO to SlangCompiler::Define
        // Count defines (options.data() is null-terminated array)
        size_t defineCount = 0;
        for (const D3D_SHADER_MACRO* macro = options.data(); macro->Name != nullptr; ++macro)
            defineCount++;

        // Convert to SlangCompiler::Define format
        xr_vector<xray::render::SlangCompiler::Define> slangDefines;
        slangDefines.reserve(defineCount);
        for (size_t i = 0; i < defineCount; ++i)
        {
            xray::render::SlangCompiler::Define define;
            define.name = options.data()[i].Name;
            define.value = options.data()[i].Definition;
            slangDefines.push_back(define);
        }

        // Compile with Slang (with preprocessor defines!)
        xray::render::SlangCompiler::CompileResult compileResult =
            m_shaderLoader->GetSlangCompiler()->CompileFromSource(
                sourceCode.c_str(),
                pFunctionName,
                stage,
                xray::render::SlangCompiler::Target::DXBC,
                fullShaderPath,  // Pass full path for include resolution
                slangDefines.data(),  // Pass preprocessor defines
                slangDefines.size()
            );

        if (!compileResult.IsValid())
        {
            // Store failed shader info for batch reporting
            xr_string failedInfo;
            failedInfo.append("║   - ");
            failedInfo.append(name);
            failedInfo.append(" (");
            failedInfo.append(pTarget);
            failedInfo.append(")");
            if (!compileResult.errorMessage.empty())
            {
                failedInfo.append("\n║     Error: ");
                // Only include first line of error to keep it concise
                const char* newline = strchr(compileResult.errorMessage.c_str(), '\n');
                if (newline)
                    failedInfo.append(compileResult.errorMessage.c_str(), newline - compileResult.errorMessage.c_str());
                else
                    failedInfo.append(compileResult.errorMessage.c_str());
            }
            g_failedShaders.push_back(failedInfo);

            // Log brief error but continue compiling
            Msg("! [shader_compile] Failed: %s (target: %s) - continuing...", name, pTarget);

            // Don't assert - just return E_FAIL and let it continue
            return E_FAIL;
        }

        // Extract reflection data BEFORE creating shader
        bool isVertexShader = (strcmp(extension, "vs") == 0);
        auto extractedReflection = xray::render::framegraph::ShaderReflector::ExtractReflection(
            compileResult.reflection,
            isVertexShader
        );

        // Create the shader with Slang-compiled bytecode and extracted reflection
        _result = create_shader(
            pTarget,                                      // Shader target (vs_5_0, ps_5_0, etc.)
            (DWORD*)compileResult.bytecode.data(),       // Bytecode buffer
            compileResult.bytecode.size(),                // Bytecode size
            name,                                         // Shader name
            result,                                       // Output shader object
            o.disasm,                                     // Disassemble flag
            &extractedReflection                          // Extracted reflection data
        );

        if (SUCCEEDED(_result))
        {
            Msg("* [shader_compile] Successfully compiled with Slang: %s (%zu bytes)",
                name, compileResult.bytecode.size());

            // Save bytecode + reflection to FrameGraph shader cache for future runs
            s_cache.Save(name, extension, sourceHash, compileResult.bytecode, &extractedReflection);
        }

        return _result;
    }

    char extension[3];
    strncpy_s(extension, pTarget, 2);

    pcstr renderer;
    if (HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0)
        renderer = "r4" DELIMITER;
    else if (HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_0)
        renderer = "r3" DELIMITER;
    else
    {
        renderer = "r4_level9" DELIMITER;
        R_ASSERT(!"Feature levels lower than 10.0 are unsupported");
    }

    string_path filename;
    strconcat(sizeof(filename), filename, renderer, name, ".", extension);

    string_path file_name;
    {
        string_path file;
        strconcat(sizeof(file), file, "shaders_cache_oxr" DELIMITER, filename, DELIMITER, sh_name.c_str());
        strconcat(sizeof(filename), filename, filename, DELIMITER, sh_name.c_str());
        FS.update_path(file_name, "$app_data_root$", file);
    }

    string_path shadersFolder;
    FS.update_path(shadersFolder, "$game_shaders$", RImplementation.getShaderPath());

    u32 fileCrc = 0;
    getFileCrc32(fs, shadersFolder, fileCrc);
    fs->seek(0);

    if (FS.exist(file_name))
    {
        IReader* file = FS.r_open(file_name);
        if (file->length() > 4)
        {
            const bool dx9compatibility = file->r_u32();

            u32 savedFileCrc = file->r_u32();
            if (savedFileCrc == fileCrc)
            {
                u32 savedBytecodeCrc = file->r_u32();
                u32 bytecodeCrc = crc32(file->pointer(), file->elapsed());
                if (bytecodeCrc == savedBytecodeCrc)
                {
#ifdef DEBUG
                    Log("* Loading shader:", file_name);
#endif
                    _result =
                        create_shader(pTarget, (DWORD*)file->pointer(), file->elapsed(),
                            filename, result, o.disasm, nullptr);
                }
            }
        }
        file->close();
    }

    if (FAILED(_result))
    {
        includer Includer;
        LPD3DBLOB pShaderBuf = NULL;
        LPD3DBLOB pErrorBuf = NULL;
        _result = HW.D3DCompile(fs->pointer(), fs->length(), "", options.data(),
            &Includer, pFunctionName, pTarget, Flags, 0, &pShaderBuf, &pErrorBuf);

        if (FAILED(_result) && pErrorBuf)
        {
            cpcstr str = static_cast<cpcstr>(pErrorBuf->GetBufferPointer());
            if (strstr(str, "error X3523")) // is there a better way?
            {
                pErrorBuf = nullptr;
                Flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
                _result = HW.D3DCompile(fs->pointer(), fs->length(), "", options.data(),
                    &Includer, pFunctionName, pTarget, Flags, 0, &pShaderBuf, &pErrorBuf);
            }
        }

        if (SUCCEEDED(_result))
        {
            const bool dx9compatibility = Flags & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
            IWriter* file = FS.w_open(file_name);

            file->w_u32(dx9compatibility);
            file->w_u32(fileCrc);

            u32 bytecodeCrc = crc32(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize());
            file->w_u32(bytecodeCrc); // Do not write anything below this line, take a look at reading (crc32)

            file->w(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize());
            FS.w_close(file);
#ifdef DEBUG
            Log("- Compile shader:", file_name);
#endif
            _result = create_shader(pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(),
                filename, result, o.disasm, nullptr);
        }
        else
        {
            Log("! ", file_name);
            if (pErrorBuf)
                Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
            else
                Msg("Can't compile shader hr=0x%08x", _result);
        }
    }

    return _result;
}
} // namespace xray::render::RENDER_NAMESPACE
