#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "ShaderCache.h"
#include "xrCommon/xr_set.h"
#include <filesystem>

#include "Layers/xrRender/Shaders/SlangCompiler.h"

namespace slang {
    struct ShaderReflection;
    struct IComponentType;
}

namespace xray::render::framegraph {

class ShaderLoader {
public:
    ShaderLoader(xray::render::SlangCompiler* slangCompiler);
    ~ShaderLoader();

    struct ShaderResult {
        nvrhi::ShaderHandle handle;
        ExtractedReflection* reflection = nullptr;
        xr_vector<u8> bytecode;

        ~ShaderResult() {
            if (reflection) {
                xr_delete(reflection);
                reflection = nullptr;
            }
        }

        ShaderResult() = default;
        ShaderResult(const ShaderResult&) = delete;
        ShaderResult& operator=(const ShaderResult&) = delete;
        ShaderResult(ShaderResult&& other) noexcept
            : handle(std::move(other.handle))
            , reflection(other.reflection)
            , bytecode(std::move(other.bytecode))
        {
            other.reflection = nullptr;
        }
        ShaderResult& operator=(ShaderResult&& other) noexcept {
            if (this != &other) {
                if (reflection) xr_delete(reflection);

                handle = std::move(other.handle);
                reflection = other.reflection;
                bytecode = std::move(other.bytecode);

                other.reflection = nullptr;
            }
            return *this;
        }
    };

    ShaderResult LoadVertexShader(
        const char* name,
        const char* entryPoint = "main"
    );

    ShaderResult LoadPixelShader(
        const char* name,
        const char* entryPoint = "main"
    );

    ShaderResult LoadComputeShader(
        const char* name,
        const char* entryPoint = "main"
    );

    ShaderResult LoadAmplificationShader(
        const char* name,
        const char* entryPoint = "main"
    );

    ShaderResult LoadMeshShader(
        const char* name,
        const char* entryPoint = "main"
    );

    const ShaderCache::Stats& GetCacheStats() const { return m_cache.GetStats(); }

    xray::render::SlangCompiler* GetSlangCompiler() const { return m_slangCompiler; }

    bool CompileShaderWithDefines(
        const char* shaderName,
        const char* extension,
        const char* entryPoint,
        xray::render::SlangCompiler::Stage stage,
        const xr_vector<xray::render::SlangCompiler::Define>& defines,
        xr_vector<u8>& outBytecode
    );

    ExtractedReflection* GetCachedReflection(
        const char* shaderName,
        const char* extension
    );

    bool CheckForChangedFiles();
    bool ValidateChangedFiles();
    void ClearAllCaches();

private:
    bool CompileShader(
        const char* shaderName,
        const char* extension,
        const char* entryPoint,
        xray::render::SlangCompiler::Stage stage,
        xr_vector<u8>& outBytecode
    );

    IReader* OpenShaderFile(const char* name, const char* extension);

    u32 ComputeSourceHash(const char* source, size_t sourceLen, const char* macros = nullptr);
    void AccumulateIncludeHashes(const char* source, size_t sourceLen, u32& hash, xr_set<xr_string>& visited);

public:
    void SetTarget(xray::render::SlangCompiler::Target target);
    xray::render::SlangCompiler::Target GetTarget() const { return m_target; }

private:
    void WatchShaderFile(
        const xr_string& cacheKey,
        const char* name,
        const char* extension,
        const char* entryPoint,
        xray::render::SlangCompiler::Stage stage
    );

    xray::render::SlangCompiler* m_slangCompiler;
    xray::render::SlangCompiler::Target m_target = xray::render::SlangCompiler::Target::DXIL;
    ShaderCache m_cache;

    xr_map<xr_string, ExtractedReflection*> m_reflectionCache;

    xr_map<xr_string, nvrhi::ShaderHandle> m_handleCache;

    struct WatchedFile {
        xr_string absolutePath;
        xr_string shaderName;
        xr_string extension;
        xr_string entryPoint;
        xray::render::SlangCompiler::Stage stage = xray::render::SlangCompiler::Stage::Vertex;
        std::filesystem::file_time_type lastWriteTime;
    };
    xr_map<xr_string, WatchedFile> m_watchedFiles;
    bool m_hotReloadPrimed = false;
};

}
