#pragma once

#include "BindlessTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace ng {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::render::RENDER_NAMESPACE::bindless {

class TerrainMaterialBuffer {
public:
    static TerrainMaterialBuffer& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    u32 RegisterMaterial(const TerrainMaterialData& material);

    void UpdateMaterial(u32 materialID, const TerrainMaterialData& material);

    const TerrainMaterialData* GetMaterial(u32 materialID) const;

    void Upload(ng::RenderContext* ctx);

    nvrhi::IBuffer* GetBuffer() const { return m_buffer.Get(); }

    u32 GetMaterialCount() const { return m_materialCount; }

    bool IsInitialized() const { return m_initialized; }

private:
    TerrainMaterialBuffer();
    ~TerrainMaterialBuffer();

    ng::RenderDevice* m_device = nullptr;
    nvrhi::BufferHandle m_buffer;
    bool m_initialized = false;

    xr_vector<TerrainMaterialData> m_materials;
    u32 m_materialCount = 0;

    bool m_fullUploadNeeded = true;
    u32 m_dirtyRangeStart = 0;
    u32 m_dirtyRangeEnd = 0;
};

}
