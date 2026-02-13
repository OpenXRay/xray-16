#include "stdafx.h"
#include "TerrainMaterialBuffer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

TerrainMaterialBuffer& TerrainMaterialBuffer::Instance()
{
    static TerrainMaterialBuffer instance;
    return instance;
}

void TerrainMaterialBuffer::Initialize(ng::RenderDevice* device)
{
    if (IsInitialized())
        return;

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

    m_data.resize(MAX_TERRAIN_MATERIALS);
    for (auto& mat : m_data)
        mat = defaultMat;

    if (!GPUStructuredBuffer::Initialize(device, "Bindless_TerrainMaterialBuffer", MAX_TERRAIN_MATERIALS))
    {
        Msg("! [TerrainMaterial] Failed to create terrain material buffer");
        return;
    }

    m_uploadCount = MAX_TERRAIN_MATERIALS;
    m_fullUploadNeeded = true;
    Msg("* [TerrainMaterial] Terrain material buffer initialized (max: %u materials, %u KB)",
        MAX_TERRAIN_MATERIALS, (MAX_TERRAIN_MATERIALS * sizeof(TerrainMaterialData)) / 1024);
}

void TerrainMaterialBuffer::Shutdown()
{
    GPUStructuredBuffer::Shutdown();
    m_materialCount = 0;
}

u32 TerrainMaterialBuffer::RegisterMaterial(const TerrainMaterialData& material)
{
    if (!IsInitialized() || m_materialCount >= MAX_TERRAIN_MATERIALS)
        return UINT32_MAX;

    u32 id = m_materialCount++;
    Set(id, material);
    return id;
}

void TerrainMaterialBuffer::UpdateMaterial(u32 materialID, const TerrainMaterialData& material)
{
    if (!IsInitialized() || materialID >= m_materialCount)
        return;
    Set(materialID, material);
}

} // namespace xray::render::RENDER_NAMESPACE::bindless
