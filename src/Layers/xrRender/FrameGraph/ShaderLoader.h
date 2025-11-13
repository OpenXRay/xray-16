#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "ShaderCache.h"

#include "Layers/xrRender/Shaders/SlangCompiler.h"

namespace xray::render::framegraph {

class ShaderLoader {
public:
    ShaderLoader(ng::RenderDevice* device, xray::render::SlangCompiler* slangCompiler);
    ~ShaderLoader();

    nvrhi::ShaderHandle LoadVertexShader(
        const char* name,
        const char* entryPoint = "main",
        xr_vector<u8>* outBytecode = nullptr
    );

    nvrhi::ShaderHandle LoadPixelShader(
        const char* name,
        const char* entryPoint = "main",
        xr_vector<u8>* outBytecode = nullptr
    );

    const ShaderCache::Stats& GetCacheStats() const { return m_cache.GetStats(); }

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
    ng::RenderDevice* m_device;
    xray::render::SlangCompiler* m_slangCompiler;
    ShaderCache m_cache;
};

}
