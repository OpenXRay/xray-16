// xrRender/FrameGraph/ShaderLoader.cpp
#include "stdafx.h"
#include "ShaderLoader.h"
#include "xrCore/FileCRC32.h"
#include <d3dcompiler.h>

namespace xray::render::framegraph {

// Include helper for D3DCompile
class ShaderIncluder : public ID3DInclude {
public:
    HRESULT __stdcall Open(
        D3D10_INCLUDE_TYPE IncludeType,
        LPCSTR pFileName,
        LPCVOID pParentData,
        LPCVOID* ppData,
        UINT* pBytes) override
    {
        string_path pname;
        strconcat(pname, "r3" DELIMITER, pFileName);  // For r3 renderer
        IReader* R = FS.r_open("$game_shaders$", pname);

        if (nullptr == R)
        {
            // Try direct path (for shared files)
            R = FS.r_open("$game_shaders$", pFileName);
            if (nullptr == R)
                return E_FAIL;
        }

        // Duplicate and zero-terminate
        const size_t size = R->length();
        u8* data = xr_alloc<u8>(size + 1);
        CopyMemory(data, R->pointer(), size);
        data[size] = 0;
        FS.r_close(R);

        *ppData = data;
        *pBytes = size;
        return D3D_OK;
    }

    HRESULT __stdcall Close(LPCVOID pData) override
    {
        auto mutableData = const_cast<LPVOID>(pData);
        xr_free(mutableData);
        return D3D_OK;
    }
};

ShaderLoader::ShaderLoader(ng::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device != nullptr);
    Msg("* [ShaderLoader] Created");
}

ShaderLoader::~ShaderLoader()
{
    Msg("* [ShaderLoader] Destroyed");
}

IReader* ShaderLoader::OpenShaderFile(const char* name, const char* extension)
{
    string_path filename;
    strconcat(sizeof(filename), filename, "r3" DELIMITER, name, extension);

    IReader* R = FS.r_open("$game_shaders$", filename);
    if (!R)
    {
        Msg("! [ShaderLoader] Failed to open shader: %s", filename);
    }
    return R;
}

ID3DBlob* ShaderLoader::CompileShader(
    const char* shaderPath,
    const char* entryPoint,
    const char* target)
{
    Msg("~ [ShaderLoader] Compiling shader: %s (entry: %s, target: %s)",
        shaderPath, entryPoint, target);

    // Open shader source file
    IReader* fs = FS.r_open("$game_shaders$", shaderPath);
    if (!fs)
    {
        Msg("! [ShaderLoader] Failed to open: %s", shaderPath);
        return nullptr;
    }

    // Compilation flags
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    // No macros for now (FrameGraph shaders don't need legacy renderer options)
    D3D_SHADER_MACRO macros[] = {
        { "SM_5", "1" },  // Shader Model 5.0
        { nullptr, nullptr }
    };

    // Compile
    ShaderIncluder includer;
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        fs->pointer(),
        fs->length(),
        shaderPath,
        macros,
        &includer,
        entryPoint,
        target,
        flags,
        0,
        &shaderBlob,
        &errorBlob
    );

    fs->close();

    if (FAILED(hr))
    {
        Msg("! [ShaderLoader] Compilation failed for: %s", shaderPath);
        if (errorBlob)
        {
            Msg("! Error: %s", (const char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return nullptr;
    }

    if (errorBlob)
        errorBlob->Release();

    Msg("  ✓ Compiled %s successfully (%u bytes)",
        shaderPath, (u32)shaderBlob->GetBufferSize());

    return shaderBlob;
}

nvrhi::IShader* ShaderLoader::LoadVertexShader(const char* name, const char* entryPoint)
{
    string_path shaderPath;
    strconcat(sizeof(shaderPath), shaderPath, "r3" DELIMITER, name, ".vs");

    ID3DBlob* bytecode = CompileShader(shaderPath, entryPoint, "vs_5_0");
    if (!bytecode)
        return nullptr;

    // Create NVRHI shader descriptor
    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Vertex;
    desc.debugName = name;

    // Create NVRHI shader from bytecode
    nvrhi::IShader* shader = m_device->GetNVRHIDevice()->createShader(
        desc,
        bytecode->GetBufferPointer(),
        bytecode->GetBufferSize()
    );

    bytecode->Release();

    if (!shader)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI vertex shader: %s", name);
        return nullptr;
    }

    Msg("  ✓ Loaded vertex shader: %s", name);
    return shader;
}

nvrhi::IShader* ShaderLoader::LoadPixelShader(const char* name, const char* entryPoint)
{
    string_path shaderPath;
    strconcat(sizeof(shaderPath), shaderPath, "r3" DELIMITER, name, ".ps");

    ID3DBlob* bytecode = CompileShader(shaderPath, entryPoint, "ps_5_0");
    if (!bytecode)
        return nullptr;

    // Create NVRHI shader descriptor
    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Pixel;
    desc.debugName = name;

    // Create NVRHI shader from bytecode
    nvrhi::IShader* shader = m_device->GetNVRHIDevice()->createShader(
        desc,
        bytecode->GetBufferPointer(),
        bytecode->GetBufferSize()
    );

    bytecode->Release();

    if (!shader)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI pixel shader: %s", name);
        return nullptr;
    }

    Msg("  ✓ Loaded pixel shader: %s", name);
    return shader;
}

} // namespace xray::render::framegraph
