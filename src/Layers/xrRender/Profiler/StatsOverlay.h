#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include "GPUProfiler.h"
#include <nvrhi/nvrhi.h>

namespace xray::profiler
{

struct RenderStats
{
    u32 totalBatches = 0;
    u32 staticBatches = 0;
    u32 dynamicBatches = 0;
    u32 skinnedBatches = 0;
    u32 terrainBatches = 0;
    u32 particleBatches = 0;

    u32 totalTriangles = 0;
    u32 staticTriangles = 0;
    u32 dynamicTriangles = 0;
    u32 skinnedTriangles = 0;
    u32 terrainTriangles = 0;

    u32 megaBufferVertices = 0;
    u32 megaBufferIndices = 0;

    u32 particleCullSubmitted = 0;
    u32 particleCullVisible = 0;
    u32 particleQuadsSubmitted = 0;
    u32 particleQuadsVisible = 0;

    u32 clusterEntries = 0;
    u32 clusterVisible = 0;
    u32 clusterTerrainEntries = 0;
    u32 clusterStaticEntries = 0;
    u32 residualStatic = 0;
    u32 residualTerrain = 0;
    u32 residualDynamic = 0;
    u32 clusterOcclusionCandidates = 0;
    u32 clusterOcclusionRecovered = 0;
    u32 clusterTrianglesDrawn = 0;
    u32 clusterTerrainTrianglesDrawn = 0;
    u32 clusterTerrainVisible = 0;

    bool vsmActive = false;
    bool vsmSunMoving = false;
    u32 vsmPages = 0;
    u32 vsmDirtyPages = 0;
    u32 vsmWrongPages = 0;
    u32 vsmBinDraws = 0;
    u32 vsmBinInstances = 0;
    u32 vsmBinMaxPages = 0;
    u32 vsmBinLodCulled = 0;
    u32 vsmBinDrops = 0;
    u32 vsmLevelPages[6] = {0, 0, 0, 0, 0, 0};

    u32 lightsClustered = 0;
    u32 lightsHiZVisible = 0;
    u32 lightsPoint = 0;
    u32 lightsSpot = 0;
    u32 lightsOmni = 0;
    u32 lightTiles[4] = {0, 0, 0, 0};
    u32 lightTilesTotal = 0;

    u32 detailInstances = 0;
    u32 detailSlots = 0;
    u32 detailTrisPerBlade[3] = {0, 0, 0};

    u32 detailVisibleSlots = 0;
    u32 detailVisibleLOD0 = 0;
    u32 detailVisibleLOD1 = 0;
    u32 detailVisibleLOD2 = 0;
    u32 detailVisibleDecals = 0;

    u32 detailGeneratedInstances = 0;
    u32 detailVisibleCapacity = 0;
    u32 detailDecalCapacity = 0;

    u32 fgArenaUsed = 0;
    u32 fgArenaPeak = 0;
    u32 fgArenaCapacity = 0;
    u32 fgArenaFallbacks = 0;

    void Reset()
    {
        totalBatches = staticBatches = dynamicBatches = skinnedBatches = terrainBatches = particleBatches = 0;
        totalTriangles = staticTriangles = dynamicTriangles = skinnedTriangles = terrainTriangles = 0;
        megaBufferVertices = megaBufferIndices = 0;
        particleCullSubmitted = particleCullVisible = particleQuadsSubmitted = particleQuadsVisible = 0;
        clusterEntries = clusterVisible = clusterTerrainEntries = clusterTerrainVisible = 0;
        clusterStaticEntries = clusterTrianglesDrawn = clusterTerrainTrianglesDrawn = 0;
        residualStatic = residualTerrain = residualDynamic = 0;
        clusterOcclusionCandidates = clusterOcclusionRecovered = 0;
        vsmActive = vsmSunMoving = false;
        vsmPages = vsmDirtyPages = vsmWrongPages = 0;
        vsmBinDraws = vsmBinInstances = vsmBinMaxPages = vsmBinLodCulled = vsmBinDrops = 0;
        for (u32 i = 0; i < 6; ++i) vsmLevelPages[i] = 0;
        lightsClustered = lightsHiZVisible = lightsPoint = lightsSpot = lightsOmni = 0;
        lightTiles[0] = lightTiles[1] = lightTiles[2] = lightTiles[3] = lightTilesTotal = 0;
        detailInstances = detailSlots = 0;
        detailTrisPerBlade[0] = detailTrisPerBlade[1] = detailTrisPerBlade[2] = 0;
        detailVisibleSlots = detailVisibleLOD0 = detailVisibleLOD1 = detailVisibleLOD2 = 0;
        detailVisibleDecals = 0;
        detailGeneratedInstances = detailVisibleCapacity = detailDecalCapacity = 0;
        fgArenaUsed = fgArenaPeak = fgArenaCapacity = fgArenaFallbacks = 0;
    }
};

class StatsOverlay
{
public:
    StatsOverlay();
    ~StatsOverlay();

    void SetGPUProfiler(GPUProfiler* gpuProfiler) { m_gpuProfiler = gpuProfiler; }

    void SetRenderStats(const RenderStats& stats) { m_renderStats = stats; }
    const RenderStats& GetRenderStats() const { return m_renderStats; }

    void Render();

    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    void ToggleVisible() { m_visible = !m_visible; }

    void SetInspectorRTList(const xr_vector<shared_str>& names, const xr_vector<u8>& isDepth) { m_rtNames = names; m_rtIsDepth = isDepth; }
    void SetInspectorPreview(nvrhi::ITexture* tex) { m_inspectorPreview = tex; }
    shared_str GetSelectedRTName() const { return m_selectedRTName; }
    int GetChannelMode() const { return m_channelMode; }
    void SetSelectedRTMipCount(u32 mips) { m_selectedRTMipCount = mips; }
    void SetSelectedRTSize(u32 w, u32 h) { m_sourceWidth = w; m_sourceHeight = h; }
    int GetSelectedMipLevel() const { return m_selectedMipLevel; }

    struct WallmarkSplat {
        float u, v;
        float uvRadius;
        float r, g, b, a;
        u32 mode = 0;
        float seed = 0.f;
        nvrhi::ITexture* stampTex = nullptr;
    };
    struct WallmarkTexGroup { nvrhi::ITexture* diffuseTex = nullptr; xr_string texName; xr_vector<WallmarkSplat> splats; };
    struct WallmarkObjectData { void* objKey = nullptr; xr_vector<WallmarkTexGroup> groups; };
    void SetWallmarkData(xr_vector<WallmarkObjectData> data) { m_wallmarkData = std::move(data); }

private:
    void RenderCPUSection();
    void RenderGPUSection();
    void RenderGPUPassList(const xr_vector<GPUPassTiming>& passTimings, float totalGPU, bool asyncOnly);
    void RenderGeometrySection();
    void RenderAllocationsSection();
    void RenderInspectorSection();
    void RenderWallmarksSection();
    void RenderZoneTree(u32 zoneId, const xr_vector<ZoneData>& zones, float parentTime);

    static const char* FormatTime(float ms, int slot = -1);
    static u32 GetTimeColor(float ms, float parentMs);
    static const char* FormatNumber(u32 value);
    static const char* FormatBytes(u64 bytes, int slot = -1);
    static void AppendZoneText(xr_string& out, u32 zoneId, const xr_vector<ZoneData>& zones, int depth);
    static void CopyZoneTreeToClipboard();

private:
    GPUProfiler* m_gpuProfiler = nullptr;
    RenderStats m_renderStats;
    bool m_visible = false;

    bool m_cpuExpanded = true;
    bool m_gpuExpanded = true;
    bool m_geometryExpanded = true;
    bool m_allocExpanded = true;
    bool m_allocObjectClassProfiling = false;

    xr_vector<shared_str> m_rtNames;
    int m_inspectorSelectedRT = -1;
    nvrhi::ITexture* m_inspectorPreview = nullptr;
    int m_channelMode = 0;
    int m_selectedMipLevel = 0;
    u32 m_selectedRTMipCount = 1;
    u32 m_sourceWidth = 0;
    u32 m_sourceHeight = 0;
    shared_str m_selectedRTName;
    xr_vector<u8> m_rtIsDepth;

    xr_vector<WallmarkObjectData> m_wallmarkData;
    void* m_wallmarkSelectedKey = nullptr;
    int m_wallmarkSelectedGroup = 0;
};

}
