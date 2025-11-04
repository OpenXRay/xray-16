#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"

namespace xray::render::framegraph {

class ShaderLoader {
public:
    ShaderLoader(ng::RenderDevice* device);
    ~ShaderLoader();

    nvrhi::ShaderHandle LoadVertexShader(const char* name, const char* entryPoint = "main", ID3DBlob** outBytecode = nullptr);
    nvrhi::ShaderHandle LoadPixelShader(const char* name, const char* entryPoint = "main", ID3DBlob** outBytecode = nullptr);

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
