#include "stdafx.h"
#include "SamplerCache.h"
#include "../RenderContext/RenderDevice.h"

// Sampler Cache Implementation
// Week 2 - Day 4: Task 4.3

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  SAMPLER DESCRIPTOR HASH
// ═══════════════════════════════════════════════════

u64 SamplerDesc::GetHash() const {
    // Simple hash combining all fields
    u64 hash = 0;
    hash = (hash * 31) + (u64)minFilter;
    hash = (hash * 31) + (u64)magFilter;
    hash = (hash * 31) + (u64)mipFilter;
    hash = (hash * 31) + (u64)addressU;
    hash = (hash * 31) + (u64)addressV;
    hash = (hash * 31) + (u64)addressW;
    hash = (hash * 31) + (u64)(mipLODBias * 1000.0f);
    hash = (hash * 31) + maxAnisotropy;
    hash = (hash * 31) + (enableComparison ? 1 : 0);
    return hash;
}

bool SamplerDesc::operator==(const SamplerDesc& other) const {
    return GetHash() == other.GetHash();
}

// ═══════════════════════════════════════════════════
//  SAMPLER CACHE IMPLEMENTATION
// ═══════════════════════════════════════════════════

SamplerCache::SamplerCache(xray::render::ng::RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);
    Msg("! [SamplerCache] Created");
}

SamplerCache::~SamplerCache() {
    Msg("! [SamplerCache] Destroyed (cached %u samplers)", GetCacheSize());
}

SamplerHandle SamplerCache::GetSampler(const SamplerDesc& desc) {
    u64 hash = desc.GetHash();

    // Check cache
    auto it = m_cache.find(hash);
    if (it != m_cache.end()) {
        return it->second;
    }

    // Create new sampler
    nvrhi::SamplerDesc nvrhiDesc;

    // Convert filter
    auto convertFilter = [](SamplerDesc::Filter f) {
        switch (f) {
            case SamplerDesc::Filter::Point: return false;
            case SamplerDesc::Filter::Linear: return true;
            case SamplerDesc::Filter::Anisotropic: return true;
            default: return true;
        }
    };

    nvrhiDesc.minFilter = convertFilter(desc.minFilter);
    nvrhiDesc.magFilter = convertFilter(desc.magFilter);
    nvrhiDesc.mipFilter = convertFilter(desc.mipFilter);

    // Convert address mode
    auto convertAddress = [](SamplerDesc::AddressMode a) -> nvrhi::SamplerAddressMode {
        switch (a) {
            case SamplerDesc::AddressMode::Wrap:
                return nvrhi::SamplerAddressMode::Wrap;
            case SamplerDesc::AddressMode::Clamp:
                return nvrhi::SamplerAddressMode::Clamp;
            case SamplerDesc::AddressMode::Mirror:
                return nvrhi::SamplerAddressMode::Mirror;
            case SamplerDesc::AddressMode::Border:
                return nvrhi::SamplerAddressMode::Border;
            default:
                return nvrhi::SamplerAddressMode::Clamp;
        }
    };

    nvrhiDesc.addressU = convertAddress(desc.addressU);
    nvrhiDesc.addressV = convertAddress(desc.addressV);
    nvrhiDesc.addressW = convertAddress(desc.addressW);

    nvrhiDesc.mipBias = desc.mipLODBias;
    nvrhiDesc.maxAnisotropy = (desc.minFilter == SamplerDesc::Filter::Anisotropic) ?
        (float)desc.maxAnisotropy : 1.0f;

    nvrhiDesc.borderColor = nvrhi::Color(
        desc.borderColor[0],
        desc.borderColor[1],
        desc.borderColor[2],
        desc.borderColor[3]
    );

    // Create sampler
    nvrhi::SamplerHandle nvrhiSampler =
        m_device->GetNativeDevice()->createSampler(nvrhiDesc);

    if (!nvrhiSampler) {
        Msg("! [SamplerCache] ❌ Failed to create sampler");
        return SamplerHandle();
    }

    // Store
    u32 index = (u32)m_samplers.size();
    m_samplers.push_back(nvrhiSampler);

    SamplerHandle handle(index, 0);
    m_cache[hash] = handle;

    Msg("! [SamplerCache] Created sampler (hash=0x%llX, index=%u)", hash, index);

    return handle;
}

nvrhi::ISampler* SamplerCache::GetNVRHISampler(SamplerHandle handle) {
    if (!handle.IsValid() || handle.index >= m_samplers.size()) {
        return nullptr;
    }

    return m_samplers[handle.index].Get();
}

// ═══════════════════════════════════════════════════
//  COMMON SAMPLERS
// ═══════════════════════════════════════════════════

SamplerHandle SamplerCache::GetLinearClamp() {
    if (!m_linearClamp.IsValid()) {
        SamplerDesc desc;
        desc.minFilter = SamplerDesc::Filter::Linear;
        desc.magFilter = SamplerDesc::Filter::Linear;
        desc.addressU = SamplerDesc::AddressMode::Clamp;
        desc.addressV = SamplerDesc::AddressMode::Clamp;
        m_linearClamp = GetSampler(desc);
    }
    return m_linearClamp;
}

SamplerHandle SamplerCache::GetLinearWrap() {
    if (!m_linearWrap.IsValid()) {
        SamplerDesc desc;
        desc.minFilter = SamplerDesc::Filter::Linear;
        desc.magFilter = SamplerDesc::Filter::Linear;
        desc.addressU = SamplerDesc::AddressMode::Wrap;
        desc.addressV = SamplerDesc::AddressMode::Wrap;
        m_linearWrap = GetSampler(desc);
    }
    return m_linearWrap;
}

SamplerHandle SamplerCache::GetPointClamp() {
    if (!m_pointClamp.IsValid()) {
        SamplerDesc desc;
        desc.minFilter = SamplerDesc::Filter::Point;
        desc.magFilter = SamplerDesc::Filter::Point;
        desc.addressU = SamplerDesc::AddressMode::Clamp;
        desc.addressV = SamplerDesc::AddressMode::Clamp;
        m_pointClamp = GetSampler(desc);
    }
    return m_pointClamp;
}

SamplerHandle SamplerCache::GetAnisotropicWrap() {
    if (!m_anisotropicWrap.IsValid()) {
        SamplerDesc desc;
        desc.minFilter = SamplerDesc::Filter::Anisotropic;
        desc.magFilter = SamplerDesc::Filter::Anisotropic;
        desc.maxAnisotropy = 16;
        desc.addressU = SamplerDesc::AddressMode::Wrap;
        desc.addressV = SamplerDesc::AddressMode::Wrap;
        m_anisotropicWrap = GetSampler(desc);
    }
    return m_anisotropicWrap;
}

SamplerHandle SamplerCache::GetShadowSampler() {
    if (!m_shadowSampler.IsValid()) {
        SamplerDesc desc;
        desc.minFilter = SamplerDesc::Filter::Linear;
        desc.magFilter = SamplerDesc::Filter::Linear;
        desc.addressU = SamplerDesc::AddressMode::Clamp;
        desc.addressV = SamplerDesc::AddressMode::Clamp;
        desc.enableComparison = true;
        m_shadowSampler = GetSampler(desc);
    }
    return m_shadowSampler;
}

} // namespace xray::render::resources
