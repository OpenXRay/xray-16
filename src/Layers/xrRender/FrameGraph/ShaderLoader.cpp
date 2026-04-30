#include "stdafx.h"
#include "ShaderLoader.h"
#include "xrCore/FileCRC32.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/xrRender_console.h"

namespace xray::render::framegraph {
using namespace fg;

namespace {

bool TryGetNvrhiShaderType(xray::render::SlangCompiler::Stage stage, nvrhi::ShaderType& outType)
{
    switch (stage)
    {
    case xray::render::SlangCompiler::Stage::Vertex:
        outType = nvrhi::ShaderType::Vertex;
        return true;
    case xray::render::SlangCompiler::Stage::Pixel:
        outType = nvrhi::ShaderType::Pixel;
        return true;
    case xray::render::SlangCompiler::Stage::Compute:
        outType = nvrhi::ShaderType::Compute;
        return true;
    case xray::render::SlangCompiler::Stage::Amplification:
        outType = nvrhi::ShaderType::Amplification;
        return true;
    case xray::render::SlangCompiler::Stage::Mesh:
        outType = nvrhi::ShaderType::Mesh;
        return true;
    default:
        return false;
    }
}

void ResolveShaderSourceRelativePath(
    const char* name,
    const char* extension,
    char* outRelativePath,
    size_t outRelativePathSize)
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
        if (len > 2 && shName[len - 2] == '_' && shName[len - 1] >= '0' && shName[len - 1] <= '4')
        {
            string_path testName;
            xr_strcpy(testName, shName);
            testName[len - 2] = 0;

            string_path testFilename;
            strconcat(sizeof(testFilename), testFilename, "r5" DELIMITER, testName, extension);

            if (FS.exist("$game_shaders$", testFilename))
            {
                xr_strcpy(shName, testName);
            }
        }
    }

    strconcat(outRelativePathSize, outRelativePath, "r5" DELIMITER, shName, extension);
}

}

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
    string_path filename;
    ResolveShaderSourceRelativePath(name, extension, filename, sizeof(filename));

    IReader* R = FS.r_open("$game_shaders$", filename);
    return R;
}

void ShaderLoader::WatchShaderFile(
    const xr_string& cacheKey,
    const char* name,
    const char* extension,
    const char* entryPoint,
    xray::render::SlangCompiler::Stage stage)
{
    if (!ps_fg_hot_reload_shaders)
        return;

    string_path filename;
    ResolveShaderSourceRelativePath(name, extension, filename, sizeof(filename));

    string_path fullPath;
    FS.update_path(fullPath, "$game_shaders$", filename);

    std::error_code ec;
    const auto writeTime = std::filesystem::last_write_time(fullPath, ec);
    if (ec)
        return;

    WatchedFile watchedFile;
    watchedFile.absolutePath = fullPath;
    watchedFile.shaderName = name;
    watchedFile.extension = extension;
    watchedFile.entryPoint = entryPoint ? entryPoint : "main";
    watchedFile.stage = stage;
    watchedFile.lastWriteTime = writeTime;

    m_watchedFiles[cacheKey] = std::move(watchedFile);
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
        m_target,
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
    WatchShaderFile(cacheKey, name, ".vs", entryPoint, xray::render::SlangCompiler::Stage::Vertex);

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

        result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
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
        m_target,
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

    result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
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
        compileResult.linkedProgram,
        compileResult.vkShifts
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
    WatchShaderFile(cacheKey, name, ".ps", entryPoint, xray::render::SlangCompiler::Stage::Pixel);

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

        result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
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
        m_target,
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

    result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
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
        compileResult.linkedProgram,
        compileResult.vkShifts
    );

    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    m_cache.Save(name, ".ps", sourceHash, result.bytecode, &extractedReflection);

    m_handleCache[cacheKey] = result.handle;
    m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(extractedReflection);

    return result;
}

ShaderLoader::ShaderResult ShaderLoader::LoadComputeShader(
    const char* name,
    const char* entryPoint)
{
    ShaderResult result;

    xr_string cacheKey = xr_string(name) + ".cs";
    if (entryPoint && xr_strcmp(entryPoint, "main") != 0)
        cacheKey += xr_string(":") + entryPoint;
    auto handleIt = m_handleCache.find(cacheKey);
    if (handleIt != m_handleCache.end()) {
        result.handle = handleIt->second;
        auto reflIt = m_reflectionCache.find(cacheKey);
        if (reflIt != m_reflectionCache.end()) {
            result.reflection = xr_new<ExtractedReflection>(*reflIt->second);
        }
        return result;
    }

    IReader* fs = OpenShaderFile(name, ".cs");
    if (!fs)
        return result;
    WatchShaderFile(cacheKey, name, ".cs", entryPoint, xray::render::SlangCompiler::Stage::Compute);

    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    xr_string diskCacheName = name;
    if (entryPoint && xr_strcmp(entryPoint, "main") != 0)
        diskCacheName += xr_string("_") + entryPoint;

    ExtractedReflection deserializedReflection;
    bool cacheHit = m_cache.TryLoad(diskCacheName.c_str(), ".cs", sourceHash, result.bytecode, &deserializedReflection);

    if (cacheHit)
    {
        nvrhi::ShaderDesc desc;
        desc.shaderType = nvrhi::ShaderType::Compute;
        desc.debugName = name;

        result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
            desc,
            result.bytecode.data(),
            result.bytecode.size()
        );

        fs->close();

        if (!result.handle)
        {
            Msg("! [ShaderLoader] Failed to create NVRHI compute shader from cache: %s", name);
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
        ".cs");

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        xray::render::SlangCompiler::Stage::Compute,
        m_target,
        fullPath
    );

    fs->close();

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s.cs", name);
        if (!compileResult.errorMessage.empty())
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        return result;
    }

    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Compute;
    desc.debugName = name;

    result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
        desc,
        compileResult.bytecode.data(),
        compileResult.bytecode.size()
    );

    if (!result.handle)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI compute shader: %s", name);
        return result;
    }

    result.bytecode = std::move(compileResult.bytecode);

    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection,
        compileResult.linkedProgram,
        compileResult.vkShifts
    );

    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    m_cache.Save(diskCacheName.c_str(), ".cs", sourceHash, result.bytecode, &extractedReflection);

    m_handleCache[cacheKey] = result.handle;
    m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(extractedReflection);

    Msg("* [ShaderLoader] Compiled compute shader: %s.cs (entry: %s)", name, entryPoint);

    return result;
}

ShaderLoader::ShaderResult ShaderLoader::LoadAmplificationShader(
    const char* name,
    const char* entryPoint)
{
    ShaderResult result;

    xr_string cacheKey = xr_string(name) + ".as";
    auto handleIt = m_handleCache.find(cacheKey);
    if (handleIt != m_handleCache.end()) {
        result.handle = handleIt->second;
        auto reflIt = m_reflectionCache.find(cacheKey);
        if (reflIt != m_reflectionCache.end()) {
            result.reflection = xr_new<ExtractedReflection>(*reflIt->second);
        }
        return result;
    }

    IReader* fs = OpenShaderFile(name, ".as");
    if (!fs)
        return result;
    WatchShaderFile(cacheKey, name, ".as", entryPoint, xray::render::SlangCompiler::Stage::Amplification);

    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    ExtractedReflection cachedReflection;
    if (m_cache.TryLoad(name, ".as", sourceHash, result.bytecode, &cachedReflection))
    {
        fs->close();

        nvrhi::ShaderDesc desc;
        desc.shaderType = nvrhi::ShaderType::Amplification;
        desc.debugName = name;

        result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
            desc, result.bytecode.data(), result.bytecode.size()
        );

        if (result.handle) {
            result.reflection = xr_new<ExtractedReflection>(cachedReflection);
            m_handleCache[cacheKey] = result.handle;
            m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(cachedReflection);
            return result;
        }
    }

    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    string_path fullPath;
    strconcat(sizeof(fullPath), fullPath, GEnv.Render->getShaderPath(), name, ".as");

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        xray::render::SlangCompiler::Stage::Amplification,
        m_target,
        fullPath
    );

    fs->close();

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s.as", name);
        if (!compileResult.errorMessage.empty())
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        return result;
    }

    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Amplification;
    desc.debugName = name;

    result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
        desc, compileResult.bytecode.data(), compileResult.bytecode.size()
    );

    if (!result.handle)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI amplification shader: %s", name);
        return result;
    }

    result.bytecode = std::move(compileResult.bytecode);

    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection, compileResult.linkedProgram, compileResult.vkShifts);
    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    m_cache.Save(name, ".as", sourceHash, result.bytecode, &extractedReflection);
    m_handleCache[cacheKey] = result.handle;
    m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(extractedReflection);

    Msg("* [ShaderLoader] Compiled amplification shader: %s.as", name);

    return result;
}

ShaderLoader::ShaderResult ShaderLoader::LoadMeshShader(
    const char* name,
    const char* entryPoint)
{
    ShaderResult result;

    xr_string cacheKey = xr_string(name) + ".ms";
    auto handleIt = m_handleCache.find(cacheKey);
    if (handleIt != m_handleCache.end()) {
        result.handle = handleIt->second;
        auto reflIt = m_reflectionCache.find(cacheKey);
        if (reflIt != m_reflectionCache.end()) {
            result.reflection = xr_new<ExtractedReflection>(*reflIt->second);
        }
        return result;
    }

    IReader* fs = OpenShaderFile(name, ".ms");
    if (!fs)
        return result;
    WatchShaderFile(cacheKey, name, ".ms", entryPoint, xray::render::SlangCompiler::Stage::Mesh);

    u32 sourceHash = ShaderCache::ComputeHash(
        (const char*)fs->pointer(),
        fs->length()
    );

    ExtractedReflection cachedReflection;
    if (m_cache.TryLoad(name, ".ms", sourceHash, result.bytecode, &cachedReflection))
    {
        fs->close();

        nvrhi::ShaderDesc desc;
        desc.shaderType = nvrhi::ShaderType::Mesh;
        desc.debugName = name;

        result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
            desc, result.bytecode.data(), result.bytecode.size()
        );

        if (result.handle) {
            result.reflection = xr_new<ExtractedReflection>(cachedReflection);
            m_handleCache[cacheKey] = result.handle;
            m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(cachedReflection);
            return result;
        }
    }

    xr_string sourceCode;
    sourceCode.assign((const char*)fs->pointer(), fs->length());

    string_path fullPath;
    strconcat(sizeof(fullPath), fullPath, GEnv.Render->getShaderPath(), name, ".ms");

    auto compileResult = m_slangCompiler->CompileFromSource(
        sourceCode.c_str(),
        entryPoint,
        xray::render::SlangCompiler::Stage::Mesh,
        m_target,
        fullPath
    );

    fs->close();

    if (!compileResult.IsValid())
    {
        Msg("! [ShaderLoader] Compilation failed for %s.ms", name);
        if (!compileResult.errorMessage.empty())
            Msg("! Error: %s", compileResult.errorMessage.c_str());
        return result;
    }

    nvrhi::ShaderDesc desc;
    desc.shaderType = nvrhi::ShaderType::Mesh;
    desc.debugName = name;

    result.handle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
        desc, compileResult.bytecode.data(), compileResult.bytecode.size()
    );

    if (!result.handle)
    {
        Msg("! [ShaderLoader] Failed to create NVRHI mesh shader: %s", name);
        return result;
    }

    result.bytecode = std::move(compileResult.bytecode);

    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection, compileResult.linkedProgram, compileResult.vkShifts);
    result.reflection = xr_new<ExtractedReflection>(extractedReflection);

    m_cache.Save(name, ".ms", sourceHash, result.bytecode, &extractedReflection);
    m_handleCache[cacheKey] = result.handle;
    m_reflectionCache[cacheKey] = xr_new<ExtractedReflection>(extractedReflection);

    Msg("* [ShaderLoader] Compiled mesh shader: %s.ms", name);

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
        m_target,
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

    auto extractedReflection = ShaderReflector::ExtractReflection(
        compileResult.reflection,
        compileResult.linkedProgram,
        compileResult.vkShifts
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

bool ShaderLoader::CheckForChangedFiles()
{
    if (!ps_fg_hot_reload_shaders)
        return false;

    if (m_watchedFiles.empty() && !m_handleCache.empty())
    {
        Msg("* [ShaderLoader] Priming hot-reload file watch list");
        ClearAllCaches();
        return false;
    }

    for (const auto& watchedEntry : m_watchedFiles)
    {
        const auto& info = watchedEntry.second;
        std::error_code ec;
        const auto currentTime = std::filesystem::last_write_time(info.absolutePath, ec);
        if (ec)
            continue;

        if (currentTime != info.lastWriteTime)
        {
            Msg("* Shader hot-reload: %s changed", info.absolutePath.c_str());
            return true;
        }
    }

    return false;
}

bool ShaderLoader::ValidateChangedFiles()
{
    if (!GEnv.Render || !GEnv.Render->GetRenderDevice())
    {
        Msg("! [ShaderLoader] Hot-reload validation failed: render device is unavailable");
        return false;
    }

    bool hasChangedFiles = false;
    for (const auto& watchedEntry : m_watchedFiles)
    {
        const auto& info = watchedEntry.second;

        std::error_code ec;
        const auto currentTime = std::filesystem::last_write_time(info.absolutePath, ec);
        if (ec || currentTime == info.lastWriteTime)
            continue;

        hasChangedFiles = true;

        IReader* shaderFile = OpenShaderFile(info.shaderName.c_str(), info.extension.c_str());
        if (!shaderFile)
        {
            Msg("! [ShaderLoader] Hot-reload validation failed: cannot open %s%s",
                info.shaderName.c_str(), info.extension.c_str());
            return false;
        }

        xr_string sourceCode;
        sourceCode.assign((const char*)shaderFile->pointer(), shaderFile->length());
        shaderFile->close();

        string_path fullPath;
        strconcat(sizeof(fullPath), fullPath,
            GEnv.Render->getShaderPath(),
            info.shaderName.c_str(),
            info.extension.c_str());

        auto compileResult = m_slangCompiler->CompileFromSource(
            sourceCode.c_str(),
            info.entryPoint.c_str(),
            info.stage,
            m_target,
            fullPath);

        if (!compileResult.IsValid())
        {
            Msg("! [ShaderLoader] Hot-reload validation compile failed: %s%s (entry: %s)",
                info.shaderName.c_str(), info.extension.c_str(), info.entryPoint.c_str());
            if (!compileResult.errorMessage.empty())
                Msg("! Error: %s", compileResult.errorMessage.c_str());
            return false;
        }

        nvrhi::ShaderType shaderType;
        if (!TryGetNvrhiShaderType(info.stage, shaderType))
        {
            Msg("! [ShaderLoader] Hot-reload validation failed: unsupported stage for %s%s",
                info.shaderName.c_str(), info.extension.c_str());
            return false;
        }

        nvrhi::ShaderDesc desc;
        desc.shaderType = shaderType;
        desc.debugName = info.shaderName.c_str();

        nvrhi::ShaderHandle testHandle = GEnv.Render->GetRenderDevice()->GetNVRHIDevice()->createShader(
            desc,
            compileResult.bytecode.data(),
            compileResult.bytecode.size());

        if (!testHandle)
        {
            Msg("! [ShaderLoader] Hot-reload validation failed: shader creation failed for %s%s (entry: %s)",
                info.shaderName.c_str(), info.extension.c_str(), info.entryPoint.c_str());
            return false;
        }
    }

    if (hasChangedFiles)
        Msg("* [ShaderLoader] Hot-reload validation succeeded");

    return true;
}

void ShaderLoader::ClearAllCaches()
{
    m_handleCache.clear();

    for (auto& [key, reflection] : m_reflectionCache)
    {
        if (reflection)
            xr_delete(reflection);
    }
    m_reflectionCache.clear();

    for (auto& watchedEntry : m_watchedFiles)
    {
        auto& info = watchedEntry.second;
        std::error_code ec;
        const auto currentTime = std::filesystem::last_write_time(info.absolutePath, ec);
        if (!ec)
            info.lastWriteTime = currentTime;
    }
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
