#include "stdafx.h"
#include "r3.h"
#include "Layers/xrRender/ShaderResourceTraits.h"
#include "xrCore/FileCRC32.h"

template <typename T>
static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    T*& result, bool const disasm)
{
    // XXX: disasm it

    result->sh = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size);

    ID3DShaderReflection* pReflection = 0;
    HRESULT const _hr = D3D10ReflectShader(buffer, buffer_size, &pReflection);

    if (SUCCEEDED(_hr) && pReflection)
    {
        // Parse constant table data
        result->constants.parse(pReflection, ShaderTypeTraits<T>::GetShaderDest());

        _RELEASE(pReflection);
    }
    else
    {
        Msg("! D3DReflectShader %s hr == 0x%08x", file_name, _hr);
    }

    return _hr;
}

static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    void*& result, bool const disasm)
{
    // XXX: what's going on with casts here???
    HRESULT _result = E_FAIL;
    if (pTarget[0] == 'p')
    {
        _result = create_shader(pTarget, buffer, buffer_size, file_name, (SPS*&)result, disasm);
    }
    else if (pTarget[0] == 'v')
    {
        // XXX: try to use code below
        // _result = create_shader(pTarget, buffer, buffer_size, file_name, (SVS*&)result, disasm);

        SVS* svs_result = (SVS*)result;
        _result = HW.pDevice->CreateVertexShader(buffer, buffer_size, &svs_result->sh);

        if (!SUCCEEDED(_result))
        {
            Log("! VS: ", file_name);
            Msg("! CreatePixelShader hr == 0x%08x", _result);
            return E_FAIL;
        }

        ID3DShaderReflection* pReflection = 0;
        _result = D3D10ReflectShader(buffer, buffer_size, &pReflection);

        //	Parse constant, texture, sampler binding
        //	Store input signature blob
        if (SUCCEEDED(_result) && pReflection)
        {
            //	TODO: DX10: share the same input signatures

            //	Store input signature (need only for VS)
            // CHK_DX( D3DxxGetInputSignatureBlob(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(),
            // &_vs->signature) );
            ID3DBlob* pSignatureBlob;
            CHK_DX(D3DGetInputSignatureBlob(buffer, buffer_size, &pSignatureBlob));
            VERIFY(pSignatureBlob);

            svs_result->signature = RImplementation.Resources->_CreateInputSignature(pSignatureBlob);

            _RELEASE(pSignatureBlob);

            //	Let constant table parse it's data
            svs_result->constants.parse(pReflection, RC_dest_vertex);

            _RELEASE(pReflection);
        }
        else
        {
            Log("! VS: ", file_name);
            Msg("! D3DXFindShaderComment hr == 0x%08x", _result);
        }
    }
    else if (pTarget[0] == 'g')
    {
        _result = create_shader(pTarget, buffer, buffer_size, file_name, (SGS*&)result, disasm);
    }
    else
    {
        NODEFAULT;
    }

    if (disasm)
    {
        ID3DBlob* disasm = 0;
        D3DDisassemble(buffer, buffer_size, FALSE, 0, &disasm);
        // D3DXDisassembleShader		(LPDWORD(code->GetBufferPointer()), FALSE, 0, &disasm );
        string_path dname;
        strconcat(sizeof(dname), dname, "disasm" DELIMITER, file_name,
            ('v' == pTarget[0]) ? ".vs" : ('p' == pTarget[0]) ? ".ps" : ".gs");
        IWriter* W = FS.w_open("$logs$", dname);
        W->w(disasm->GetBufferPointer(), (u32)disasm->GetBufferSize());
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
        strconcat(sizeof(pname), pname, GEnv.Render->getShaderPath(), pFileName);
        IReader* R = FS.r_open("$game_shaders$", pname);
        if (nullptr == R)
        {
            // possibly in shared directory or somewhere else - open directly
            R = FS.r_open("$game_shaders$", pFileName);
            if (nullptr == R)
                return E_FAIL;
        }

        // duplicate and zero-terminate
        u32 size = R->length();
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
        xr_free(pData);
        return D3D_OK;
    }
};

static inline bool match_shader_id(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result);

HRESULT CRender::shader_compile(LPCSTR name, IReader* fs, LPCSTR pFunctionName, LPCSTR pTarget, DWORD Flags, void*& result)
{
    D3D_SHADER_MACRO defines[128];
    int def_it = 0;
    char c_smapsize[32];
    char c_gloss[32];
    char c_sun_shafts[32];
    char c_ssao[32];
    char c_sun_quality[32];

    char sh_name[MAX_PATH] = "";
    u32 len = 0;
    // options
    {
        xr_sprintf(c_smapsize, "%04d", u32(o.smapsize));
        defines[def_it].Name = "SMAP_size";
        defines[def_it].Definition = c_smapsize;
        def_it++;
        VERIFY(xr_strlen(c_smapsize) == 4 || atoi(c_smapsize) < 16384);
        xr_strcat(sh_name, c_smapsize);
        len += 4;
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

    // Be carefull!!!!! this should be at the end to correctly generate
    // compiled shader name;
    // add a #define for DX10_1 MSAA support
    if (o.dx10_msaa)
    {
        defines[def_it].Name = "USE_MSAA";
        defines[def_it].Definition = "1";
        def_it++;

        static char samples[2];

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

    sh_name[len] = '\0';

    // finish
    defines[def_it].Name = nullptr;
    defines[def_it].Definition = nullptr;
    def_it++;

    if (0 == xr_strcmp(pFunctionName, "main"))
    {
        if ('v' == pTarget[0])
        {
            if (HW.pDevice1 == 0)
                pTarget = D3D10GetVertexShaderProfile(HW.pDevice); // vertex	"vs_4_0";
            else
                pTarget = "vs_4_1"; // pixel	"ps_4_0";
        }
        else if ('p' == pTarget[0])
        {
            if (HW.pDevice1 == 0)
                pTarget = D3D10GetPixelShaderProfile(HW.pDevice); // pixel	"ps_4_0";
            else
                pTarget = "ps_4_1"; // pixel	"ps_4_0";
        }
        else if ('g' == pTarget[0])
        {
            if (HW.pDevice1 == 0)
                pTarget = D3D10GetGeometryShaderProfile(HW.pDevice); // geometry	"gs_4_0";
            else
                pTarget = "gs_4_1"; // pixel	"ps_4_0";
        }
    }

    HRESULT _result = E_FAIL;

    char extension[3];
    string_path folder_name, folder;

    strncpy_s(extension, pTarget, 2);
    strconcat(sizeof(folder), folder, "r3" DELIMITER "objects" DELIMITER "r3" DELIMITER, name, ".", extension);

    FS.update_path(folder_name, "$game_shaders$", folder);
    xr_strcat(folder_name, DELIMITER);

    m_file_set.clear();
    FS.file_list(m_file_set, folder_name, FS_ListFiles | FS_RootOnly, "*");

    string_path temp_file_name, file_name;
    if (!match_shader_id(name, sh_name, m_file_set, temp_file_name))
    {
        string_path file;
        strconcat(sizeof(file), file, "shaders_cache" DELIMITER "r3" DELIMITER, name, ".", extension, DELIMITER, sh_name);
        FS.update_path(file_name, "$app_data_root$", file);
    }
    else
    {
        xr_strcpy(file_name, folder_name);
        xr_strcat(file_name, temp_file_name);
    }

    string_path shadersFolder;
    FS.update_path(shadersFolder, "$game_shaders$", GEnv.Render->getShaderPath());

    u32 fileCrc = 0;
    getFileCrc32(fs, shadersFolder, fileCrc);
    fs->seek(0);

    if (FS.exist(file_name))
    {
        IReader* file = FS.r_open(file_name);
        if (file->length() > 4)
        {
            u32 savedFileCrc = file->r_u32();
            if (savedFileCrc == fileCrc)
            {
                u32 savedBytecodeCrc = file->r_u32();
                u32 bytecodeCrc = crc32(file->pointer(), file->elapsed());
                if (bytecodeCrc == savedBytecodeCrc)
                    _result =
                        create_shader(pTarget, (DWORD*)file->pointer(), file->elapsed(), file_name, result, o.disasm);
            }
        }
        file->close();
    }

    if (FAILED(_result))
    {
        includer Includer;
        LPD3DBLOB pShaderBuf = NULL;
        LPD3DBLOB pErrorBuf = NULL;
        _result = D3DCompile(fs->pointer(), fs->length(), "", defines, &Includer, pFunctionName, pTarget, Flags, 0,
            &pShaderBuf, &pErrorBuf);

#if 0
        if (pErrorBuf)
        {
            std::string shaderErrStr = std::string((const char*)pErrorBuf->GetBufferPointer(), pErrorBuf->GetBufferSize());
            Msg("shader: %s \n %s", name, shaderErrStr.c_str());
        }
#endif
        if (SUCCEEDED(_result))
        {
            IWriter* file = FS.w_open(file_name);

            file->w_u32(fileCrc);

            u32 bytecodeCrc = crc32(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize());
            file->w_u32(bytecodeCrc);

            file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());

            FS.w_close(file);

            _result = create_shader(pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(),
                file_name, result, o.disasm);
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

static inline bool match_shader(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, LPCSTR const mask, size_t const mask_length)
{
    u32 const full_shader_id_length = xr_strlen(full_shader_id);
    R_ASSERT2(full_shader_id_length == mask_length,
        make_string("bad cache for shader %s, [%s], [%s]", debug_shader_id, mask, full_shader_id));
    char const* i = full_shader_id;
    char const* const e = full_shader_id + full_shader_id_length;
    char const* j = mask;
    for (; i != e; ++i, ++j)
    {
        if (*i == *j)
            continue;

        if (*j == '_')
            continue;

        return false;
    }

    return true;
}

static inline bool match_shader_id(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result)
{
#if 1
    strcpy_s(result, "");
    return false;
#else // #if 1
#ifdef DEBUG
    LPCSTR temp = "";
    bool found = false;
    FS_FileSet::const_iterator i = file_set.begin();
    FS_FileSet::const_iterator const e = file_set.end();
    for (; i != e; ++i)
    {
        if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size()))
        {
            VERIFY(!found);
            found = true;
            temp = (*i).name.c_str();
        }
    }

    xr_strcpy(result, temp);
    return found;
#else // #ifdef DEBUG
    FS_FileSet::const_iterator i = file_set.begin();
    FS_FileSet::const_iterator const e = file_set.end();
    for (; i != e; ++i)
    {
        if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size()))
        {
            xr_strcpy(result, (*i).name.c_str());
            return true;
        }
    }

    return false;
#endif // #ifdef DEBUG
#endif // #if 1
}
