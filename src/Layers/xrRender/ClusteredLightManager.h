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
    Fvector4 localShadowRect;
};
static_assert(sizeof(GPULightData) == 144, "GPULightData must be 144 bytes");

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

/// One atlas slice for a shadowed spot / OMNIPART face.
struct LocalShadowTile {
    const light* L = nullptr;
    Fmatrix clipVP;
    u32 lightIndex = 0;
    u32 posX = 0;
    u32 posY = 0;
    u32 size = 0;
    u32 page = 0;
    bool needsRedraw = true;
    bool mandatoryRedraw = false;
    bool needsStaticRedraw = true;
    bool needsDynamicRedraw = true;
};

struct alignas(16) GPUShadowData {
    Fvector4 atlasOffsetScale;
    Fmatrix viewProj;
};
static_assert(sizeof(GPUShadowData) == 80, "GPUShadowData must be 80 bytes");

static constexpr u32 CLUSTER_TILE_SIZE = 64;
static constexpr u32 CLUSTER_NUM_SLICES = 24;
static constexpr u32 MAX_LIGHTS = 1024;
static constexpr u32 MAX_LIGHT_INDICES = 1024 * 1024;
static constexpr u32 MAX_LOCAL_SHADOW_TILES = 512;
static constexpr u32 MAX_LOCAL_SHADOW_PAGES = 12;

u32 ClusterTileSize();
u32 LocalShadowAtlasSize();
u32 LocalShadowPageCount();

class ClusteredLightManager {
public:
    static ClusteredLightManager& Instance();

    void Initialize(fg::RenderDevice* device);
    void Shutdown();
    void BeginFrame();
    void CollectLight(const light* L);
    void CollectLightsParallel(const xr_vector<const light*>& lights);
    void BuildLightBuffer(const light_Package& package);
    /// Pick top-N shadowed faces and pack tile index into GPULightData.w (call after collect, before Upload).
    void AssignLocalShadowTiles(const Fvector& cameraPos);
    void RefreshHudSpotXForms();
    void BuildDISampleTable();
    void Upload(nvrhi::ICommandList* cmdList);
    void UploadAllVisible(nvrhi::ICommandList* cmdList);

    nvrhi::IBuffer* GetLightDataBuffer() const { return m_lightDataBuffer; }
    nvrhi::IBuffer* GetShadowDataBuffer() const { return m_shadowDataBuffer; }
    nvrhi::IBuffer* GetClusterGridBuffer() const { return m_clusterGridBuffer; }
    nvrhi::IBuffer* GetLightIndexListBuffer() const { return m_lightIndexListBuffer; }
    nvrhi::IBuffer* GetLightIndexCounterBuffer() const { return m_lightIndexCounterBuffer; }
    nvrhi::IBuffer* GetVisibleLightIndicesBuffer() const { return m_visibleLightIndicesBuffer; }
    nvrhi::IBuffer* GetVisibleLightCountBuffer() const { return m_visibleLightCountBuffer; }
    nvrhi::IBuffer* GetDILightIndicesBuffer() const { return m_diLightIndicesBuffer; }
    nvrhi::IBuffer* GetDILightCDFBuffer() const { return m_diLightCDFBuffer; }
    u32 GetShadowDataCount() const { return static_cast<u32>(m_shadowDataCPU.size()); }
    u32 GetDILightCount() const { return m_diLightCount; }
    float GetDIPowerSum() const { return m_diPowerSum; }

    u32 GetLightCount() const { return m_numLights; }
    u32 GetPointCount() const { return m_numPoint; }
    u32 GetSpotCount() const { return m_numSpot; }
    u32 GetOmniCount() const { return m_numOmni; }
    u32 GetTilesX() const { return m_tilesX; }
    u32 GetTilesY() const { return m_tilesY; }

    const xr_vector<LocalShadowTile>& GetLocalShadowTiles() const { return m_localShadowTiles; }
    u32 GetLocalShadowCandidateCount() const { return m_localShadowCandidates; }
    u32 GetLocalShadowDroppedCount() const { return m_localShadowDropped; }
    u32 GetLocalShadowRedrawCount() const { return m_localShadowRedraw; }

    ClusterCB BuildClusterCB(u32 screenWidth, u32 screenHeight, float zNear, float zFar) const;

    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList);
    void ProcessStatsReadback();
    u32 GetVisibleLightCount() const { return m_visibleLightCountCPU; }

    bool IsReady() const { return m_lightDataBuffer != nullptr; }

    static Fmatrix BuildSpotClipVP(const light* L);

private:
    void AddLight(const light* L, u32 type);
    GPULightData BuildGPULightData(const light* L);
    u32 GetOrLoadSpotTexture(const shared_str& name);

    nvrhi::DeviceHandle m_device;

    xr_vector<GPULightData> m_lightsCPU;
    xr_vector<GPUShadowData> m_shadowDataCPU;
    xr_vector<const light*> m_lightSources;
    xr_vector<const light*> m_expandedLights;
    xr_vector<LocalShadowTile> m_localShadowTiles;
    struct LocalShadowSticky
    {
        u32 page = 0;
        u32 posX = 0;
        u32 posY = 0;
        u32 size = 0;
        u32 missFrames = 0;
        u32 shrinkHold = 0;
        Fvector lightPos{};
        Fvector lightDir{};
        Fmatrix lastClipVP{};
        bool hasClipVP = false;
        float fade = 0.f;
    };
    xr_map<const light*, LocalShadowSticky> m_localShadowSticky;
    xr_map<const light*, LocalShadowSticky> m_localShadowStickyScratch;
    xr_map<const light*, u32> m_localShadowCandidateIndex;
    xr_vector<u8> m_localShadowKept;
    u32 m_localShadowCandidates = 0;
    u32 m_localShadowDropped = 0;
    u32 m_localShadowRedraw = 0;
    u32 m_localShadowRefreshCursor = 0;
    std::array<u32, MAX_LIGHTS> m_identityIndices;
    xr_map<shared_str, u32> m_spotTextureCache;
    u32 m_numLights = 0;
    u32 m_numPoint = 0;
    u32 m_numSpot = 0;
    u32 m_numOmni = 0;

    nvrhi::BufferHandle m_lightDataBuffer;
    nvrhi::BufferHandle m_shadowDataBuffer;
    nvrhi::BufferHandle m_clusterGridBuffer;
    nvrhi::BufferHandle m_lightIndexListBuffer;
    nvrhi::BufferHandle m_lightIndexCounterBuffer;
    nvrhi::BufferHandle m_visibleLightIndicesBuffer;
    nvrhi::BufferHandle m_visibleLightCountBuffer;
    nvrhi::BufferHandle m_diLightIndicesBuffer;
    nvrhi::BufferHandle m_diLightCDFBuffer;
    xr_vector<u32> m_diIndicesCPU;
    xr_vector<float> m_diCDFCPU;
    u32 m_diLightCount = 0;
    float m_diPowerSum = 0.f;
    static constexpr u32 STATS_READBACK_SLOTS = 6;
    nvrhi::BufferHandle m_statsReadbackBuffers[STATS_READBACK_SLOTS];
    u32 m_statsWriteSlot = 0;
    u32 m_statsScheduled = 0;
    u32 m_visibleLightCountCPU = 0;
    u32 m_statsFrameCounter = 0;

    u32 m_tilesX = 0;
    u32 m_tilesY = 0;
};

}
