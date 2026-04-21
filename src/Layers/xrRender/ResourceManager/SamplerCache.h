#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

// Sampler Cache for Modern ResourceManager
// Week 2 - Day 4: Task 4.3

namespace xray::render::fg {
    class RenderDevice;  // Forward declaration
}

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  SAMPLER DESCRIPTOR
// ═══════════════════════════════════════════════════

struct SamplerDesc {
    enum class Filter {
        Point,
        Linear,
        Anisotropic
    };

    enum class AddressMode {
        Wrap,
        Clamp,
        Mirror,
        Border
    };

    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;
    Filter mipFilter = Filter::Linear;

    AddressMode addressU = AddressMode::Wrap;
    AddressMode addressV = AddressMode::Wrap;
    AddressMode addressW = AddressMode::Wrap;

    float mipLODBias = 0.0f;
    u32 maxAnisotropy = 16;

    float borderColor[4] = {0, 0, 0, 0};

    // Comparison (for shadow maps)
    bool enableComparison = false;

    // Generate hash for deduplication
    u64 GetHash() const;

    bool operator==(const SamplerDesc& other) const;
};

// ═══════════════════════════════════════════════════
//  SAMPLER CACHE
// ═══════════════════════════════════════════════════

class SamplerCache {
public:
    explicit SamplerCache(xray::render::fg::RenderDevice* device);
    ~SamplerCache();

    // Get or create sampler
    SamplerHandle GetSampler(const SamplerDesc& desc);

    // Get NVRHI sampler
    nvrhi::ISampler* GetNVRHISampler(SamplerHandle handle);

    // Common samplers (convenience)
    SamplerHandle GetLinearClamp();
    SamplerHandle GetLinearWrap();
    SamplerHandle GetPointClamp();
    SamplerHandle GetAnisotropicWrap();
    SamplerHandle GetShadowSampler();  // Comparison sampler

    // Statistics
    u32 GetCacheSize() const { return (u32)m_cache.size(); }

private:
    xray::render::fg::RenderDevice* m_device;

    // Hash → Sampler cache
    xr_map<u64, SamplerHandle> m_cache;

    // Handle → NVRHI sampler
    xr_vector<nvrhi::SamplerHandle> m_samplers;

    // Common samplers (cached on first use)
    SamplerHandle m_linearClamp;
    SamplerHandle m_linearWrap;
    SamplerHandle m_pointClamp;
    SamplerHandle m_anisotropicWrap;
    SamplerHandle m_shadowSampler;
};

} // namespace xray::render::resources
