#pragma once

#include "GPUStructuredBuffer.h"
#include "BindlessTypes.h"

namespace xray::render::fg::bindless {

class TerrainMaterialBuffer : public GPUStructuredBuffer<TerrainMaterialData> {
public:
    static TerrainMaterialBuffer& Instance();

    void Initialize(fg::RenderDevice* device);
    void Shutdown();

    u32 RegisterMaterial(const TerrainMaterialData& material);
    void UpdateMaterial(u32 materialID, const TerrainMaterialData& material);
    const TerrainMaterialData* GetMaterial(u32 materialID) const { return Get(materialID); }

    u32 GetMaterialCount() const { return m_materialCount; }
    u64 GetRevision() const;

private:
    TerrainMaterialBuffer() = default;

    u32 m_materialCount = 0;
    u64 m_revision = 0;
};

} // namespace xray::render::fg::bindless
