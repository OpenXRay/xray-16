#include "stdafx.h"
#include "VolumetricFogManager.h"
#include <utility>

namespace xray::render::fg {

VolumetricFogManager& VolumetricFogManager::Instance()
{
    static VolumetricFogManager instance;
    return instance;
}

void VolumetricFogManager::Init(nvrhi::IDevice* device)
{
    m_device = device;
    Ensure();
}

void VolumetricFogManager::Shutdown()
{
    m_density = nullptr;
    m_lighting = nullptr;
    m_accumulated = nullptr;
    m_lightingHist = nullptr;
    if (m_device) {
        m_device->waitForIdle();
        m_device->runGarbageCollection();
    }
    m_device = nullptr;
}

void VolumetricFogManager::Ensure()
{
    if (!m_device)
        return;
    if (m_density && m_lighting && m_accumulated && m_lightingHist)
        return;
    CreateVolumes();
}

void VolumetricFogManager::CreateVolumes()
{
    auto makeVol = [&](const char* name) {
        nvrhi::TextureDesc desc;
        desc.debugName = name;
        desc.width = kWidth;
        desc.height = kHeight;
        desc.depth = kDepth;
        desc.dimension = nvrhi::TextureDimension::Texture3D;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        return m_device->createTexture(desc);
    };
    m_density = makeVol("VolFog_Density");
    m_lighting = makeVol("VolFog_Lighting");
    m_accumulated = makeVol("VolFog_Accum");
    m_lightingHist = makeVol("VolFog_LightingHist");
    Msg("* [VolFog] Persistent volumes %ux%ux%u RGBA16F", kWidth, kHeight, kDepth);
}

void VolumetricFogManager::SwapLightingHistory()
{
    std::swap(m_lighting, m_lightingHist);
}

}
