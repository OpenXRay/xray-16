#include "stdafx.h"
#include "NVRHIUIRenderer.h"
#include "Geometry/MaterialCache.h"
#include "Shader.h"

namespace xray::render::ui
{

NVRHIUIRenderer::NVRHIUIRenderer() = default;

NVRHIUIRenderer::~NVRHIUIRenderer()
{
    Shutdown();
}

void NVRHIUIRenderer::Initialize(ng::RenderDevice* device, MaterialCache* matCache)
{
    VERIFY(device);
    VERIFY(matCache);

    m_device = device;
    m_matCache = matCache;

    Msg("  [NVRHIUIRenderer::Initialize] Starting initialization");

    CreateBuffers();

    m_initialized = true;
    Msg("* [NVRHIUIRenderer] Initialized successfully");
}

void NVRHIUIRenderer::Shutdown()
{
    m_constantBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_vertexBuffer = nullptr;
    m_matCache = nullptr;
    m_device = nullptr;
    m_initialized = false;
}

void NVRHIUIRenderer::RenderBatches(
    nvrhi::ICommandList* commandList,
    const xr_vector<UIGeometryBatch>& batches,
    nvrhi::IFramebuffer* framebuffer,
    u32 screenWidth,
    u32 screenHeight)
{
    if (!m_initialized)
    {
        Msg("! [NVRHIUIRenderer] Not initialized!");
        return;
    }

    if (batches.empty())
    {
        return;
    }

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (const auto& batch : batches)
    {
        totalVertices += batch.vertices.size();
        totalIndices += batch.indices.size();
    }

    EnsureBufferCapacity(totalVertices, totalIndices);

    struct UIConstants
    {
        float screenWidth;
        float screenHeight;
        float invScreenWidth;
        float invScreenHeight;
    };

    u8 cbData[256] = {};
    UIConstants* constants = reinterpret_cast<UIConstants*>(cbData);
    constants->screenWidth = static_cast<float>(screenWidth);
    constants->screenHeight = static_cast<float>(screenHeight);
    constants->invScreenWidth = 1.0f / constants->screenWidth;
    constants->invScreenHeight = 1.0f / constants->screenHeight;

    commandList->writeBuffer(m_constantBuffer, cbData, 256);

    IUIShader* lastUIShader = nullptr;
    MaterialPSO* currentPSO = nullptr;

    for (const auto& batch : batches)
    {
        if (batch.IsEmpty() || !batch.uiShader)
            continue;

        if (batch.uiShader != lastUIShader)
        {
            currentPSO = m_matCache->GetOrCreateUIPSO(batch.uiShader, 0, framebuffer);
            if (!currentPSO)
            {
                Msg("! [NVRHIUIRenderer] Failed to get PSO, skipping batch");
                continue;
            }
            lastUIShader = batch.uiShader;
        }

        xr_vector<const UIGeometryBatch*> singleBatch = { &batch };
        RenderBatchesWithShader(commandList, singleBatch, currentPSO, framebuffer, screenWidth, screenHeight);
    }
}

void NVRHIUIRenderer::RenderBatchesWithShader(
    nvrhi::ICommandList* commandList,
    const xr_vector<const UIGeometryBatch*>& batches,
    MaterialPSO* pso,
    nvrhi::IFramebuffer* framebuffer,
    u32 screenWidth,
    u32 screenHeight)
{
    if (!pso || !pso->pso)
        return;

    nvrhi::IGraphicsPipeline* nativePipeline = pso->pso->GetNativePipeline();
    if (!nativePipeline)
        return;

    m_matCache->GetOrCreateBindingSet(pso);

    if (!pso->vsBindingSet || !pso->psBindingSet) {
        Msg("! [NVRHIUIRenderer] Failed to create binding sets for shader");
        return;
    }

    nvrhi::GraphicsState state;
    state.pipeline = nativePipeline;
    state.framebuffer = framebuffer;
    state.viewport = nvrhi::ViewportState()
        .addViewportAndScissorRect(nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight)));

    state.addBindingSet(pso->vsBindingSet);
    state.addBindingSet(pso->psBindingSet);

    nvrhi::VertexBufferBinding vbBinding;
    vbBinding.buffer = m_vertexBuffer;
    vbBinding.slot = 0;
    vbBinding.offset = 0;
    state.addVertexBuffer(vbBinding);

    state.indexBuffer.buffer = m_indexBuffer;
    state.indexBuffer.format = nvrhi::Format::R16_UINT;
    state.indexBuffer.offset = 0;

    commandList->setGraphicsState(state);

    u32 vertexOffset = 0;
    u32 indexOffset = 0;

    for (const UIGeometryBatch* batch : batches)
    {
        if (batch->IsEmpty())
            continue;

        if (batch->hasScissor) {
            nvrhi::Rect scissor;
            scissor.minX = batch->scissorRect.x1;
            scissor.minY = batch->scissorRect.y1;
            scissor.maxX = batch->scissorRect.x2;
            scissor.maxY = batch->scissorRect.y2;

            state.viewport = nvrhi::ViewportState()
                .addViewport(nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight)))
                .addScissorRect(scissor);
        } else {
            state.viewport = nvrhi::ViewportState()
                .addViewportAndScissorRect(nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight)));
        }

        commandList->setGraphicsState(state);

        u32 batchVertexOffset = vertexOffset;
        u32 batchIndexOffset = indexOffset;
        UploadBatchGeometry(commandList, *batch, vertexOffset, indexOffset);

        nvrhi::DrawArguments drawArgs;
        drawArgs.vertexCount = static_cast<u32>(batch->indices.size());
        drawArgs.instanceCount = 1;
        drawArgs.startIndexLocation = batchIndexOffset;
        drawArgs.startVertexLocation = batchVertexOffset;

        commandList->drawIndexed(drawArgs);
    }
}

bool NVRHIUIRenderer::CreateBuffers()
{
    const size_t initialVertexCount = 4096;
    const size_t initialIndexCount = 8192;

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = 256;
    cbDesc.isConstantBuffer = true;
    cbDesc.debugName = "UI_ConstantBuffer";
    cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
    cbDesc.keepInitialState = true;

    m_constantBuffer = m_device->GetNVRHIDevice()->createBuffer(cbDesc);
    if (!m_constantBuffer)
    {
        Msg("! [NVRHIUIRenderer] Failed to create constant buffer");
        return false;
    }

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = initialVertexCount * sizeof(UIVertex);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "UI_VertexBuffer";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;

    m_vertexBuffer = m_device->GetNVRHIDevice()->createBuffer(vbDesc);
    if (!m_vertexBuffer)
    {
        Msg("! [NVRHIUIRenderer] Failed to create vertex buffer");
        return false;
    }
    m_vertexBufferSize = initialVertexCount;

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = initialIndexCount * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "UI_IndexBuffer";
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;

    m_indexBuffer = m_device->GetNVRHIDevice()->createBuffer(ibDesc);
    if (!m_indexBuffer)
    {
        Msg("! [NVRHIUIRenderer] Failed to create index buffer");
        return false;
    }
    m_indexBufferSize = initialIndexCount;

    return true;
}

void NVRHIUIRenderer::EnsureBufferCapacity(size_t vertexCount, size_t indexCount)
{
    if (vertexCount > m_vertexBufferSize)
    {
        size_t newSize = vertexCount * 2;

        nvrhi::BufferDesc vbDesc;
        vbDesc.byteSize = newSize * sizeof(UIVertex);
        vbDesc.isVertexBuffer = true;
        vbDesc.debugName = "UI_VertexBuffer";
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        vbDesc.keepInitialState = true;

        m_vertexBuffer = m_device->GetNVRHIDevice()->createBuffer(vbDesc);
        m_vertexBufferSize = newSize;

        Msg("  [NVRHIUIRenderer] Resized vertex buffer to %zu vertices", newSize);
    }

    if (indexCount > m_indexBufferSize)
    {
        size_t newSize = indexCount * 2;

        nvrhi::BufferDesc ibDesc;
        ibDesc.byteSize = newSize * sizeof(u16);
        ibDesc.isIndexBuffer = true;
        ibDesc.debugName = "UI_IndexBuffer";
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
        ibDesc.keepInitialState = true;

        m_indexBuffer = m_device->GetNVRHIDevice()->createBuffer(ibDesc);
        m_indexBufferSize = newSize;

        Msg("  [NVRHIUIRenderer] Resized index buffer to %zu indices", newSize);
    }
}

void NVRHIUIRenderer::UploadBatchGeometry(nvrhi::ICommandList* commandList,
                                          const UIGeometryBatch& batch,
                                          u32& vertexOffset, u32& indexOffset)
{
    if (batch.vertices.empty() || batch.indices.empty())
        return;

    const size_t vertexDataSize = batch.vertices.size() * sizeof(UIVertex);
    commandList->writeBuffer(m_vertexBuffer, batch.vertices.data(), vertexDataSize,
                            vertexOffset * sizeof(UIVertex));

    const size_t indexDataSize = batch.indices.size() * sizeof(u16);
    commandList->writeBuffer(m_indexBuffer, batch.indices.data(), indexDataSize,
                            indexOffset * sizeof(u16));

    vertexOffset += static_cast<u32>(batch.vertices.size());
    indexOffset += static_cast<u32>(batch.indices.size());
}

}
