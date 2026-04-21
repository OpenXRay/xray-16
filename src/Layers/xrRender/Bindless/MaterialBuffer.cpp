#include "stdafx.h"
#include "MaterialBuffer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

MaterialBuffer& MaterialBuffer::Instance()
{
    static MaterialBuffer instance;
    return instance;
}

void MaterialBuffer::Initialize(fg::RenderDevice* device)
{
    if (IsInitialized())
        return;

    if (!GPUStructuredBuffer::Initialize(device, "Bindless_MaterialBuffer", MAX_MATERIALS))
    {
        Msg("! [BindlessMaterial] Failed to create material buffer");
        return;
    }

    m_fullUploadNeeded = true;
    Msg("* [BindlessMaterial] Material buffer initialized (max: %u materials, %u KB)",
        MAX_MATERIALS, (MAX_MATERIALS * sizeof(MaterialData)) / 1024);
}

void MaterialBuffer::Shutdown()
{
    GPUStructuredBuffer::Shutdown();
    m_materialCount = 0;
}

u32 MaterialBuffer::RegisterMaterial(const MaterialData& material)
{
    if (!IsInitialized() || m_materialCount >= MAX_MATERIALS)
        return UINT32_MAX;

    u32 id = m_materialCount++;
    Set(id, material);
    m_uploadCount = m_materialCount;
    return id;
}

void MaterialBuffer::UpdateMaterial(u32 materialID, const MaterialData& material)
{
    if (!IsInitialized() || materialID >= m_materialCount)
        return;
    Set(materialID, material);
}

DrawMaterialIDBuffer::~DrawMaterialIDBuffer()
{
    Shutdown();
}

DrawMaterialIDBuffer& DrawMaterialIDBuffer::Instance()
{
    static DrawMaterialIDBuffer instance;
    return instance;
}

void DrawMaterialIDBuffer::Initialize(fg::RenderDevice* device, u32 maxDraws)
{
    if (m_initialized)
        return;

    m_device = device;
    m_maxDraws = maxDraws;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    nvrhi::BufferDesc desc;
    desc.debugName = "Bindless_DrawMaterialIDs";
    desc.byteSize = maxDraws * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    m_buffer = nvDevice->createBuffer(desc);
    if (!m_buffer)
    {
        Msg("! [BindlessMaterial] Failed to create draw material ID buffer");
        return;
    }

    m_materialIDs.resize(maxDraws, 0);
    m_initialized = true;
    Msg("* [BindlessMaterial] Draw material ID buffer initialized (max: %u draws)", maxDraws);
}

void DrawMaterialIDBuffer::Shutdown()
{
    m_buffer = nullptr;
    m_materialIDs.clear();
    m_maxDraws = 0;
    m_initialized = false;
}

void DrawMaterialIDBuffer::SetMaterialID(u32 drawIndex, u32 materialID)
{
    if (drawIndex < m_maxDraws)
        m_materialIDs[drawIndex] = materialID;
}

void DrawMaterialIDBuffer::Upload(fg::RenderContext* ctx, u32 drawCount)
{
    if (!m_initialized || !m_buffer)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    if (drawCount > 0)
    {
        drawCount = std::min(drawCount, m_maxDraws);
        cmdList->writeBuffer(m_buffer, m_materialIDs.data(), drawCount * sizeof(u32));
    }
}

} // namespace xray::render::RENDER_NAMESPACE::bindless
