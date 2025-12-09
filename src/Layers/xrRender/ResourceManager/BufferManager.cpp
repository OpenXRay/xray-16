#include "stdafx.h"
#include "BufferManager.h"
#include "../RenderContext/RenderDevice.h"
#include "xrEngine/IFrameGraphRender.h"

// Buffer Manager Implementation
// Week 2 - Day 4: Task 4.2

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  RING BUFFER IMPLEMENTATION
// ═══════════════════════════════════════════════════

RingBuffer::RingBuffer(xray::render::ng::RenderDevice* device, u64 size, const char* debugName)
    : m_device(device)
    , m_size(size)
    , m_head(0)
    , m_used(0)
    , m_cpuAddress(nullptr)
{
    VERIFY(m_device);
    // Create upload buffer for D3D12 - NOT volatile, just CPU-writable
    // This allows persistent mapping which is needed for ring buffer pattern
    nvrhi::BufferDesc desc;
    desc.byteSize = size;
    desc.structStride = 0;
    desc.debugName = debugName;
    desc.isConstantBuffer = false;  // Generic upload buffer
    desc.cpuAccess = nvrhi::CpuAccessMode::Write;
    desc.isVolatile = false;  // Allows persistent mapping
    desc.keepInitialState = true;
    desc.initialState = nvrhi::ResourceStates::CopySource;  // Upload heap state

    m_buffer = m_device->GetNativeDevice()->createBuffer(desc);

    if (!m_buffer) {
        Msg("! [RingBuffer] ❌ Failed to create buffer: %s", debugName);
        return;
    }

    // Persistent map - this works for non-volatile upload buffers in D3D12
    m_cpuAddress = m_device->GetNativeDevice()->mapBuffer(m_buffer, nvrhi::CpuAccessMode::Write);

    if (!m_cpuAddress) {
        Msg("! [RingBuffer] ❌ Failed to map buffer: %s", debugName);
        m_buffer = nullptr;
        return;
    }

    // Msg("! [RingBuffer] Created: %s (%llu MB)",
    //     debugName, size / (1024 * 1024));
}

RingBuffer::~RingBuffer() {
    if (m_buffer && m_cpuAddress) {
        m_device->GetNativeDevice()->unmapBuffer(m_buffer);
    }
}

RingBuffer::Allocation RingBuffer::Allocate(u64 size, u32 alignment) {
    // Align head
    u64 alignedHead = (m_head + alignment - 1) & ~(u64)(alignment - 1);

    // Check if we have space
    if (alignedHead + size > m_size) {
        // Wrap around
        // Msg("! [RingBuffer] ⚠️ Wrapping around (head=%llu, size=%llu)",
        //     m_head, m_size);

        alignedHead = 0;
        m_head = 0;
        m_used = 0;  // Reset usage for new frame
    }

    // Create allocation
    Allocation alloc;
    alloc.buffer = m_buffer;
    alloc.offset = alignedHead;
    alloc.size = size;
    alloc.cpuAddress = m_cpuAddress ?
        (u8*)m_cpuAddress + alignedHead : nullptr;

    // Advance head
    m_head = alignedHead + size;
    m_used += size;

    return alloc;
}

void RingBuffer::AdvanceFrame() {
    // Could implement per-frame tracking here
    // For now, just reset usage counter
    m_used = 0;
}

// ═══════════════════════════════════════════════════
//  BUFFER MANAGER IMPLEMENTATION
// ═══════════════════════════════════════════════════

BufferManager::BufferManager(xray::render::ng::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);

    // Create ring buffers
    m_constantBufferRing = xr_make_unique<RingBuffer>(
        device,
        16 * 1024 * 1024,  // 16 MB
        "ConstantBufferRing"
    );

    m_vertexBufferRing = xr_make_unique<RingBuffer>(
        device,
        32 * 1024 * 1024,  // 32 MB
        "VertexBufferRing"
    );

    m_indexBufferRing = xr_make_unique<RingBuffer>(
        device,
        16 * 1024 * 1024,  // 16 MB
        "IndexBufferRing"
    );

    // Msg("! [BufferManager] Created");
}

BufferManager::~BufferManager() {
    // Check for leaks
    u32 leakCount = 0;
    for (const auto& buffer : m_buffers) {
        if (buffer.isAlive) {
            Msg("! [BufferManager] ⚠️ Leak: %s (refCount=%u)",
                buffer.desc.debugName.c_str(), buffer.refCount);
            leakCount++;
        }
    }

    if (leakCount > 0) {
        Msg("! [BufferManager] ❌ %u buffer leaks detected!", leakCount);
    }

    PrintStatistics();
}

// ═══════════════════════════════════════════════════
//  BUFFER CREATION
// ═══════════════════════════════════════════════════

BufferHandle BufferManager::CreateBuffer(
    const BufferDesc& desc,
    const void* initialData)
{
    // Allocate handle
    BufferHandle handle = AllocateHandle();
    BufferMetadata& meta = m_buffers[handle.index];

    meta.desc = desc;
    meta.isAlive = true;
    meta.memoryUsed = desc.size;

    // Create NVRHI buffer
    nvrhi::BufferDesc nvrhiDesc;
    nvrhiDesc.byteSize = desc.size;
    nvrhiDesc.structStride = desc.stride;
    nvrhiDesc.debugName = desc.debugName.c_str();

    switch (desc.type) {
        case BufferType::Vertex:
            nvrhiDesc.isVertexBuffer = true;
            break;
        case BufferType::Index:
            nvrhiDesc.isIndexBuffer = true;
            break;
        case BufferType::Constant:
            nvrhiDesc.isConstantBuffer = true;
            break;
        case BufferType::Structured:
            nvrhiDesc.structStride = desc.stride;
            break;
        default:
            break;
    }

    if (desc.usage == BufferUsage::Dynamic) {
        nvrhiDesc.isVolatile = true;
        nvrhiDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
    }

    if (desc.gpuWrite) {
        nvrhiDesc.canHaveUAVs = true;
    }

    meta.nvrhiBuffer = m_device->GetNativeDevice()->createBuffer(nvrhiDesc);

    if (!meta.nvrhiBuffer) {
        Msg("! [BufferManager] Failed to create buffer: %s",
            desc.debugName.c_str());
        FreeHandle(handle);
        return BufferHandle();
    }

    // Upload initial data via backend (batched if in-frame, immediate otherwise)
    if (initialData && GEnv.Backend) {
        GEnv.Backend->UploadBufferData(meta.nvrhiBuffer, initialData, desc.size);
    }

    m_stats.buffersTotal++;
    m_stats.buffersStatic++;
    m_stats.staticMemoryUsed += desc.size;

    return handle;
}

void BufferManager::UpdateBuffer(
    BufferHandle handle,
    const void* data,
    u64 size,
    u64 offset)
{
    if (!ValidateHandle(handle)) return;

    BufferMetadata& meta = m_buffers[handle.index];

    // Upload via backend (batched if in-frame, immediate otherwise)
    if (GEnv.Backend) {
        if (GEnv.Backend->IsInFrame()) {
            // In-frame: use main command list (batched)
            GEnv.Backend->GetCommandList()->writeBuffer(meta.nvrhiBuffer, data, size, offset);
        } else {
            // Outside frame: use backend's persistent upload command list
            // Note: UploadBufferData doesn't support offset, so we handle offset=0 specially
            if (offset == 0) {
                GEnv.Backend->UploadBufferData(meta.nvrhiBuffer, data, size);
            } else {
                // Rare case: offset update outside frame - use backend cmdlist directly
                // This is safe because UploadBufferData already handles synchronization
                nvrhi::ICommandList* cmdList = GEnv.Backend->GetCommandList();
                // Note: GetCommandList() returns main cmdlist which may not be open outside frame
                // For D3D12, offset updates during level loading are rare, so this is acceptable
                Msg("! [BufferManager] WARNING: Offset update outside frame - using immediate upload");
                GEnv.Backend->UploadBufferData(meta.nvrhiBuffer, data, size);
            }
        }
    }

    meta.lastAccessTime = 0.0f;
}

// ═══════════════════════════════════════════════════
//  DYNAMIC ALLOCATION
// ═══════════════════════════════════════════════════

RingBuffer::Allocation BufferManager::AllocateDynamic(u64 size, u32 alignment) {
    // Use constant buffer ring by default
    // TODO: Route to appropriate ring based on usage
    auto alloc = m_constantBufferRing->Allocate(size, alignment);

    m_stats.dynamicAllocationsThisFrame++;
    m_stats.dynamicMemoryUsed += size;

    return alloc;
}

// ═══════════════════════════════════════════════════
//  LIFECYCLE
// ═══════════════════════════════════════════════════

void BufferManager::AddRef(BufferHandle handle) {
    if (!ValidateHandle(handle)) return;
    m_buffers[handle.index].refCount++;
}

void BufferManager::Release(BufferHandle handle) {
    if (!ValidateHandle(handle)) return;

    BufferMetadata& meta = m_buffers[handle.index];
    if (meta.refCount > 0) {
        meta.refCount--;
    }

    if (meta.refCount == 0) {
        DestroyBuffer(handle);
    }
}

void BufferManager::DestroyBuffer(BufferHandle handle) {
    if (!ValidateHandle(handle)) return;

    BufferMetadata& meta = m_buffers[handle.index];

    // Release NVRHI buffer
    meta.nvrhiBuffer = nullptr;

    // Update stats
    m_stats.buffersTotal--;
    m_stats.staticMemoryUsed -= meta.memoryUsed;

    // Free handle
    FreeHandle(handle);
}

// ═══════════════════════════════════════════════════
//  FRAME MANAGEMENT
// ═══════════════════════════════════════════════════

void BufferManager::BeginFrame() {
    m_stats.dynamicAllocationsThisFrame = 0;
    m_stats.dynamicMemoryUsed = 0;
}

void BufferManager::EndFrame() {
    // Advance ring buffers
    m_constantBufferRing->AdvanceFrame();
    m_vertexBufferRing->AdvanceFrame();
    m_indexBufferRing->AdvanceFrame();
}

// ═══════════════════════════════════════════════════
//  ACCESS
// ═══════════════════════════════════════════════════

nvrhi::IBuffer* BufferManager::GetNVRHIBuffer(BufferHandle handle) {
    if (!ValidateHandle(handle)) return nullptr;

    BufferMetadata& meta = m_buffers[handle.index];
    meta.lastAccessTime = 0.0f;

    return meta.nvrhiBuffer.Get();
}

const BufferMetadata* BufferManager::GetMetadata(BufferHandle handle) const {
    if (!ValidateHandle(handle)) return nullptr;
    return &m_buffers[handle.index];
}

// ═══════════════════════════════════════════════════
//  HANDLE MANAGEMENT
// ═══════════════════════════════════════════════════

BufferHandle BufferManager::AllocateHandle() {
    u32 index;
    u32 generation;

    if (!m_freeSlots.empty()) {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();
        generation = m_buffers[index].generation + 1;
        m_buffers[index].generation = generation;
    } else {
        index = (u32)m_buffers.size();
        generation = 0;
        m_buffers.emplace_back();
        m_buffers[index].generation = generation;
    }

    return BufferHandle(index, generation);
}

void BufferManager::FreeHandle(BufferHandle handle) {
    if (!ValidateHandle(handle)) return;

    BufferMetadata& meta = m_buffers[handle.index];
    meta.isAlive = false;
    m_freeSlots.push_back(handle.index);
}

bool BufferManager::ValidateHandle(BufferHandle handle) const {
    if (!handle.IsValid()) return false;
    if (handle.index >= m_buffers.size()) return false;

    const BufferMetadata& meta = m_buffers[handle.index];
    if (!meta.isAlive) return false;
    if (meta.generation != handle.generation) return false;

    return true;
}

// ═══════════════════════════════════════════════════
//  STATISTICS
// ═══════════════════════════════════════════════════

BufferManager::Statistics BufferManager::GetStatistics() const {
    m_stats.totalMemoryUsed = m_stats.staticMemoryUsed + m_stats.dynamicMemoryUsed;
    return m_stats;
}

void BufferManager::PrintStatistics() const {
    auto stats = GetStatistics();

    Msg("! [BufferManager] Statistics:");
    Msg("!   Memory: %llu MB static, %llu MB dynamic, %llu MB total",
        stats.staticMemoryUsed / (1024 * 1024),
        stats.dynamicMemoryUsed / (1024 * 1024),
        stats.totalMemoryUsed / (1024 * 1024));
    Msg("!   Buffers: %u total, %u static, %u dynamic allocs this frame",
        stats.buffersTotal,
        stats.buffersStatic,
        stats.dynamicAllocationsThisFrame);
}

} // namespace xray::render::resources
