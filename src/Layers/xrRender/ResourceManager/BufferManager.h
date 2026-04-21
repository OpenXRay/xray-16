#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

// Buffer Manager for Modern ResourceManager
// Week 2 - Day 4: Task 4.1

namespace xray::render::fg {
    class RenderDevice;  // Forward declaration
}

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  BUFFER TYPES
// ═══════════════════════════════════════════════════

enum class BufferType : u8 {
    Vertex,          // Vertex buffer
    Index,           // Index buffer
    Constant,        // Constant buffer (uniform buffer)
    Structured,      // Structured buffer (SSBO)
    Raw,             // Raw buffer (ByteAddressBuffer)
};

enum class BufferUsage : u8 {
    Static,          // Written once, read many (geometry)
    Dynamic,         // Written every frame (per-frame constants)
    Staging,         // CPU-visible for readback
};

// ═══════════════════════════════════════════════════
//  BUFFER DESCRIPTOR
// ═══════════════════════════════════════════════════

struct BufferDesc {
    BufferType type = BufferType::Vertex;
    BufferUsage usage = BufferUsage::Static;

    u64 size = 0;           // Size in bytes
    u32 stride = 0;         // For structured buffers

    // Access flags
    bool cpuAccess = false;    // Can map from CPU?
    bool gpuWrite = false;     // UAV?

    shared_str debugName;

    BufferDesc() = default;

    BufferDesc(BufferType t, u64 s, const char* name = nullptr)
        : type(t), size(s), debugName(name) {}
};

// ═══════════════════════════════════════════════════
//  BUFFER METADATA
// ═══════════════════════════════════════════════════

struct BufferMetadata {
    BufferDesc desc;
    u32 generation = 0;
    bool isAlive = true;

    // Physical resource
    nvrhi::BufferHandle nvrhiBuffer;

    // Usage tracking
    u32 refCount = 0;
    float lastAccessTime = 0.0f;

    // Memory
    u64 memoryUsed = 0;
};

// ═══════════════════════════════════════════════════
//  RING BUFFER (For Dynamic Allocations)
// ═══════════════════════════════════════════════════

class RingBuffer {
public:
    RingBuffer(xray::render::fg::RenderDevice* device, u64 size, const char* debugName);
    ~RingBuffer();

    // Allocate from ring buffer
    struct Allocation {
        nvrhi::BufferHandle buffer;
        u64 offset;
        u64 size;
        void* cpuAddress;  // If mapped
    };

    Allocation Allocate(u64 size, u32 alignment = 256);

    // Advance to next frame (wrap around)
    void AdvanceFrame();

    // Get statistics
    u64 GetSize() const { return m_size; }
    u64 GetUsed() const { return m_used; }
    u64 GetAvailable() const { return m_size - m_used; }

private:
    xray::render::fg::RenderDevice* m_device;
    nvrhi::BufferHandle m_buffer;

    u64 m_size;
    u64 m_head;      // Current write position
    u64 m_used;      // Bytes used this frame

    void* m_cpuAddress;  // Persistent mapping
};

// ═══════════════════════════════════════════════════
//  BUFFER MANAGER
// ═══════════════════════════════════════════════════

class BufferManager {
public:
    explicit BufferManager(xray::render::fg::RenderDevice* device);
    ~BufferManager();

    // ═══════════════════════════════════════════════════
    //  STATIC BUFFERS
    // ═══════════════════════════════════════════════════

    // Create static buffer (geometry, etc.)
    BufferHandle CreateBuffer(
        const BufferDesc& desc,
        const void* initialData = nullptr
    );

    // Update static buffer (rare, expensive)
    void UpdateBuffer(
        BufferHandle handle,
        const void* data,
        u64 size,
        u64 offset = 0
    );

    // ═══════════════════════════════════════════════════
    //  DYNAMIC BUFFERS (Ring Buffer Allocation)
    // ═══════════════════════════════════════════════════

    // Allocate from ring buffer (per-frame data)
    RingBuffer::Allocation AllocateDynamic(u64 size, u32 alignment = 256);

    // Allocate constant buffer data (convenience)
    template<typename T>
    RingBuffer::Allocation AllocateConstants(const T& data) {
        auto alloc = AllocateDynamic(sizeof(T), 256);
        if (alloc.cpuAddress) {
            memcpy(alloc.cpuAddress, &data, sizeof(T));
        }
        return alloc;
    }

    // ═══════════════════════════════════════════════════
    //  ACCESS
    // ═══════════════════════════════════════════════════

    nvrhi::IBuffer* GetNVRHIBuffer(BufferHandle handle);
    const BufferMetadata* GetMetadata(BufferHandle handle) const;

    // ═══════════════════════════════════════════════════
    //  LIFECYCLE
    // ═══════════════════════════════════════════════════

    void AddRef(BufferHandle handle);
    void Release(BufferHandle handle);
    void DestroyBuffer(BufferHandle handle);

    // ═══════════════════════════════════════════════════
    //  FRAME MANAGEMENT
    // ═══════════════════════════════════════════════════

    void BeginFrame();
    void EndFrame();

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

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
    xray::render::fg::RenderDevice* m_device;

    // Static buffers
    xr_vector<BufferMetadata> m_buffers;
    xr_vector<u32> m_freeSlots;

    // Dynamic buffers (ring buffers)
    xr_unique_ptr<RingBuffer> m_constantBufferRing;
    xr_unique_ptr<RingBuffer> m_vertexBufferRing;
    xr_unique_ptr<RingBuffer> m_indexBufferRing;

    // Handle management
    BufferHandle AllocateHandle();
    void FreeHandle(BufferHandle handle);
    bool ValidateHandle(BufferHandle handle) const;

    // Statistics
    mutable Statistics m_stats;
};

} // namespace xray::render::resources
