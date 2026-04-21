#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::resources {

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

    bool enableComparison = false;

    u64 GetHash() const;

    bool operator==(const SamplerDesc& other) const;
};

class SamplerCache {
public:
    explicit SamplerCache(xray::render::fg::RenderDevice* device);
    ~SamplerCache();

    SamplerHandle GetSampler(const SamplerDesc& desc);

    nvrhi::ISampler* GetNVRHISampler(SamplerHandle handle);

    SamplerHandle GetLinearClamp();
    SamplerHandle GetLinearWrap();
    SamplerHandle GetPointClamp();
    SamplerHandle GetAnisotropicWrap();
    SamplerHandle GetShadowSampler();

    u32 GetCacheSize() const { return (u32)m_cache.size(); }

private:
    xray::render::fg::RenderDevice* m_device;

    xr_map<u64, SamplerHandle> m_cache;

    xr_vector<nvrhi::SamplerHandle> m_samplers;

    SamplerHandle m_linearClamp;
    SamplerHandle m_linearWrap;
    SamplerHandle m_pointClamp;
    SamplerHandle m_anisotropicWrap;
    SamplerHandle m_shadowSampler;
};

}
