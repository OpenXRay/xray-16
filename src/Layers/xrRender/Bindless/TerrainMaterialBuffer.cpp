// xrRender/Bindless/TerrainMaterialBuffer.cpp
#include "stdafx.h"
#include "TerrainMaterialBuffer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════
//  TERRAIN MATERIAL BUFFER IMPLEMENTATION
// ═══════════════════════════════════════════════════════

TerrainMaterialBuffer::TerrainMaterialBuffer()
{
    m_materials.resize(MAX_TERRAIN_MATERIALS);

    // Initialize all materials with INVALID_TEXTURE_INDEX to avoid reading index 0
    // when an unregistered material slot is accessed
    TerrainMaterialData defaultMat = {};
    defaultMat.baseAlbedoIndex = INVALID_TEXTURE_INDEX;
    defaultMat.blendMaskIndex = INVALID_TEXTURE_INDEX;
    defaultMat.detailR_Index = INVALID_TEXTURE_INDEX;
    defaultMat.detailG_Index = INVALID_TEXTURE_INDEX;
    defaultMat.detailB_Index = INVALID_TEXTURE_INDEX;
    defaultMat.detailA_Index = INVALID_TEXTURE_INDEX;
    defaultMat.normalR_Index = INVALID_TEXTURE_INDEX;
    defaultMat.normalG_Index = INVALID_TEXTURE_INDEX;
    defaultMat.normalB_Index = INVALID_TEXTURE_INDEX;
    defaultMat.normalA_Index = INVALID_TEXTURE_INDEX;
    defaultMat.pbrR_Index = INVALID_TEXTURE_INDEX;
    defaultMat.pbrG_Index = INVALID_TEXTURE_INDEX;
    defaultMat.pbrB_Index = INVALID_TEXTURE_INDEX;
    defaultMat.pbrA_Index = INVALID_TEXTURE_INDEX;
    defaultMat.detailScale = 4.0f;
    defaultMat.flags = 0;

    for (auto& mat : m_materials) {
        mat = defaultMat;
    }
}

TerrainMaterialBuffer::~TerrainMaterialBuffer()
{
    Shutdown();
}

TerrainMaterialBuffer& TerrainMaterialBuffer::Instance()
{
    static TerrainMaterialBuffer instance;
    return instance;
}

void TerrainMaterialBuffer::Initialize(ng::RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    // Create GPU buffer for terrain materials (register t9)
    // NOTE: Do NOT use keepInitialState - buffer needs state transitions for writeBuffer!
    nvrhi::BufferDesc desc;
    desc.debugName = "Bindless_TerrainMaterialBuffer";
    desc.byteSize = MAX_TERRAIN_MATERIALS * sizeof(TerrainMaterialData);
    desc.structStride = sizeof(TerrainMaterialData);
    desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

    m_buffer = nvDevice->createBuffer(desc);
    if (!m_buffer) {
        Msg("! [TerrainMaterial] Failed to create terrain material buffer");
        return;
    }

    m_initialized = true;
    m_fullUploadNeeded = true;
    Msg("* [TerrainMaterial] Terrain material buffer initialized (max: %u materials, %u KB)",
        MAX_TERRAIN_MATERIALS, (MAX_TERRAIN_MATERIALS * sizeof(TerrainMaterialData)) / 1024);
}

void TerrainMaterialBuffer::Shutdown()
{
    m_buffer = nullptr;
    m_materialCount = 0;
    m_initialized = false;
}

u32 TerrainMaterialBuffer::RegisterMaterial(const TerrainMaterialData& material)
{
    if (!m_initialized || m_materialCount >= MAX_TERRAIN_MATERIALS)
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

void TerrainMaterialBuffer::UpdateMaterial(u32 materialID, const TerrainMaterialData& material)
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

const TerrainMaterialData* TerrainMaterialBuffer::GetMaterial(u32 materialID) const
{
    if (materialID >= m_materialCount)
        return nullptr;
    return &m_materials[materialID];
}

void TerrainMaterialBuffer::Upload(ng::RenderContext* ctx)
{
    if (!m_initialized || !m_buffer)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    if (m_fullUploadNeeded) {
        // Upload ALL terrain materials (including unused slots with INVALID defaults)
        // This ensures any out-of-range material ID reads INVALID_TEXTURE_INDEX, not garbage
        // Use explicit state tracking for proper buffer transitions
        cmdList->beginTrackingBufferState(m_buffer, nvrhi::ResourceStates::CopyDest);
        cmdList->writeBuffer(m_buffer, m_materials.data(), MAX_TERRAIN_MATERIALS * sizeof(TerrainMaterialData));
        cmdList->setBufferState(m_buffer, nvrhi::ResourceStates::ShaderResource);
        m_fullUploadNeeded = false;
        m_dirtyRangeStart = UINT32_MAX;
        m_dirtyRangeEnd = 0;

        static bool s_logOnce = false;
        if (!s_logOnce) {
            Msg("* [TerrainMaterial] Uploaded %u materials to GPU (buffer size: %u)",
                m_materialCount, MAX_TERRAIN_MATERIALS);
            s_logOnce = true;
        }
    } else if (m_dirtyRangeEnd > m_dirtyRangeStart) {
        // Partial update - also needs state tracking
        cmdList->beginTrackingBufferState(m_buffer, nvrhi::ResourceStates::CopyDest);
        u32 offset = m_dirtyRangeStart * sizeof(TerrainMaterialData);
        u32 size = (m_dirtyRangeEnd - m_dirtyRangeStart) * sizeof(TerrainMaterialData);
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

} // namespace xray::render::RENDER_NAMESPACE::bindless
