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
//  - Compiles HLSL to DXBC (DX11) using Slang
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
    /// </summary>
    /// <param name="name">Shader name without extension (e.g., "gbuffer")</param>
    /// <param name="entryPoint">Entry point function name (default: "main")</param>
    /// <param name="outBytecode">Optional: output bytecode for reflection (DEPRECATED - use ShaderResult)</param>
    nvrhi::ShaderHandle LoadVertexShader(
        const char* name,
        const char* entryPoint = "main",
        xr_vector<u8>* outBytecode = nullptr
    );

    /// <summary>
    /// Load and compile vertex shader with Slang reflection
    /// </summary>
    ShaderResult LoadVertexShaderWithReflection(
        const char* name,
        const char* entryPoint = "main"
    );

    /// <summary>
    /// Load and compile pixel shader from res/gamedata/shaders/r5/
    /// </summary>
    nvrhi::ShaderHandle LoadPixelShader(
        const char* name,
        const char* entryPoint = "main",
        xr_vector<u8>* outBytecode = nullptr
    );

    /// <summary>
    /// Load and compile pixel shader with Slang reflection
    /// </summary>
    ShaderResult LoadPixelShaderWithReflection(
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
};

} // namespace xray::render::framegraph
