#include "stdafx.h"
#include "VolumetricRenderer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg
{

namespace
{
nvrhi::TextureHandle CreateFroxelTex(nvrhi::IDevice* nv, const char* name)
{
    nvrhi::TextureDesc desc;
    desc.debugName = name;
    desc.dimension = nvrhi::TextureDimension::Texture3D;
    desc.width = VolumetricRenderer::kFroxelX;
    desc.height = VolumetricRenderer::kFroxelY;
    desc.depth = VolumetricRenderer::kFroxelZ;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.mipLevels = 1;
    desc.isUAV = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    return nv->createTexture(desc);
}
} // namespace

void VolumetricRenderer::Initialize(RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    nvrhi::IDevice* nv = device ? device->GetNVRHIDevice() : nullptr;
    if (!nv)
    {
        Msg("! [VolumetricRenderer] No NVRHI device");
        return;
    }

    m_froxelVolume = CreateFroxelTex(nv, "FroxelVolume");
    m_prevFroxelVolume = CreateFroxelTex(nv, "FroxelVolume_Prev");
    if (!m_froxelVolume)
    {
        Msg("! [VolumetricRenderer] Failed to create froxel volume");
        return;
    }

    m_initialized = true;
    m_hasHistory = false;
    Msg("* [VolumetricRenderer] Initialized froxel volume %ux%ux%u (+temporal)",
        kFroxelX, kFroxelY, kFroxelZ);
}

void VolumetricRenderer::Shutdown()
{
    m_sources.clear();
    m_froxelVolume = nullptr;
    m_prevFroxelVolume = nullptr;
    m_device = nullptr;
    m_initialized = false;
    m_hasHistory = false;
}

void VolumetricRenderer::RegisterSource(IVolumetricSource* source)
{
    if (!source)
        return;
    for (auto* s : m_sources)
    {
        if (s == source)
            return;
    }
    m_sources.push_back(source);
}

void VolumetricRenderer::ClearSources()
{
    m_sources.clear();
}

void VolumetricRenderer::BeginFrame()
{
    ClearSources();
    RegisterSource(&m_worldFog);
    m_lightShaft.PrepareGPUData();
    if (m_lightShaft.GetIntensity() > 1e-4f)
        RegisterSource(&m_lightShaft);
    RegisterSource(&m_particleEmitter);

    for (auto* s : m_sources)
    {
        if (s)
            s->PrepareGPUData();
    }
}

void VolumetricRenderer::SwapFroxelHistory()
{
    m_hasHistory = (m_froxelVolume != nullptr && m_prevFroxelVolume != nullptr);
}

} // namespace xray::render::fg
