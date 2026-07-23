#pragma once

#include "IVolumetricSource.h"
#include "WorldFogSource.h"
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
    static constexpr u32 kFroxelX = 160;
    static constexpr u32 kFroxelY = 90;
    static constexpr u32 kFroxelZ = 16;

    void Initialize(RenderDevice* device);
    void Shutdown();

    void RegisterSource(IVolumetricSource* source);
    void ClearSources();
    void BeginFrame(); // refresh WorldFog from Environment
    void SwapFroxelHistory(); // after frame: current → prev

    WorldFogSource& GetWorldFog() { return m_worldFog; }
    const WorldFogSource& GetWorldFog() const { return m_worldFog; }
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
    xr_vector<IVolumetricSource*> m_sources;
};

} // namespace xray::render::fg
