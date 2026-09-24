#include "stdafx.h"
#include "BindlessTextureRegistry.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::backend {

bool BindlessTextureRegistry::Initialize(nvrhi::IDevice* device, IRenderBackend* backend, u32 capacity)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!device || !backend || capacity < 2)
        return false;

    nvrhi::BindlessLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.firstSlot = 0;
    layoutDesc.maxCapacity = capacity;
    layoutDesc.registerSpaces = { nvrhi::BindingLayoutItem::Texture_SRV(1) };

    nvrhi::BindingLayoutHandle layout = device->createBindlessLayout(layoutDesc);
    if (!layout)
        return false;
    nvrhi::DescriptorTableHandle table = device->createDescriptorTable(layout);
    if (!table)
        return false;
    device->resizeDescriptorTable(table, capacity, false);
    if (!device->writeDescriptorTable(table, nvrhi::BindingSetItem::None(0)))
        return false;

    m_device = device;
    m_backend = backend;
    m_layout = layout;
    m_table = table;
    m_capacity = capacity;
    m_next = 1;
    m_frame = 0;
    m_textures.assign(1, nullptr);
    m_references.assign(1, 0);
    m_indices.clear();
    m_free.clear();
    m_retiring.clear();
    return true;
}

void BindlessTextureRegistry::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_backend)
    {
        for (const RetirementBatch& batch : m_retiring)
        {
            if (batch.lease)
                m_backend->ReleaseSubmissionLease(batch.lease);
        }
    }
    m_retiring.clear();
    m_indices.clear();
    m_textures.clear();
    m_references.clear();
    m_free.clear();
    m_table = nullptr;
    m_layout = nullptr;
    m_backend = nullptr;
    m_device = nullptr;
    m_capacity = 0;
    m_next = 1;
}

u32 BindlessTextureRegistry::Register(nvrhi::ITexture* texture)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_table || !texture)
        return UINT32_MAX;

    const auto found = m_indices.find(texture);
    if (found != m_indices.end())
    {
        u32& references = m_references[found->second];
        R_ASSERT(references != 0 && references != UINT32_MAX);
        ++references;
        return found->second;
    }

    CollectRetiredLocked();

    u32 index;
    if (!m_free.empty())
    {
        index = m_free.back();
        m_free.pop_back();
    }
    else
    {
        if (m_next >= m_capacity)
        {
            Msg("! [Bindless] texture table exhausted (%u slots, %u retiring batches)", m_capacity, u32(m_retiring.size()));
            return UINT32_MAX;
        }
        index = m_next++;
        m_textures.resize(m_next);
        m_references.resize(m_next);
    }

    if (!m_device->writeDescriptorTable(m_table, nvrhi::BindingSetItem::Texture_SRV(index, texture)))
    {
        m_free.push_back(index);
        return UINT32_MAX;
    }

    m_indices.emplace(texture, index);
    m_textures[index] = texture;
    m_references[index] = 1;
    return index;
}

nvrhi::ITexture* BindlessTextureRegistry::Get(u32 index)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_references.size() || m_references[index] == 0)
        return nullptr;
    return m_textures[index].Get();
}

bool BindlessTextureRegistry::Retain(const u32* indices, u32 count)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (u32 i = 0; i < count; ++i)
    {
        if (indices[i] >= m_references.size() || m_references[indices[i]] == 0)
            return false;
        R_ASSERT(m_references[indices[i]] <= UINT32_MAX - count);
    }
    for (u32 i = 0; i < count; ++i)
        ++m_references[indices[i]];
    return true;
}

void BindlessTextureRegistry::Release(const u32* indices, u32 count, bool recording)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_table)
        return;

    RetirementBatch* batch = nullptr;
    for (u32 i = 0; i < count; ++i)
    {
        const u32 index = indices[i];
        R_ASSERT(index != 0 && index < m_references.size() && m_references[index] != 0);
        if (--m_references[index] != 0)
            continue;
        m_indices.erase(m_textures[index].Get());
        if (!batch)
            batch = &AcquireBatchLocked(recording);
        batch->indices.push_back(index);
    }

    if (batch && !recording && batch->lease)
        m_backend->CloseSubmissionLease(batch->lease);
    if (batch)
        CollectRetiredLocked();
}

void BindlessTextureRegistry::BeginFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ++m_frame;
    if (m_table)
        CollectRetiredLocked();
}

nvrhi::IBindingLayout* BindlessTextureRegistry::GetLayout() const
{
    return m_layout.Get();
}

nvrhi::IDescriptorTable* BindlessTextureRegistry::GetTable() const
{
    return m_table.Get();
}

u32 BindlessTextureRegistry::GetCapacity() const
{
    return m_capacity;
}

BindlessTextureRegistry::RetirementBatch& BindlessTextureRegistry::AcquireBatchLocked(bool recording)
{
    if (recording && !m_retiring.empty())
    {
        RetirementBatch& last = m_retiring.back();
        if (last.recording && last.frame == m_frame)
            return last;
    }

    RetirementBatch& batch = m_retiring.emplace_back();
    batch.recording = recording;
    batch.frame = m_frame;
    batch.lease = m_backend->SupportsSubmissionLeases() ? m_backend->OpenSubmissionLease() : 0;
    return batch;
}

void BindlessTextureRegistry::RetireLocked(u32 index)
{
    R_ASSERT2(m_device->writeDescriptorTable(m_table, nvrhi::BindingSetItem::None(index)),
        "[Bindless] texture retirement failed");
    m_textures[index] = nullptr;
    m_free.push_back(index);
}

void BindlessTextureRegistry::CollectRetiredLocked()
{
    for (auto batch = m_retiring.begin(); batch != m_retiring.end();)
    {
        if (batch->lease)
        {
            const auto state = m_backend->PollSubmissionLease(batch->lease);
            if (state == IRenderBackend::SubmissionLeaseState::Open ||
                state == IRenderBackend::SubmissionLeaseState::Pending)
            {
                ++batch;
                continue;
            }
            m_backend->ReleaseSubmissionLease(batch->lease);
        }
        for (u32 index : batch->indices)
            RetireLocked(index);
        batch = m_retiring.erase(batch);
    }
}

}
