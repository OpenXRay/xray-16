#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::resources {

enum class BufferType : u8 {
    Vertex,
    Index,
    Constant,
    Structured,
    Raw,
};

enum class BufferUsage : u8 {
    Static,
    Dynamic,
    Staging,
};

struct BufferDesc {
    BufferType type = BufferType::Vertex;
    BufferUsage usage = BufferUsage::Static;

    u64 size = 0;
    u32 stride = 0;

    bool cpuAccess = false;
    bool gpuWrite = false;
    bool indirectArgs = false;

    shared_str debugName;

    BufferDesc() = default;

    BufferDesc(BufferType t, u64 s, const char* name = nullptr)
        : type(t), size(s), debugName(name) {}
};

struct BufferMetadata {
    BufferDesc desc;
    u32 generation = 0;
    bool isAlive = true;

    nvrhi::BufferHandle nvrhiBuffer;

    u32 refCount = 0;
    float lastAccessTime = 0.0f;

    u64 memoryUsed = 0;
};

class BufferManager {
public:
    explicit BufferManager(xray::render::fg::RenderDevice* device);
    ~BufferManager();

    BufferHandle CreateBuffer(
        const BufferDesc& desc,
        const void* initialData = nullptr
    );

    void UpdateBuffer(
        BufferHandle handle,
        const void* data,
        u64 size,
        u64 offset = 0
    );

    nvrhi::IBuffer* GetNVRHIBuffer(BufferHandle handle);
    const BufferMetadata* GetMetadata(BufferHandle handle) const;

    void AddRef(BufferHandle handle);
    void Release(BufferHandle handle);
    void DestroyBuffer(BufferHandle handle);

    class Statistics
    {
    public:
        u64 staticMemoryUsed = 0;
        u64 totalMemoryUsed = 0;

        u32 buffersTotal = 0;
        u32 buffersStatic = 0;
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    xray::render::fg::RenderDevice* m_device;

    xr_vector<BufferMetadata> m_buffers;
    xr_vector<u32> m_freeSlots;

    BufferHandle AllocateHandle();
    void FreeHandle(BufferHandle handle);
    bool ValidateHandle(BufferHandle handle) const;

    mutable Statistics m_stats;
};

}
