#pragma once

#include <nvrhi/nvrhi.h>
#include "xrCore/xrCore.h"

namespace xray::render::fg {

class VolumetricFogManager
{
public:
    static constexpr u32 kWidth = 160;
    static constexpr u32 kHeight = 90;
    static constexpr u32 kDepth = 64;

    static VolumetricFogManager& Instance();

    void Init(nvrhi::IDevice* device);
    void Shutdown();
    void Ensure();

    nvrhi::ITexture* GetDensity() const { return m_density; }
    nvrhi::ITexture* GetLighting() const { return m_lighting; }
    nvrhi::ITexture* GetAccumulated() const { return m_accumulated; }
    nvrhi::ITexture* GetLightingHist() const { return m_lightingHist; }
    bool IsReady() const { return m_density != nullptr && m_lighting != nullptr && m_accumulated != nullptr; }
    void SwapLightingHistory();

private:
    void CreateVolumes();

    nvrhi::DeviceHandle m_device;
    nvrhi::TextureHandle m_density;
    nvrhi::TextureHandle m_lighting;
    nvrhi::TextureHandle m_accumulated;
    nvrhi::TextureHandle m_lightingHist;
};

}
