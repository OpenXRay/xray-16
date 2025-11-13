// xrRender/FrameGraph/ShaderLoader.cpp
#include "stdafx.h"
#include "ShaderLoader.h"
#include "xrCore/FileCRC32.h"

namespace xray::render::framegraph {

ShaderLoader::ShaderLoader(ng::RenderDevice* device, xray::render::SlangCompiler* slangCompiler)
    : m_device(device)
    , m_slangCompiler(slangCompiler)
{
    VERIFY(m_device != nullptr);
    VERIFY(m_slangCompiler != nullptr);
    VERIFY(m_slangCompiler->IsInitialized());
    Msg("* [ShaderLoader] Created (using Slang compiler)");
}

ShaderLoader::~ShaderLoader()
{
    const auto& stats = m_cache.GetStats();
    Msg("* [ShaderLoader] Destroyed (Cache - Hits: %u, Misses: %u, Saves: %u)",
        stats.hits, stats.misses, stats.saves);
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

bool ShaderLoader::CompileShader(
    const char* shaderName,
    const char* extension,
    const char* entryPoint,
    xray::render::SlangCompiler::Stage stage,
    xr_vector<u8>& outBytecode)
{
    // Open shader source file
    IReader* fs = OpenShaderFile(shaderName, extension);
    if (!fs)
        return false;

    // Compute hash of shader source
    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    // Try to load from cache first
    if (m_cache.TryLoad(shaderName, extension, sourceHash, outBytecode))
    {
        fs->close();
        return true;  // Cache hit!
    }

    // Cache miss - compile with Slang
    Msg("~ [ShaderLoader] Compiling %s%s (entry: %s, stage: %s)",
        shaderName, extension, entryPoint,
        xray::render::SlangCompiler::GetStageName(stage));

    // Copy source to null-terminated string (Slang expects C-string)
    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    auto result = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        stage,
        xray::render::SlangCompiler::Target::DXBC,  // DX11 target
        shaderName
    );

    fs->close();

    if (!result.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s%s", shaderName, extension);
        if (!result.errorMessage.empty())
            Msg("! Error: %s", result.errorMessage.c_str());
        return false;
    }

    // Save to cache
    outBytecode = std::move(result.bytecode);
    m_cache.Save(shaderName, extension, sourceHash, outBytecode);

    return true;
}

nvrhi::ShaderHandle ShaderLoader::LoadVertexShader(
    const char* name,
    const char* entryPoint,
    xr_vector<u8>* outBytecode)
{
    xr_vector<u8> bytecode;
    if (!CompileShader(name, ".vs", entryPoint, xray::render::SlangCompiler::Stage::Vertex, bytecode))
        return nullptr;

    // Create NVRHI shader descriptor
    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Vertex;
    desc.debugName = name;

    // Create NVRHI shader from bytecode
    nvrhi::ShaderHandle shader = m_device->GetNVRHIDevice()->createShader(
        desc,
        bytecode.data(),
        bytecode.size()
    );

    if (!shader)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI vertex shader: %s", name);
        return nullptr;
    }

    // Store bytecode if caller requested it (for reflection)
    if (outBytecode)
        *outBytecode = std::move(bytecode);

    Msg("  ✓ Loaded vertex shader: %s (%zu bytes)", name, bytecode.size());
    return shader;
}

nvrhi::ShaderHandle ShaderLoader::LoadPixelShader(
    const char* name,
    const char* entryPoint,
    xr_vector<u8>* outBytecode)
{
    xr_vector<u8> bytecode;
    if (!CompileShader(name, ".ps", entryPoint, xray::render::SlangCompiler::Stage::Pixel, bytecode))
        return nullptr;

    // Create NVRHI shader descriptor
    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Pixel;
    desc.debugName = name;

    // Create NVRHI shader from bytecode
    nvrhi::ShaderHandle shader = m_device->GetNVRHIDevice()->createShader(
        desc,
        bytecode.data(),
        bytecode.size()
    );

    if (!shader)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI pixel shader: %s", name);
        return nullptr;
    }

    // Store bytecode if caller requested it (for reflection)
    if (outBytecode)
        *outBytecode = std::move(bytecode);

    Msg("  ✓ Loaded pixel shader: %s (%zu bytes)", name, bytecode.size());
    return shader;
}

} // namespace xray::render::framegraph
