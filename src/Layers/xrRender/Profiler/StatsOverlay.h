#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include "GPUProfiler.h"
#include <nvrhi/nvrhi.h>

namespace xray::profiler
{

// ═══════════════════════════════════════════════════════
//  RENDER STATISTICS
// ═══════════════════════════════════════════════════════
// Collected per-frame from geometry collector and GPU culling

struct RenderStats
{
    // Geometry counts
    u32 totalBatches = 0;        // Total draw batches
    u32 staticBatches = 0;       // Static geometry batches
    u32 dynamicBatches = 0;      // Dynamic geometry batches
    u32 skinnedBatches = 0;      // Skinned mesh batches
    u32 terrainBatches = 0;      // Terrain batches
    u32 particleBatches = 0;     // Particle system batches

    // Triangle counts
    u32 totalTriangles = 0;      // Total triangles submitted
    u32 staticTriangles = 0;     // Static geometry triangles
    u32 dynamicTriangles = 0;    // Dynamic geometry triangles
    u32 skinnedTriangles = 0;    // Skinned mesh triangles
    u32 terrainTriangles = 0;    // Terrain triangles

    u32 megaBufferVertices = 0;
    u32 megaBufferIndices = 0;

    // Skinned culling stats (from GPU Hi-Z culling)

    // Particle culling stats (from GPU Hi-Z culling)
    u32 particleCullSubmitted = 0;  // World particle batches submitted for culling
    u32 particleCullVisible = 0;    // World particle batches that passed culling
    u32 particleQuadsSubmitted = 0; // Total particle quads submitted
    u32 particleQuadsVisible = 0;   // Particle quads in visible batches

    // Cluster LOD stats
    u32 clusterEntries = 0;         // Total cluster entries in the entry table
    u32 clusterVisible = 0;         // Entries drawn last frame (post cut + Hi-Z)
    u32 clusterTerrainEntries = 0;  // Terrain share of the entry table
    u32 clusterStaticEntries = 0;
    u32 residualStatic = 0;
    u32 residualTerrain = 0;
    u32 residualDynamic = 0;
    u32 residualTransparent = 0;
    u32 clusterOcclusionCandidates = 0;
    u32 clusterOcclusionRecovered = 0;
    u32 clusterTrianglesDrawn = 0;
    u32 clusterTerrainTrianglesDrawn = 0;
    u32 clusterTerrainVisible = 0;  // Terrain entries drawn last frame
    u32 clusterInstanceVisits = 0;
    u32 clusterNodeVisits = 0;
    u32 clusterLeafVisits = 0;
    u32 clusterDeferredInstances = 0;
    u32 clusterDeferredNodes = 0;
    u32 clusterOverflow = 0;
    u64 geometrySharedBytes = 0;
    u64 geometryInstanceBytes = 0;
    u64 geometryPayloadBytes = 0;
    u64 geometryVertexBytes = 0;
    u64 geometryRetainedBytes = 0;
    u64 geometryForwardDrawBytes = 0;
    u64 geometryRetiringSourceBytes = 0;
    u64 geometrySourceStagingBytes = 0;
    u64 geometryHostSourceBytes = 0;
    u64 geometryRetiringArenaBytes = 0;
    u64 geometryShadowSnapshotBytes = 0;
    u64 geometryShadowHostBytes = 0;
    u64 geometryRTSourceBytes = 0;
    u64 geometryRTGenerationBytes = 0;
    u64 geometryRTAccelerationBytes = 0;
    u32 geometryForwardUploadLeases = 0;
    u32 geometryRTGenerations = 0;
    u32 geometryRTLeases = 0;
    u32 geometryPageBudgetMiB = 0;
    bool geometryPagingRequested = false;
    bool geometryPolicyPending = false;
    bool geometryPagingDetails = false;
    bool geometryRTAccelerationKnown = true;
    u64 geometryResidencyArenaBytes = 0;
    u64 geometryResidencyUsedBytes = 0;
    u64 geometryResidencyPinnedBytes = 0;
    u64 geometryResidencyStagingBytes = 0;
    u32 geometryResidentGroups = 0;
    u32 geometryPinnedGroups = 0;
    u32 geometryDesiredGroups = 0;
    u32 geometryActivatingGroups = 0;
    u32 geometryBlockedGroups = 0;
    u32 geometryResidentPages = 0;
    u32 geometryPinnedPages = 0;
    u32 geometryReadingPages = 0;
    u32 geometryUploadingPages = 0;
    u32 geometryRetiringPages = 0;
    u32 geometryUploadsRecorded = 0;
    u32 geometryUploadKiB = 0;
    u32 geometryReadsIssued = 0;
    u32 geometryReadsFailed = 0;
    u32 geometryUploadsDiscarded = 0;
    u32 geometryAllocationDeferrals = 0;
    u32 geometryBudgetDeferrals = 0;
    u32 geometryEvictions = 0;
    u32 geometryLiveSnapshots = 0;
    u32 geometryFailedSnapshots = 0;
    u32 geometryCutRevision = 0;
    bool geometryStreaming = false;
    u64 geometryPageVertexSlots = 0;
    u64 geometryUniqueVertices = 0;
    u64 geometryClusterVertexReferences = 0;
    u32 geometryPages = 0;
    u32 geometryReclusterSplits = 0;
    u32 geometryMaxClusterVertices = 0;
    u32 geometryMaxClusterTriangles = 0;

    bool vsmActive = false;
    bool vsmSunMoving = false;
    u32 vsmPages = 0;
    u32 vsmDirtyPages = 0;
    u32 vsmWrongPages = 0;
    u32 vsmBinDraws = 0;
    u32 vsmBinInstances = 0;
    u32 vsmBinMaxVisited = 0;
    u32 vsmBinLodCulled = 0;
    u32 vsmBinDrops = 0;
    u32 vsmLevelPages[6] = {0, 0, 0, 0, 0, 0};
    u32 localShadowSpots = 0;
    u32 localShadowPoints = 0;
    u32 localShadowAccepted = 0;
    u32 localShadowDeferred = 0;
    u32 localShadowDyn = 0;
    u32 localShadowPairs = 0;
    u32 localShadowDrops = 0;
    u32 localShadowDynDrops = 0;
    u32 localShadowAtlas = 0;
    u32 localShadowPages = 0;
    u32 localShadowOverflow = 0;
    u32 localShadowHudViews = 0;
    u32 localShadowBatches = 0;
    u32 localShadowExtraCasters = 0;

    u32 lightsFrustum = 0;
    u32 lightsTouching = 0;
    u32 lightsInvalidSector = 0;
    u32 lightsLodCulled = 0;
    u32 lightsHomCulled = 0;

    u32 lightsClustered = 0;
    u32 lightsHiZVisible = 0;
    u32 lightsPoint = 0;
    u32 lightsSpot = 0;
    u32 lightsOmni = 0;
    u32 lightTiles[4] = {0, 0, 0, 0};
    u32 lightTilesTotal = 0;

    u32 detailInstances = 0;
    u32 detailSlots = 0;         // Detail slots
    u32 detailTrisPerBlade[3] = {0, 0, 0};  // Triangles per blade per LOD (from bladeIndexCount/3)

    // Detail culling stats (from GPU readback)
    u32 detailVisibleSlots = 0;  // Slots that passed frustum culling
    u32 detailVisibleLOD0 = 0;   // Instances in LOD0 (close, 9 segments)
    u32 detailVisibleLOD1 = 0;   // Instances in LOD1 (mid, 4 segments)
    u32 detailVisibleLOD2 = 0;   // Instances in LOD2 (far, 2 segments)
    u32 detailVisibleDecals = 0; // Visible decal instances

    // Detail buffer sizing
    u32 detailGeneratedInstances = 0;  // Total generated instances (from readback)
    u32 detailVisibleCapacity = 0;     // Current visible buffer capacity per LOD
    u32 detailDecalCapacity = 0;       // Current decal buffer capacity

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
        residualStatic = residualTerrain = residualDynamic = residualTransparent = 0;
        clusterOcclusionCandidates = clusterOcclusionRecovered = 0;
        clusterInstanceVisits = clusterNodeVisits = clusterLeafVisits = 0;
        clusterDeferredInstances = clusterDeferredNodes = clusterOverflow = 0;
        geometrySharedBytes = geometryInstanceBytes = geometryPayloadBytes = geometryVertexBytes = geometryRetainedBytes = 0;
        geometryForwardDrawBytes = geometryRetiringSourceBytes = geometrySourceStagingBytes = geometryHostSourceBytes = 0;
        geometryRetiringArenaBytes = geometryShadowSnapshotBytes = geometryShadowHostBytes = 0;
        geometryRTSourceBytes = geometryRTGenerationBytes = geometryRTAccelerationBytes = 0;
        geometryForwardUploadLeases = geometryRTGenerations = geometryRTLeases = geometryPageBudgetMiB = 0;
        geometryPagingRequested = geometryPolicyPending = geometryPagingDetails = false;
        geometryRTAccelerationKnown = true;
        geometryPageVertexSlots = geometryUniqueVertices = geometryClusterVertexReferences = 0;
        geometryPages = geometryReclusterSplits = geometryMaxClusterVertices = geometryMaxClusterTriangles = 0;
        geometryResidencyArenaBytes = geometryResidencyUsedBytes = 0;
        geometryResidencyPinnedBytes = geometryResidencyStagingBytes = 0;
        geometryResidentGroups = geometryPinnedGroups = geometryDesiredGroups = 0;
        geometryActivatingGroups = geometryBlockedGroups = 0;
        geometryResidentPages = geometryPinnedPages = geometryReadingPages = 0;
        geometryUploadingPages = geometryRetiringPages = geometryUploadsRecorded = geometryUploadKiB = 0;
        geometryReadsIssued = geometryReadsFailed = geometryUploadsDiscarded = 0;
        geometryAllocationDeferrals = geometryBudgetDeferrals = geometryEvictions = 0;
        geometryLiveSnapshots = geometryFailedSnapshots = geometryCutRevision = 0;
        geometryStreaming = false;
        vsmActive = vsmSunMoving = false;
        vsmPages = vsmDirtyPages = vsmWrongPages = 0;
        vsmBinDraws = vsmBinInstances = vsmBinMaxVisited = vsmBinLodCulled = vsmBinDrops = 0;
        for (u32 i = 0; i < 6; ++i) vsmLevelPages[i] = 0;
        localShadowSpots = localShadowPoints = localShadowAccepted = localShadowDeferred = localShadowDyn = 0;
        localShadowPairs = localShadowDrops = localShadowDynDrops = localShadowAtlas = 0;
        localShadowPages = localShadowOverflow = localShadowBatches = localShadowExtraCasters = localShadowHudViews = 0;
        lightsFrustum = lightsTouching = lightsInvalidSector = lightsLodCulled = lightsHomCulled = 0;
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

// ImGui-based stats overlay for displaying profiling data
class StatsOverlay
{
public:
    StatsOverlay();
    ~StatsOverlay();

    // Set GPU profiler reference (optional - for GPU timing display)
    void SetGPUProfiler(GPUProfiler* gpuProfiler) { m_gpuProfiler = gpuProfiler; }

    // Set render stats for current frame
    void SetRenderStats(const RenderStats& stats) { m_renderStats = stats; }
    const RenderStats& GetRenderStats() const { return m_renderStats; }

    // Render the overlay (call during ImGui pass)
    void Render();

    // Visibility control
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
    void WriteProfileDump(u32 intervalSeconds);

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
    u32 m_lastDumpTime = 0;
    RenderStats m_renderStats;
    bool m_visible = false;

    // UI state
    bool m_cpuExpanded = true;
    bool m_gpuExpanded = true;
    bool m_geometryExpanded = true;
    bool m_allocExpanded = true;
    bool m_allocObjectClassProfiling = false;

    // Render inspector state
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

} // namespace xray::profiler
