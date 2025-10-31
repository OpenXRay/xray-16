#include "stdafx.h"
#include "RenderContext.h"

namespace xray::render::ng {

RenderContext::RenderContext(nvrhi::IDevice* device,
                             nvrhi::ICommandList* commandList)
    : m_device(device)
    , m_commandList(commandList)
{
    VERIFY(m_device != nullptr);
    VERIFY(m_commandList != nullptr);

    Msg("~ [RenderContext] Created");
}

RenderContext::~RenderContext() {
    if (m_inRenderPass) {
        Msg("! [RenderContext] Destroying while in render pass - forcing end");
        EndRenderPass();
    }

    Msg("~ [RenderContext] Destroyed");
}

void RenderContext::BeginRenderPass(const RenderPassDesc& desc) {
    VERIFY2(!m_inRenderPass, "Already in render pass!");

    m_currentRenderPass = desc;

    nvrhi::FramebufferDesc fbDesc;

    for (u32 i = 0; i < desc.numRenderTargets; i++) {
        fbDesc.addColorAttachment(desc.renderTargets[i]);
    }

    if (desc.depthStencil) {
        fbDesc.setDepthAttachment(desc.depthStencil);
    }

    m_currentFramebuffer = m_device->createFramebuffer(fbDesc);

    if (!m_currentFramebuffer) {
        Msg("! [RenderContext] Failed to create framebuffer");
        return;
    }

    m_commandList->beginMarker("RenderPass");

    m_currentState = nvrhi::GraphicsState();
    m_currentState.framebuffer = m_currentFramebuffer;

    if (desc.clearColor) {
        for (u32 i = 0; i < desc.numRenderTargets; i++) {
            nvrhi::Color clearColor(
                desc.clearValue.color[0],
                desc.clearValue.color[1],
                desc.clearValue.color[2],
                desc.clearValue.color[3]
            );
            m_commandList->clearTextureFloat(
                desc.renderTargets[i],
                nvrhi::AllSubresources,
                clearColor
            );
        }
    }

    if (desc.clearDepth || desc.clearStencil) {
        if (desc.depthStencil) {
            m_commandList->clearDepthStencilTexture(
                desc.depthStencil,
                nvrhi::AllSubresources,
                desc.clearDepth,
                desc.clearValue.depth,
                desc.clearStencil,
                desc.clearValue.stencil
            );
        }
    }

    m_inRenderPass = true;
}

void RenderContext::EndRenderPass() {
    VERIFY2(m_inRenderPass, "Not in render pass!");

    m_commandList->endMarker();

    m_inRenderPass = false;
}

void RenderContext::SetPipeline(nvrhi::IGraphicsPipeline* pipeline) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(pipeline != nullptr);

    m_currentState.pipeline = pipeline;
}

void RenderContext::SetPipeline(PipelineStateHandle pso) {
    VERIFY2(false, "Pipeline handle support not yet implemented");
}

void RenderContext::SetViewport(const Viewport& viewport) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    nvrhi::Viewport nvrhiViewport;
    nvrhiViewport.minX = viewport.x;
    nvrhiViewport.maxX = viewport.x + viewport.width;
    nvrhiViewport.minY = viewport.y;
    nvrhiViewport.maxY = viewport.y + viewport.height;
    nvrhiViewport.minZ = viewport.minDepth;
    nvrhiViewport.maxZ = viewport.maxDepth;

    m_currentState.viewport.addViewportAndScissorRect(nvrhiViewport);
}

void RenderContext::SetViewport(float x, float y, float width, float height) {
    Viewport vp;
    vp.x = x;
    vp.y = y;
    vp.width = width;
    vp.height = height;
    SetViewport(vp);
}

void RenderContext::SetScissor(const Rect& scissor) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    nvrhi::Rect nvrhiRect;
    nvrhiRect.minX = scissor.x;
    nvrhiRect.minY = scissor.y;
    nvrhiRect.maxX = scissor.x + scissor.width;
    nvrhiRect.maxY = scissor.y + scissor.height;

    m_currentState.viewport.addScissorRect(nvrhiRect);
}

void RenderContext::SetVertexBuffer(u32 slot, nvrhi::IBuffer* buffer, u64 offset) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(buffer != nullptr);

    m_currentState.vertexBuffers.resize(slot + 1);
    m_currentState.vertexBuffers[slot].buffer = buffer;
    m_currentState.vertexBuffers[slot].slot = slot;
    m_currentState.vertexBuffers[slot].offset = offset;
}

void RenderContext::SetVertexBuffer(u32 slot, BufferHandle buffer, u64 offset) {
    VERIFY2(false, "Buffer handle support not yet implemented");
}

void RenderContext::SetIndexBuffer(nvrhi::IBuffer* buffer,
                                   nvrhi::Format format,
                                   u64 offset) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(buffer != nullptr);

    m_currentState.indexBuffer.buffer = buffer;
    m_currentState.indexBuffer.format = format;
    m_currentState.indexBuffer.offset = offset;
}

void RenderContext::SetIndexBuffer(BufferHandle buffer,
                                   nvrhi::Format format,
                                   u64 offset) {
    VERIFY2(false, "Buffer handle support not yet implemented");
}

void RenderContext::SetTexture(u32 slot, nvrhi::ITexture* texture) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

}

void RenderContext::SetTexture(u32 slot, TextureHandle texture) {
    VERIFY2(false, "Texture handle support not yet implemented");
}

void RenderContext::SetSampler(u32 slot, nvrhi::ISampler* sampler) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

}

void RenderContext::SetConstantBuffer(u32 slot, nvrhi::IBuffer* buffer) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

}

void RenderContext::SetConstantBuffer(u32 slot, BufferHandle buffer) {
    VERIFY2(false, "Buffer handle support not yet implemented");
}

nvrhi::BindingLayoutHandle RenderContext::CreateBindingLayout(const BindingLayoutDesc& desc) {
    VERIFY(m_device != nullptr);
    VERIFY2(desc.numItems > 0, "Binding layout must have at least one item");

    nvrhi::BindingLayoutDesc nvrhiDesc;

    nvrhiDesc.visibility = desc.visibility;
    nvrhiDesc.registerSpace = desc.registerSpace;

    for (u32 i = 0; i < desc.numItems; i++) {
        const auto& item = desc.items[i];

        nvrhi::BindingLayoutItem nvrhiItem;
        nvrhiItem.slot = item.slot;
        nvrhiItem.type = item.type;
        nvrhiItem.size = item.size;

        nvrhiDesc.bindings.push_back(nvrhiItem);
    }

    nvrhi::BindingLayoutHandle layout = m_device->createBindingLayout(nvrhiDesc);

    if (!layout) {
        Msg("! [RenderContext] Failed to create binding layout");
        return nullptr;
    }

    Msg("~ [RenderContext] Created binding layout with %d bindings", desc.numItems);
    return layout;
}

BindingLayoutHandle RenderContext::CreateBindingLayout(const BindingLayoutDesc& desc, bool useHandle) {
    VERIFY2(false, "Binding layout handle support not yet implemented");
    BindingLayoutHandle handle;
    return handle;
}

nvrhi::BindingSetHandle RenderContext::CreateBindingSet(const BindingSetDesc& desc) {
    VERIFY(m_device != nullptr);
    VERIFY2(desc.layout != nullptr, "BindingSet requires a valid layout");
    VERIFY2(desc.numItems > 0, "BindingSet must have at least one item");

    nvrhi::BindingSetDesc nvrhiDesc;

    for (u32 i = 0; i < desc.numItems; i++) {
        const auto& item = desc.items[i];

        nvrhi::BindingSetItem nvrhiItem = {};
        nvrhiItem.resourceHandle = item.resource;
        nvrhiItem.slot = item.slot;
        nvrhiItem.arrayElement = item.arrayElement;
        nvrhiItem.type = item.type;
        nvrhiItem.format = item.format;
        nvrhiItem.dimension = item.dimension;

        if ((item.type == nvrhi::ResourceType::Texture_SRV ||
             item.type == nvrhi::ResourceType::Texture_UAV) &&
            item.dimension == nvrhi::TextureDimension::Unknown) {
            auto* texture = static_cast<nvrhi::ITexture*>(item.resource);
            if (texture) {
                const auto& texDesc = texture->getDesc();
                nvrhiItem.dimension = texDesc.dimension;
            }
        }

        if (item.type == nvrhi::ResourceType::ConstantBuffer ||
            item.type == nvrhi::ResourceType::TypedBuffer_SRV ||
            item.type == nvrhi::ResourceType::TypedBuffer_UAV ||
            item.type == nvrhi::ResourceType::StructuredBuffer_SRV ||
            item.type == nvrhi::ResourceType::StructuredBuffer_UAV ||
            item.type == nvrhi::ResourceType::RawBuffer_SRV ||
            item.type == nvrhi::ResourceType::RawBuffer_UAV) {
            nvrhiItem.range = nvrhi::BufferRange();
        }

        if (item.type == nvrhi::ResourceType::Texture_SRV ||
            item.type == nvrhi::ResourceType::Texture_UAV) {
            nvrhiItem.subresources = nvrhi::AllSubresources;
        }

        nvrhiDesc.bindings.push_back(nvrhiItem);
    }

    nvrhi::BindingSetHandle bindingSet = m_device->createBindingSet(nvrhiDesc, desc.layout);

    if (!bindingSet) {
        Msg("! [RenderContext] Failed to create binding set");
        return nullptr;
    }

    Msg("~ [RenderContext] Created binding set with %d resources", desc.numItems);
    return bindingSet;
}

BindingSetHandle RenderContext::CreateBindingSet(const BindingSetDesc& desc, bool useHandle) {
    VERIFY2(false, "Binding set handle support not yet implemented");
    BindingSetHandle handle;
    return handle;
}

void RenderContext::SetBindingSet(u32 slot, nvrhi::IBindingSet* bindingSet) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(bindingSet != nullptr);
    VERIFY2(slot < 6, "NVRHI supports up to 6 binding set slots (0-5)");

    m_currentState.bindings.resize(slot + 1);
    m_currentState.bindings[slot] = bindingSet;

}

void RenderContext::SetBindingSet(u32 slot, BindingSetHandle bindingSet) {
    VERIFY2(false, "Binding set handle support not yet implemented");
}

void RenderContext::Draw(u32 vertexCount, u32 startVertex) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    m_commandList->setGraphicsState(m_currentState);

    nvrhi::DrawArguments args;
    args.vertexCount = vertexCount;
    args.startVertexLocation = startVertex;

    m_commandList->draw(args);
}

void RenderContext::DrawIndexed(u32 indexCount, u32 startIndex, int baseVertex) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    m_commandList->setGraphicsState(m_currentState);

    nvrhi::DrawArguments args;
    args.vertexCount = indexCount;
    args.startIndexLocation = startIndex;
    args.startVertexLocation = baseVertex;

    m_commandList->drawIndexed(args);
}

void RenderContext::DrawInstanced(u32 vertexCount, u32 instanceCount,
                                  u32 startVertex, u32 startInstance) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    m_commandList->setGraphicsState(m_currentState);

    nvrhi::DrawArguments args;
    args.vertexCount = vertexCount;
    args.instanceCount = instanceCount;
    args.startVertexLocation = startVertex;
    args.startInstanceLocation = startInstance;

    m_commandList->draw(args);
}

void RenderContext::DrawIndexedInstanced(u32 indexCount, u32 instanceCount,
                                         u32 startIndex, int baseVertex,
                                         u32 startInstance) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    m_commandList->setGraphicsState(m_currentState);

    nvrhi::DrawArguments args;
    args.vertexCount = indexCount;
    args.instanceCount = instanceCount;
    args.startIndexLocation = startIndex;
    args.startVertexLocation = baseVertex;
    args.startInstanceLocation = startInstance;

    m_commandList->drawIndexed(args);
}

void RenderContext::ClearRenderTarget(nvrhi::ITexture* rt, const float color[4]) {
    VERIFY(rt != nullptr);

    nvrhi::Color clearColor(color[0], color[1], color[2], color[3]);
    m_commandList->clearTextureFloat(rt, nvrhi::AllSubresources, clearColor);
}

void RenderContext::ClearDepthStencil(nvrhi::ITexture* ds, float depth, u8 stencil) {
    VERIFY(ds != nullptr);

    m_commandList->clearDepthStencilTexture(
        ds,
        nvrhi::AllSubresources,
        true,
        depth,
        true,
        stencil
    );
}

}
