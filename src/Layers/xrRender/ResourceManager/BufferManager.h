#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::ng {
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

class RingBuffer {
public:
    RingBuffer(xray::render::ng::RenderDevice* device, u64 size, const char* debugName);
    ~RingBuffer();

    struct Allocation {
        nvrhi::BufferHandle buffer;
        u64 offset;
        u64 size;
        void* cpuAddress;
    };

    Allocation Allocate(u64 size, u32 alignment = 256);

    void AdvanceFrame();

    u64 GetSize() const { return m_size; }
    u64 GetUsed() const { return m_used; }
    u64 GetAvailable() const { return m_size - m_used; }

private:
    xray::render::ng::RenderDevice* m_device;
    nvrhi::BufferHandle m_buffer;

    u64 m_size;
    u64 m_head;
    u64 m_used;

    void* m_cpuAddress;
};

class BufferManager {
public:
    explicit BufferManager(xray::render::ng::RenderDevice* device);
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

    RingBuffer::Allocation AllocateDynamic(u64 size, u32 alignment = 256);

    template<typename T>
    RingBuffer::Allocation AllocateConstants(const T& data) {
        auto alloc = AllocateDynamic(sizeof(T), 256);
        if (alloc.cpuAddress) {
            memcpy(alloc.cpuAddress, &data, sizeof(T));
        }
        return alloc;
    }

    nvrhi::IBuffer* GetNVRHIBuffer(BufferHandle handle);
    const BufferMetadata* GetMetadata(BufferHandle handle) const;

    void AddRef(BufferHandle handle);
    void Release(BufferHandle handle);
    void DestroyBuffer(BufferHandle handle);

    void BeginFrame();
    void EndFrame();

    struct Statistics {
        u64 staticMemoryUsed = 0;
        u64 dynamicMemoryUsed = 0;
        u64 totalMemoryUsed = 0;

        u32 buffersTotal = 0;
        u32 buffersStatic = 0;
        u32 dynamicAllocationsThisFrame = 0;
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    xray::render::ng::RenderDevice* m_device;

    xr_vector<BufferMetadata> m_buffers;
    xr_vector<u32> m_freeSlots;

    xr_unique_ptr<RingBuffer> m_constantBufferRing;
    xr_unique_ptr<RingBuffer> m_vertexBufferRing;
    xr_unique_ptr<RingBuffer> m_indexBufferRing;

    BufferHandle AllocateHandle();
    void FreeHandle(BufferHandle handle);
    bool ValidateHandle(BufferHandle handle) const;

    mutable Statistics m_stats;
};

}
