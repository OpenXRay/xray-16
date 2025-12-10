// xrRender/FrameGraph/ShaderLoader.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "ShaderCache.h"

// Need full include (not just forward decl) since we use SlangCompiler::Stage
#include "Layers/xrRender/Shaders/SlangCompiler.h"

// Forward declarations for Slang reflection
namespace slang {
    struct ShaderReflection;
    struct IComponentType;
}

namespace xray::render::framegraph {

// ══════════════════════════════════════════════════════════
//  SHADER LOADER FOR FRAMEGRAPH PASSES
// ══════════════════════════════════════════════════════════
//
//  Modern shader loading system using Slang compiler with disk caching
//  - Compiles HLSL to DXIL (DX12 SM6) using Slang
//  - Supports SM6 bindless via ResourceDescriptorHeap
//  - Caches compiled bytecode to shaders_cache_fg/
//  - No D3DCompile dependency
//  - Ready for multi-API (SPIRV for Vulkan in future)
//
// ══════════════════════════════════════════════════════════

class ShaderLoader {
public:
    ShaderLoader(xray::render::SlangCompiler* slangCompiler);
    ~ShaderLoader();

    /// <summary>
    /// Shader compilation result with reflection data
    /// </summary>
    struct ShaderResult {
        nvrhi::ShaderHandle handle;
        ExtractedReflection* reflection = nullptr;  // Extracted reflection data (owned by this)
        xr_vector<u8> bytecode;  // Optional bytecode storage

        ~ShaderResult() {
            if (reflection) {
                xr_delete(reflection);
                reflection = nullptr;
            }
        }

        // Move-only type
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
                // Release existing
                if (reflection) xr_delete(reflection);

                // Move
                handle = std::move(other.handle);
                reflection = other.reflection;
                bytecode = std::move(other.bytecode);

                other.reflection = nullptr;
            }
            return *this;
        }
    };

    /// <summary>
    /// Load and compile vertex shader from res/gamedata/shaders/r5/
    /// Always returns shader with reflection data
    /// </summary>
    /// <param name="name">Shader name without extension (e.g., "gbuffer")</param>
    /// <param name="entryPoint">Entry point function name (default: "main")</param>
    ShaderResult LoadVertexShader(
        const char* name,
        const char* entryPoint = "main"
    );

    /// <summary>
    /// Load and compile pixel shader from res/gamedata/shaders/r5/
    /// Always returns shader with reflection data
    /// </summary>
    /// <param name="name">Shader name without extension (e.g., "gbuffer")</param>
    /// <param name="entryPoint">Entry point function name (default: "main")</param>
    ShaderResult LoadPixelShader(
        const char* name,
        const char* entryPoint = "main"
    );

    /// <summary>
    /// Load and compile compute shader from res/gamedata/shaders/r5/
    /// Always returns shader with reflection data
    /// </summary>
    /// <param name="name">Shader name without extension (e.g., "object_cull")</param>
    /// <param name="entryPoint">Entry point function name (default: "main")</param>
    ShaderResult LoadComputeShader(
        const char* name,
        const char* entryPoint = "main"
    );

    /// <summary>
    /// Get cache statistics
    /// </summary>
    const ShaderCache::Stats& GetCacheStats() const { return m_cache.GetStats(); }

    /// <summary>
    /// Get the underlying SlangCompiler (for direct access)
    /// </summary>
    xray::render::SlangCompiler* GetSlangCompiler() const { return m_slangCompiler; }

    /// <summary>
    /// Compile shader with preprocessor defines (for material shaders)
    /// </summary>
    bool CompileShaderWithDefines(
        const char* shaderName,
        const char* extension,
        const char* entryPoint,
        xray::render::SlangCompiler::Stage stage,
        const xr_vector<xray::render::SlangCompiler::Define>& defines,
        xr_vector<u8>& outBytecode
    );

    /// <summary>
    /// Get cached reflection data for a shader
    /// </summary>
    ExtractedReflection* GetCachedReflection(
        const char* shaderName,
        const char* extension
    );

private:
    /// <summary>
    /// Compile shader from source with caching
    /// </summary>
    /// <returns>true if compilation succeeded</returns>
    bool CompileShader(
        const char* shaderName,
        const char* extension,
        const char* entryPoint,
        xray::render::SlangCompiler::Stage stage,
        xr_vector<u8>& outBytecode
    );

    /// <summary>
    /// Read shader source from file
    /// </summary>
    IReader* OpenShaderFile(const char* name, const char* extension);

private:
    xray::render::SlangCompiler* m_slangCompiler;
    ShaderCache m_cache;

    // Cache for extracted reflection data (keyed by shader name + extension)
    xr_map<xr_string, ExtractedReflection*> m_reflectionCache;

    // In-memory cache for compiled shader handles (keyed by shader name + extension)
    // Prevents re-creating NVRHI handles every frame when passes call LoadVertexShader/LoadPixelShader
    xr_map<xr_string, nvrhi::ShaderHandle> m_handleCache;
};

} // namespace xray::render::framegraph
