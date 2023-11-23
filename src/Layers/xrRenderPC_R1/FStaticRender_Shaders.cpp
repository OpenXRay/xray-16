#include "stdafx.h"
#include "FStaticRender.h"
#include "Layers/xrRender/ShaderResourceTraits.h"
#include "Layers/xrRenderDX9/dx9shader_utils.h"
#include "xrCore/FileCRC32.h"

template <typename T>
static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    T*& result, bool const disasm)
{
    HRESULT _hr = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size, result->sh);
    if (FAILED(_hr))
    {
        Log("! Shader: ", file_name);
        Msg("! CreateHWShader hr == 0x%08x", _hr);
        return E_FAIL;
    }

    LPCVOID data = nullptr;

    _hr = FindShaderComment(buffer, MAKEFOURCC('C', 'T', 'A', 'B'), &data, nullptr);

    if (SUCCEEDED(_hr) && data)
    {
        // Parse constant table data
        result->constants.parse(const_cast<void*>(data), ShaderTypeTraits<T>::GetShaderDest());
    }
    else
        Msg("! D3DXFindShaderComment %s hr == 0x%08x", file_name, _hr);

    if (disasm)
    {
        IShaderBlob* blob = nullptr;
        DisassembleShader(buffer, buffer_size, FALSE, nullptr, &blob);
        if (!blob)
            return _hr;

        string_path dname;
        strconcat(sizeof(dname), dname, "disasm" DELIMITER, file_name, ('v' == pTarget[0]) ? ".vs" : ".ps");
        IWriter* W = FS.w_open("$app_data_root$", dname);
        W->w(blob->GetBufferPointer(), blob->GetBufferSize());
        FS.w_close(W);
        _RELEASE(blob);
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

HRESULT CRender::shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result)
{
    SHADER_MACRO defines[128];
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

#ifndef USE_D3DX
    // Required for compatibility with D3DCompile()
    defines[def_it].Name = "point";
    defines[def_it].Definition = "__pnt__";
    def_it++;
#endif

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

    string_path file_name;
    {
        string_path file;
        strconcat(sizeof(file), file, "shaders_cache_oxr" DELIMITER, filename, DELIMITER, sh_name);
        strconcat(sizeof(filename), filename, filename, DELIMITER, sh_name);
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
        ShaderIncluder includer;
        IShaderBlob* pShaderBuf = nullptr;
        IShaderBlob* pErrorBuf = nullptr;

        _result = CompileShader(fs->pointer(), fs->length(), defines, &includer,
            pFunctionName, pTarget, Flags, &pShaderBuf, &pErrorBuf);
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
