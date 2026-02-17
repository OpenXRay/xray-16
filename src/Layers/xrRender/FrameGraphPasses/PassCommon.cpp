#include "stdafx.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"

namespace xray::render::RENDER_NAMESPACE::passes {

nvrhi::BufferHandle GetOrCreateDrawIndexBuffer(const char* passName, nvrhi::IDevice* device)
{
    auto& cache = framegraph::GetPassResourceCache();
    if (cache.HasStaticBuffer(passName, "DrawIndexBuffer")) {
        nvrhi::BufferDesc desc;
        desc.byteSize = 65536 * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.isVertexBuffer = true;
        desc.debugName = "DrawIndexBuffer";
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;
        return cache.GetOrCreateStaticBuffer(passName, "DrawIndexBuffer", desc, device);
    }

    nvrhi::BufferDesc desc;
    desc.byteSize = 65536 * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.isVertexBuffer = true;
    desc.debugName = "DrawIndexBuffer";
    desc.initialState = nvrhi::ResourceStates::VertexBuffer;
    desc.keepInitialState = true;
    auto buffer = cache.GetOrCreateStaticBuffer(passName, "DrawIndexBuffer", desc, device);

    if (buffer && GEnv.Backend) {
        xr_vector<u32> drawIndices(65536);
        for (u32 i = 0; i < 65536; i++)
            drawIndices[i] = i;
        GEnv.Backend->UploadBufferData(buffer, drawIndices.data(), 65536 * sizeof(u32));
    }
    return buffer;
}

nvrhi::FramebufferHandle CreateDummyPipelineFramebuffer(nvrhi::IDevice* device)
{
    nvrhi::TextureDesc colorDesc;
    colorDesc.width = 64;
    colorDesc.height = 64;
    colorDesc.format = nvrhi::Format::RGBA16_FLOAT;
    colorDesc.isRenderTarget = true;
    colorDesc.initialState = nvrhi::ResourceStates::RenderTarget;
    colorDesc.keepInitialState = true;
    colorDesc.debugName = "DummyPipelineColor";
    auto dummyColorRT = device->createTexture(colorDesc);

    nvrhi::TextureDesc normalDesc = colorDesc;
    normalDesc.debugName = "DummyPipelineNormal";
    auto dummyNormalRT = device->createTexture(normalDesc);

    nvrhi::TextureDesc depthDesc;
    depthDesc.width = 64;
    depthDesc.height = 64;
    depthDesc.format = nvrhi::Format::D32;
    depthDesc.isRenderTarget = true;
    depthDesc.initialState = nvrhi::ResourceStates::DepthWrite;
    depthDesc.keepInitialState = true;
    depthDesc.debugName = "DummyPipelineDepth";
    auto dummyDepthRT = device->createTexture(depthDesc);

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(dummyColorRT);
    fbDesc.addColorAttachment(dummyNormalRT);
    fbDesc.setDepthAttachment(dummyDepthRT);
    return device->createFramebuffer(fbDesc);
}

} // namespace xray::render::RENDER_NAMESPACE::passes
