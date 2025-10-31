// xrRender/FrameGraph/ShaderLoader.h
#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"

namespace xray::render::framegraph {

// ══════════════════════════════════════════════════════════
//  SHADER LOADER FOR FRAMEGRAPH PASSES
// ══════════════════════════════════════════════════════════

class ShaderLoader {
public:
    ShaderLoader(ng::RenderDevice* device);
    ~ShaderLoader();

    // Load and compile shader from res/gamedata/shaders/r3/
    // name: shader name without extension (e.g. "gbuffer", "lighting")
    // Returns null on failure
    nvrhi::IShader* LoadVertexShader(const char* name, const char* entryPoint = "main");
    nvrhi::IShader* LoadPixelShader(const char* name, const char* entryPoint = "main");

    // Compile HLSL source to bytecode
    // Returns bytecode blob (caller must Release it)
    ID3DBlob* CompileShader(
        const char* shaderPath,  // Full path to .vs or .ps file
        const char* entryPoint,
        const char* target       // "vs_5_0", "ps_5_0", etc.
    );

private:
    ng::RenderDevice* m_device;

    // Helper: Read shader source from file
    IReader* OpenShaderFile(const char* name, const char* extension);
};

} // namespace xray::render::framegraph
