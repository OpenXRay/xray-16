#pragma once

#include <nvrhi/nvrhi.h>
#include "xrCore/xrCore.h"

namespace xray::render::ng { class RenderDevice; }

namespace xray::render::RENDER_NAMESPACE
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

static constexpr u32 CLUSTER_TILE_SIZE = 64;
static constexpr u32 CLUSTER_NUM_SLICES = 24;
static constexpr u32 MAX_LIGHTS = 1024;
static constexpr u32 MAX_LIGHT_INDICES = 1024 * 1024;

class ClusteredLightManager {
public:
    static ClusteredLightManager& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();
    void BuildLightBuffer(const light_Package& package);
    void Upload(nvrhi::ICommandList* cmdList);

    nvrhi::IBuffer* GetLightDataBuffer() const { return m_lightDataBuffer; }
    nvrhi::IBuffer* GetClusterGridBuffer() const { return m_clusterGridBuffer; }
    nvrhi::IBuffer* GetLightIndexListBuffer() const { return m_lightIndexListBuffer; }
    nvrhi::IBuffer* GetLightIndexCounterBuffer() const { return m_lightIndexCounterBuffer; }

    u32 GetLightCount() const { return m_numLights; }
    u32 GetTilesX() const { return m_tilesX; }
    u32 GetTilesY() const { return m_tilesY; }

    ClusterCB BuildClusterCB(u32 screenWidth, u32 screenHeight, float zNear, float zFar) const;

    bool IsReady() const { return m_lightDataBuffer != nullptr; }

private:
    void AddLight(const light* L, u32 type);

    nvrhi::DeviceHandle m_device;

    xr_vector<GPULightData> m_lightsCPU;
    u32 m_numLights = 0;

    nvrhi::BufferHandle m_lightDataBuffer;
    nvrhi::BufferHandle m_clusterGridBuffer;
    nvrhi::BufferHandle m_lightIndexListBuffer;
    nvrhi::BufferHandle m_lightIndexCounterBuffer;

    u32 m_tilesX = 0;
    u32 m_tilesY = 0;
};

}
