// xrRender/FrameGraph/ShaderLoader.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "ShaderCache.h"

// Need full include (not just forward decl) since we use SlangCompiler::Stage
#include "Layers/xrRender/Shaders/SlangCompiler.h"

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
    ShaderLoader(ng::RenderDevice* device, xray::render::SlangCompiler* slangCompiler);
    ~ShaderLoader();

    /// <summary>
    /// Load and compile vertex shader from res/gamedata/shaders/r3/
    /// </summary>
    /// <param name="name">Shader name without extension (e.g., "gbuffer")</param>
    /// <param name="entryPoint">Entry point function name (default: "main")</param>
    /// <param name="outBytecode">Optional: output bytecode for reflection</param>
    nvrhi::ShaderHandle LoadVertexShader(
        const char* name,
        const char* entryPoint = "main",
        xr_vector<u8>* outBytecode = nullptr
    );

    /// <summary>
    /// Load and compile pixel shader from res/gamedata/shaders/r3/
    /// </summary>
    nvrhi::ShaderHandle LoadPixelShader(
        const char* name,
        const char* entryPoint = "main",
        xr_vector<u8>* outBytecode = nullptr
    );

    /// <summary>
    /// Get cache statistics
    /// </summary>
    const ShaderCache::Stats& GetCacheStats() const { return m_cache.GetStats(); }

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
    ng::RenderDevice* m_device;
    xray::render::SlangCompiler* m_slangCompiler;
    ShaderCache m_cache;
};

} // namespace xray::render::framegraph
