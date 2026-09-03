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

    UIVertex vert;
    vert.set(x, y, z, C, u, v, UINT32_MAX);
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

    u32 texIdx = UINT32_MAX;
    if (auto* dxShader = static_cast<fgUIShader*>(m_currentUIShader))
        texIdx = dxShader->GetBindlessIndex();
    for (auto& v : m_currentVertices)
        v.texIndex = texIdx;

    UIPrimitiveType uiPrimType = ConvertPrimitiveType(m_primitiveType);

    UIGeometryBatch* batch = GetOrCreateBatch(uiPrimType, m_currentVertices.size());
    VERIFY(batch);

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

UIGeometryBatch* FGUIRender::GetOrCreateBatch(UIPrimitiveType primType, size_t incomingVertexCount)
{
    if (!m_batches.empty())
    {
        UIGeometryBatch& lastBatch = m_batches.back();
        if (lastBatch.CanMergeWith(m_currentUIShader, m_currentAlphaRef, m_hasScissor, m_hasScissor ? &m_scissorRect : nullptr, m_cullMode, primType, incomingVertexCount))
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
    newBatch.primitiveType = primType;
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
}

void FGUIRender::Shutdown()
{
    m_frames.clear();
    m_matCache = nullptr;
    m_device = nullptr;
    m_initialized = false;
}

bool FGUIRender::CreateBuffers()
{
    constexpr size_t kInitialVertices = 4096;
    constexpr size_t kInitialIndices = 8192;

    u32 frameCount = GEnv.Backend ? GEnv.Backend->GetBackBufferCount() : 2u;
    if (frameCount < 2)
        frameCount = 2;

    m_frames.resize(frameCount);
    for (auto& frame : m_frames)
    {
        if (!CreateFrameBuffers(frame, kInitialVertices, kInitialIndices))
            return false;
    }
    return true;
}

bool FGUIRender::CreateFrameBuffers(FrameBuffers& frame, size_t vertexCount, size_t indexCount)
{
    nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = vertexCount * sizeof(UIVertex);
    vbDesc.isVertexBuffer = true;
    vbDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
    vbDesc.debugName = "FGUIRender_VB";
    frame.vertexBuffer = nvrhiDevice->createBuffer(vbDesc);
    frame.vertexCapacity = frame.vertexBuffer ? vertexCount : 0;

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = indexCount * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
    ibDesc.debugName = "FGUIRender_IB";
    frame.indexBuffer = nvrhiDevice->createBuffer(ibDesc);
    frame.indexCapacity = frame.indexBuffer ? indexCount : 0;

    return frame.vertexBuffer && frame.indexBuffer;
}

FGUIRender::FrameBuffers& FGUIRender::AcquireFrameBuffers(size_t vertexCount, size_t indexCount)
{
    if (m_frameStamp != Device.dwFrame)
    {
        m_frameStamp = Device.dwFrame;
        m_frameSlot = (m_frameSlot + 1) % static_cast<u32>(m_frames.size());
        m_frameVertexUsed = 0;
        m_frameIndexUsed = 0;
    }

    FrameBuffers& frame = m_frames[m_frameSlot];
    if (m_frameVertexUsed + vertexCount > frame.vertexCapacity || m_frameIndexUsed + indexCount > frame.indexCapacity)
    {
        CreateFrameBuffers(frame, (m_frameVertexUsed + vertexCount) * 2, (m_frameIndexUsed + indexCount) * 2);
        m_frameVertexUsed = 0;
        m_frameIndexUsed = 0;
    }
    return frame;
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

    const FrameBuffers& frame = m_frames[m_frameSlot];

    nvrhi::GraphicsState state;
    state.pipeline = nativePipeline;
    state.framebuffer = framebuffer;
    state.addBindingSet(pso->vsBindingSet);
    if (pso->psBindingSet)
        state.addBindingSet(pso->psBindingSet);
    if (auto* bindlessTable = GEnv.Backend ? GEnv.Backend->GetBindlessDescriptorTable() : nullptr)
        state.addBindingSet(bindlessTable);

    nvrhi::VertexBufferBinding vbBinding;
    vbBinding.buffer = frame.vertexBuffer;
    vbBinding.slot = 0;
    vbBinding.offset = 0;
    state.addVertexBuffer(vbBinding);

    if (batch.UsesIndexBuffer())
    {
        state.indexBuffer.buffer = frame.indexBuffer;
        state.indexBuffer.format = nvrhi::Format::R16_UINT;
        state.indexBuffer.offset = 0;
    }

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
    drawArgs.instanceCount = 1;
    drawArgs.startVertexLocation = vertexOffset;
    if (batch.UsesIndexBuffer())
    {
        drawArgs.vertexCount = static_cast<u32>(batch.indices.size());
        drawArgs.startIndexLocation = indexOffset;
        cmdList->drawIndexed(drawArgs);
    }
    else
    {
        drawArgs.vertexCount = static_cast<u32>(batch.vertices.size());
        cmdList->draw(drawArgs);
    }
}

void FGUIRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, u32 screenWidth, u32 screenHeight)
{
    if (!m_initialized || m_batches.empty())
        return;

    m_vertexScratch.clear();
    m_indexScratch.clear();
    for (const auto& batch : m_batches)
    {
        if (batch.IsEmpty() || !batch.uiShader)
            continue;
        m_vertexScratch.insert(m_vertexScratch.end(), batch.vertices.begin(), batch.vertices.end());
        if (batch.UsesIndexBuffer())
            m_indexScratch.insert(m_indexScratch.end(), batch.indices.begin(), batch.indices.end());
    }
    if (m_vertexScratch.empty())
        return;

    FrameBuffers& frame = AcquireFrameBuffers(m_vertexScratch.size(), m_indexScratch.size());
    if (!frame.vertexBuffer || !frame.indexBuffer)
        return;

    nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();
    const u32 baseVertex = static_cast<u32>(m_frameVertexUsed);
    const u32 baseIndex = static_cast<u32>(m_frameIndexUsed);

    void* vertexData = nvrhiDevice->mapBuffer(frame.vertexBuffer, nvrhi::CpuAccessMode::Write);
    if (!vertexData)
        return;
    memcpy(static_cast<u8*>(vertexData) + baseVertex * sizeof(UIVertex), m_vertexScratch.data(), m_vertexScratch.size() * sizeof(UIVertex));
    nvrhiDevice->unmapBuffer(frame.vertexBuffer);

    if (!m_indexScratch.empty())
    {
        void* indexData = nvrhiDevice->mapBuffer(frame.indexBuffer, nvrhi::CpuAccessMode::Write);
        if (!indexData)
            return;
        memcpy(static_cast<u8*>(indexData) + baseIndex * sizeof(u16), m_indexScratch.data(), m_indexScratch.size() * sizeof(u16));
        nvrhiDevice->unmapBuffer(frame.indexBuffer);
    }

    m_frameVertexUsed += m_vertexScratch.size();
    m_frameIndexUsed += m_indexScratch.size();

    IUIShader* lastUIShader = nullptr;
    render::MaterialPSO* currentPSO = nullptr;

    u32 vertexOffset = baseVertex;
    u32 indexOffset = baseIndex;

    UIPrimitiveType lastTopology = UIPrimitiveType::TriList;
    for (const auto& batch : m_batches)
    {
        if (batch.IsEmpty() || !batch.uiShader)
            continue;

        fg::PrimitiveTopology psoTopology = fg::PrimitiveTopology::TriangleList;
        switch (batch.primitiveType)
        {
        case UIPrimitiveType::LineList:
            psoTopology = fg::PrimitiveTopology::LineList;
            break;
        case UIPrimitiveType::LineStrip:
            psoTopology = fg::PrimitiveTopology::LineStrip;
            break;
        case UIPrimitiveType::TriList:
            psoTopology = fg::PrimitiveTopology::TriangleList;
            break;
        case UIPrimitiveType::TriStrip:
            psoTopology = fg::PrimitiveTopology::TriangleStrip;
            break;
        }

        const u32 batchVertexOffset = vertexOffset;
        const u32 batchIndexOffset = indexOffset;
        vertexOffset += static_cast<u32>(batch.vertices.size());
        if (batch.UsesIndexBuffer())
            indexOffset += static_cast<u32>(batch.indices.size());

        if (batch.uiShader != lastUIShader || batch.primitiveType != lastTopology)
        {
            currentPSO = m_matCache->GetOrCreateUIPSO(batch.uiShader, 0, framebuffer, psoTopology);
            lastUIShader = batch.uiShader;
            lastTopology = batch.primitiveType;
        }
        if (!currentPSO)
            continue;

        RenderBatchWithShader(cmdList, batch, currentPSO, framebuffer, screenWidth, screenHeight, batchVertexOffset, batchIndexOffset);
    }
}
}
