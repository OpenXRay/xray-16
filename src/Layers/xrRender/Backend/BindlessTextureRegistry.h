#pragma once

#include "xrCore/xrCore.h"
#include "xrCommon/xr_map.h"
#include "xrCommon/xr_vector.h"

#include <mutex>
#include <nvrhi/nvrhi.h>

class IRenderBackend;

namespace xray::render::backend {

class BindlessTextureRegistry
{
public:
    bool Initialize(nvrhi::IDevice* device, IRenderBackend* backend, u32 capacity);
    void Shutdown();

    u32 Register(nvrhi::ITexture* texture);
    nvrhi::ITexture* Get(u32 index);
    bool Retain(const u32* indices, u32 count);
    void Release(const u32* indices, u32 count, bool recording);
    void BeginFrame();

    nvrhi::IBindingLayout* GetLayout() const;
    nvrhi::IDescriptorTable* GetTable() const;
    u32 GetCapacity() const;

private:
    class RetirementBatch
    {
    public:
        u64 lease = 0;
        u64 frame = 0;
        bool recording = false;
        xr_vector<u32> indices;
    };

    RetirementBatch& AcquireBatchLocked(bool recording);
    void RetireLocked(u32 index);
    void CollectRetiredLocked();

    std::mutex m_mutex;
    nvrhi::IDevice* m_device = nullptr;
    IRenderBackend* m_backend = nullptr;
    nvrhi::BindingLayoutHandle m_layout;
    nvrhi::DescriptorTableHandle m_table;
    xr_map<nvrhi::ITexture*, u32> m_indices;
    xr_vector<nvrhi::TextureHandle> m_textures;
    xr_vector<u32> m_references;
    xr_vector<u32> m_free;
    xr_vector<RetirementBatch> m_retiring;
    u64 m_frame = 0;
    u32 m_capacity = 0;
    u32 m_next = 1;
};

}
