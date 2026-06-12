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

namespace {

using ReflectionKey = std::pair<const void*, const void*>;
xr_map<ReflectionKey, BindingSetBuilder::ReflectedLists> s_reflectedListsCache;

void Collect(BindingSetBuilder::ReflectedLists& lists,
    xr_vector<BindingSetBuilder::ReflectedResource>& samplers,
    const ExtractedReflection& reflection)
{
    for (const auto& tex : reflection.rtBindings.inputTextures) {
        nvrhi::ResourceType rt = nvrhi::ResourceType::Texture_SRV;
        switch (tex.shape) {
        case ResourceShape::StructuredBuffer: rt = nvrhi::ResourceType::StructuredBuffer_SRV; break;
        case ResourceShape::RawBuffer:        rt = nvrhi::ResourceType::RawBuffer_SRV; break;
        case ResourceShape::AccelStruct:      rt = nvrhi::ResourceType::RayTracingAccelStruct; break;
        default: break;
        }
        lists.srvs.push_back({ tex.name.c_str(), tex.slot, rt });
    }

    for (const auto& uav : reflection.rtBindings.uavBindings) {
        nvrhi::ResourceType rt = nvrhi::ResourceType::Texture_UAV;
        switch (uav.shape) {
        case ResourceShape::StructuredBuffer: rt = nvrhi::ResourceType::StructuredBuffer_UAV; break;
        case ResourceShape::RawBuffer:        rt = nvrhi::ResourceType::RawBuffer_UAV; break;
        default: break;
        }
        lists.uavs.push_back({ uav.name.c_str(), uav.slot, rt });
    }

    for (const auto& cb : reflection.constantLayout.constantBuffers.buffers)
        lists.cbs.push_back({ cb.name.c_str(), cb.slot, nvrhi::ResourceType::VolatileConstantBuffer });

    for (const auto& smp : reflection.rtBindings.samplers)
        samplers.push_back({ smp.name.c_str(), smp.slot, nvrhi::ResourceType::Sampler });
}

const BindingSetBuilder::ReflectedLists& GetOrBuildReflectedLists(
    const ExtractedReflection* a, const ExtractedReflection* b, nvrhi::IDevice* device)
{
    const ReflectionKey key{ a, b };
    auto it = s_reflectedListsCache.find(key);
    if (it != s_reflectedListsCache.end())
        return it->second;

    BindingSetBuilder::ReflectedLists lists;
    xr_vector<BindingSetBuilder::ReflectedResource> samplers;
    Collect(lists, samplers, *a);
    if (b) {
        Collect(lists, samplers, *b);
        DeduplicateBySlotAndClass(lists.srvs);
        DeduplicateBySlotAndClass(lists.uavs);
        DeduplicateBySlotAndClass(lists.cbs);
        DeduplicateBySlotAndClass(samplers);
    }

    auto& cache = GetPassResourceCache();
    for (const auto& smp : samplers) {
        nvrhi::ISampler* sampler = cache.GetSamplerByName(smp.name, device);
        if (sampler)
            lists.samplerItems.push_back(nvrhi::BindingSetItem::Sampler(smp.slot, sampler));
    }

    return s_reflectedListsCache.emplace(key, std::move(lists)).first->second;
}

}

void BindingSetBuilder::InvalidateReflectionCache()
{
    s_reflectedListsCache.clear();
}

BindingSetBuilder::BindingSetBuilder(const ExtractedReflection& reflection, nvrhi::IDevice* device,
    const char*)
    : m_lists(&GetOrBuildReflectedLists(&reflection, nullptr, device))
{
    m_desc.bindings.reserve(m_lists->srvs.size() + m_lists->uavs.size()
        + m_lists->cbs.size() + m_lists->samplerItems.size());
}

BindingSetBuilder::BindingSetBuilder(
    const ExtractedReflection& vsReflection,
    const ExtractedReflection& psReflection,
    nvrhi::IDevice* device,
    const char*)
    : m_lists(&GetOrBuildReflectedLists(&vsReflection, &psReflection, device))
{
    m_desc.bindings.reserve(m_lists->srvs.size() + m_lists->uavs.size()
        + m_lists->cbs.size() + m_lists->samplerItems.size());
}

int BindingSetBuilder::FindSRVSlot(const char* name) const
{
    for (const auto& r : m_lists->srvs)
        if (NameMatches(r.name, name)) return static_cast<int>(r.slot);
    Msg("! [BindingSetBuilder] SRV '%s' not found in reflection (have %u SRVs)", name, m_lists->srvs.size());
    for (const auto& r : m_lists->srvs)
        Msg("    SRV: '%s' @ t%u", r.name ? r.name : "(null)", r.slot);
    return -1;
}

int BindingSetBuilder::FindUAVSlot(const char* name) const
{
    for (const auto& r : m_lists->uavs)
        if (NameMatches(r.name, name)) return static_cast<int>(r.slot);
    Msg("! [BindingSetBuilder] UAV '%s' not found in reflection (have %u UAVs)", name, m_lists->uavs.size());
    for (const auto& r : m_lists->uavs)
        Msg("    UAV: '%s' @ u%u", r.name ? r.name : "(null)", r.slot);
    return -1;
}

int BindingSetBuilder::FindCBSlot(const char* name) const
{
    for (const auto& r : m_lists->cbs)
        if (NameMatches(r.name, name)) return static_cast<int>(r.slot);
    Msg("! [BindingSetBuilder] CB '%s' not found in reflection (have %u CBs)", name, m_lists->cbs.size());
    for (const auto& r : m_lists->cbs)
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
        for (const auto& r : m_lists->srvs) {
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
        for (const auto& r : m_lists->uavs) {
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
    for (const auto& item : m_lists->samplerItems)
        m_desc.bindings.push_back(item);
}

static int GetBindingSetRegisterClass(nvrhi::ResourceType type)
{
    switch (type) {
    case nvrhi::ResourceType::Texture_SRV:
    case nvrhi::ResourceType::TypedBuffer_SRV:
    case nvrhi::ResourceType::StructuredBuffer_SRV:
    case nvrhi::ResourceType::RawBuffer_SRV:
    case nvrhi::ResourceType::RayTracingAccelStruct:
        return 0;
    case nvrhi::ResourceType::Texture_UAV:
    case nvrhi::ResourceType::TypedBuffer_UAV:
    case nvrhi::ResourceType::StructuredBuffer_UAV:
    case nvrhi::ResourceType::RawBuffer_UAV:
        return 1;
    case nvrhi::ResourceType::ConstantBuffer:
    case nvrhi::ResourceType::VolatileConstantBuffer:
        return 2;
    case nvrhi::ResourceType::Sampler:
        return 3;
    default:
        return -1;
    }
}

nvrhi::BindingSetDesc BindingSetBuilder::Build()
{
    AddSamplers();

    std::sort(m_desc.bindings.begin(), m_desc.bindings.end(),
        [](const nvrhi::BindingSetItem& a, const nvrhi::BindingSetItem& b) {
            int classA = GetBindingSetRegisterClass(a.type);
            int classB = GetBindingSetRegisterClass(b.type);
            if (classA != classB) return classA < classB;
            return a.slot < b.slot;
        });

    return std::move(m_desc);
}

}
