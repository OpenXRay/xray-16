#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "ShaderCache.h"

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

private:
    bool CompileShader(
        const char* shaderName,
        const char* extension,
        const char* entryPoint,
        xray::render::SlangCompiler::Stage stage,
        xr_vector<u8>& outBytecode
    );

    IReader* OpenShaderFile(const char* name, const char* extension);

private:
    xray::render::SlangCompiler* m_slangCompiler;
    ShaderCache m_cache;

    xr_map<xr_string, ExtractedReflection*> m_reflectionCache;
};

}
