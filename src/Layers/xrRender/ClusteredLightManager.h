#pragma once

#include <nvrhi/nvrhi.h>
#include <array>
#include "xrCore/xrCore.h"

namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg
{
class light;
class light_Package;

struct GPULightData {
    Fvector4 positionAndInvRangeSq;
    Fvector4 colorAndRange;
    Fvector4 directionAndSpotScale;
    Fvector4 spotParamsAndType;
    Fmatrix  spotVP;
};
static_assert(sizeof(GPULightData) == 128, "GPULightData must be 128 bytes");

struct alignas(16) ClusterCB {
    Fvector4 gridDims;
    Fvector4 screenSize;
    Fvector4 depthParams;
    Fvector4 pad;
};
static_assert(sizeof(ClusterCB) == 64, "ClusterCB must be 64 bytes");

struct alignas(16) LightHiZCullCB {
    Fmatrix  prevViewProj;
    Fmatrix  curViewProj;
    Fvector4 cameraPos;
    u32      numLights;
    u32      hizWidth;
    u32      hizHeight;
    u32      hizMipLevels;
};
static_assert(sizeof(LightHiZCullCB) == 160, "LightHiZCullCB must be 160 bytes");

static constexpr u32 CLUSTER_TILE_SIZE = 64;
static constexpr u32 CLUSTER_NUM_SLICES = 24;
static constexpr u32 MAX_LIGHTS = 1024;
static constexpr u32 MAX_LIGHT_INDICES = 1024 * 1024;

class ClusteredLightManager {
public:
    static ClusteredLightManager& Instance();

    void Initialize(fg::RenderDevice* device);
    void Shutdown();
    void BeginFrame();
    void CollectLight(const light* L);
    void CollectLightsParallel(const xr_vector<const light*>& lights);
    void BuildLightBuffer(const light_Package& package);
    void Upload(nvrhi::ICommandList* cmdList);
    void UploadAllVisible(nvrhi::ICommandList* cmdList);

    nvrhi::IBuffer* GetLightDataBuffer() const { return m_lightDataBuffer; }
    nvrhi::IBuffer* GetClusterGridBuffer() const { return m_clusterGridBuffer; }
    nvrhi::IBuffer* GetLightIndexListBuffer() const { return m_lightIndexListBuffer; }
    nvrhi::IBuffer* GetLightIndexCounterBuffer() const { return m_lightIndexCounterBuffer; }
    nvrhi::IBuffer* GetVisibleLightIndicesBuffer() const { return m_visibleLightIndicesBuffer; }
    nvrhi::IBuffer* GetVisibleLightCountBuffer() const { return m_visibleLightCountBuffer; }

    u32 GetLightCount() const { return m_numLights; }
    u32 GetPointCount() const { return m_numPoint; }
    u32 GetSpotCount() const { return m_numSpot; }
    u32 GetOmniCount() const { return m_numOmni; }
    u32 GetTilesX() const { return m_tilesX; }
    u32 GetTilesY() const { return m_tilesY; }

    ClusterCB BuildClusterCB(u32 screenWidth, u32 screenHeight, float zNear, float zFar) const;

    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList);
    void ProcessStatsReadback();
    u32 GetVisibleLightCount() const { return m_visibleLightCountCPU; }

    bool IsReady() const { return m_lightDataBuffer != nullptr; }

private:
    void AddLight(const light* L, u32 type);
    GPULightData BuildGPULightData(const light* L);
    u32 GetOrLoadSpotTexture(const shared_str& name);

    nvrhi::DeviceHandle m_device;

    xr_vector<GPULightData> m_lightsCPU;
    std::array<u32, MAX_LIGHTS> m_identityIndices;
    xr_map<shared_str, u32> m_spotTextureCache;
    u32 m_numLights = 0;
    u32 m_numPoint = 0;
    u32 m_numSpot = 0;
    u32 m_numOmni = 0;

    nvrhi::BufferHandle m_lightDataBuffer;
    nvrhi::BufferHandle m_clusterGridBuffer;
    nvrhi::BufferHandle m_lightIndexListBuffer;
    nvrhi::BufferHandle m_lightIndexCounterBuffer;
    nvrhi::BufferHandle m_visibleLightIndicesBuffer;
    nvrhi::BufferHandle m_visibleLightCountBuffer;
    nvrhi::BufferHandle m_statsReadbackBuffer;
    bool m_statsReadbackPending = false;
    u32 m_visibleLightCountCPU = 0;
    u32 m_statsFrameCounter = 0;

    u32 m_tilesX = 0;
    u32 m_tilesY = 0;
};

}
