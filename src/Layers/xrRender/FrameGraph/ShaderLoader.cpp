// xrRender/FrameGraph/ShaderLoader.cpp
#include "stdafx.h"
#include "ShaderLoader.h"
#include "xrCore/FileCRC32.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render::framegraph {

ShaderLoader::ShaderLoader(xray::render::SlangCompiler* slangCompiler)
    : m_slangCompiler(slangCompiler)
{
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
    strconcat(sizeof(filename), filename, "r5" DELIMITER, name, extension);

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
    nvrhi::ShaderHandle shader = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createShader(
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
    nvrhi::ShaderHandle shader = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createShader(
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

// ═══════════════════════════════════════════════════
//  NEW: SHADER LOADING WITH SLANG REFLECTION
// ═══════════════════════════════════════════════════

ShaderLoader::ShaderResult ShaderLoader::LoadVertexShaderWithReflection(
    const char* name,
    const char* entryPoint)
{
    ShaderResult result;

    // Open shader source file
    IReader* fs = OpenShaderFile(name, ".vs");
    if (!fs)
        return result;  // Empty result

    // Compute hash of shader source
    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    // Try to load bytecode + reflection from cache
    ExtractedReflection deserializedReflection;
    bool cacheHit = m_cache.TryLoad(name, ".vs", sourceHash, result.bytecode, &deserializedReflection);

    if (cacheHit)
    {
        // Cache hit - create shader from cached bytecode + deserialized reflection
        nvrhi::ShaderDesc desc;
        desc.shaderType = nvrhi::ShaderType::Vertex;
        desc.debugName = name;

        result.handle = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createShader(
            desc,
            result.bytecode.data(),
            result.bytecode.size()
        );

        fs->close();

        if (!result.handle)
        {
            Msg("! [ShaderLoader] Failed to create NVRHI vertex shader from cache: %s", name);
            return result;
        }

        // Store deserialized reflection
        result.reflection = xr_new<ExtractedReflection>(deserializedReflection);

        Msg("  ✓ Loaded vertex shader from cache (with reflection): %s (%zu bytes)", name, result.bytecode.size());
        return result;
    }

    // Cache miss - compile with Slang (gets reflection!)
    Msg("~ [ShaderLoader] Compiling %s.vs (entry: %s)", name, entryPoint);

    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        xray::render::SlangCompiler::Stage::Vertex,
        xray::render::SlangCompiler::Target::DXBC,
        name
    );

    fs->close();

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s.vs", name);
        if (!compileResult.errorMessage.empty())
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        return result;  // Empty result
    }

    // Create NVRHI shader
    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Vertex;
    desc.debugName = name;

    result.handle = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createShader(
        desc,
        compileResult.bytecode.data(),
        compileResult.bytecode.size()
    );

    if (!result.handle)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI vertex shader: %s", name);
        return result;
    }

    // ═══════════════════════════════════════════════════
    //  EXTRACT AND STORE REFLECTION
    // ═══════════════════════════════════════════════════
    result.bytecode = std::move(compileResult.bytecode);

    // Extract reflection from Slang for storage and caching
    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection,
        true  // isVertexShader
    );

    // Store extracted reflection in result
    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    // Save bytecode + reflection to cache
    m_cache.Save(name, ".vs", sourceHash, result.bytecode, &extractedReflection);

    Msg("  ✓ Loaded vertex shader with reflection: %s (%zu bytes)", name, result.bytecode.size());
    return result;
}

ShaderLoader::ShaderResult ShaderLoader::LoadPixelShaderWithReflection(
    const char* name,
    const char* entryPoint)
{
    ShaderResult result;

    // Open shader source file
    IReader* fs = OpenShaderFile(name, ".ps");
    if (!fs)
        return result;  // Empty result

    // Compute hash of shader source
    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    // Try to load bytecode + reflection from cache
    ExtractedReflection deserializedReflection;
    bool cacheHit = m_cache.TryLoad(name, ".ps", sourceHash, result.bytecode, &deserializedReflection);

    if (cacheHit)
    {
        // Cache hit - create shader from cached bytecode + deserialized reflection
        nvrhi::ShaderDesc desc;
        desc.shaderType = nvrhi::ShaderType::Pixel;
        desc.debugName = name;

        result.handle = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createShader(
            desc,
            result.bytecode.data(),
            result.bytecode.size()
        );

        fs->close();

        if (!result.handle)
        {
            Msg("! [ShaderLoader] Failed to create NVRHI pixel shader from cache: %s", name);
            return result;
        }

        // Store deserialized reflection
        result.reflection = xr_new<ExtractedReflection>(deserializedReflection);

        Msg("  ✓ Loaded pixel shader from cache (with reflection): %s (%zu bytes)", name, result.bytecode.size());
        return result;
    }

    // Cache miss - compile with Slang (gets reflection!)
    Msg("~ [ShaderLoader] Compiling %s.ps (entry: %s)", name, entryPoint);

    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        xray::render::SlangCompiler::Stage::Pixel,
        xray::render::SlangCompiler::Target::DXBC,
        name
    );

    fs->close();

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s.ps", name);
        if (!compileResult.errorMessage.empty())
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        return result;  // Empty result
    }

    // Create NVRHI shader
    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Pixel;
    desc.debugName = name;

    result.handle = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createShader(
        desc,
        compileResult.bytecode.data(),
        compileResult.bytecode.size()
    );

    if (!result.handle)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI pixel shader: %s", name);
        return result;
    }

    // ═══════════════════════════════════════════════════
    //  EXTRACT AND STORE REFLECTION
    // ═══════════════════════════════════════════════════
    result.bytecode = std::move(compileResult.bytecode);

    // Extract reflection from Slang for storage and caching
    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection,
        false  // isVertexShader (this is pixel shader)
    );

    // Store extracted reflection in result
    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    // Save bytecode + reflection to cache
    m_cache.Save(name, ".ps", sourceHash, result.bytecode, &extractedReflection);

    Msg("  ✓ Loaded pixel shader with reflection: %s (%zu bytes)", name, result.bytecode.size());
    return result;
}

} // namespace xray::render::framegraph
