// xrRender/Bindless/MaterialBuffer.cpp
#include "stdafx.h"
#include "MaterialBuffer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════
//  MATERIAL BUFFER IMPLEMENTATION
// ═══════════════════════════════════════════════════════

MaterialBuffer::MaterialBuffer()
{
    m_materials.resize(MAX_MATERIALS);
}

MaterialBuffer::~MaterialBuffer()
{
    Shutdown();
}

MaterialBuffer& MaterialBuffer::Instance()
{
    static MaterialBuffer instance;
    return instance;
}

void MaterialBuffer::Initialize(ng::RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    // Create GPU buffer for materials
    // NOTE: Do NOT use keepInitialState - buffer needs state transitions for writeBuffer!
    nvrhi::BufferDesc desc;
    desc.debugName = "Bindless_MaterialBuffer";
    desc.byteSize = MAX_MATERIALS * sizeof(MaterialData);
    desc.structStride = sizeof(MaterialData);
    desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

    m_buffer = nvDevice->createBuffer(desc);
    if (!m_buffer) {
        Msg("! [BindlessMaterial] Failed to create material buffer");
        return;
    }

    m_initialized = true;
    m_fullUploadNeeded = true;
    Msg("* [BindlessMaterial] Material buffer initialized (max: %u materials, %u KB)",
        MAX_MATERIALS, (MAX_MATERIALS * sizeof(MaterialData)) / 1024);
}

void MaterialBuffer::Shutdown()
{
    m_buffer = nullptr;
    m_materialCount = 0;
    m_initialized = false;
}

u32 MaterialBuffer::RegisterMaterial(const MaterialData& material)
{
    if (!m_initialized || m_materialCount >= MAX_MATERIALS)
        return UINT32_MAX;

    u32 id = m_materialCount++;
    m_materials[id] = material;

    // Mark dirty
    if (!m_fullUploadNeeded) {
        m_dirtyRangeStart = std::min(m_dirtyRangeStart, id);
        m_dirtyRangeEnd = std::max(m_dirtyRangeEnd, id + 1);
    }

    return id;
}

void MaterialBuffer::UpdateMaterial(u32 materialID, const MaterialData& material)
{
    if (!m_initialized || materialID >= m_materialCount)
        return;

    m_materials[materialID] = material;

    // Mark dirty
    if (!m_fullUploadNeeded) {
        m_dirtyRangeStart = std::min(m_dirtyRangeStart, materialID);
        m_dirtyRangeEnd = std::max(m_dirtyRangeEnd, materialID + 1);
    }
}

const MaterialData* MaterialBuffer::GetMaterial(u32 materialID) const
{
    if (materialID >= m_materialCount)
        return nullptr;
    return &m_materials[materialID];
}

void MaterialBuffer::Upload(ng::RenderContext* ctx)
{
    if (!m_initialized || !m_buffer)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    if (m_fullUploadNeeded && m_materialCount > 0) {
        // Upload all materials with explicit state tracking
        cmdList->beginTrackingBufferState(m_buffer, nvrhi::ResourceStates::CopyDest);
        cmdList->writeBuffer(m_buffer, m_materials.data(), m_materialCount * sizeof(MaterialData));
        cmdList->setBufferState(m_buffer, nvrhi::ResourceStates::ShaderResource);
        m_fullUploadNeeded = false;
        m_dirtyRangeStart = UINT32_MAX;
        m_dirtyRangeEnd = 0;
    } else if (m_dirtyRangeEnd > m_dirtyRangeStart) {
        // Partial update - also needs state tracking
        cmdList->beginTrackingBufferState(m_buffer, nvrhi::ResourceStates::CopyDest);
        u32 offset = m_dirtyRangeStart * sizeof(MaterialData);
        u32 size = (m_dirtyRangeEnd - m_dirtyRangeStart) * sizeof(MaterialData);
        cmdList->writeBuffer(m_buffer, &m_materials[m_dirtyRangeStart], size, offset);
        cmdList->setBufferState(m_buffer, nvrhi::ResourceStates::ShaderResource);
        m_dirtyRangeStart = UINT32_MAX;
        m_dirtyRangeEnd = 0;
    } else {
        // No upload needed, but still need to track state for NVRHI
        // The buffer should be in ShaderResource state from previous upload
        cmdList->beginTrackingBufferState(m_buffer, nvrhi::ResourceStates::ShaderResource);
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW MATERIAL ID BUFFER IMPLEMENTATION
// ═══════════════════════════════════════════════════════

DrawMaterialIDBuffer::DrawMaterialIDBuffer()
{
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

void DrawMaterialIDBuffer::Initialize(ng::RenderDevice* device, u32 maxDraws)
{
    if (m_initialized)
        return;

    m_device = device;
    m_maxDraws = maxDraws;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    // Create GPU buffer for draw material IDs
    // NOTE: Do NOT use keepInitialState - buffer needs state transitions for writeBuffer!
    nvrhi::BufferDesc desc;
    desc.debugName = "Bindless_DrawMaterialIDs";
    desc.byteSize = maxDraws * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

    m_buffer = nvDevice->createBuffer(desc);
    if (!m_buffer) {
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

void DrawMaterialIDBuffer::Upload(ng::RenderContext* ctx, u32 drawCount)
{
    if (!m_initialized || !m_buffer)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    if (drawCount > 0) {
        drawCount = std::min(drawCount, m_maxDraws);
        // Use explicit state tracking for proper buffer transitions
        cmdList->beginTrackingBufferState(m_buffer, nvrhi::ResourceStates::CopyDest);
        cmdList->writeBuffer(m_buffer, m_materialIDs.data(), drawCount * sizeof(u32));
        cmdList->setBufferState(m_buffer, nvrhi::ResourceStates::ShaderResource);
    } else {
        // No upload needed, but still need to track state for NVRHI
        cmdList->beginTrackingBufferState(m_buffer, nvrhi::ResourceStates::ShaderResource);
    }
}

} // namespace xray::render::RENDER_NAMESPACE::bindless
