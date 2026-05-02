#include "stdafx.h"

#include "fgUIRender.h"

#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/fgUIShader.h"
#include "Layers/xrRender/HWCaps.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg
{
using ui::UIGeometryBatch;
using ui::UIPrimitiveType;
using ui::UIVertex;

FGUIRender::FGUIRender()
{
    m_xformWorld.identity();
}

FGUIRender::~FGUIRender()
{
    Shutdown();
}

void FGUIRender::SetShader(IUIShader& shader)
{
    m_currentUIShader = &shader;
}

void FGUIRender::SetAlphaRef(int aref)
{
    m_currentAlphaRef = aref;
}

void FGUIRender::SetScissor(Irect* rect)
{
    if (rect)
    {
        m_hasScissor = true;
        m_scissorRect = *rect;
    }
    else
    {
        m_hasScissor = false;
    }
}

void FGUIRender::PushPoint(float x, float y, float z, u32 C, float u, float v)
{
    VERIFY(m_primitiveType != ptNone);
    VERIFY(m_currentVertices.size() < m_maxVerts);

    u32 texIdx = UINT32_MAX;
    if (auto* dxShader = static_cast<fgUIShader*>(m_currentUIShader))
        texIdx = dxShader->GetBindlessIndex();

    UIVertex vert;
    vert.set(x, y, z, C, u, v, texIdx);
    m_currentVertices.push_back(vert);
}

void FGUIRender::StartPrimitive(u32 iMaxVerts, ePrimitiveType primType, ePointType pointType)
{
    VERIFY(m_primitiveType == ptNone);
    VERIFY(m_pointType == pttNone);

    m_maxVerts = iMaxVerts;
    m_primitiveType = primType;
    m_pointType = pointType;
    m_currentVertices.clear();
    m_currentVertices.reserve(iMaxVerts);
}

void FGUIRender::FlushPrimitive()
{
    if (m_currentVertices.empty())
    {
        m_primitiveType = ptNone;
        m_pointType = pttNone;
        return;
    }

    UIGeometryBatch* batch = GetOrCreateBatch();
    VERIFY(batch);

    UIPrimitiveType uiPrimType = ConvertPrimitiveType(m_primitiveType);
    batch->AddPrimitive(m_currentVertices, uiPrimType);

    m_currentVertices.clear();
    m_primitiveType = ptNone;
    m_pointType = pttNone;
}

LPCSTR FGUIRender::UpdateShaderName(LPCSTR tex_name, LPCSTR sh_name)
{
    string_path buff;
    const auto& caps = GEnv.Backend->GetCapabilities();
    u32 v_dev = CAP_VERSION(caps.raster_major, caps.raster_minor);
    u32 v_need = CAP_VERSION(2, 0);

    if ((v_dev >= v_need) && FS.exist(buff, "$game_textures$", tex_name, ".ogm"))
        return "hud" DELIMITER "movie";
    return sh_name;
}

void FGUIRender::CacheSetXformWorld(const Fmatrix& M)
{
    m_xformWorld = M;
}

void FGUIRender::CacheSetCullMode(CullMode mode)
{
    m_cullMode = static_cast<int>(mode);
}

void FGUIRender::Clear()
{
    m_batches.clear();
    m_currentVertices.clear();
    m_primitiveType = ptNone;
    m_pointType = pttNone;
    m_currentAlphaRef = 0;
    m_hasScissor = false;
    m_cullMode = 0;
    m_xformWorld.identity();
}

UIPrimitiveType FGUIRender::ConvertPrimitiveType(ePrimitiveType primType)
{
    switch (primType)
    {
    case ptTriList:   return UIPrimitiveType::TriList;
    case ptTriStrip:  return UIPrimitiveType::TriStrip;
    case ptLineStrip: return UIPrimitiveType::LineStrip;
    case ptLineList:  return UIPrimitiveType::LineList;
    default:          VERIFY(!"Unknown primitive type"); return UIPrimitiveType::TriList;
    }
}

UIGeometryBatch* FGUIRender::GetOrCreateBatch()
{
    if (!m_batches.empty())
    {
        UIGeometryBatch& lastBatch = m_batches.back();
        if (lastBatch.CanMergeWith(m_currentUIShader, m_currentAlphaRef, m_hasScissor, m_hasScissor ? &m_scissorRect : nullptr, m_cullMode))
        {
            return &lastBatch;
        }
    }

    m_batches.emplace_back();
    UIGeometryBatch& newBatch = m_batches.back();
    newBatch.uiShader = m_currentUIShader;
    newBatch.alphaRef = m_currentAlphaRef;
    newBatch.hasScissor = m_hasScissor;
    if (m_hasScissor)
        newBatch.scissorRect = m_scissorRect;
    newBatch.xformWorld = m_xformWorld;
    newBatch.cullMode = m_cullMode;
    return &newBatch;
}

void FGUIRender::Initialize(RenderDevice* device, render::MaterialCache* matCache)
{
    R_ASSERT(device);
    R_ASSERT(matCache);

    m_device = device;
    m_matCache = matCache;

    R_ASSERT2(CreateBuffers(), "FGUIRender: failed to create initial buffers");

    m_initialized = true;
    Msg("* [FGUIRender] Initialized");
}

void FGUIRender::Shutdown()
{
    m_constantBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_vertexBuffer = nullptr;
    m_matCache = nullptr;
    m_device = nullptr;
    m_initialized = false;
}

bool FGUIRender::CreateBuffers()
{
    constexpr size_t kInitialVertices = 4096;
    constexpr size_t kInitialIndices = 8192;

    nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = 256;
    cbDesc.isConstantBuffer = true;
    cbDesc.debugName = "FGUIRender_CB";
    cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
    cbDesc.keepInitialState = true;
    m_constantBuffer = nvrhiDevice->createBuffer(cbDesc);
    if (!m_constantBuffer)
        return false;

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = kInitialVertices * sizeof(UIVertex);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "FGUIRender_VB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    m_vertexBuffer = nvrhiDevice->createBuffer(vbDesc);
    if (!m_vertexBuffer)
        return false;
    m_vertexBufferSize = kInitialVertices;

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = kInitialIndices * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "FGUIRender_IB";
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    m_indexBuffer = nvrhiDevice->createBuffer(ibDesc);
    if (!m_indexBuffer)
        return false;
    m_indexBufferSize = kInitialIndices;

    return true;
}

void FGUIRender::EnsureBufferCapacity(size_t vertexCount, size_t indexCount)
{
    nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();

    if (vertexCount > m_vertexBufferSize)
    {
        const size_t newSize = vertexCount * 2;
        nvrhi::BufferDesc vbDesc;
        vbDesc.byteSize = newSize * sizeof(UIVertex);
        vbDesc.isVertexBuffer = true;
        vbDesc.debugName = "FGUIRender_VB";
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        vbDesc.keepInitialState = true;
        m_vertexBuffer = nvrhiDevice->createBuffer(vbDesc);
        m_vertexBufferSize = newSize;
    }
    if (indexCount > m_indexBufferSize)
    {
        const size_t newSize = indexCount * 2;
        nvrhi::BufferDesc ibDesc;
        ibDesc.byteSize = newSize * sizeof(u16);
        ibDesc.isIndexBuffer = true;
        ibDesc.debugName = "FGUIRender_IB";
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
        ibDesc.keepInitialState = true;
        m_indexBuffer = nvrhiDevice->createBuffer(ibDesc);
        m_indexBufferSize = newSize;
    }
}

void FGUIRender::UploadBatchGeometry(nvrhi::ICommandList* cmdList, const UIGeometryBatch& batch, u32& vertexOffset, u32& indexOffset)
{
    if (batch.vertices.empty() || batch.indices.empty())
        return;

    const size_t vertexDataSize = batch.vertices.size() * sizeof(UIVertex);
    cmdList->writeBuffer(m_vertexBuffer, batch.vertices.data(), vertexDataSize, vertexOffset * sizeof(UIVertex));

    const size_t indexDataSize = batch.indices.size() * sizeof(u16);
    cmdList->writeBuffer(m_indexBuffer, batch.indices.data(), indexDataSize, indexOffset * sizeof(u16));

    vertexOffset += static_cast<u32>(batch.vertices.size());
    indexOffset += static_cast<u32>(batch.indices.size());
}

void FGUIRender::RenderBatchWithShader(nvrhi::ICommandList* cmdList, const UIGeometryBatch& batch, render::MaterialPSO* pso, nvrhi::IFramebuffer* framebuffer,
    u32 screenWidth, u32 screenHeight, u32 vertexOffset, u32 indexOffset)
{
    if (!pso || !pso->pso)
        return;

    nvrhi::IGraphicsPipeline* nativePipeline = pso->pso->GetNativePipeline();
    if (!nativePipeline)
        return;

    m_matCache->GetOrCreateBindingSet(pso);
    if (!pso->vsBindingSet)
        return;

    cmdList->setBufferState(m_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_indexBuffer, nvrhi::ResourceStates::IndexBuffer);

    nvrhi::GraphicsState state;
    state.pipeline = nativePipeline;
    state.framebuffer = framebuffer;
    state.addBindingSet(pso->vsBindingSet);
    if (pso->psBindingSet)
        state.addBindingSet(pso->psBindingSet);
    if (auto* bindlessTable = GEnv.Backend ? GEnv.Backend->GetBindlessDescriptorTable() : nullptr)
        state.addBindingSet(bindlessTable);

    nvrhi::VertexBufferBinding vbBinding;
    vbBinding.buffer = m_vertexBuffer;
    vbBinding.slot = 0;
    vbBinding.offset = 0;
    state.addVertexBuffer(vbBinding);

    state.indexBuffer.buffer = m_indexBuffer;
    state.indexBuffer.format = nvrhi::Format::R16_UINT;
    state.indexBuffer.offset = 0;

    if (batch.hasScissor)
    {
        nvrhi::Rect scissor;
        scissor.minX = batch.scissorRect.x1;
        scissor.minY = batch.scissorRect.y1;
        scissor.maxX = batch.scissorRect.x2;
        scissor.maxY = batch.scissorRect.y2;
        state.viewport =
            nvrhi::ViewportState().addViewport(nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight))).addScissorRect(scissor);
    }
    else
    {
        state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight)));
    }

    cmdList->setGraphicsState(state);

    nvrhi::DrawArguments drawArgs;
    drawArgs.vertexCount = static_cast<u32>(batch.indices.size());
    drawArgs.instanceCount = 1;
    drawArgs.startIndexLocation = indexOffset;
    drawArgs.startVertexLocation = vertexOffset;
    cmdList->drawIndexed(drawArgs);
}

void FGUIRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, u32 screenWidth, u32 screenHeight)
{
    if (!m_initialized || m_batches.empty())
        return;

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (const auto& batch : m_batches)
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
    cmdList->writeBuffer(m_constantBuffer, cbData, 256);

    IUIShader* lastUIShader = nullptr;
    render::MaterialPSO* currentPSO = nullptr;

    u32 vertexOffset = 0;
    u32 indexOffset = 0;

    for (const auto& batch : m_batches)
    {
        if (batch.IsEmpty() || !batch.uiShader)
            continue;

        if (batch.uiShader != lastUIShader)
        {
            currentPSO = m_matCache->GetOrCreateUIPSO(batch.uiShader, 0, framebuffer);
            if (!currentPSO)
                continue;
            lastUIShader = batch.uiShader;
        }

        const u32 batchVertexOffset = vertexOffset;
        const u32 batchIndexOffset = indexOffset;
        UploadBatchGeometry(cmdList, batch, vertexOffset, indexOffset);
        RenderBatchWithShader(cmdList, batch, currentPSO, framebuffer, screenWidth, screenHeight, batchVertexOffset, batchIndexOffset);
    }
}
} // namespace xray::render::fg
