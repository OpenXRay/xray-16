// xrRender/Bindless/TerrainMaterialBuffer.h
// GPU-side terrain material buffer for bindless rendering
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

// ═══════════════════════════════════════════════════════
//  TERRAIN MATERIAL BUFFER
// ═══════════════════════════════════════════════════════
// Manages GPU-side structured buffer of TerrainMaterialData
// Terrain uses 4-layer detail blending with RGBA mask
// Bound at register t9 in shaders

class TerrainMaterialBuffer {
public:
    static TerrainMaterialBuffer& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Register a terrain material and get its ID
    // Returns UINT32_MAX if buffer is full
    u32 RegisterMaterial(const TerrainMaterialData& material);

    // Update an existing terrain material
    void UpdateMaterial(u32 materialID, const TerrainMaterialData& material);

    // Get terrain material data by ID
    const TerrainMaterialData* GetMaterial(u32 materialID) const;

    // Upload dirty materials to GPU
    // Call once per frame before rendering
    void Upload(ng::RenderContext* ctx);

    // Get the GPU buffer for shader binding (register t9)
    nvrhi::IBuffer* GetBuffer() const { return m_buffer.Get(); }

    // Get material count
    u32 GetMaterialCount() const { return m_materialCount; }

    bool IsInitialized() const { return m_initialized; }

private:
    TerrainMaterialBuffer();
    ~TerrainMaterialBuffer();

    ng::RenderDevice* m_device = nullptr;
    nvrhi::BufferHandle m_buffer;
    bool m_initialized = false;

    // CPU-side material data
    xr_vector<TerrainMaterialData> m_materials;
    u32 m_materialCount = 0;

    // Dirty tracking for partial updates
    bool m_fullUploadNeeded = true;
    u32 m_dirtyRangeStart = 0;
    u32 m_dirtyRangeEnd = 0;
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
