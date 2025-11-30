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
}

ShaderLoader::~ShaderLoader()
{
    for (auto& [key, reflection] : m_reflectionCache)
    {
        if (reflection)
        {
            xr_delete(reflection);
        }
    }
    m_reflectionCache.clear();

    const auto& stats = m_cache.GetStats();
}

IReader* ShaderLoader::OpenShaderFile(const char* name, const char* extension)
{
    string_path shName;
    {
        pcstr pchr = strchr(name, '(');
        ptrdiff_t size = pchr ? pchr - name : xr_strlen(name);
        strncpy(shName, name, size);
        shName[size] = 0;
    }

    bool isVertexShader = (xr_strcmp(extension, ".vs") == 0);
    if (isVertexShader)
    {
        size_t len = xr_strlen(shName);
        if (len > 2 && shName[len-2] == '_' && shName[len-1] >= '0' && shName[len-1] <= '4')
        {
            string_path testName;
            xr_strcpy(testName, shName);
            testName[len-2] = 0;

            string_path testFilename;
            strconcat(sizeof(testFilename), testFilename, "r5" DELIMITER, testName, extension);

            if (FS.exist("$game_shaders$", testFilename))
            {
                xr_strcpy(shName, testName);
            }
        }
    }

    string_path filename;
    strconcat(sizeof(filename), filename, "r5" DELIMITER, shName, extension);

    IReader* R = FS.r_open("$game_shaders$", filename);
    return R;
}

bool ShaderLoader::CompileShader(
    const char* shaderName,
    const char* extension,
    const char* entryPoint,
    xray::render::SlangCompiler::Stage stage,
    xr_vector<u8>& outBytecode)
{
    IReader* fs = OpenShaderFile(shaderName, extension);
    if (!fs)
        return false;

    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    if (m_cache.TryLoad(shaderName, extension, sourceHash, outBytecode))
    {
        fs->close();
        return true;
    }

    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    auto result = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        stage,
        xray::render::SlangCompiler::Target::DXBC,
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

    outBytecode = std::move(result.bytecode);
    m_cache.Save(shaderName, extension, sourceHash, outBytecode);

    return true;
}

ShaderLoader::ShaderResult ShaderLoader::LoadVertexShader(
    const char* name,
    const char* entryPoint)
{
    ShaderResult result;

    xr_string cacheKey = xr_string(name) + ".vs";
    auto handleIt = m_handleCache.find(cacheKey);
    if (handleIt != m_handleCache.end()) {
        result.handle = handleIt->second;
        auto reflIt = m_reflectionCache.find(cacheKey);
        if (reflIt != m_reflectionCache.end()) {
            result.reflection = xr_new<ExtractedReflection>(*reflIt->second);
        }
        return result;
    }

    IReader* fs = OpenShaderFile(name, ".vs");
    if (!fs)
        return result;

    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    ExtractedReflection deserializedReflection;
    bool cacheHit = m_cache.TryLoad(name, ".vs", sourceHash, result.bytecode, &deserializedReflection);

    if (cacheHit)
    {
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

        result.reflection = xr_new<ExtractedReflection>(deserializedReflection);

        m_handleCache[cacheKey] = result.handle;
        m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(deserializedReflection);

        return result;
    }

    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    string_path fullPath;
    strconcat(sizeof(fullPath), fullPath,
        GEnv.Render->getShaderPath(),
        name,
        ".vs");

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        xray::render::SlangCompiler::Stage::Vertex,
        xray::render::SlangCompiler::Target::DXBC,
        fullPath
    );

    fs->close();

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s.vs", name);
        if (!compileResult.errorMessage.empty())
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        return result;
    }

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

    result.bytecode = std::move(compileResult.bytecode);

    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection,
        true
    );

    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    m_cache.Save(name, ".vs", sourceHash, result.bytecode, &extractedReflection);

    m_handleCache[cacheKey] = result.handle;
    m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(extractedReflection);

    return result;
}

ShaderLoader::ShaderResult ShaderLoader::LoadPixelShader(
    const char* name,
    const char* entryPoint)
{
    ShaderResult result;

    xr_string cacheKey = xr_string(name) + ".ps";
    auto handleIt = m_handleCache.find(cacheKey);
    if (handleIt != m_handleCache.end()) {
        result.handle = handleIt->second;
        auto reflIt = m_reflectionCache.find(cacheKey);
        if (reflIt != m_reflectionCache.end()) {
            result.reflection = xr_new<ExtractedReflection>(*reflIt->second);
        }
        return result;
    }

    IReader* fs = OpenShaderFile(name, ".ps");
    if (!fs)
        return result;

    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    ExtractedReflection deserializedReflection;
    bool cacheHit = m_cache.TryLoad(name, ".ps", sourceHash, result.bytecode, &deserializedReflection);

    if (cacheHit)
    {
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

        result.reflection = xr_new<ExtractedReflection>(deserializedReflection);

        m_handleCache[cacheKey] = result.handle;
        m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(deserializedReflection);

        return result;
    }

    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    string_path fullPath;
    strconcat(sizeof(fullPath), fullPath,
        GEnv.Render->getShaderPath(),
        name,
        ".ps");

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        xray::render::SlangCompiler::Stage::Pixel,
        xray::render::SlangCompiler::Target::DXBC,
        fullPath
    );

    fs->close();

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s.ps", name);
        if (!compileResult.errorMessage.empty())
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        return result;
    }

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

    result.bytecode = std::move(compileResult.bytecode);

    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection,
        false
    );

    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    m_cache.Save(name, ".ps", sourceHash, result.bytecode, &extractedReflection);

    m_handleCache[cacheKey] = result.handle;
    m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(extractedReflection);

    return result;
}

bool ShaderLoader::CompileShaderWithDefines(
    const char* shaderName,
    const char* extension,
    const char* entryPoint,
    xray::render::SlangCompiler::Stage stage,
    const xr_vector<xray::render::SlangCompiler::Define>& defines,
    xr_vector<u8>& outBytecode)
{
    VERIFY(shaderName);
    VERIFY(extension);
    VERIFY(entryPoint);

    IReader* shaderFile = OpenShaderFile(shaderName, extension);
    if (!shaderFile)
    {
        Msg("! [ShaderLoader] Failed to open shader file: %s%s", shaderName, extension);
        return false;
    }

    xr_string sourceCode;
    sourceCode.assign((const char*)shaderFile->pointer(), shaderFile->length());
    shaderFile->close();

    xr_string definesStr;
    for (const auto& define : defines)
    {
        definesStr.append(define.name);
        definesStr.append("=");
        if (define.value)
            definesStr.append(define.value);
        definesStr.append(";");
    }

    u32 cacheKey = ShaderCache::ComputeHash(
        sourceCode.c_str(),
        sourceCode.length(),
        definesStr.c_str()
    );

    ExtractedReflection cachedReflection;
    bool cacheHit = m_cache.TryLoad(
        shaderName,
        extension,
        cacheKey,
        outBytecode,
        &cachedReflection
    );

    if (cacheHit && !cachedReflection.IsEmpty())
    {
        xr_string cacheKeyStr = xr_string(shaderName) + extension;

        auto it = m_reflectionCache.find(cacheKeyStr);
        if (it != m_reflectionCache.end())
        {
            xr_delete(it->second);
        }

        auto* reflection = xr_new<ExtractedReflection>();
        *reflection = std::move(cachedReflection);
        m_reflectionCache[cacheKeyStr] = reflection;

        Msg("  [ShaderLoader] Cache HIT: %s%s (with defines)", shaderName, extension);
        return true;
    }

    string_path fullPath;
    strconcat(sizeof(fullPath), fullPath,
        GEnv.Render->getShaderPath(),
        shaderName,
        extension);

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        stage,
        xray::render::SlangCompiler::Target::DXBC,
        fullPath,
        defines.data(),
        defines.size()
    );

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Slang compilation failed: %s%s", shaderName, extension);
        if (!compileResult.errorMessage.empty())
        {
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        }
        return false;
    }

    bool isVertexShader = (strcmp(extension, ".vs") == 0);
    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection,
        isVertexShader
    );

    xr_string cacheKeyStr = xr_string(shaderName) + extension;
    auto* reflection = xr_new<ExtractedReflection>();
    *reflection = std::move(extractedReflection);
    m_reflectionCache[cacheKeyStr] = reflection;

    m_cache.Save(shaderName, extension, cacheKey, compileResult.bytecode, reflection);

    outBytecode = std::move(compileResult.bytecode);

    Msg("[ShaderLoader] Compilation SUCCESS: %s%s (%zu bytes)",
        shaderName, extension, outBytecode.size());

    return true;
}

ExtractedReflection* ShaderLoader::GetCachedReflection(
    const char* shaderName,
    const char* extension)
{
    xr_string key = xr_string(shaderName) + extension;
    auto it = m_reflectionCache.find(key);

    if (it != m_reflectionCache.end())
    {
        return it->second;
    }

    return nullptr;
}

}
