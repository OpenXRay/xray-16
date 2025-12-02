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

class MaterialBuffer {
public:
    static MaterialBuffer& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    u32 RegisterMaterial(const MaterialData& material);

    void UpdateMaterial(u32 materialID, const MaterialData& material);

    const MaterialData* GetMaterial(u32 materialID) const;

    void Upload(ng::RenderContext* ctx);

    nvrhi::IBuffer* GetBuffer() const { return m_buffer.Get(); }

    u32 GetMaterialCount() const { return m_materialCount; }

    bool IsInitialized() const { return m_initialized; }

private:
    MaterialBuffer();
    ~MaterialBuffer();

    ng::RenderDevice* m_device = nullptr;
    nvrhi::BufferHandle m_buffer;
    bool m_initialized = false;

    xr_vector<MaterialData> m_materials;
    u32 m_materialCount = 0;

    bool m_fullUploadNeeded = true;
    u32 m_dirtyRangeStart = 0;
    u32 m_dirtyRangeEnd = 0;
};

class DrawMaterialIDBuffer {
public:
    static DrawMaterialIDBuffer& Instance();

    void Initialize(ng::RenderDevice* device, u32 maxDraws);
    void Shutdown();

    void SetMaterialID(u32 drawIndex, u32 materialID);

    void Upload(ng::RenderContext* ctx, u32 drawCount);

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

}
