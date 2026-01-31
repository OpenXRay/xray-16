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

        for (const auto& pass : passTimings)
        {
            u32 color = GetTimeColor(pass.timeMs, totalGPU);
            ImGui::PushStyleColor(ImGuiCol_Text, color);

            // Calculate percentage
            float percent = totalGPU > 0.0f ? (pass.timeMs / totalGPU) * 100.0f : 0.0f;

            ImGui::BulletText("%s", pass.name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s (%.1f%%)", FormatTime(pass.timeMs), percent);

            ImGui::PopStyleColor();
        }

        ImGui::Unindent();
    }
    else
    {
        m_gpuExpanded = false;
    }
}

} // namespace xray::profiler
