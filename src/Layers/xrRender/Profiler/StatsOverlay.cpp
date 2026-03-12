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

    ImGui::Separator();

    // Render Inspector Section
    RenderInspectorSection();

    ImGui::Separator();

    // Wallmarks Section
    RenderWallmarksSection();

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

void StatsOverlay::RenderGPUPassList(const xr_vector<GPUPassTiming>& passTimings, float totalGPU, bool asyncOnly)
{
    for (const auto& pass : passTimings)
    {
        if (pass.isAsync != asyncOnly)
            continue;
        if (strchr(pass.name.c_str(), '.') != nullptr)
            continue;

        u32 color = GetTimeColor(pass.timeMs, totalGPU);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        float percent = totalGPU > 0.0f ? (pass.timeMs / totalGPU) * 100.0f : 0.0f;

        ImGui::BulletText("%s", pass.name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s (%.1f%%)", FormatTime(pass.timeMs), percent);

        ImGui::PopStyleColor();

        xr_string parentPrefix(pass.name.c_str());
        parentPrefix += '.';

        for (const auto& sub : passTimings)
        {
            if (sub.isAsync != asyncOnly)
                continue;
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

        float asyncTotal = 0.0f;
        float graphicsTotal = 0.0f;
        bool hasAsync = false;
        for (const auto& pass : passTimings)
        {
            bool isSubPass = (strchr(pass.name.c_str(), '.') != nullptr);
            if (isSubPass) continue;
            if (pass.isAsync) {
                asyncTotal += pass.timeMs;
                hasAsync = true;
            } else {
                graphicsTotal += pass.timeMs;
            }
        }

        if (hasAsync)
        {
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 180, 255, 255));
            ImGui::Text("Async Compute: %s", FormatTime(asyncTotal));
            ImGui::PopStyleColor();
            ImGui::Indent();
            RenderGPUPassList(passTimings, totalGPU, true);
            ImGui::Unindent();

            ImGui::Text("Graphics: %s", FormatTime(graphicsTotal));
            ImGui::Indent();
            RenderGPUPassList(passTimings, totalGPU, false);
            ImGui::Unindent();
            ImGui::Unindent();
        }
        else
        {
            ImGui::Indent();
            RenderGPUPassList(passTimings, totalGPU, false);
            ImGui::Unindent();
        }
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

        if (s.lightsClustered > 0)
        {
            ImGui::Text("Lights:");
            ImGui::Indent();
            ImGui::Text("Clustered: %u  (point: %u, spot: %u, shadowed: %u)",
                s.lightsClustered, s.lightsPoint, s.lightsSpot, s.lightsShadowed);
            ImGui::Unindent();
        }
    }
    else
    {
        m_geometryExpanded = false;
    }
}

void StatsOverlay::RenderInspectorSection()
{
    if (!ImGui::CollapsingHeader("Render Inspector"))
        return;

    if (m_rtNames.empty())
    {
        ImGui::TextDisabled("No render targets registered");
        return;
    }

    ImGui::Indent();

    const char* previewLabel = m_inspectorSelectedRT >= 0 && m_inspectorSelectedRT < (int)m_rtNames.size()
        ? m_rtNames[m_inspectorSelectedRT].c_str()
        : "Select RT...";

    if (ImGui::BeginCombo("RT", previewLabel))
    {
        for (int i = 0; i < (int)m_rtNames.size(); i++)
        {
            bool selected = (m_inspectorSelectedRT == i);
            if (ImGui::Selectable(m_rtNames[i].c_str(), selected))
            {
                m_inspectorSelectedRT = i;
                m_selectedRTName = m_rtNames[i];
                m_selectedMipLevel = 0;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (m_inspectorPreview && m_inspectorSelectedRT >= 0)
    {
        ImGui::RadioButton("RGB", &m_channelMode, 0); ImGui::SameLine();
        ImGui::RadioButton("R", &m_channelMode, 1); ImGui::SameLine();
        ImGui::RadioButton("G", &m_channelMode, 2); ImGui::SameLine();
        ImGui::RadioButton("B", &m_channelMode, 3); ImGui::SameLine();
        ImGui::RadioButton("A", &m_channelMode, 4);

        if (m_selectedRTMipCount > 1)
        {
            int maxMip = (int)m_selectedRTMipCount - 1;
            ImGui::SliderInt("Mip Level", &m_selectedMipLevel, 0, maxMip);
        }

        float availWidth = ImGui::GetContentRegionAvail().x;
        float aspect = 1.0f;
        auto desc = m_inspectorPreview->getDesc();
        if (desc.height > 0)
            aspect = (float)desc.width / (float)desc.height;

        float displayW = std::min(availWidth, 400.0f);
        float displayH = displayW / aspect;

        ImGui::Image(
            reinterpret_cast<ImTextureID>(m_inspectorPreview),
            ImVec2(displayW, displayH)
        );

        if (m_selectedRTMipCount > 1)
        {
            u32 srcW = m_sourceWidth;
            u32 srcH = m_sourceHeight;
            u32 mipW = std::max(1u, srcW >> m_selectedMipLevel);
            u32 mipH = std::max(1u, srcH >> m_selectedMipLevel);
            ImGui::TextDisabled("%ux%u (mip %d: %ux%u, %u mips)",
                srcW, srcH, m_selectedMipLevel, mipW, mipH, m_selectedRTMipCount);
        }
        else
        {
            ImGui::TextDisabled("%ux%u", m_sourceWidth, m_sourceHeight);
        }
    }
}

void StatsOverlay::RenderWallmarksSection()
{
    if (!ImGui::CollapsingHeader("Wallmarks"))
        return;

    if (m_wallmarkData.empty())
    {
        ImGui::TextDisabled("No active wallmarks");
        return;
    }

    ImGui::Text("Objects: %d", (int)m_wallmarkData.size());

    // Object selector (stable by pointer key)
    const WallmarkObjectData* selectedObj = nullptr;
    int selectedObjIdx = 0;
    for (int i = 0; i < (int)m_wallmarkData.size(); i++)
    {
        if (m_wallmarkData[i].objKey == m_wallmarkSelectedKey)
        {
            selectedObj = &m_wallmarkData[i];
            selectedObjIdx = i;
            break;
        }
    }
    if (!selectedObj)
    {
        selectedObj = &m_wallmarkData[0];
        m_wallmarkSelectedKey = m_wallmarkData[0].objKey;
    }

    int totalSplats = 0;
    for (const auto& g : selectedObj->groups) totalSplats += (int)g.splats.size();

    char objLabel[64];
    xr_sprintf(objLabel, sizeof(objLabel), "Object %d  (%d splats, %d meshes)",
        selectedObjIdx, totalSplats, (int)selectedObj->groups.size());

    if (ImGui::BeginCombo("##wm_obj", objLabel))
    {
        for (int i = 0; i < (int)m_wallmarkData.size(); i++)
        {
            int n = 0;
            for (const auto& g : m_wallmarkData[i].groups) n += (int)g.splats.size();
            char label[64];
            xr_sprintf(label, sizeof(label), "Object %d  (%d splats)", i, n);
            bool sel = (m_wallmarkData[i].objKey == m_wallmarkSelectedKey);
            if (ImGui::Selectable(label, sel))
            {
                m_wallmarkSelectedKey = m_wallmarkData[i].objKey;
                m_wallmarkSelectedGroup = 0;
            }
        }
        ImGui::EndCombo();
    }

    if (selectedObj->groups.empty())
    {
        ImGui::TextDisabled("No painted meshes");
        return;
    }

    // Group (submesh/texture) selector
    m_wallmarkSelectedGroup = std::min(m_wallmarkSelectedGroup, (int)selectedObj->groups.size() - 1);
    const WallmarkTexGroup& group = selectedObj->groups[m_wallmarkSelectedGroup];

    if (selectedObj->groups.size() > 1)
    {
        char grpLabel[128];
        const char* baseName = group.texName.empty() ? "unknown" : group.texName.c_str();
        xr_sprintf(grpLabel, sizeof(grpLabel), "%s  (%d)", baseName, (int)group.splats.size());
        if (ImGui::BeginCombo("##wm_grp", grpLabel))
        {
            for (int i = 0; i < (int)selectedObj->groups.size(); i++)
            {
                const char* n = selectedObj->groups[i].texName.empty() ? "unknown" : selectedObj->groups[i].texName.c_str();
                char label[128];
                xr_sprintf(label, sizeof(label), "%s  (%d)", n, (int)selectedObj->groups[i].splats.size());
                if (ImGui::Selectable(label, m_wallmarkSelectedGroup == i))
                    m_wallmarkSelectedGroup = i;
            }
            ImGui::EndCombo();
        }
    }

    float canvasW = ImGui::GetContentRegionAvail().x;
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasBR(canvasPos.x + canvasW, canvasPos.y + canvasW);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvasPos, canvasBR, IM_COL32(20, 20, 20, 255));

    if (group.diffuseTex)
        dl->AddImage(reinterpret_cast<ImTextureID>(group.diffuseTex), canvasPos, canvasBR);

    dl->AddRect(canvasPos, canvasBR, IM_COL32(80, 80, 80, 255));

    ImU32 gridCol = group.diffuseTex ? IM_COL32(255, 255, 255, 30) : IM_COL32(40, 40, 40, 255);
    for (int i = 1; i < 4; i++)
    {
        float t = i * 0.25f;
        dl->AddLine(ImVec2(canvasPos.x + t * canvasW, canvasPos.y),
                    ImVec2(canvasPos.x + t * canvasW, canvasPos.y + canvasW), gridCol);
        dl->AddLine(ImVec2(canvasPos.x, canvasPos.y + t * canvasW),
                    ImVec2(canvasPos.x + canvasW, canvasPos.y + t * canvasW), gridCol);
    }

    constexpr u32 WM_MODE_PROCEDURAL = 1u;
    const float minStampRadiusPx = std::max(3.0f, canvasW * 0.01f);
    for (const auto& s : group.splats)
    {
        float px = canvasPos.x + s.u * canvasW;
        float py = canvasPos.y + s.v * canvasW;
        float radiusPx = std::max(minStampRadiusPx, s.uvRadius * canvasW);
        ImU32 tint = IM_COL32((int)(s.r * 255), (int)(s.g * 255), (int)(s.b * 255), (int)(s.a * 255));

        if (s.mode == WM_MODE_PROCEDURAL)
        {
            constexpr int kSegments = 24;
            ImVec2 pts[kSegments];
            for (int i = 0; i < kSegments; i++)
            {
                float t = (float)i / (float)kSegments;
                float a = t * PI_MUL_2;
                float n0 = 0.5f + 0.5f * _sin(a * (5.0f + _abs(s.seed) * 0.013f) + s.seed * 0.071f);
                float n1 = 0.5f + 0.5f * _sin(a * (9.0f + _abs(s.seed) * 0.007f) - s.seed * 0.113f);
                float r = radiusPx * (0.72f + 0.20f * n0 + 0.18f * n1);
                pts[i] = ImVec2(px + _cos(a) * r, py + _sin(a) * r);
            }

            dl->AddConvexPolyFilled(pts, kSegments, tint);
            dl->AddPolyline(pts, kSegments, IM_COL32(255, 255, 255, 70), true, 1.0f);

            ImU32 core = IM_COL32((int)(s.r * 180), (int)(s.g * 140), (int)(s.b * 140), (int)(s.a * 210));
            dl->AddCircleFilled(ImVec2(px, py), radiusPx * 0.35f, core);
        }
        else if (s.stampTex)
        {
            ImVec2 p0(px - radiusPx, py - radiusPx);
            ImVec2 p1(px + radiusPx, py + radiusPx);
            dl->AddImage(reinterpret_cast<ImTextureID>(s.stampTex), p0, p1, ImVec2(0, 0), ImVec2(1, 1), tint);
            dl->AddCircle(ImVec2(px, py), radiusPx, IM_COL32(255, 255, 255, 60));
        }
        else
        {
            dl->AddCircleFilled(ImVec2(px, py), radiusPx, tint);
            dl->AddCircle(ImVec2(px, py), radiusPx, IM_COL32(255, 255, 255, 80));
        }
    }

    ImGui::Dummy(ImVec2(canvasW, canvasW));
    ImGui::TextDisabled("%d splat(s)  --  UV space [0,1]x[0,1]", (int)group.splats.size());
}

} // namespace xray::profiler
