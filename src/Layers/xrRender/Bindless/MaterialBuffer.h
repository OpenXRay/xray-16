#pragma once

#include "GPUStructuredBuffer.h"
#include "BindlessTypes.h"

namespace xray::render::fg::bindless {

class MaterialBuffer : public GPUStructuredBuffer<MaterialData> {
public:
    static MaterialBuffer& Instance();

    void Initialize(fg::RenderDevice* device);
    void Shutdown();

    u32 RegisterMaterial(const MaterialData& material);
    void UpdateMaterial(u32 materialID, const MaterialData& material);
    const MaterialData* GetMaterial(u32 materialID) const { return Get(materialID); }

    u32 GetMaterialCount() const { return m_materialCount; }

    u32 GetShaderVariant(u32 materialID) const {
        const auto* mat = GetMaterial(materialID);
        return mat ? mat->shaderVariant : 0;
    }

private:
    MaterialBuffer() = default;

    u32 m_materialCount = 0;
};

class DrawMaterialIDBuffer {
public:
    static DrawMaterialIDBuffer& Instance();

    void Initialize(fg::RenderDevice* device, u32 maxDraws);
    void Shutdown();

    void SetMaterialID(u32 drawIndex, u32 materialID);
    void Upload(fg::RenderContext* ctx, u32 drawCount);

    nvrhi::IBuffer* GetBuffer() const { return m_buffer.Get(); }
    bool IsInitialized() const { return m_initialized; }

private:
    DrawMaterialIDBuffer() = default;
    ~DrawMaterialIDBuffer();

    fg::RenderDevice* m_device = nullptr;
    nvrhi::BufferHandle m_buffer;
    bool m_initialized = false;

    xr_vector<u32> m_materialIDs;
    u32 m_maxDraws = 0;
};

} // namespace xray::render::fg::bindless
