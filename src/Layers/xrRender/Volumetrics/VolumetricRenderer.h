#pragma once

#include "IVolumetricSource.h"
#include "WorldFogSource.h"
#include "LightShaftSource.h"
#include "ParticleEmitterSource.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;

class VolumetricRenderer
{
public:
    static constexpr u32 kFroxelX = 128;
    static constexpr u32 kFroxelY = 72;
    static constexpr u32 kFroxelZ = 16;

    void Initialize(RenderDevice* device);
    void Shutdown();

    void RegisterSource(IVolumetricSource* source);
    void ClearSources();
    void BeginFrame();
    void SwapFroxelHistory();

    WorldFogSource& GetWorldFog() { return m_worldFog; }
    const WorldFogSource& GetWorldFog() const { return m_worldFog; }
    LightShaftSource& GetLightShaft() { return m_lightShaft; }
    ParticleEmitterSource& GetParticleEmitter() { return m_particleEmitter; }
    const xr_vector<IVolumetricSource*>& GetSources() const { return m_sources; }
    nvrhi::ITexture* GetFroxelVolume() const { return m_froxelVolume.Get(); }
    nvrhi::ITexture* GetPrevFroxelVolume() const { return m_prevFroxelVolume.Get(); }
    bool HasFroxelHistory() const { return m_hasHistory && m_prevFroxelVolume; }
    bool IsReady() const { return m_initialized && m_froxelVolume; }

    RenderDevice* GetDevice() const { return m_device; }

private:
    RenderDevice* m_device = nullptr;
    bool m_initialized = false;
    bool m_hasHistory = false;
    nvrhi::TextureHandle m_froxelVolume;
    nvrhi::TextureHandle m_prevFroxelVolume;
    WorldFogSource m_worldFog;
    LightShaftSource m_lightShaft;
    ParticleEmitterSource m_particleEmitter;
    xr_vector<IVolumetricSource*> m_sources;
};

} // namespace xray::render::fg
