// xrRender/Bindless/MaterialBuffer.h
// GPU-side material buffer for bindless rendering
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
//  MATERIAL BUFFER
// ═══════════════════════════════════════════════════════
// Manages GPU-side structured buffer of MaterialData
// Each material has a unique ID used by shaders to index into this buffer

class MaterialBuffer {
public:
    static MaterialBuffer& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Register a material and get its ID
    // Returns UINT32_MAX if buffer is full
    u32 RegisterMaterial(const MaterialData& material);

    // Update an existing material
    void UpdateMaterial(u32 materialID, const MaterialData& material);

    // Get material data by ID
    const MaterialData* GetMaterial(u32 materialID) const;

    // Upload dirty materials to GPU
    // Call once per frame before rendering
    void Upload(ng::RenderContext* ctx);

    // Get the GPU buffer for shader binding
    nvrhi::IBuffer* GetBuffer() const { return m_buffer.Get(); }

    // Get material count
    u32 GetMaterialCount() const { return m_materialCount; }

    bool IsInitialized() const { return m_initialized; }

private:
    MaterialBuffer();
    ~MaterialBuffer();

    ng::RenderDevice* m_device = nullptr;
    nvrhi::BufferHandle m_buffer;
    bool m_initialized = false;

    // CPU-side material data
    xr_vector<MaterialData> m_materials;
    u32 m_materialCount = 0;

    // Dirty tracking for partial updates
    bool m_fullUploadNeeded = true;
    u32 m_dirtyRangeStart = 0;
    u32 m_dirtyRangeEnd = 0;
};

// ═══════════════════════════════════════════════════════
//  DRAW MATERIAL ID BUFFER
// ═══════════════════════════════════════════════════════
// Parallel buffer to draw args - stores material ID for each draw
// Shader reads: materialID = g_DrawMaterialIDs[drawIndex]

class DrawMaterialIDBuffer {
public:
    static DrawMaterialIDBuffer& Instance();

    void Initialize(ng::RenderDevice* device, u32 maxDraws);
    void Shutdown();

    // Set material ID for a draw
    void SetMaterialID(u32 drawIndex, u32 materialID);

    // Upload to GPU (call after all SetMaterialID calls)
    void Upload(ng::RenderContext* ctx, u32 drawCount);

    // Get the GPU buffer for shader binding
    nvrhi::IBuffer* GetBuffer() const { return m_buffer.Get(); }

    bool IsInitialized() const { return m_initialized; }

private:
    DrawMaterialIDBuffer();
    ~DrawMaterialIDBuffer();

    ng::RenderDevice* m_device = nullptr;
    nvrhi::BufferHandle m_buffer;
    bool m_initialized = false;

    xr_vector<u32> m_materialIDs;
    u32 m_maxDraws = 0;
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
