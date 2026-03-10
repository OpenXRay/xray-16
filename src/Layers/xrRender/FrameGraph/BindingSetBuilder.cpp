#include "stdafx.h"
#include "BindingSetBuilder.h"
#include "ShaderCache.h"
#include "ShaderReflection.h"
#include "PassResourceCache.h"
#include "BindingLayoutBuilder.h"

namespace xray::render::framegraph {

static bool NameMatches(const char* reflName, const char* queryName)
{
    return reflName && queryName && xr_strcmp(reflName, queryName) == 0;
}

void BindingSetBuilder::CollectReflection(const ExtractedReflection& reflection)
{
    for (const auto& tex : reflection.rtBindings.inputTextures) {
        nvrhi::ResourceType rt = nvrhi::ResourceType::Texture_SRV;
        switch (tex.shape) {
        case ResourceShape::StructuredBuffer: rt = nvrhi::ResourceType::StructuredBuffer_SRV; break;
        case ResourceShape::RawBuffer:        rt = nvrhi::ResourceType::RawBuffer_SRV; break;
        case ResourceShape::AccelStruct:      rt = nvrhi::ResourceType::RayTracingAccelStruct; break;
        default: break;
        }
        m_srvs.push_back({ tex.name.c_str(), tex.slot, rt });
    }

    for (const auto& uav : reflection.rtBindings.uavBindings) {
        nvrhi::ResourceType rt = nvrhi::ResourceType::Texture_UAV;
        switch (uav.shape) {
        case ResourceShape::StructuredBuffer: rt = nvrhi::ResourceType::StructuredBuffer_UAV; break;
        case ResourceShape::RawBuffer:        rt = nvrhi::ResourceType::RawBuffer_UAV; break;
        default: break;
        }
        m_uavs.push_back({ uav.name.c_str(), uav.slot, rt });
    }

    for (const auto& cb : reflection.constantLayout.constantBuffers.buffers)
        m_cbs.push_back({ cb.name.c_str(), cb.slot, nvrhi::ResourceType::VolatileConstantBuffer });

    for (const auto& smp : reflection.rtBindings.samplers)
        m_samplers.push_back({ smp.name.c_str(), smp.slot, nvrhi::ResourceType::Sampler });
}

static void DeduplicateBySlotAndClass(xr_vector<BindingSetBuilder::ReflectedResource>& vec)
{
    for (size_t i = 0; i < vec.size(); ++i) {
        for (size_t j = i + 1; j < vec.size(); ) {
            if (vec[i].slot == vec[j].slot)
                vec.erase(vec.begin() + j);
            else
                ++j;
        }
    }
}

BindingSetBuilder::BindingSetBuilder(const ExtractedReflection& reflection, nvrhi::IDevice* device,
    const char*)
    : m_device(device)
{
    CollectReflection(reflection);
}

BindingSetBuilder::BindingSetBuilder(
    const ExtractedReflection& vsReflection,
    const ExtractedReflection& psReflection,
    nvrhi::IDevice* device,
    const char*)
    : m_device(device)
{
    CollectReflection(vsReflection);
    CollectReflection(psReflection);
    DeduplicateBySlotAndClass(m_srvs);
    DeduplicateBySlotAndClass(m_uavs);
    DeduplicateBySlotAndClass(m_cbs);
    DeduplicateBySlotAndClass(m_samplers);
}

int BindingSetBuilder::FindSRVSlot(const char* name) const
{
    for (const auto& r : m_srvs)
        if (NameMatches(r.name, name)) return static_cast<int>(r.slot);
    Msg("! [BindingSetBuilder] SRV '%s' not found in reflection (have %u SRVs)", name, m_srvs.size());
    for (const auto& r : m_srvs)
        Msg("    SRV: '%s' @ t%u", r.name ? r.name : "(null)", r.slot);
    return -1;
}

int BindingSetBuilder::FindUAVSlot(const char* name) const
{
    for (const auto& r : m_uavs)
        if (NameMatches(r.name, name)) return static_cast<int>(r.slot);
    Msg("! [BindingSetBuilder] UAV '%s' not found in reflection (have %u UAVs)", name, m_uavs.size());
    for (const auto& r : m_uavs)
        Msg("    UAV: '%s' @ u%u", r.name ? r.name : "(null)", r.slot);
    return -1;
}

int BindingSetBuilder::FindCBSlot(const char* name) const
{
    for (const auto& r : m_cbs)
        if (NameMatches(r.name, name)) return static_cast<int>(r.slot);
    Msg("! [BindingSetBuilder] CB '%s' not found in reflection (have %u CBs)", name, m_cbs.size());
    for (const auto& r : m_cbs)
        Msg("    CB: '%s' @ b%u", r.name ? r.name : "(null)", r.slot);
    return -1;
}

BindingSetBuilder& BindingSetBuilder::Texture(const char* name, nvrhi::ITexture* texture,
    nvrhi::Format format, nvrhi::TextureSubresourceSet subresources)
{
    int slot = FindSRVSlot(name);
    if (slot >= 0)
        m_desc.bindings.push_back(nvrhi::BindingSetItem::Texture_SRV(slot, texture, format, subresources));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::TextureUAV(const char* name, nvrhi::ITexture* texture,
    nvrhi::Format format, nvrhi::TextureSubresourceSet subresources)
{
    int slot = FindUAVSlot(name);
    if (slot >= 0)
        m_desc.bindings.push_back(nvrhi::BindingSetItem::Texture_UAV(slot, texture, format, subresources));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::BufferSRV(const char* name, nvrhi::IBuffer* buffer)
{
    int slot = FindSRVSlot(name);
    if (slot >= 0) {
        for (const auto& r : m_srvs) {
            if (NameMatches(r.name, name)) {
                if (r.layoutType == nvrhi::ResourceType::RawBuffer_SRV)
                    m_desc.bindings.push_back(nvrhi::BindingSetItem::RawBuffer_SRV(slot, buffer));
                else
                    m_desc.bindings.push_back(nvrhi::BindingSetItem::StructuredBuffer_SRV(slot, buffer));
                break;
            }
        }
    }
    return *this;
}

BindingSetBuilder& BindingSetBuilder::BufferUAV(const char* name, nvrhi::IBuffer* buffer)
{
    int slot = FindUAVSlot(name);
    if (slot >= 0) {
        for (const auto& r : m_uavs) {
            if (NameMatches(r.name, name)) {
                if (r.layoutType == nvrhi::ResourceType::RawBuffer_UAV)
                    m_desc.bindings.push_back(nvrhi::BindingSetItem::RawBuffer_UAV(slot, buffer));
                else
                    m_desc.bindings.push_back(nvrhi::BindingSetItem::StructuredBuffer_UAV(slot, buffer));
                break;
            }
        }
    }
    return *this;
}

BindingSetBuilder& BindingSetBuilder::ConstantBuffer(const char* name, nvrhi::IBuffer* buffer)
{
    int slot = FindCBSlot(name);
    if (slot >= 0)
        m_desc.bindings.push_back(nvrhi::BindingSetItem::ConstantBuffer(slot, buffer));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::AccelStruct(const char* name, nvrhi::rt::IAccelStruct* as)
{
    int slot = FindSRVSlot(name);
    if (slot >= 0)
        m_desc.bindings.push_back(nvrhi::BindingSetItem::RayTracingAccelStruct(slot, as));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::TextureSlot(u32 slot, nvrhi::ITexture* texture,
    nvrhi::Format format, nvrhi::TextureSubresourceSet subresources)
{
    m_desc.bindings.push_back(nvrhi::BindingSetItem::Texture_SRV(slot, texture, format, subresources));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::TextureUAVSlot(u32 slot, nvrhi::ITexture* texture,
    nvrhi::Format format, nvrhi::TextureSubresourceSet subresources)
{
    m_desc.bindings.push_back(nvrhi::BindingSetItem::Texture_UAV(slot, texture, format, subresources));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::BufferSRVSlot(u32 slot, nvrhi::IBuffer* buffer)
{
    m_desc.bindings.push_back(nvrhi::BindingSetItem::StructuredBuffer_SRV(slot, buffer));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::BufferUAVSlot(u32 slot, nvrhi::IBuffer* buffer)
{
    m_desc.bindings.push_back(nvrhi::BindingSetItem::StructuredBuffer_UAV(slot, buffer));
    return *this;
}

BindingSetBuilder& BindingSetBuilder::ConstantBufferSlot(u32 slot, nvrhi::IBuffer* buffer)
{
    m_desc.bindings.push_back(nvrhi::BindingSetItem::ConstantBuffer(slot, buffer));
    return *this;
}

void BindingSetBuilder::AddSamplers()
{
    auto& cache = GetPassResourceCache();
    for (const auto& smp : m_samplers) {
        nvrhi::ISampler* sampler = cache.GetSamplerByName(smp.name, m_device);
        if (sampler)
            m_desc.bindings.push_back(nvrhi::BindingSetItem::Sampler(smp.slot, sampler));
    }
}

nvrhi::BindingSetDesc BindingSetBuilder::Build()
{
    AddSamplers();
    return m_desc;
}

}
