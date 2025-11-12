// xrRender/NVRHIUIRenderer.cpp
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
    Msg("  [NVRHIUIRenderer::RenderBatches] Called with %zu batches", batches.size());

    if (!m_initialized)
    {
        Msg("! [NVRHIUIRenderer] Not initialized!");
        return;
    }

    if (batches.empty())
    {
        Msg("  [NVRHIUIRenderer] No batches to render");
        return;
    }

    // Calculate total geometry size
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (const auto& batch : batches)
    {
        totalVertices += batch.vertices.size();
        totalIndices += batch.indices.size();
    }

    Msg("  [NVRHIUIRenderer] Total geometry: %zu vertices, %zu indices", totalVertices, totalIndices);

    // Ensure buffers are large enough
    EnsureBufferCapacity(totalVertices, totalIndices);

    // Update constant buffer with screen size
    struct UIConstants
    {
        float screenWidth;
        float screenHeight;
        float invScreenWidth;
        float invScreenHeight;
    };

    // CRITICAL: Constant buffer is 256 bytes (see CreateBuffers), must write full size!
    // D3D11 requires constant buffers to be fully updated, not partially.
    u8 cbData[256] = {};  // Zero-initialize full buffer
    UIConstants* constants = reinterpret_cast<UIConstants*>(cbData);
    constants->screenWidth = static_cast<float>(screenWidth);
    constants->screenHeight = static_cast<float>(screenHeight);
    constants->invScreenWidth = 1.0f / constants->screenWidth;
    constants->invScreenHeight = 1.0f / constants->screenHeight;

    commandList->writeBuffer(m_constantBuffer, cbData, 256);

    // ═══════════════════════════════════════════════════════
    //  RENDER BATCHES IN SUBMISSION ORDER (IMPORTANT FOR Z-ORDER!)
    // ═══════════════════════════════════════════════════════
    // NOTE: We used to group by shader for efficiency, but that destroys
    // the submission order (cursor needs to render LAST, not first!).
    // Now we render in exact submission order, changing PSO as needed.

    Shader* lastShader = nullptr;
    MaterialPSO* currentPSO = nullptr;

    for (const auto& batch : batches)
    {
        if (batch.IsEmpty() || !batch.shader)
            continue;

        Shader* shader = batch.shader._get();
        if (!shader)
            continue;

        // Only change PSO when shader changes
        if (shader != lastShader)
        {
            currentPSO = m_matCache->GetOrCreateUIPSO(shader, 0, framebuffer);
            if (!currentPSO)
            {
                Msg("! [NVRHIUIRenderer] Failed to get PSO, skipping batch");
                continue;
            }
            lastShader = shader;
        }

        // Render this single batch
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

    // Get native pipeline from PipelineState wrapper
    nvrhi::IGraphicsPipeline* nativePipeline = pso->pso->GetNativePipeline();
    if (!nativePipeline)
        return;

    // ═══════════════════════════════════════════════════════
    //  GET OR CREATE CACHED BINDING SETS (VS + PS)
    // ═══════════════════════════════════════════════════════
    // MaterialCache creates BOTH vsBindingSet and psBindingSet and caches them
    // This automatically includes:
    //   - VS: Constant buffers for vertex shader
    //   - PS: Constant buffers + Textures + Samplers for pixel shader

    // Use m_constantBuffer as the per-object CB for UI rendering
    // (UI shaders may not need per-object CB, but we pass it anyway for consistency)
    m_matCache->GetOrCreateBindingSet(pso, m_constantBuffer, pso->pass);

    // Verify binding sets were created
    if (!pso->vsBindingSet || !pso->psBindingSet) {
        Msg("! [NVRHIUIRenderer] Failed to create binding sets for shader");
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  SET GRAPHICS STATE
    // ═══════════════════════════════════════════════════════

    nvrhi::GraphicsState state;
    state.pipeline = nativePipeline;
    state.framebuffer = framebuffer;
    state.viewport = nvrhi::ViewportState()
        .addViewportAndScissorRect(nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight)));

    // Bind both per-stage binding sets:
    // Slot 0: VS binding set (VS constant buffers)
    // Slot 1: PS binding set (PS constant buffers + textures + samplers)
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

    // ═══════════════════════════════════════════════════════
    //  UPLOAD AND DRAW BATCHES
    // ═══════════════════════════════════════════════════════

    u32 vertexOffset = 0;
    u32 indexOffset = 0;

    for (const UIGeometryBatch* batch : batches)
    {
        if (batch->IsEmpty())
            continue;

        // Update scissor rect if batch has one (fonts use scissor)
        if (batch->hasScissor) {
            nvrhi::Rect scissor;
            scissor.minX = batch->scissorRect.x1;
            scissor.minY = batch->scissorRect.y1;
            scissor.maxX = batch->scissorRect.x2;
            scissor.maxY = batch->scissorRect.y2;

            // Update viewport state with scissor
            state.viewport = nvrhi::ViewportState()
                .addViewport(nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight)))
                .addScissorRect(scissor);

            commandList->setGraphicsState(state);  // Reapply with new scissor
        }

        // Upload geometry
        u32 batchVertexOffset = vertexOffset;
        u32 batchIndexOffset = indexOffset;
        UploadBatchGeometry(commandList, *batch, vertexOffset, indexOffset);

        // Draw
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
    // Create initial buffers (will be resized as needed)
    const size_t initialVertexCount = 4096;
    const size_t initialIndexCount = 8192;

    // Create constant buffer
    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = 256; // Enough for UIConstants
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

    // Create vertex buffer
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

    // Create index buffer
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
    // Resize vertex buffer if needed
    if (vertexCount > m_vertexBufferSize)
    {
        size_t newSize = vertexCount * 2; // Double the size for growth

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

    // Resize index buffer if needed
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

    // Upload vertices
    const size_t vertexDataSize = batch.vertices.size() * sizeof(UIVertex);
    commandList->writeBuffer(m_vertexBuffer, batch.vertices.data(), vertexDataSize,
                            vertexOffset * sizeof(UIVertex));

    // Upload indices
    const size_t indexDataSize = batch.indices.size() * sizeof(u16);
    commandList->writeBuffer(m_indexBuffer, batch.indices.data(), indexDataSize,
                            indexOffset * sizeof(u16));

    vertexOffset += static_cast<u32>(batch.vertices.size());
    indexOffset += static_cast<u32>(batch.indices.size());
}

} // namespace xray::render::ui
