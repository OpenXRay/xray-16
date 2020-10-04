#include "stdafx.h"
#include "FStaticRender.h"
#include "Layers/xrRender/ShaderResourceTraits.h"
#include "xrCore/FileCRC32.h"

template <typename T>
static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    T*& result, bool const disasm)
{
    HRESULT _hr = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size, result->sh);
    if (!SUCCEEDED(_hr))
    {
        Log("! Shader: ", file_name);
        Msg("! CreateHWShader hr == 0x%08x", _hr);
        return E_FAIL;
    }

    LPCVOID data = nullptr;

    _hr = D3DXFindShaderComment(buffer, MAKEFOURCC('C', 'T', 'A', 'B'), &data, nullptr);

    if (SUCCEEDED(_hr) && data)
    {
        // Parse constant table data
        LPD3DXSHADER_CONSTANTTABLE pConstants = LPD3DXSHADER_CONSTANTTABLE(data);
        result->constants.parse(pConstants, ShaderTypeTraits<T>::GetShaderDest());
    }
    else
        Msg("! D3DXFindShaderComment %s hr == 0x%08x", file_name, _hr);

    if (disasm)
    {
        ID3DXBuffer* disasm = nullptr;
        D3DXDisassembleShader(LPDWORD(buffer), FALSE, nullptr, &disasm);
        if (!disasm)
            return _hr;

        string_path dname;
        strconcat(sizeof(dname), dname, "disasm" DELIMITER, file_name, ('v' == pTarget[0]) ? ".vs" : ".ps");
        IWriter* W = FS.w_open("$app_data_root$", dname);
        W->w(disasm->GetBufferPointer(), disasm->GetBufferSize());
        FS.w_close(W);
        _RELEASE(disasm);
    }

    return _hr;
}

inline HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    void*& result, bool const disasm)
{
    if (pTarget[0] == 'p')
        return create_shader(pTarget, buffer, buffer_size, file_name, (SPS*&)result, disasm);

    if (pTarget[0] == 'v')
        return create_shader(pTarget, buffer, buffer_size, file_name, (SVS*&)result, disasm);

    NODEFAULT;
    return E_FAIL;
}


class includer : public ID3DXInclude
{
public:
    HRESULT __stdcall Open(
        D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
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
        xr_free(pData);
        return D3D_OK;
    }
};

static inline bool match_shader_id(
    LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result);


HRESULT CRender::shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result)
{
    D3DXMACRO defines[128];
    int def_it = 0;

    char sh_name[MAX_PATH] = "";
    u32 len = 0;
    // options
    if (o.forceskinw)
    {
        defines[def_it].Name = "SKIN_COLOR";
        defines[def_it].Definition = "1";
        def_it++;
    }
    sh_name[len] = '0' + char(o.forceskinw);
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

    sh_name[len] = '\0';

    // finish
    defines[def_it].Name = nullptr;
    defines[def_it].Definition = nullptr;
    def_it++;
    R_ASSERT(def_it < 128);

    HRESULT _result = E_FAIL;

    char extension[3];
    strncpy_s(extension, pTarget, 2);

    string_path filename;
    strconcat(sizeof(filename), filename, "r1" DELIMITER, name, ".", extension);

    string_path folder_name, folder;
    strconcat(sizeof(folder), folder, "r1" DELIMITER "objects" DELIMITER, filename);

    FS.update_path(folder_name, "$game_shaders$", folder);
    xr_strcat(folder_name, DELIMITER);

    m_file_set.clear();
    FS.file_list(m_file_set, folder_name, FS_ListFiles | FS_RootOnly, "*");

    string_path temp_file_name, file_name;
    if (!match_shader_id(name, sh_name, m_file_set, temp_file_name))
    {
        string_path file;
        strconcat(sizeof(file), file, "shaders_cache_oxr" DELIMITER, filename, DELIMITER, sh_name);
        strconcat(sizeof(filename), filename, filename, DELIMITER, sh_name);
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
                {
#ifdef DEBUG
                    Log("* Loading shader:", file_name);
#endif
                    _result =
                        create_shader(pTarget, (DWORD*)file->pointer(), file->elapsed(), filename, result, o.disasm);
                }
            }
        }
        file->close();
    }

    if (FAILED(_result))
    {
        includer Includer;
        LPD3DXBUFFER pShaderBuf = nullptr;
        LPD3DXBUFFER pErrorBuf = nullptr;
        LPD3DXCONSTANTTABLE pConstants = nullptr;
        LPD3DXINCLUDE pInclude = (LPD3DXINCLUDE)&Includer;

        _result = D3DXCompileShader((LPCSTR)fs->pointer(), fs->length(), defines, pInclude, pFunctionName, pTarget,
            Flags | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &pShaderBuf, &pErrorBuf, &pConstants);
        if (SUCCEEDED(_result))
        {
            IWriter* file = FS.w_open(file_name);

            file->w_u32(fileCrc);

            u32 crc = crc32(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize());
            file->w_u32(crc); // Do not write anything below this line, take a look at reading (crc32)

            file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());

            FS.w_close(file);
#ifdef DEBUG
            Log("- Compile shader:", file_name);
#endif
            _result = create_shader(pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(),
                filename, result, o.disasm);
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
