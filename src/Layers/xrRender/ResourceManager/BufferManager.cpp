#include "stdafx.h"
#include "BufferManager.h"
#include "../RenderContext/RenderDevice.h"

namespace xray::render::resources {

BufferManager::BufferManager(xray::render::fg::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);
}

BufferManager::~BufferManager() {
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

BufferHandle BufferManager::CreateBuffer(
    const BufferDesc& desc,
    const void* initialData)
{
    BufferHandle handle = AllocateHandle();
    BufferMetadata& meta = m_buffers[handle.index];

    meta.desc = desc;
    meta.isAlive = true;
    meta.memoryUsed = desc.size;

    nvrhi::BufferDesc nvrhiDesc;
    nvrhiDesc.byteSize = desc.size;
    nvrhiDesc.structStride = desc.stride;
    nvrhiDesc.debugName = desc.debugName.c_str();
    nvrhiDesc.initialState = nvrhi::ResourceStates::Common;
    nvrhiDesc.keepInitialState = true;

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
        if (desc.type == BufferType::Constant)
            nvrhiDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    }

    if (desc.gpuWrite) {
        nvrhiDesc.canHaveUAVs = true;
    }

    if (desc.indirectArgs) {
        nvrhiDesc.isDrawIndirectArgs = true;
    }

    meta.nvrhiBuffer = m_device->GetNativeDevice()->createBuffer(nvrhiDesc);

    if (!meta.nvrhiBuffer) {
        Msg("! [BufferManager] Failed to create buffer: %s",
            desc.debugName.c_str());
        FreeHandle(handle);
        return BufferHandle();
    }

    if (initialData && GEnv.Backend) {
        GEnv.Backend->UploadBufferData(meta.nvrhiBuffer, initialData, desc.size, 0);
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

    if (GEnv.Backend) {
        if (GEnv.Backend->IsInFrame())
            GEnv.Backend->GetCommandList()->writeBuffer(meta.nvrhiBuffer, data, size, offset);
        else
            GEnv.Backend->UploadBufferData(meta.nvrhiBuffer, data, size, offset);
    }

    meta.lastAccessTime = 0.0f;
}

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

    meta.nvrhiBuffer = nullptr;

    m_stats.buffersTotal--;
    m_stats.staticMemoryUsed -= meta.memoryUsed;

    FreeHandle(handle);
}

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

BufferManager::Statistics BufferManager::GetStatistics() const
{
    m_stats.totalMemoryUsed = m_stats.staticMemoryUsed;
    return m_stats;
}

void BufferManager::PrintStatistics() const
{
    auto stats = GetStatistics();

    Msg("! [BufferManager] Statistics:");
    Msg("!   Memory: %llu MB static, %llu MB total",
        stats.staticMemoryUsed / (1024 * 1024),
        stats.totalMemoryUsed / (1024 * 1024));
    Msg("!   Buffers: %u total, %u static",
        stats.buffersTotal,
        stats.buffersStatic);
}

}
