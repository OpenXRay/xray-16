#include "stdafx.h"
#include "StatsOverlay.h"
#include "xrCore/Profiler/Profiler.h"
#include "xrEngine/Device.h"
#include <imgui.h>

namespace xray::profiler
{

StatsOverlay::StatsOverlay()
{
    // Get the ImGui context from Device - required for proper input handling
    ImGui::SetCurrentContext(Device.GetImGuiContext());
}

StatsOverlay::~StatsOverlay() = default;

const char* StatsOverlay::FormatTime(float ms, int slot)
{
    // Use rotating buffers to allow multiple FormatTime calls in single statement
    static constexpr int NUM_BUFFERS = 4;
    static char buffers[NUM_BUFFERS][32];
    static int currentBuffer = 0;

    // Use specified slot or rotate through buffers
    int bufferIdx = (slot >= 0 && slot < NUM_BUFFERS) ? slot : (currentBuffer++ % NUM_BUFFERS);
    char* buffer = buffers[bufferIdx];

    if (ms >= 1.0f)
        xr_sprintf(buffer, sizeof(buffers[0]), "%.2fms", ms);
    else if (ms >= 0.1f)
        xr_sprintf(buffer, sizeof(buffers[0]), "%.3fms", ms);
    else
        xr_sprintf(buffer, sizeof(buffers[0]), "%.0fus", ms * 1000.0f);
    return buffer;
}

u32 StatsOverlay::GetTimeColor(float ms, float parentMs)
{
    if (parentMs <= 0.0f)
        parentMs = 16.67f;  // Default to 60fps frame budget

    float ratio = ms / parentMs;

    // Green -> Yellow -> Red gradient based on time ratio
    if (ratio < 0.25f)
        return IM_COL32(100, 200, 100, 255);  // Green - fast
    else if (ratio < 0.5f)
        return IM_COL32(200, 200, 100, 255);  // Yellow - moderate
    else if (ratio < 0.75f)
        return IM_COL32(255, 180, 80, 255);   // Orange - slow
    else
        return IM_COL32(255, 100, 100, 255);  // Red - very slow
}

void StatsOverlay::Render()
{
    if (!m_visible)
        return;

    // Ensure we're using the correct ImGui context
    ImGui::SetCurrentContext(Device.GetImGuiContext());

    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

    // Check if IDE is in interactive mode
    // When IDE is active, allow full interaction; otherwise display-only
    bool ideActive = Device.editor().IsActiveState();

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (!ideActive)
    {
        // Display-only when IDE not active - game input takes priority
        flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav;
    }

    if (!ImGui::Begin("Performance Stats", &m_visible, flags))
    {
        ImGui::End();
        return;
    }

    // Frame time header
    CPUProfiler& cpuProfiler = GetCPUProfiler();
    float cpuFrameTime = cpuProfiler.GetFrameTimeMs();
    float gpuFrameTime = m_gpuProfiler ? m_gpuProfiler->GetTotalGPUTimeMs() : 0.0f;
    float fps = cpuFrameTime > 0.0f ? 1000.0f / cpuFrameTime : 0.0f;

    ImGui::Text("Frame: %s (%.1f FPS)", FormatTime(cpuFrameTime), fps);
    if (!ideActive)
    {
        ImGui::TextDisabled("(Press editor key to interact)");
    }

    // Settings section (collapsible)
    if (ImGui::CollapsingHeader("Settings"))
    {
        int throttle = static_cast<int>(cpuProfiler.GetThrottleInterval());
        if (ImGui::SliderInt("Sample Interval", &throttle, 1, 120, "%d frames"))
        {
            cpuProfiler.SetThrottleInterval(static_cast<u32>(throttle));
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Profile every N frames.\n1 = every frame (highest overhead)\n30 = default (~2%% overhead)\n60+ = minimal overhead");
        }
    }

    ImGui::Separator();

    // Geometry Section (first - most important for debugging)
    RenderGeometrySection();

    ImGui::Separator();

    // CPU Section
    RenderCPUSection();

    ImGui::Separator();

    // GPU Section
    RenderGPUSection();

    ImGui::End();
}

void StatsOverlay::RenderCPUSection()
{
    CPUProfiler& profiler = GetCPUProfiler();
    const auto& zones = profiler.GetZones();
    const auto& rootZones = profiler.GetRootZones();
    float frameTime = profiler.GetFrameTimeMs();

    ImGui::SetNextItemOpen(m_cpuExpanded, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("CPU"))
    {
        m_cpuExpanded = true;

        if (rootZones.empty())
        {
            ImGui::TextDisabled("No CPU zones recorded");
        }
        else
        {
            ImGui::Text("Total: %s", FormatTime(frameTime));
            ImGui::Indent();

            for (u32 rootId : rootZones)
            {
                RenderZoneTree(rootId, zones, frameTime);
            }

            ImGui::Unindent();
        }
    }
    else
    {
        m_cpuExpanded = false;
    }
}

void StatsOverlay::RenderZoneTree(u32 zoneId, const xr_vector<ZoneData>& zones, float parentTime)
{
    if (zoneId >= zones.size())
        return;

    const ZoneData& zone = zones[zoneId];
    if (!zone.info || zone.timing.callCount == 0)
        return;

    // Push unique ID to avoid conflicts with duplicate zone names
    ImGui::PushID(static_cast<int>(zoneId));

    const char* name = zone.info->name;
    float totalTime = zone.timing.totalTimeMs;
    float selfTime = zone.timing.selfTimeMs;
    u32 callCount = zone.timing.callCount;

    // Color based on time contribution
    u32 color = GetTimeColor(totalTime, parentTime);
    ImGui::PushStyleColor(ImGuiCol_Text, color);

    // Build display string
    char label[256];
    if (callCount > 1)
        xr_sprintf(label, sizeof(label), "%s x%u", name, callCount);
    else
        xr_strcpy(label, sizeof(label), name);

    bool hasChildren = !zone.childIds.empty();

    if (hasChildren)
    {
        // Tree node for zones with children
        bool open = ImGui::TreeNode(label);
        ImGui::SameLine();
        // Use explicit buffer slots since we call FormatTime twice in one statement
        ImGui::TextDisabled("%s (self: %s)", FormatTime(totalTime, 0), FormatTime(selfTime, 1));

        ImGui::PopStyleColor();

        if (open)
        {
            for (u32 childId : zone.childIds)
            {
                RenderZoneTree(childId, zones, totalTime);
            }
            ImGui::TreePop();
        }
    }
    else
    {
        // Leaf node (no children)
        ImGui::BulletText("%s", label);
        ImGui::SameLine();
        ImGui::TextDisabled("%s", FormatTime(totalTime));
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
}

void StatsOverlay::RenderGPUSection()
{
    ImGui::SetNextItemOpen(m_gpuExpanded, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("GPU"))
    {
        m_gpuExpanded = true;

        if (!m_gpuProfiler || !m_gpuProfiler->IsInitialized())
        {
            ImGui::TextDisabled("GPU profiler not initialized");
            return;
        }

        const auto& passTimings = m_gpuProfiler->GetPassTimings();
        float totalGPU = m_gpuProfiler->GetTotalGPUTimeMs();

        if (passTimings.empty())
        {
            ImGui::TextDisabled("No GPU passes recorded");
            return;
        }

        ImGui::Text("Total: %s", FormatTime(totalGPU));
        ImGui::Indent();

        // Render parent passes first, then their sub-passes immediately after.
        // Sub-passes are nested timer queries that resolve before the parent,
        // so we can't rely on list order. Instead: iterate parents, then find children.
        for (const auto& pass : passTimings)
        {
            // Skip sub-passes (handled below their parent)
            if (strchr(pass.name.c_str(), '.') != nullptr)
                continue;

            u32 color = GetTimeColor(pass.timeMs, totalGPU);
            ImGui::PushStyleColor(ImGuiCol_Text, color);

            float percent = totalGPU > 0.0f ? (pass.timeMs / totalGPU) * 100.0f : 0.0f;

            ImGui::BulletText("%s", pass.name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s (%.1f%%)", FormatTime(pass.timeMs), percent);

            ImGui::PopStyleColor();

            // Find and render sub-passes matching "ParentName.*"
            xr_string parentPrefix(pass.name.c_str());
            parentPrefix += '.';

            for (const auto& sub : passTimings)
            {
                if (strncmp(sub.name.c_str(), parentPrefix.c_str(), parentPrefix.size()) != 0)
                    continue;

                const char* subName = sub.name.c_str() + parentPrefix.size();
                u32 subColor = GetTimeColor(sub.timeMs, totalGPU);
                ImGui::PushStyleColor(ImGuiCol_Text, subColor);

                float subPercent = totalGPU > 0.0f ? (sub.timeMs / totalGPU) * 100.0f : 0.0f;

                ImGui::Indent();
                ImGui::BulletText("%s", subName);
                ImGui::SameLine();
                ImGui::TextDisabled("%s (%.1f%%)", FormatTime(sub.timeMs), subPercent);
                ImGui::Unindent();

                ImGui::PopStyleColor();
            }
        }

        ImGui::Unindent();
    }
    else
    {
        m_gpuExpanded = false;
    }
}

const char* StatsOverlay::FormatNumber(u32 value)
{
    static char buffer[32];

    if (value >= 1000000)
        xr_sprintf(buffer, sizeof(buffer), "%.2fM", value / 1000000.0f);
    else if (value >= 1000)
        xr_sprintf(buffer, sizeof(buffer), "%.1fK", value / 1000.0f);
    else
        xr_sprintf(buffer, sizeof(buffer), "%u", value);

    return buffer;
}

void StatsOverlay::RenderGeometrySection()
{
    ImGui::SetNextItemOpen(m_geometryExpanded, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Geometry"))
    {
        m_geometryExpanded = true;

        const RenderStats& s = m_renderStats;

        // ═══════════════════════════════════════════════════
        //  TRIANGLE COUNTS
        // ═══════════════════════════════════════════════════
        ImGui::Text("Triangles:");
        ImGui::Indent();

        // Total triangles with visibility ratio
        if (s.objectsSubmitted > 0)
        {
            float visRatio = s.objectsVisible > 0 ?
                (float)s.objectsVisible / s.objectsSubmitted * 100.0f : 0.0f;
            ImGui::Text("Total: %s (%.0f%% visible)", FormatNumber(s.totalTriangles), visRatio);
        }
        else
        {
            ImGui::Text("Total: %s", FormatNumber(s.totalTriangles));
        }

        // Breakdown by type
        if (s.staticTriangles > 0)
            ImGui::BulletText("Static:  %s", FormatNumber(s.staticTriangles));
        if (s.dynamicTriangles > 0)
            ImGui::BulletText("Dynamic: %s", FormatNumber(s.dynamicTriangles));
        if (s.skinnedTriangles > 0)
            ImGui::BulletText("Skinned: %s", FormatNumber(s.skinnedTriangles));
        if (s.terrainTriangles > 0)
            ImGui::BulletText("Terrain: %s", FormatNumber(s.terrainTriangles));

        ImGui::Unindent();

        // ═══════════════════════════════════════════════════
        //  BATCH COUNTS
        // ═══════════════════════════════════════════════════
        ImGui::Text("Batches: %u total", s.totalBatches);
        ImGui::Indent();

        if (s.staticBatches > 0)
            ImGui::BulletText("Static:   %u", s.staticBatches);
        if (s.dynamicBatches > 0)
            ImGui::BulletText("Dynamic:  %u", s.dynamicBatches);
        if (s.skinnedBatches > 0)
            ImGui::BulletText("Skinned:  %u", s.skinnedBatches);
        if (s.terrainBatches > 0)
            ImGui::BulletText("Terrain:  %u", s.terrainBatches);
        if (s.particleBatches > 0)
            ImGui::BulletText("Particles: %u", s.particleBatches);

        ImGui::Unindent();

        // ═══════════════════════════════════════════════════
        //  GPU CULLING STATS
        // ═══════════════════════════════════════════════════
        if (s.objectsSubmitted > 0)
        {
            ImGui::Text("Culling:");
            ImGui::Indent();

            float cullPercent = s.objectsCulled > 0 ?
                (float)s.objectsCulled / s.objectsSubmitted * 100.0f : 0.0f;

            ImGui::Text("Submitted: %u", s.objectsSubmitted);
            ImGui::Text("Visible:   %u", s.objectsVisible);
            ImGui::Text("Culled:    %u (%.0f%%)", s.objectsCulled, cullPercent);

            ImGui::Unindent();
        }

        // ═══════════════════════════════════════════════════
        //  SKINNING STATS
        // ═══════════════════════════════════════════════════
        if (s.skinnedMeshes > 0 || s.skinnedSubmitted > 0)
        {
            ImGui::Text("Skinning:");
            ImGui::Indent();

            if (s.skinnedMeshes > 0) {
                ImGui::Text("Meshes: %u", s.skinnedMeshes);
                ImGui::Text("Bones:  %u (max: %u)", s.totalBones, s.maxBonesPerMesh);
            }

            // Skinned Hi-Z culling stats
            if (s.skinnedSubmitted > 0) {
                float cullRate = s.skinnedSubmitted > 0 ?
                    (100.0f * s.skinnedCulled / s.skinnedSubmitted) : 0.0f;
                ImGui::Text("Hi-Z:   %u/%u visible (%.0f%% culled)",
                    s.skinnedVisible, s.skinnedSubmitted, cullRate);
            }

            ImGui::Unindent();
        }

        // ═══════════════════════════════════════════════════
        //  MEGA-BUFFER STATS
        // ═══════════════════════════════════════════════════
        if (s.megaBufferVertices > 0)
        {
            ImGui::Text("Mega-Buffer:");
            ImGui::Indent();

            ImGui::Text("Vertices: %s", FormatNumber(s.megaBufferVertices));
            ImGui::Text("Indices:  %s", FormatNumber(s.megaBufferIndices));

            // Estimate memory usage (UnifiedVertex = 48 bytes, index = 4 bytes)
            float vertMB = (s.megaBufferVertices * 48) / (1024.0f * 1024.0f);
            float idxMB = (s.megaBufferIndices * 4) / (1024.0f * 1024.0f);
            ImGui::TextDisabled("~%.1f MB verts, ~%.1f MB idx", vertMB, idxMB);

            ImGui::Unindent();
        }

        // ═══════════════════════════════════════════════════
        //  DETAIL/GRASS STATS
        // ═══════════════════════════════════════════════════
        if (s.detailSlots > 0)
        {
            ImGui::Text("Grass/Detail:");
            ImGui::Indent();

            if (s.detailVisibleSlots > 0)
            {
                float slotCullRate = 100.0f * (1.0f - float(s.detailVisibleSlots) / float(s.detailSlots));
                ImGui::Text("Slots: %u/%u visible (%.0f%% culled)",
                    s.detailVisibleSlots, s.detailSlots, slotCullRate);
            }
            else
            {
                ImGui::Text("Slots: %u", s.detailSlots);
            }

            u32 totalVisible = s.detailVisibleLOD0 + s.detailVisibleLOD1 + s.detailVisibleLOD2;
            if (totalVisible > 0)
            {
                if (s.detailGeneratedInstances > 0)
                {
                    float cullRate = 100.0f * (1.0f - float(totalVisible) / float(s.detailGeneratedInstances));
                    char visStr[32];
                    xr_strcpy(visStr, FormatNumber(totalVisible));
                    ImGui::Text("Blades: %s/%s visible (%.0f%% culled)",
                        visStr, FormatNumber(s.detailGeneratedInstances), cullRate);
                }
                else
                {
                    ImGui::Text("Blades: %s visible", FormatNumber(totalVisible));
                }

                ImGui::Indent();
                if (s.detailVisibleLOD0 > 0)
                    ImGui::BulletText("LOD0 (9seg): %s", FormatNumber(s.detailVisibleLOD0));
                if (s.detailVisibleLOD1 > 0)
                    ImGui::BulletText("LOD1 (4seg): %s", FormatNumber(s.detailVisibleLOD1));
                if (s.detailVisibleLOD2 > 0)
                    ImGui::BulletText("LOD2 (2seg): %s", FormatNumber(s.detailVisibleLOD2));
                ImGui::Unindent();

                u32 actualTris = s.detailVisibleLOD0 * s.detailTrisPerBlade[0]
                               + s.detailVisibleLOD1 * s.detailTrisPerBlade[1]
                               + s.detailVisibleLOD2 * s.detailTrisPerBlade[2];
                ImGui::TextDisabled("Blade tris: %s", FormatNumber(actualTris));
            }

            if (s.detailVisibleDecals > 0)
                ImGui::Text("Decals: %s visible", FormatNumber(s.detailVisibleDecals));

            if (s.detailVisibleCapacity > 0)
            {
                float totalMB = (s.detailVisibleCapacity * 4.0f * 3 + s.detailDecalCapacity * 4.0f) / (1024.0f * 1024.0f);
                char capStr[32];
                xr_strcpy(capStr, FormatNumber(s.detailVisibleCapacity));
                ImGui::TextDisabled("Buffers: %s/LOD + %s decal (%.1f MB)",
                    capStr, FormatNumber(s.detailDecalCapacity), totalMB);
            }

            ImGui::Unindent();
        }
    }
    else
    {
        m_geometryExpanded = false;
    }
}

} // namespace xray::profiler
