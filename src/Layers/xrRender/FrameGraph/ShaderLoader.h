#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"

namespace xray::render::framegraph {

class ShaderLoader {
public:
    ShaderLoader(ng::RenderDevice* device);
    ~ShaderLoader();

    nvrhi::IShader* LoadVertexShader(const char* name, const char* entryPoint = "main");
    nvrhi::IShader* LoadPixelShader(const char* name, const char* entryPoint = "main");

    ID3DBlob* CompileShader(
        const char* shaderPath,
        const char* entryPoint,
        const char* target
    );

private:
    ng::RenderDevice* m_device;

    IReader* OpenShaderFile(const char* name, const char* extension);
};

}
