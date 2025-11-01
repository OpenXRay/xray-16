#include "stdafx.h"
#include "RenderContext.h"
#include "RenderDevice.h"

namespace xray::render::ng {

RenderContext::RenderContext(RenderDevice* device,
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

    // Log final statistics
    Msg("~ [RenderContext] Stats: %d draws, %d pipeline changes, %d redundant calls avoided",
        m_stats.numDrawCalls + m_stats.numDrawIndexedCalls +
        m_stats.numDrawInstancedCalls + m_stats.numDrawIndexedInstancedCalls,
        m_stats.numPipelineChanges,
        m_stats.numRedundantPipeline + m_stats.numRedundantViewport +
        m_stats.numRedundantVertexBuffer + m_stats.numRedundantIndexBuffer +
        m_stats.numRedundantBindingSet);

    Msg("~ [RenderContext] Destroyed");
}

// ═══════════════════════════════════════════════════════
//  RENDER PASS MANAGEMENT
// ═══════════════════════════════════════════════════════

void RenderContext::BeginRenderPass(const RenderPassDesc& desc) {
    VERIFY2(!m_inRenderPass, "Already in render pass!");

    // Reset state cache for new render pass
    m_stateCache.Reset();

    // Store current render pass
    m_currentRenderPass = desc;

    // Build NVRHI framebuffer descriptor
    nvrhi::FramebufferDesc fbDesc;

    for (u32 i = 0; i < desc.numRenderTargets; i++) {
        fbDesc.addColorAttachment(desc.renderTargets[i]);
    }

    if (desc.depthStencil) {
        fbDesc.setDepthAttachment(desc.depthStencil);
    }

    // Create framebuffer through our abstraction layer
    m_currentFramebuffer = m_device->CreateFramebuffer(fbDesc);

    if (!m_currentFramebuffer) {
        Msg("! [RenderContext] Failed to create framebuffer");
        return;
    }

    // Begin render pass in command list
    m_commandList->beginMarker("RenderPass");  // Debug marker

    // Initialize current graphics state with framebuffer
    m_currentState = nvrhi::GraphicsState();
    m_currentState.framebuffer = m_currentFramebuffer;

    // Clear if requested
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

    m_commandList->endMarker();  // End debug marker

    m_inRenderPass = false;
}

// ═══════════════════════════════════════════════════════
//  PIPELINE STATE
// ═══════════════════════════════════════════════════════

void RenderContext::SetPipeline(nvrhi::IGraphicsPipeline* pipeline) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(pipeline != nullptr);

    // Check cache - skip if already set
    if (m_stateCache.pipeline == pipeline) {
        m_stats.numRedundantPipeline++;
        return;
    }

    // Update cache
    m_stateCache.pipeline = pipeline;
    m_stats.numPipelineChanges++;

    m_currentState.pipeline = pipeline;
    // Don't call setGraphicsState yet - batch state changes
}

void RenderContext::SetPipeline(PipelineStateHandle pso) {
    // TODO: Implement once we have pipeline state cache
    VERIFY2(false, "Pipeline handle support not yet implemented");
}

// ═══════════════════════════════════════════════════════
//  VIEWPORT & SCISSOR
// ═══════════════════════════════════════════════════════

void RenderContext::SetViewport(const Viewport& viewport) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    // Check cache - compare viewport values
    if (m_stateCache.viewportSet &&
        m_stateCache.viewport.x == viewport.x &&
        m_stateCache.viewport.y == viewport.y &&
        m_stateCache.viewport.width == viewport.width &&
        m_stateCache.viewport.height == viewport.height &&
        m_stateCache.viewport.minDepth == viewport.minDepth &&
        m_stateCache.viewport.maxDepth == viewport.maxDepth) {
        m_stats.numRedundantViewport++;
        return;
    }

    // Update cache
    m_stateCache.viewport = viewport;
    m_stateCache.viewportSet = true;
    m_stats.numViewportChanges++;

    nvrhi::Viewport nvrhiViewport;
    nvrhiViewport.minX = viewport.x;
    nvrhiViewport.maxX = viewport.x + viewport.width;
    nvrhiViewport.minY = viewport.y;
    nvrhiViewport.maxY = viewport.y + viewport.height;
    nvrhiViewport.minZ = viewport.minDepth;
    nvrhiViewport.maxZ = viewport.maxDepth;

    m_currentState.viewport.addViewportAndScissorRect(nvrhiViewport);
    // Don't call setGraphicsState yet - batch state changes
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
    // Don't call setGraphicsState yet - batch state changes
}

// ═══════════════════════════════════════════════════════
//  VERTEX & INDEX BUFFERS
// ═══════════════════════════════════════════════════════

void RenderContext::SetVertexBuffer(u32 slot, nvrhi::IBuffer* buffer, u64 offset) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(buffer != nullptr);
    VERIFY2(slot < 8, "Vertex buffer slot out of range");

    // Check cache - compare buffer and offset
    if (m_stateCache.vertexBuffers[slot] == buffer &&
        m_stateCache.vertexBufferOffsets[slot] == offset) {
        m_stats.numRedundantVertexBuffer++;
        return;
    }

    // Update cache
    m_stateCache.vertexBuffers[slot] = buffer;
    m_stateCache.vertexBufferOffsets[slot] = offset;
    m_stats.numVertexBufferChanges++;

    m_currentState.vertexBuffers.resize(slot + 1);
    m_currentState.vertexBuffers[slot].buffer = buffer;
    m_currentState.vertexBuffers[slot].slot = slot;
    m_currentState.vertexBuffers[slot].offset = offset;
    // Don't call setGraphicsState yet - batch state changes
}

void RenderContext::SetVertexBuffer(u32 slot, BufferHandle buffer, u64 offset) {
    // TODO: Implement once we have buffer manager
    VERIFY2(false, "Buffer handle support not yet implemented");
}

void RenderContext::SetIndexBuffer(nvrhi::IBuffer* buffer,
                                   nvrhi::Format format,
                                   u64 offset) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(buffer != nullptr);

    // Check cache - compare buffer, format, and offset
    if (m_stateCache.indexBuffer == buffer &&
        m_stateCache.indexBufferFormat == format &&
        m_stateCache.indexBufferOffset == offset) {
        m_stats.numRedundantIndexBuffer++;
        return;
    }

    // Update cache
    m_stateCache.indexBuffer = buffer;
    m_stateCache.indexBufferFormat = format;
    m_stateCache.indexBufferOffset = offset;
    m_stats.numIndexBufferChanges++;

    m_currentState.indexBuffer.buffer = buffer;
    m_currentState.indexBuffer.format = format;
    m_currentState.indexBuffer.offset = offset;
    // Don't call setGraphicsState yet - batch state changes
}

void RenderContext::SetIndexBuffer(BufferHandle buffer,
                                   nvrhi::Format format,
                                   u64 offset) {
    // TODO: Implement once we have buffer manager
    VERIFY2(false, "Buffer handle support not yet implemented");
}

// ═══════════════════════════════════════════════════════
//  TEXTURES & SAMPLERS
// ═══════════════════════════════════════════════════════

void RenderContext::SetTexture(u32 slot, nvrhi::ITexture* texture) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    // TODO: Bind via descriptor sets once we have binding system
    // For now, just store and apply before draw
}

void RenderContext::SetTexture(u32 slot, TextureHandle texture) {
    // TODO: Implement once we have texture manager
    VERIFY2(false, "Texture handle support not yet implemented");
}

void RenderContext::SetSampler(u32 slot, nvrhi::ISampler* sampler) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    // TODO: Bind via descriptor sets
}

// ═══════════════════════════════════════════════════════
//  CONSTANT BUFFERS
// ═══════════════════════════════════════════════════════

void RenderContext::SetConstantBuffer(u32 slot, nvrhi::IBuffer* buffer) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(buffer != nullptr);
    VERIFY(m_commandList != nullptr);

    // Step 1: Create binding layout (describes what we're binding)
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::ConstantBuffer(slot)
    };

    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        Msg("! [RenderContext] Failed to create binding layout for constant buffer");
        return;
    }

    // Step 2: Create binding set (binds actual buffer resource)
    nvrhi::BindingSetDesc bindingDesc;
    bindingDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(slot, buffer)
    };

    nvrhi::BindingSetHandle bindingSet = m_device->CreateBindingSet(bindingDesc, layout);
    if (!bindingSet) {
        Msg("! [RenderContext] Failed to create binding set for constant buffer");
        return;
    }

    // Step 3: Update current state (don't apply yet - batch with other state changes)
    m_currentState.bindings[slot] = bindingSet.Get();

    // Don't call setGraphicsState yet - batch state changes
}

void RenderContext::SetConstantBuffer(u32 slot, BufferHandle buffer) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(buffer.IsValid());

    // Resolve handle to native buffer through our abstraction layer
    nvrhi::IBuffer* nativeBuffer = m_device->GetNativeBuffer(buffer);
    VERIFY(nativeBuffer != nullptr);

    // Use the direct buffer path
    SetConstantBuffer(slot, nativeBuffer);
}

// ═══════════════════════════════════════════════════════
//  BINDING LAYOUTS & DESCRIPTOR SETS
// ═══════════════════════════════════════════════════════

nvrhi::BindingLayoutHandle RenderContext::CreateBindingLayout(const BindingLayoutDesc& desc) {
    VERIFY(m_device != nullptr);
    VERIFY2(desc.numItems > 0, "Binding layout must have at least one item");

    // Convert our descriptor to NVRHI's binding layout descriptor
    nvrhi::BindingLayoutDesc nvrhiDesc;

    // Set visibility for all bindings in this layout
    nvrhiDesc.visibility = desc.visibility;
    nvrhiDesc.registerSpace = desc.registerSpace;

    // Add all binding items
    for (u32 i = 0; i < desc.numItems; i++) {
        const auto& item = desc.items[i];

        nvrhi::BindingLayoutItem nvrhiItem;
        nvrhiItem.slot = item.slot;
        nvrhiItem.type = item.type;
        nvrhiItem.size = item.size;  // Array size

        nvrhiDesc.bindings.push_back(nvrhiItem);
    }

    // Create the binding layout through our abstraction layer
    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(nvrhiDesc);

    if (!layout) {
        Msg("! [RenderContext] Failed to create binding layout");
        return nullptr;
    }

    Msg("~ [RenderContext] Created binding layout with %d bindings", desc.numItems);
    return layout;
}

BindingLayoutHandle RenderContext::CreateBindingLayout(const BindingLayoutDesc& desc, bool useHandle) {
    // TODO: Implement once we have resource manager
    VERIFY2(false, "Binding layout handle support not yet implemented");
    BindingLayoutHandle handle;
    return handle;
}

nvrhi::BindingSetHandle RenderContext::CreateBindingSet(const BindingSetDesc& desc) {
    VERIFY(m_device != nullptr);
    VERIFY2(desc.layout != nullptr, "BindingSet requires a valid layout");
    VERIFY2(desc.numItems > 0, "BindingSet must have at least one item");

    // Convert our descriptor to NVRHI's binding set descriptor
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

        // Set dimension for textures if not already set
        if ((item.type == nvrhi::ResourceType::Texture_SRV ||
             item.type == nvrhi::ResourceType::Texture_UAV) &&
            item.dimension == nvrhi::TextureDimension::Unknown) {
            // Try to infer dimension from texture
            auto* texture = static_cast<nvrhi::ITexture*>(item.resource);
            if (texture) {
                const auto& texDesc = texture->getDesc();
                nvrhiItem.dimension = texDesc.dimension;
            }
        }

        // For buffers with ranges, use default whole buffer range
        if (item.type == nvrhi::ResourceType::ConstantBuffer ||
            item.type == nvrhi::ResourceType::TypedBuffer_SRV ||
            item.type == nvrhi::ResourceType::TypedBuffer_UAV ||
            item.type == nvrhi::ResourceType::StructuredBuffer_SRV ||
            item.type == nvrhi::ResourceType::StructuredBuffer_UAV ||
            item.type == nvrhi::ResourceType::RawBuffer_SRV ||
            item.type == nvrhi::ResourceType::RawBuffer_UAV) {
            // Use entire buffer (default BufferRange)
            nvrhiItem.range = nvrhi::BufferRange();
        }

        // For textures with subresources, use all subresources
        if (item.type == nvrhi::ResourceType::Texture_SRV ||
            item.type == nvrhi::ResourceType::Texture_UAV) {
            nvrhiItem.subresources = nvrhi::AllSubresources;
        }

        nvrhiDesc.bindings.push_back(nvrhiItem);
    }

    // Create the binding set through our abstraction layer
    nvrhi::BindingSetHandle bindingSet = m_device->CreateBindingSet(nvrhiDesc, desc.layout);

    if (!bindingSet) {
        Msg("! [RenderContext] Failed to create binding set");
        return nullptr;
    }

    Msg("~ [RenderContext] Created binding set with %d resources", desc.numItems);
    return bindingSet;
}

BindingSetHandle RenderContext::CreateBindingSet(const BindingSetDesc& desc, bool useHandle) {
    // TODO: Implement once we have resource manager
    VERIFY2(false, "Binding set handle support not yet implemented");
    BindingSetHandle handle;
    return handle;
}

void RenderContext::SetBindingSet(u32 slot, nvrhi::IBindingSet* bindingSet) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");
    VERIFY(bindingSet != nullptr);
    VERIFY2(slot < 6, "NVRHI supports up to 6 binding set slots (0-5)");

    // Check cache - compare binding set
    if (m_stateCache.bindingSets[slot] == bindingSet) {
        m_stats.numRedundantBindingSet++;
        return;
    }

    // Update cache
    m_stateCache.bindingSets[slot] = bindingSet;
    m_stats.numBindingSetChanges++;

    // Add binding set to graphics state
    // NVRHI stores binding sets in an array
    m_currentState.bindings.resize(slot + 1);
    m_currentState.bindings[slot] = bindingSet;

    // Don't call setGraphicsState yet - batch state changes
}

void RenderContext::SetBindingSet(u32 slot, BindingSetHandle bindingSet) {
    // TODO: Implement once we have resource manager
    VERIFY2(false, "Binding set handle support not yet implemented");
}

// ═══════════════════════════════════════════════════════
//  DRAW CALLS
// ═══════════════════════════════════════════════════════

void RenderContext::Draw(u32 vertexCount, u32 startVertex) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    // Track draw call
    m_stats.numDrawCalls++;

    // Apply all batched state changes before draw
    m_commandList->setGraphicsState(m_currentState);

    nvrhi::DrawArguments args;
    args.vertexCount = vertexCount;
    args.startVertexLocation = startVertex;

    m_commandList->draw(args);
}

void RenderContext::DrawIndexed(u32 indexCount, u32 startIndex, int baseVertex) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    // Track draw call
    m_stats.numDrawIndexedCalls++;

    // Apply all batched state changes before draw
    m_commandList->setGraphicsState(m_currentState);

    nvrhi::DrawArguments args;
    args.vertexCount = indexCount;  // Actually index count for indexed draws
    args.startIndexLocation = startIndex;
    args.startVertexLocation = baseVertex;

    m_commandList->drawIndexed(args);
}

void RenderContext::DrawInstanced(u32 vertexCount, u32 instanceCount,
                                  u32 startVertex, u32 startInstance) {
    VERIFY2(m_inRenderPass, "Must be in render pass!");

    // Track draw call
    m_stats.numDrawInstancedCalls++;

    // Apply all batched state changes before draw
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

    // Track draw call
    m_stats.numDrawIndexedInstancedCalls++;

    // Apply all batched state changes before draw
    m_commandList->setGraphicsState(m_currentState);

    nvrhi::DrawArguments args;
    args.vertexCount = indexCount;
    args.instanceCount = instanceCount;
    args.startIndexLocation = startIndex;
    args.startVertexLocation = baseVertex;
    args.startInstanceLocation = startInstance;

    m_commandList->drawIndexed(args);
}

// ═══════════════════════════════════════════════════════
//  CLEAR OPERATIONS
// ═══════════════════════════════════════════════════════

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
        true,  // Clear depth
        depth,
        true,  // Clear stencil
        stencil
    );
}

} // namespace xray::render::ng
