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
    bool indirectArgs = false;

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
    //  STATISTICS
    // ═══════════════════════════════════════════════════

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

    // Static buffers
    xr_vector<BufferMetadata> m_buffers;
    xr_vector<u32> m_freeSlots;

    // Handle management
    BufferHandle AllocateHandle();
    void FreeHandle(BufferHandle handle);
    bool ValidateHandle(BufferHandle handle) const;

    // Statistics
    mutable Statistics m_stats;
};

} // namespace xray::render::resources
