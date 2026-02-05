#pragma once

#include "xrCore/Profiler/ProfilerTypes.h"
#include "GPUProfiler.h"

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

    u32 objectsSubmitted = 0;
    u32 objectsCulled = 0;
    u32 objectsVisible = 0;

    u32 megaBufferVertices = 0;
    u32 megaBufferIndices = 0;

    u32 skinnedMeshes = 0;
    u32 totalBones = 0;
    u32 maxBonesPerMesh = 0;

    u32 skinnedSubmitted = 0;
    u32 skinnedVisible = 0;
    u32 skinnedCulled = 0;

    u32 detailInstances = 0;
    u32 detailSlots = 0;
    u32 detailTrisPerBlade[3] = {0, 0, 0};

    u32 detailVisibleSlots = 0;
    u32 detailVisibleLOD0 = 0;
    u32 detailVisibleLOD1 = 0;
    u32 detailVisibleLOD2 = 0;

    void Reset()
    {
        totalBatches = staticBatches = dynamicBatches = skinnedBatches = terrainBatches = particleBatches = 0;
        totalTriangles = staticTriangles = dynamicTriangles = skinnedTriangles = terrainTriangles = 0;
        objectsSubmitted = objectsCulled = objectsVisible = 0;
        megaBufferVertices = megaBufferIndices = 0;
        skinnedMeshes = totalBones = maxBonesPerMesh = 0;
        skinnedSubmitted = skinnedVisible = skinnedCulled = 0;
        detailInstances = detailSlots = 0;
        detailTrisPerBlade[0] = detailTrisPerBlade[1] = detailTrisPerBlade[2] = 0;
        detailVisibleSlots = detailVisibleLOD0 = detailVisibleLOD1 = detailVisibleLOD2 = 0;
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

private:
    void RenderCPUSection();
    void RenderGPUSection();
    void RenderGeometrySection();
    void RenderZoneTree(u32 zoneId, const xr_vector<ZoneData>& zones, float parentTime);

    static const char* FormatTime(float ms, int slot = -1);
    static u32 GetTimeColor(float ms, float parentMs);
    static const char* FormatNumber(u32 value);

private:
    GPUProfiler* m_gpuProfiler = nullptr;
    RenderStats m_renderStats;
    bool m_visible = false;

    bool m_cpuExpanded = true;
    bool m_gpuExpanded = true;
    bool m_geometryExpanded = true;
};

}
