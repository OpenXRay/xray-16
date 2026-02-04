#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include "GPUProfiler.h"

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

    // Culling stats (from GPU culling manager)
    u32 objectsSubmitted = 0;    // Total objects submitted for culling
    u32 objectsCulled = 0;       // Objects culled (frustum + occlusion)
    u32 objectsVisible = 0;      // Objects that passed culling

    // Mega-buffer stats
    u32 megaBufferVertices = 0;  // Total vertices in mega-buffer
    u32 megaBufferIndices = 0;   // Total indices in mega-buffer

    // Skinning stats
    u32 skinnedMeshes = 0;       // Number of skinned meshes
    u32 totalBones = 0;          // Total bones across all skinned meshes
    u32 maxBonesPerMesh = 0;     // Maximum bones in a single mesh

    // Skinned culling stats (from GPU Hi-Z culling)
    u32 skinnedSubmitted = 0;    // Total skinned meshes submitted for culling
    u32 skinnedVisible = 0;      // Skinned meshes that passed culling
    u32 skinnedCulled = 0;       // Skinned meshes culled by Hi-Z

    // Detail/grass stats
    u32 detailInstances = 0;     // Total grass instances
    u32 detailSlots = 0;         // Detail slots
    u32 detailTrianglesPerBlade = 0;  // Triangles per grass blade (LOD dependent)

    void Reset()
    {
        totalBatches = staticBatches = dynamicBatches = skinnedBatches = terrainBatches = particleBatches = 0;
        totalTriangles = staticTriangles = dynamicTriangles = skinnedTriangles = terrainTriangles = 0;
        objectsSubmitted = objectsCulled = objectsVisible = 0;
        megaBufferVertices = megaBufferIndices = 0;
        skinnedMeshes = totalBones = maxBonesPerMesh = 0;
        skinnedSubmitted = skinnedVisible = skinnedCulled = 0;
        detailInstances = detailSlots = detailTrianglesPerBlade = 0;
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

private:
    void RenderCPUSection();
    void RenderGPUSection();
    void RenderGeometrySection();
    void RenderZoneTree(u32 zoneId, const xr_vector<ZoneData>& zones, float parentTime);

    // Helper to format time with appropriate precision
    // Use different slot values (0-3) when calling multiple times in one statement
    static const char* FormatTime(float ms, int slot = -1);
    static u32 GetTimeColor(float ms, float parentMs);
    static const char* FormatNumber(u32 value);  // Format with K/M suffixes

private:
    GPUProfiler* m_gpuProfiler = nullptr;
    RenderStats m_renderStats;
    bool m_visible = false;

    // UI state
    bool m_cpuExpanded = true;
    bool m_gpuExpanded = true;
    bool m_geometryExpanded = true;
};

} // namespace xray::profiler
