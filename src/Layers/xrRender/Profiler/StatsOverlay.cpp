#include "stdafx.h"
#include "StatsOverlay.h"
#include "xrCore/Profiler/Profiler.h"
#include "xrCore/MemoryStats.h"
#include "xrEngine/device.h"
#include "xrEngine/IRenderBackend.h"
#include "Layers/xrRender/GpuParticleManager.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>

static bool FormatSubmitThreadLine(char* buf, size_t size)
{
    IRenderBackend::SubmitThreadTimings t;
    if (!GEnv.Backend || !GEnv.Backend->GetSubmitThreadTimings(t))
        return false;
    xr_sprintf(buf, size,
        "FrameSync max (us): slot %llu | cap %llu | gpu %llu | acquire %llu || Submit: latency %llu | qlock %llu | semWait %llu | encode %llu | plock %llu | present %llu | gc %llu",
        (unsigned long long)t.slotWaitUs, (unsigned long long)t.capacityWaitUs,
        (unsigned long long)t.gpuWaitUs, (unsigned long long)t.acquireUs,
        (unsigned long long)t.jobLatencyUs, (unsigned long long)t.queueLockUs,
        (unsigned long long)t.semWaitUs, (unsigned long long)t.encodeUs,
        (unsigned long long)t.presentLockUs, (unsigned long long)t.presentUs,
        (unsigned long long)t.gcUs);
    return true;
}

static bool FormatQueueTimingsLine(char* buf, size_t size)
{
    IRenderBackend::QueueTimings t;
    if (!GEnv.Backend || !GEnv.Backend->GetQueueTimings(t))
        return false;
    xr_sprintf(buf, size,
        "GPU queues (us): graphics %llu | compute %llu | overlap %llu | span %llu",
        (unsigned long long)t.graphicsUs, (unsigned long long)t.computeUs,
        (unsigned long long)t.overlapUs, (unsigned long long)t.spanUs);
    return true;
}

namespace xray::profiler
{

const char* PathTracerDiagnosticName(u32 mode)
{
    static constexpr pcstr kNames[] = {
        "beauty", "albedo", "normal", "roughness", "metallic",
        "direct diffuse", "direct specular", "indirect diffuse", "indirect specular",
        "visible emission", "path length", "invalid samples", "raw radiance", "transport coverage",
    };
    return mode < sizeof(kNames) / sizeof(kNames[0]) ? kNames[mode] : "unknown";
}

static bool HasGPUPassNamed(const xr_vector<GPUPassTiming>& passTimings, const char* name)
{
    for (const auto& pass : passTimings)
    {
        if (strcmp(pass.name.c_str(), name) == 0)
            return true;
    }
    return false;
}

static bool IsGPUPassChildName(const char* parentName, const char* childName)
{
    const size_t parentLength = strlen(parentName);
    const size_t childLength = strlen(childName);
    return childLength > parentLength + 1 && childName[parentLength] == '.' &&
        strncmp(childName, parentName, parentLength) == 0;
}

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

const char* StatsOverlay::FormatBytes(u64 bytes, int slot)
{
    static constexpr int NUM_BUFFERS = 6;
    static char buffers[NUM_BUFFERS][32];
    static int currentBuffer = 0;

    int bufferIdx = (slot >= 0 && slot < NUM_BUFFERS) ? slot : (currentBuffer++ % NUM_BUFFERS);
    char* buffer = buffers[bufferIdx];

    if (bytes >= 1024ull * 1024ull)
        xr_sprintf(buffer, sizeof(buffers[0]), "%.2f MB", double(bytes) / (1024.0 * 1024.0));
    else if (bytes >= 1024ull)
        xr_sprintf(buffer, sizeof(buffers[0]), "%.1f KB", double(bytes) / 1024.0);
    else
        xr_sprintf(buffer, sizeof(buffers[0]), "%llu B", (unsigned long long)bytes);
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
    const float frameTime = Device.fTimeDeltaReal * 1000.f;
    const float fps = Device.GetStats().fFPS;

    ImGui::Text("Frame: %s (%.1f FPS)", FormatTime(frameTime), fps);
    if (!ideActive)
    {
        ImGui::TextDisabled("(Press editor key to interact)");
    }
    const auto& lighting = m_renderStats.lighting;
    ImGui::Text("Lighting: %s -> %s", render::fg::LightingModeName(lighting.requested), render::fg::LightingModeName(lighting.effective));
    ImGui::Text("Opaque pass: %s", lighting.opaqueScheduled ? render::fg::LightingModeName(lighting.scheduled) : "none");
    ImGui::Text("Deferred raster lighting: %s", !lighting.opaqueScheduled ? "not evaluated"
        : (lighting.scheduled == render::fg::LightingMode::Raster ? "selected" : "omitted (RT dispatch)"));
    ImGui::Text("RT dispatch pass: %s", lighting.opaqueScheduled && lighting.scheduled != render::fg::LightingMode::Raster ? "scheduled" : "none");
    if (lighting.conflictingRequests)
        ImGui::TextDisabled("Both RT switches enabled: PT takes precedence");
    if (lighting.fallback != render::fg::LightingFallback::None)
        ImGui::Text("Fallback: %s%s", render::fg::LightingFallbackName(lighting.fallback),
            lighting.frameFailed ? " (RT frame failure, not preflight)"
            : (lighting.recoveryActive ? " (raster recovery latch held)" : " (preflight, raster selected)"));
    if (lighting.frameFailed)
        ImGui::Text("Failed RT frame: world clear %s", lighting.failureCleared ? "recorded (opaque black)" : "left to present");
    if (lighting.recoveryActive)
        ImGui::Text("Raster recovery: latched for %s%s | rearm on mode or RTGI profile change, shader reload, level load or reset",
            render::fg::LightingModeName(lighting.requested), lighting.rtgiProfile ? " [diagnostic]" : "");
    if (lighting.requested != render::fg::LightingMode::Raster)
        ImGui::Text("RT dispatch recorded: %s", lighting.recorded ? "yes" : "no");
    ImGui::Text("Surface history: %s", lighting.previousSurfacesValid ? "valid" : "rejected");
    if (lighting.requested != render::fg::LightingMode::Raster)
    {
        ImGui::Text("RT history used: %s", lighting.historyUsed ? "yes" : "no");
        ImGui::Text("RT scene revision: %llu | pose revision: %llu", static_cast<unsigned long long>(lighting.sceneRevision),
            static_cast<unsigned long long>(lighting.poseRevision));
        ImGui::Text("RT static detail instances: %u (%s)", lighting.rayStaticDetailInstances,
            lighting.recorded ? "recorded scene" : "not recorded");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Static DO_NO_WAVING detail meshes use the bounded ray-detail membership in either grass mode.\n"
                "They retain atlas cutouts, standard shading and zero foliage transmission.\n"
                "Frozen reference scenes retain the captured membership; recapture to change the envelope.");
    }
    if (lighting.requested == render::fg::LightingMode::RTGI)
    {
        ImGui::Text("RTGI implementation: %s", render::fg::RTGIImplementationName());
        ImGui::Text("RTGI budgets: samples %u | bounces %u | ray distance %.0f m (%s)",
            lighting.rtgiSamples, lighting.rtgiBounces, lighting.rtgiRayDistance,
            lighting.rawSignalsRecorded ? "recorded" : "configured, not recorded");
        ImGui::Text("RTGI reuse: requested %s | available %s | reservoirs %s",
            lighting.reuseRequested ? "yes" : "no", lighting.reuseAvailable ? "yes" : "no",
            lighting.reuseReservoirs ? "active" : "inactive");
        if (lighting.reuseRequested && !lighting.reuseAvailable)
            ImGui::TextDisabled("Reuse requested but unavailable: no reservoir reuse runs");
        ImGui::Text("RTGI raw guides: %s", lighting.rawSignalsRecorded ? "recorded" : "not recorded");
        const char* grassCoverage = !lighting.rayGrassEnabled ? "disabled"
            : (lighting.rayGrassPending ? "pending/unavailable" : "covered");
        ImGui::Text("RTGI ray scope: scene radius %.0f m (offscreen dynamic admission envelope) | grass radius %.0f m%s | grass coverage %s | frustum/HiZ-independent",
            lighting.raySceneRadius, lighting.rayGrassRadius,
            lighting.rayGrassRadius > 0.0f ? "" : " (radius 0)", grassCoverage);
        ImGui::TextDisabled("Scene radius adds offscreen dynamic admission only; resident static coverage is retained and camera/shadow-admitted dynamics may extend farther");
        ImGui::TextDisabled("RTGI rays: world only (HUD excluded from occlusion/reflection) | primary geometric normal = resolved shading normal (approximation) | not reconstructed | not cached | not clamped");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("RTGI traces raw multibounce transport from raster primaries with world-only rays; HUD geometry stays visible in color but is excluded from world occlusion and reflection.\n"
                "The scene radius only adds an offscreen dynamic admission envelope: resident static coverage is retained (static geometry is not clipped to that radius) and camera- or shadow-admitted dynamics may extend beyond it.\n"
                "The primary geometric normal is the resolved primary shading normal, used as an approximation because no independent geometric normal exists.\n"
                "Grass coverage is the actual ray membership, not the requested radius: disabled when details or RT support are unavailable or the radius is 0; pending/unavailable without a completed frame covering the current requested sphere and resident source; covered only with usable completed membership.\n"
                "No legacy temporal estimator, no reference accumulation, no reservoir reuse, no reconstruction cache and no radiance clamp.\n"
                "Admission and grass coverage are bounded radii independent of frustum and HiZ culling, so coverage is not the full world and cost is not uniform.");
    }
    if (lighting.requested == render::fg::LightingMode::ReferencePT)
    {
        const RenderStats& rs = m_renderStats;
        ImGui::Text("PT submitted samples: %u", rs.pathTracerSamples);
        ImGui::Text("PT recorded samples: %u", lighting.recordedSamples);
        ImGui::Text("PT reference: %s (diagnostic %u of 13)", PathTracerDiagnosticName(rs.pathTracerDiagnosticMode), rs.pathTracerDiagnosticMode);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Path length: R = traced scattering depth / 16, G = requested budget / 16.\n"
                "Invalid samples: magenta marks rejected samples.\n"
                "Transport coverage: standard green, foliage yellow, alpha blend cyan,\n"
                "thin-water fallback magenta, miss blue.");
        ImGui::Text("PT integrator: bounces %u | diffuse %u | null events %u | sample cap %u%s",
            rs.pathTracerBounces, rs.pathTracerDiffuseMode, rs.pathTracerMaxNullEvents, rs.pathTracerMaxSamples,
            rs.pathTracerMaxSamples ? "" : " (uncapped)");
        ImGui::Text("PT sun radius: %.2f deg%s", rad2deg(rs.pathTracerSunAngularRadius),
            rs.pathTracerSunAngularRadius > 0.0f ? "" : " (delta sun)");
        ImGui::Text("PT scene: %s", rs.pathTracerFreezeRequested ?
            (rs.pathTracerSnapshotValid ? "frozen snapshot" : (rs.pathTracerCapturePending ? "frozen capture awaiting completion" : "live fallback (no snapshot)")) :
            "live scene (freeze off)");
        ImGui::Text("PT snapshot: requested %s | pending %s | valid %s | fallback %s | frozen %s | cdf %s",
            rs.pathTracerFreezeRequested ? "yes" : "no", rs.pathTracerCapturePending ? "yes" : "no",
            rs.pathTracerSnapshotValid ? "yes" : "no", rs.pathTracerSnapshotFallback ? "yes" : "no",
            rs.pathTracerFrozen ? "yes" : "no", rs.pathTracerCdfActive ? "active" : "inactive");
        ImGui::Text("PT scene contents: %u lights | %u emitters (%s)",
            rs.pathTracerLightCount, rs.pathTracerEmissiveCount, rs.pathTracerFrozen ? "frozen snapshot" : "live scene");
        if (rs.pathTracerFrozen)
        {
            ImGui::Text("PT frozen revision: scene %llu | textures %llu | lighting %016llx",
                static_cast<unsigned long long>(rs.pathTracerFrozenSceneRevision),
                static_cast<unsigned long long>(rs.pathTracerFrozenTextureRevision),
                static_cast<unsigned long long>(rs.pathTracerLightingSignature));
            ImGui::Text("PT frozen clone: %u textures (%s) | retained scene %s",
                rs.pathTracerCapturedTextures, FormatBytes(rs.pathTracerCapturedTextureBytes, 0),
                FormatBytes(rs.pathTracerRetainedSceneBytes, 1));
        }
        else
            ImGui::TextDisabled("PT frozen snapshot: none (live scene)");
        ImGui::TextDisabled("Reference: finite depth, RR from depth 3 | not reconstructed | not cached | not clamped");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Finite-depth reference with Russian roulette from depth 3, GGX min roughness 0.04 and ray-cone filtering.\n"
                "Ray reach is finite (10000 m) and every path has a per-ray null-event budget (candidates bounded at 64x the budget).\n"
                "No reservoir reconstruction, no radiance cache and no radiance clamp, so this is not an exact physical solution.");
        ImGui::TextDisabled("Supported: admitted/resident RT geometry + bounded grass");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Supported: admitted and resident RT geometry with bounded grass coverage.\n"
                "Deferred: solid dielectric refraction, volumetric absorption and particle/volume transport.\n"
                "Live raster particles, smoke, weather sprites, distortion and lens flares are excluded from reference output.\n"
                "Not a complete full-world geometry ground truth.");
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

    RenderAllocationsSection();

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

        if (memstats::BacktraceCaptureSupported())
        {
            bool autoCapture = memstats::AllocationSpikeCaptureEnabled();
            if (ImGui::Checkbox("Auto-capture allocation spikes", &autoCapture))
                memstats::SetAllocationSpikeCaptureEnabled(autoCapture);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Learns each call path over 8 sampled measurements, then tracks a moving average.\n"
                    "Triggers above both 2x the average and 256 extra allocations.\n"
                    "Captures the remaining allocations in that same measurement, including child zones.\n"
                    "At most 4096 stacks per spike; pauses after capture to preserve the report.");
            if (autoCapture)
            {
                if (memstats::AllocationSpikeCapturePaused())
                {
                    if (memstats::BacktraceCaptureArmed())
                        ImGui::TextDisabled("Capture in progress...");
                    else
                    {
                        ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "Auto-capture paused; see Allocations / Backtraces");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Resume##allocspikes"))
                            memstats::ResumeAllocationSpikeCapture();
                    }
                }
                else if (memstats::BacktraceCaptureArmed())
                    ImGui::TextDisabled("Automatic detection waits for the manual capture");
                else
                    ImGui::TextDisabled("Watching sampled measurements (every %u frames; 8-sample warmup)",
                        profiler.GetThrottleInterval());
            }
        }

        if (rootZones.empty())
        {
            ImGui::TextDisabled("No CPU zones recorded");
        }
        else
        {
            ImGui::Text("Total: %s", FormatTime(frameTime));
            char submitLine[320];
            if (FormatSubmitThreadLine(submitLine, sizeof(submitLine)))
                ImGui::TextDisabled("%s", submitLine);
            if (FormatQueueTimingsLine(submitLine, sizeof(submitLine)))
                ImGui::TextDisabled("%s", submitLine);
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
    if (!zone.info)
        return;

    // Push unique ID to avoid conflicts with duplicate zone names
    ImGui::PushID(static_cast<int>(zoneId));

    const char* name = zone.info->name;
    float totalTime = zone.timing.totalTimeMs;
    float selfTime = zone.timing.selfTimeMs;
    u32 callCount = zone.timing.callCount;

    // Color based on time contribution
    u32 color = callCount == 0 ? IM_COL32(110, 110, 110, 255) : GetTimeColor(totalTime, parentTime);
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
        bool open = ImGui::TreeNode("zone", "%s", label);
        if (ImGui::BeginPopupContextItem("zone_ctx"))
        {
            if (memstats::BacktraceCaptureSupported() && ImGui::MenuItem("Capture alloc backtraces"))
                memstats::ArmBacktraceCapture(zone.zoneId, zone.info->name);
            if (ImGui::MenuItem("Copy tree to clipboard"))
                CopyZoneTreeToClipboard();
            if (ImGui::MenuItem("Reset tree"))
                GetCPUProfiler().ResetTree();
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        // Use explicit buffer slots since we call FormatTime twice in one statement
        ImGui::TextDisabled("%s (self: %s)", FormatTime(totalTime, 0), FormatTime(selfTime, 1));

        ImGui::PopStyleColor();

        if (zone.timing.allocCalls > 0)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(215, 170, 100, 255));
            ImGui::Text("%u alloc (%s), self %u",
                (u32)zone.timing.allocCalls, FormatBytes(zone.timing.allocBytes, 2),
                (u32)zone.timing.selfAllocCalls);
            ImGui::PopStyleColor();
        }

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
        if (ImGui::BeginPopupContextItem("zone_ctx"))
        {
            if (memstats::BacktraceCaptureSupported() && ImGui::MenuItem("Capture alloc backtraces"))
                memstats::ArmBacktraceCapture(zone.zoneId, zone.info->name);
            if (ImGui::MenuItem("Copy tree to clipboard"))
                CopyZoneTreeToClipboard();
            if (ImGui::MenuItem("Reset tree"))
                GetCPUProfiler().ResetTree();
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", FormatTime(totalTime));
        ImGui::PopStyleColor();

        if (zone.timing.allocCalls > 0)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(215, 170, 100, 255));
            ImGui::Text("%u alloc (%s)",
                (u32)zone.timing.allocCalls, FormatBytes(zone.timing.allocBytes, 2));
            ImGui::PopStyleColor();
        }
    }

    ImGui::PopID();
}

void StatsOverlay::BuildGPUPassTree(const xr_vector<GPUPassTiming>& passTimings, xr_vector<GPUPassNode>& nodes, xr_vector<u32>& roots, bool asyncOnly, bool includeOrphans) const
{
    nodes.clear();
    roots.clear();

    xr_vector<GPUPassNode> candidates;
    for (const auto& pass : passTimings)
    {
        if (pass.isAsync != asyncOnly)
            continue;

        GPUPassNode node;
        node.name = pass.name.c_str();
        node.label = pass.name.c_str();
        node.timeMs = pass.timeMs;
        node.isAsync = pass.isAsync;
        candidates.push_back(node);
    }

    const u32 count = u32(candidates.size());
    for (u32 child = 0; child < count; ++child)
    {
        u32 parent = GPUPassNode::InvalidIndex;
        size_t parentLength = 0;

        for (u32 candidate = 0; candidate < count; ++candidate)
        {
            if (candidate == child)
                continue;

            const size_t candidateLength = candidates[candidate].name.size();
            if (candidateLength <= parentLength ||
                !IsGPUPassChildName(candidates[candidate].name.c_str(), candidates[child].name.c_str()))
                continue;

            parent = candidate;
            parentLength = candidateLength;
        }

        candidates[child].parent = parent;
        if (parent != GPUPassNode::InvalidIndex)
            candidates[child].label = candidates[child].name.c_str() + parentLength + 1;
    }

    xr_vector<u32> remap(count, GPUPassNode::InvalidIndex);
    for (u32 index = 0; index < count; ++index)
    {
        u32 ancestor = index;
        while (candidates[ancestor].parent != GPUPassNode::InvalidIndex)
            ancestor = candidates[ancestor].parent;

        if (!includeOrphans && candidates[ancestor].name.find('.') != xr_string::npos)
            continue;

        remap[index] = u32(nodes.size());

        GPUPassNode node = candidates[index];
        node.parent = GPUPassNode::InvalidIndex;
        nodes.push_back(node);
    }

    for (u32 index = 0; index < count; ++index)
    {
        if (remap[index] == GPUPassNode::InvalidIndex)
            continue;

        const u32 parent = candidates[index].parent;
        if (parent == GPUPassNode::InvalidIndex)
        {
            roots.push_back(remap[index]);
            continue;
        }

        nodes[remap[index]].parent = remap[parent];
        nodes[remap[parent]].children.push_back(remap[index]);
    }
}

void StatsOverlay::RenderGPUPassNode(const xr_vector<GPUPassNode>& nodes, u32 index, float totalGPU) const
{
    const GPUPassNode& node = nodes[index];
    const float percent = totalGPU > 0.0f ? (node.timeMs / totalGPU) * 100.0f : 0.0f;

    ImGui::PushID(node.name.c_str());
    ImGui::PushID(node.isAsync ? 1 : 0);
    ImGui::PushStyleColor(ImGuiCol_Text, GetTimeColor(node.timeMs, totalGPU));

    const bool nested = !node.children.empty() && node.parent != GPUPassNode::InvalidIndex;
    bool expanded = false;

    if (nested)
    {
        expanded = ImGui::TreeNodeEx(node.label.c_str(), ImGuiTreeNodeFlags_None, "%s", node.label.c_str());
        ImGui::SameLine();
    }
    else
    {
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::Text("%s", node.label.c_str());
        ImGui::SameLine();
    }

    ImGui::TextDisabled("%s (%.1f%%)", FormatTime(node.timeMs), percent);
    ImGui::PopStyleColor();

    if (nested)
    {
        if (expanded)
        {
            for (u32 child : node.children)
                RenderGPUPassNode(nodes, child, totalGPU);
            ImGui::TreePop();
        }
    }
    else if (!node.children.empty())
    {
        ImGui::Indent();
        for (u32 child : node.children)
            RenderGPUPassNode(nodes, child, totalGPU);
        ImGui::Unindent();
    }

    ImGui::PopID();
    ImGui::PopID();
}

void StatsOverlay::RenderGPUPassList(const xr_vector<GPUPassTiming>& passTimings, float totalGPU, bool asyncOnly)
{
    xr_vector<GPUPassNode> nodes;
    xr_vector<u32> roots;
    BuildGPUPassTree(passTimings, nodes, roots, asyncOnly, false);

    for (u32 root : roots)
        RenderGPUPassNode(nodes, root, totalGPU);
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
        if (const u64 sampleId = m_gpuProfiler->GetCompletedSampleId())
            ImGui::TextDisabled("Completed GPU sample: %llu", static_cast<unsigned long long>(sampleId));

        if (passTimings.empty())
        {
            ImGui::TextDisabled("No GPU passes recorded");
            return;
        }

        if (HasGPUPassNamed(passTimings, "RTGI Raw Transport [diagnostic]"))
        {
            ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "RTGI diagnostic sample: expand light + shadows for emitters");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Staged RTGI Raw Transport sample, not the normal monolithic kernel.\n"
                    "Leaf rows bracket individual dispatches, including scratch traffic and synchronization.\n"
                    "Repeated tile/sample scopes are summed per name and queue.\n"
                    "Primary/Step N light + shadows are inclusive groups. Expand for:\n"
                    "Sun, Local lights, Environment, and Emissive + accumulate.\n"
                    "Emitter rows include scratch, serialization, and timer overhead.\n"
                    "Emissive + accumulate performs the direct-light finite-value check and applies all four groups,\n"
                    "even with zero emissive emitters.\n"
                    "Group and child timers have separate boundaries, so their sums need not match exactly.\n"
                    "Only top-level rows contribute to the frame total. Do not add parents and children.\n"
                    "Step N is a ray iteration; water/null events may not advance a bounce.\n"
                    "Tail + resolve finishes remaining water paths and writes guides on the final sample.\n"
                    "Normal mode still uses one dispatch; staged numbers are not its phase breakdown.");
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
        ImGui::Text("Total: %s", FormatNumber(s.totalTriangles));
        if (s.clusterEntries > 0)
        {
            const u32 drawn = s.clusterTrianglesDrawn + s.clusterTerrainTrianglesDrawn;
            const u32 baked = s.staticTriangles + s.terrainTriangles;
            ImGui::Text("Drawn: %s clustered (%.0f%% of static+terrain)", FormatNumber(drawn),
                baked > 0 ? 100.0f * drawn / baked : 0.0f);
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
        //  CLUSTER LOD STATS
        // ═══════════════════════════════════════════════════
        if (s.clusterEntries > 0)
        {
            ImGui::Text("Clusters:");
            ImGui::Indent();
            float drawRate = 100.0f * (s.clusterVisible + s.clusterTerrainVisible) / s.clusterEntries;
            ImGui::Text("Entries: %u/%u drawn (%.1f%%)",
                s.clusterVisible + s.clusterTerrainVisible, s.clusterEntries, drawRate);
            ImGui::Text("Static:  %u/%u drawn, %s tris", s.clusterVisible, s.clusterStaticEntries, FormatNumber(s.clusterTrianglesDrawn));
            if (s.clusterTerrainEntries > 0)
                ImGui::Text("Terrain: %u/%u drawn, %s tris", s.clusterTerrainVisible, s.clusterTerrainEntries, FormatNumber(s.clusterTerrainTrianglesDrawn));
            ImGui::Text("Occlusion: %u held by last frame's Hi-Z, %u recovered by the retest", s.clusterOcclusionCandidates, s.clusterOcclusionRecovered);
            ImGui::Text("Hierarchy: %u instances, %u nodes, %u leaf refs visited", s.clusterInstanceVisits, s.clusterNodeVisits, s.clusterLeafVisits);
            ImGui::Text("Deferred: %u instances, %u nodes; overflow %u", s.clusterDeferredInstances, s.clusterDeferredNodes, s.clusterOverflow);
            ImGui::Text("Undrawn residue: %u static / %u terrain / %u dynamic / %u transparent", s.residualStatic, s.residualTerrain, s.residualDynamic, s.residualTransparent);
            ImGui::Unindent();
        }

        if (s.geometryResidencyArenaBytes || s.geometryRTSourceBytes || s.geometryForwardDrawBytes)
        {
            ImGui::Text("Geometry memory and paging:");
            ImGui::Indent();
            ImGui::Text("Geometry tables: %.2f MiB shared, %.2f MiB instances/references/BVH",
                s.geometrySharedBytes / 1048576.0, s.geometryInstanceBytes / 1048576.0);
            ImGui::Text("Compact allocation: %.2f MiB payload, %.2f MiB vertices, %u pages",
                s.geometryPayloadBytes / 1048576.0, s.geometryVertexBytes / 1048576.0, s.geometryPages);
            ImGui::Text("Retained forward: %.2f MiB geometry, %.2f MiB draw data",
                s.geometryRetainedBytes / 1048576.0, s.geometryForwardDrawBytes / 1048576.0);
            ImGui::Text("Exact RT: %.2f MiB source, %.2f MiB generation buffers (%u generations, %u leases)",
                s.geometryRTSourceBytes / 1048576.0, s.geometryRTGenerationBytes / 1048576.0,
                s.geometryRTGenerations, s.geometryRTLeases);
            if (s.geometryRTAccelerationKnown)
                ImGui::Text("RT acceleration structures: %.2f MiB (%u/%u BLAS compacted)", s.geometryRTAccelerationBytes / 1048576.0,
                    s.geometryRTCompacted, s.geometryRTCompactable);
            else
                ImGui::Text("RT acceleration structures: %.2f MiB reported; native total unavailable (%u/%u BLAS compacted)",
                    s.geometryRTAccelerationBytes / 1048576.0, s.geometryRTCompacted, s.geometryRTCompactable);
            ImGui::Text("Retiring: %.2f MiB forward, %.2f MiB arenas; forward upload leases %u",
                s.geometryRetiringSourceBytes / 1048576.0, s.geometryRetiringArenaBytes / 1048576.0,
                s.geometryForwardUploadLeases);
            ImGui::Text("CPU source storage: %.2f MiB cook/upload staging, %.2f MiB retained source hosts",
                s.geometrySourceStagingBytes / 1048576.0, s.geometryHostSourceBytes / 1048576.0);
            ImGui::Text("Cached sun views: %.2f MiB readback, %.2f MiB CPU snapshots",
                s.geometryShadowSnapshotBytes / 1048576.0, s.geometryShadowHostBytes / 1048576.0);
            ImGui::Text("Residency [%s]: %.2f / %.2f MiB arena, %.2f MiB pinned, %.2f MiB staging",
                s.geometryStreaming ? "demand paged" : "fully resident",
                s.geometryResidencyUsedBytes / 1048576.0, s.geometryResidencyArenaBytes / 1048576.0,
                s.geometryResidencyPinnedBytes / 1048576.0, s.geometryResidencyStagingBytes / 1048576.0);
            ImGui::Text("Level-load policy: paging %s, fine budget %u MiB%s",
                s.geometryPagingRequested ? "on" : "off", s.geometryPageBudgetMiB,
                s.geometryPolicyPending ? " (changed controls require a level reload)" : "");
            if (s.geometryPagingDetails)
            {
                ImGui::Text("Groups: %u resident (%u pinned), %u desired, %u activating, %u blocked, %u evicted",
                    s.geometryResidentGroups, s.geometryPinnedGroups, s.geometryDesiredGroups,
                    s.geometryActivatingGroups, s.geometryBlockedGroups, s.geometryEvictions);
                ImGui::Text("Pages: %u resident (%u pinned), %u reading, %u uploading, %u retiring",
                    s.geometryResidentPages, s.geometryPinnedPages, s.geometryReadingPages,
                    s.geometryUploadingPages, s.geometryRetiringPages);
                ImGui::Text("Streaming: %u uploads (%u KiB), %u reads, %u read failures, %u discarded uploads",
                    s.geometryUploadsRecorded, s.geometryUploadKiB, s.geometryReadsIssued,
                    s.geometryReadsFailed, s.geometryUploadsDiscarded);
                ImGui::Text("Pressure: %u allocation deferrals, %u budget deferrals; snapshots %u live, %u failed, cut revision %u",
                    s.geometryAllocationDeferrals, s.geometryBudgetDeferrals,
                    s.geometryLiveSnapshots, s.geometryFailedSnapshots, s.geometryCutRevision);
            }
            else
                ImGui::TextDisabled("r_geo_page_stats 1: detailed residency counters");
            ImGui::Text("Vertex slots: %llu page / %llu unique / %llu cluster",
                static_cast<unsigned long long>(s.geometryPageVertexSlots),
                static_cast<unsigned long long>(s.geometryUniqueVertices),
                static_cast<unsigned long long>(s.geometryClusterVertexReferences));
            ImGui::Text("Cluster maxima: %u vertices, %u triangles; %u recluster splits",
                s.geometryMaxClusterVertices, s.geometryMaxClusterTriangles, s.geometryReclusterSplits);
            ImGui::Unindent();
        }

        if (s.vsmActive)
        {
            ImGui::Text("VSM:");
            ImGui::Indent();
            ImGui::Text("Mark: %u pages (L0 %u / L1 %u / L2 %u / L3 %u / L4 %u / L5 %u), sun %s",
                s.vsmPages, s.vsmLevelPages[0], s.vsmLevelPages[1], s.vsmLevelPages[2],
                s.vsmLevelPages[3], s.vsmLevelPages[4], s.vsmLevelPages[5],
                s.vsmSunMoving ? "moving" : "static");
            ImGui::Text("Static atlas: %u dirty (%u wrong-tile)", s.vsmDirtyPages, s.vsmWrongPages);
            ImGui::Text("Bin: %u pages -> %u caster draws (max %u entries visited/page), %u lod-culled, %u drops",
                s.vsmBinDraws, s.vsmBinInstances, s.vsmBinMaxVisited, s.vsmBinLodCulled, s.vsmBinDrops);
            ImGui::Unindent();
        }

        if (s.localShadowSpots > 0 || s.localShadowPoints > 0)
        {
            ImGui::Text("Local shadows:");
            ImGui::Indent();
            ImGui::Text("%u spots / %u points, %u atlas pages (%u%% occupied)",
                s.localShadowSpots, s.localShadowPoints, s.localShadowPages, s.localShadowAtlas);
            ImGui::Text("Views: %u static refresh / %u dynamic / %u overflow / %u hud", s.localShadowAccepted, s.localShadowDyn, s.localShadowOverflow, s.localShadowHudViews);
            ImGui::Text("%u caster batches, %u additional off-camera local casters", s.localShadowBatches, s.localShadowExtraCasters);
            ImGui::Text("Bin: %u pairs, %u drops, %u dyn drops", s.localShadowPairs, s.localShadowDrops, s.localShadowDynDrops);
            ImGui::Unindent();
        }

        // ═══════════════════════════════════════════════════
        //  PARTICLE CULLING STATS
        // ═══════════════════════════════════════════════════
        if (s.particleCullSubmitted > 0)
        {
            ImGui::Text("Particle Culling:");
            ImGui::Indent();

            u32 culledBatches = s.particleCullSubmitted - s.particleCullVisible;
            float cullRate = 100.0f * culledBatches / s.particleCullSubmitted;
            ImGui::Text("Hi-Z:   %u/%u batches visible (%.0f%% culled)",
                s.particleCullVisible, s.particleCullSubmitted, cullRate);
            ImGui::Text("Quads:  %u/%u visible", s.particleQuadsVisible, s.particleQuadsSubmitted);

            ImGui::Unindent();
        }

        const auto particles = xray::render::fg::GetGpuParticleManager().GetStats();
        ImGui::Text("GPU Particles:");
        ImGui::Indent();
        ImGui::Text("Roots: %u live / %u pending  (%u programs)", particles.liveRoots, particles.pendingRoots, particles.programs);
        ImGui::Text("  collision: %u / dyn collision: %u / hud: %u / children: %u",
            particles.collisionRoots, particles.dynamicCollisionRoots, particles.hudRoots, particles.childRoots);
        if (particles.collisionRoots || particles.bvhNodes)
            ImGui::Text("  BVH: %u nodes / %u tris  dyn: %u obj / %u shapes / %u tris",
                particles.bvhNodes, particles.staticTriangles, particles.dynamicObjects,
                particles.dynamicShapes, particles.dynamicTriangles);
        ImGui::Text("Pool: %u particles / %u emitters / %u draw buckets",
            particles.particleCapacity, particles.emitterCapacity, particles.drawBuckets);
        ImGui::Text("Commands: %u / %u replayed", particles.commands, particles.replayedCommands);
        ImGui::Text("Upload: %.2f KiB / readback: %.2f KiB / bindings: %u",
            double(particles.commandBytes) / 1024.0, double(particles.readbackBytes) / 1024.0, particles.bindingSets);
        ImGui::Unindent();

        // ═══════════════════════════════════════════════════
        //  MEGA-BUFFER STATS
        // ═══════════════════════════════════════════════════
        if (s.megaBufferVertices > 0)
        {
            ImGui::Text("Logical source layout:");
            ImGui::Indent();

            ImGui::Text("Vertices: %s", FormatNumber(s.megaBufferVertices));
            ImGui::Text("Indices:  %s", FormatNumber(s.megaBufferIndices));


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
                if (s.detailVisibilityInstances > 0)
                {
                    float cullRate = 100.0f * (1.0f - float(totalVisible) / float(s.detailVisibilityInstances));
                    char visStr[32];
                    xr_strcpy(visStr, FormatNumber(totalVisible));
                    ImGui::Text("Blades: %s/%llu visible (%.0f%% culled)",
                        visStr, static_cast<unsigned long long>(s.detailVisibilityInstances), cullRate);
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
            if (s.detailVisibleMeshes > 0)
                ImGui::Text("Meshes: %s visible", FormatNumber(s.detailVisibleMeshes));

            ImGui::Text("Source %llu: %llu instances / %u chunks",
                static_cast<unsigned long long>(s.detailSourceId),
                static_cast<unsigned long long>(s.detailGeneratedInstances), s.detailSourceChunks);
            ImGui::TextDisabled("Source storage: %s active / %s resident, %u chunks / %u generations",
                FormatBytes(s.detailActiveSourceBytes, 0), FormatBytes(s.detailSourceBytes, 1),
                s.detailResidentChunks, s.detailGenerations);
            ImGui::Text("Visibility frame %llu / source %llu: %u/%u packets, overflow 0x%X",
                static_cast<unsigned long long>(s.detailVisibilityFrame),
                static_cast<unsigned long long>(s.detailVisibilitySource),
                s.detailPackets, s.detailPacketCapacity, s.detailOverflow);
            ImGui::TextDisabled("Work capacities: %u / %u / %u LOD, %u mesh, %u decal",
                s.detailVisibleCapacity[0], s.detailVisibleCapacity[1], s.detailVisibleCapacity[2],
                s.detailVisibleCapacity[3], s.detailVisibleCapacity[4]);
            ImGui::TextDisabled("Visibility storage: %s / %u frames, generation scratch %s",
                FormatBytes(s.detailFrameBytes, 0), s.detailFrames, FormatBytes(s.detailPendingBytes, 1));
            const char* stages[] = { "idle", "count ready", "count pending", "emit ready", "emit pending" };
            ImGui::TextDisabled("Generation: %s, %u/%u chunks complete",
                stages[s.detailGenerationStage], s.detailGenerationChunks, s.detailGenerationChunkCount);
            if (s.detailUsageKnown)
                ImGui::TextDisabled("GPU memory: %s used / %s budget, 5%% admission headroom",
                    FormatBytes(s.detailUsageBytes, 0), FormatBytes(s.detailBudgetBytes, 1));
            else
                ImGui::TextDisabled("GPU memory: usage unavailable / %s budget", FormatBytes(s.detailBudgetBytes));

            ImGui::Unindent();
        }

        if (s.lightsFrustum > 0 || s.lightsTouching > 0 || s.lightsClustered > 0)
        {
            u32 culled = (s.lightsHiZVisible < s.lightsClustered)
                ? (s.lightsClustered - s.lightsHiZVisible) : 0;
            if (culled > 0)
                ImGui::Text("Lights: %u visible / %u total (%u Hi-Z culled)", s.lightsHiZVisible, s.lightsClustered, culled);
            else
                ImGui::Text("Lights: %u clustered", s.lightsClustered);
            ImGui::TextDisabled("  %u point, %u spot, %u omni", s.lightsPoint, s.lightsSpot, s.lightsOmni);
            ImGui::TextDisabled("  %u frustum candidates, %u camera-touch admissions", s.lightsFrustum, s.lightsTouching);
            ImGui::TextDisabled("  rejected: %u invalid sector, %u LOD, %u HOM", s.lightsInvalidSector, s.lightsLodCulled, s.lightsHomCulled);
        }
        if (s.lightTilesTotal > 0)
        {
            const u32 classified = s.lightTiles[0] + s.lightTiles[1] + s.lightTiles[2] + s.lightTiles[3];
            const u32 sky = s.lightTilesTotal > classified ? s.lightTilesTotal - classified : 0;
            ImGui::Text("Light tiles: %u lit / %u total (%u sky)", classified, s.lightTilesTotal, sky);
            ImGui::TextDisabled("  sun-lit %u, sun-mixed %u, lit+lights %u, mixed+lights %u", s.lightTiles[0], s.lightTiles[1], s.lightTiles[2], s.lightTiles[3]);
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
                const bool depthRT = i < (int)m_rtIsDepth.size() && m_rtIsDepth[i] != 0;
                if (depthRT)
                    m_channelMode = 5;
                else if (m_channelMode == 5)
                    m_channelMode = 0;
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
        ImGui::RadioButton("A", &m_channelMode, 4); ImGui::SameLine();
        ImGui::RadioButton("Depth", &m_channelMode, 5);
        ImGui::TextDisabled("Depth reads raw device depth; RTGI raw/guide channel meanings on hover");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("RTGI raw signals and guides, defined for every in-bounds pixel:\n"
                "RawDiffuse: RGB = direct diffuse + giIntensity x indirect diffuse, A = 1 on a valid surface. No albedo demodulation, no clamp, no tone map.\n"
                "RawSpecular: RGB = direct specular + giIntensity x indirect specular, A = 1 on a valid surface.\n"
                "Emission: RGB = primary source emission/sky copied before the composite writes scene color.\n"
                "NormalRoughness / AlbedoMetallic: immutable primary copies (shading normal.xyz and |roughness|; albedo.rgb and metallic). The primary geometric normal fed to the integrator is this resolved shading normal, an approximation because no independent geometric normal exists.\n"
                "PathData: R = mean first diffuse-lobe segment distance, G = mean first specular-lobe segment distance (miss = ray distance, 0 when the lobe is absent), B = valid-path fraction - not temporal confidence, A = mean traced scattering depth.\n"
                "SurfaceData: R = linear camera distance to the primary, G = actual device depth including HUD encoding, B = bit flags (bit0 valid opaque primary, bit1 finite/usable motion): 0 background or invalid, 1 opaque without usable motion, 3 opaque with motion, A = 1 HUD / 0 world.\n"
                "Motion: previousUV - currentUV from the raster primaries; xy stays zero when unusable or non-finite and SurfaceData.B reports it, so true static zero motion remains distinguishable from missing motion.\n"
                "Normal/roughness, albedo/metallic and motion are immutable primary copies; no reconstruction pass consumes the guides in this build.");

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

void StatsOverlay::AppendZoneText(xr_string& out, u32 zoneId, const xr_vector<ZoneData>& zones, int depth)
{
    if (zoneId >= zones.size())
        return;

    const ZoneData& zone = zones[zoneId];
    if (!zone.info || zone.timing.callCount == 0)
        return;

    for (int i = 0; i < depth; ++i)
        out += "  ";

    char line[256];
    if (zone.timing.callCount > 1)
        xr_sprintf(line, sizeof(line), "%s x%u", zone.info->name, zone.timing.callCount);
    else
        xr_strcpy(line, sizeof(line), zone.info->name);
    out += line;

    if (!zone.childIds.empty())
        xr_sprintf(line, sizeof(line), " %s (self: %s)",
            FormatTime(zone.timing.totalTimeMs, 0), FormatTime(zone.timing.selfTimeMs, 1));
    else
        xr_sprintf(line, sizeof(line), " %s", FormatTime(zone.timing.totalTimeMs, 0));
    out += line;

    if (zone.timing.allocCalls > 0)
    {
        if (!zone.childIds.empty())
            xr_sprintf(line, sizeof(line), "  %u alloc (%s), self %u",
                (u32)zone.timing.allocCalls, FormatBytes(zone.timing.allocBytes, 2),
                (u32)zone.timing.selfAllocCalls);
        else
            xr_sprintf(line, sizeof(line), "  %u alloc (%s)",
                (u32)zone.timing.allocCalls, FormatBytes(zone.timing.allocBytes, 2));
        out += line;
    }
    out += "\n";

    for (u32 childId : zone.childIds)
        AppendZoneText(out, childId, zones, depth + 1);
}

void StatsOverlay::CopyZoneTreeToClipboard()
{
    CPUProfiler& profiler = GetCPUProfiler();
    const auto& zones = profiler.GetZones();

    xr_string text;
    text.reserve(16384);

    char header[64];
    xr_sprintf(header, sizeof(header), "Total: %s\n", FormatTime(profiler.GetFrameTimeMs(), 0));
    text += header;

    for (u32 rootId : profiler.GetRootZones())
        AppendZoneText(text, rootId, zones, 0);

    char submitLine[320];
    if (FormatSubmitThreadLine(submitLine, sizeof(submitLine)))
    {
        text += submitLine;
        text += "\n";
    }

    ImGui::SetClipboardText(text.c_str());
}

void StatsOverlay::AppendGPUPassNode(xr_string& out, const xr_vector<GPUPassNode>& nodes, u32 index, float totalGPU, u32 depth) const
{
    const GPUPassNode& node = nodes[index];
    const xr_string indent(size_t(2 + depth * 2), ' ');

    char line[256];
    xr_sprintf(line, sizeof(line), "%s%-40s %9.3f ms %5.1f%%\n", indent.c_str(), node.name.c_str(), node.timeMs,
        totalGPU > 0.0f ? node.timeMs / totalGPU * 100.0f : 0.0f);
    out += line;

    for (u32 child : node.children)
        AppendGPUPassNode(out, nodes, child, totalGPU, depth + 1);
}

void StatsOverlay::WriteProfileDump(u32 intervalSeconds)
{
    const u32 now = Device.dwTimeGlobal;
    if (m_lastDumpTime != 0 && now - m_lastDumpTime < intervalSeconds * 1000u)
        return;
    m_lastDumpTime = now;

    xr_string text;
    text.reserve(32768);
    char line[512];

    xr_sprintf(line, sizeof(line), "backend: %s | %ux%u | frame %.2f ms (%.1f FPS) | time %u ms\n",
        GEnv.Backend ? GEnv.Backend->GetAPIName() : "none", Device.dwWidth, Device.dwHeight,
        Device.fTimeDeltaReal * 1000.f, Device.GetStats().fFPS, now);
    text += line;

    const RenderStats& rs = m_renderStats;
    xr_sprintf(line, sizeof(line), "lighting: requested=%s | effective=%s | reason=%s | conflict=%s | recorded=%s | PT submitted samples=%u\n",
        render::fg::LightingModeName(rs.lighting.requested), render::fg::LightingModeName(rs.lighting.effective),
        render::fg::LightingFallbackName(rs.lighting.fallback), rs.lighting.conflictingRequests ? "PT precedence" : "none", rs.lighting.recorded ? "yes" : "no",
        rs.pathTracerSamples);
    text += line;
    xr_sprintf(line, sizeof(line), "lighting schedule: opaque=%s | deferred raster branch=%s | RT dispatch=%s | failure=%s | failure clear=%s | raster recovery latch=%s\n",
        rs.lighting.opaqueScheduled ? render::fg::LightingModeName(rs.lighting.scheduled) : "none",
        !rs.lighting.opaqueScheduled ? "not evaluated" : (rs.lighting.scheduled == render::fg::LightingMode::Raster ? "selected" : "omitted"),
        rs.lighting.opaqueScheduled && rs.lighting.scheduled != render::fg::LightingMode::Raster ? "scheduled" : "none",
        rs.lighting.frameFailed ? render::fg::LightingFallbackName(rs.lighting.fallback) : "none",
        rs.lighting.failureCleared ? "recorded" : "none",
        rs.lighting.recoveryActive ? "active" : "inactive");
    text += line;
    if (rs.lighting.frameFailed)
    {
        xr_sprintf(line, sizeof(line), "lighting recovery: frame kept %s%s scheduling | world clear=%s | black presentation fallback requested | raster latched until mode or RTGI profile change, shader reload, level load or reset\n",
            render::fg::LightingModeName(rs.lighting.scheduled), rs.lighting.rtgiProfile ? " [diagnostic]" : "",
            rs.lighting.failureCleared ? "recorded before UI" : "unavailable");
        text += line;
    }
    if (rs.lighting.requested != render::fg::LightingMode::Raster)
    {
        xr_sprintf(line, sizeof(line), "RT static detail instances: %u (%s) | bounded ray membership in both grass modes | standard cutout, zero foliage transmission\n",
            rs.lighting.rayStaticDetailInstances, rs.lighting.recorded ? "recorded scene" : "not recorded");
        text += line;
    }
    if (rs.lighting.requested == render::fg::LightingMode::RTGI)
    {
        xr_sprintf(line, sizeof(line), "RTGI: implementation=%s | budgets samples=%u bounces=%u ray distance=%.0f m (%s) | reuse requested=%s available=%s reservoirs=%s | raw guides=%s\n",
            render::fg::RTGIImplementationName(), rs.lighting.rtgiSamples, rs.lighting.rtgiBounces, rs.lighting.rtgiRayDistance,
            rs.lighting.rawSignalsRecorded ? "recorded" : "configured, not recorded",
            rs.lighting.reuseRequested ? "yes" : "no", rs.lighting.reuseAvailable ? "yes" : "no",
            rs.lighting.reuseReservoirs ? "active" : "inactive",
            rs.lighting.rawSignalsRecorded ? "recorded" : "not recorded");
        text += line;
        const char* grassCoverage = !rs.lighting.rayGrassEnabled ? "disabled"
            : (rs.lighting.rayGrassPending ? "pending/unavailable" : "covered");
        xr_sprintf(line, sizeof(line), "RTGI scope: scene radius=%.0f m adds an offscreen dynamic admission envelope (resident static coverage retained; camera/shadow-admitted dynamics may extend farther) | grass radius=%.0f m%s | grass coverage=%s (actual membership, not the requested radius) | frustum/HiZ-independent, bounded, not full-world\n",
            rs.lighting.raySceneRadius, rs.lighting.rayGrassRadius,
            rs.lighting.rayGrassRadius > 0.0f ? "" : " (radius 0)", grassCoverage);
        text += line;
        xr_sprintf(line, sizeof(line), "RTGI rays: world-only (HUD excluded from occlusion/reflection) | primary geometric normal = resolved shading normal (approximation) | raw multibounce, unreconstructed, uncached, unclamped\n");
        text += line;
    }
    xr_sprintf(line, sizeof(line), "history: surfaces=%s | used=%s | RT scene revision=%llu | RT pose revision=%llu | PT recorded samples=%u\n",
        rs.lighting.previousSurfacesValid ? "valid" : "rejected", rs.lighting.historyUsed ? "yes" : "no",
        static_cast<unsigned long long>(rs.lighting.sceneRevision), static_cast<unsigned long long>(rs.lighting.poseRevision),
        rs.lighting.recordedSamples);
    text += line;
    if (rs.lighting.requested == render::fg::LightingMode::ReferencePT)
    {
        xr_sprintf(line, sizeof(line), "PT reference: diagnostic=%u %s | bounces=%u | diffuse=%u | null events=%u | sample cap=%u%s | sun radius=%.2f deg%s\n",
            rs.pathTracerDiagnosticMode, PathTracerDiagnosticName(rs.pathTracerDiagnosticMode), rs.pathTracerBounces,
            rs.pathTracerDiffuseMode, rs.pathTracerMaxNullEvents, rs.pathTracerMaxSamples,
            rs.pathTracerMaxSamples ? "" : " (uncapped)", rad2deg(rs.pathTracerSunAngularRadius),
            rs.pathTracerSunAngularRadius > 0.0f ? "" : " (delta sun)");
        text += line;
        xr_sprintf(line, sizeof(line), "PT scene: %s | snapshot requested=%s pending=%s valid=%s fallback=%s frozen=%s | cdf=%s\n",
            rs.pathTracerFreezeRequested ?
                (rs.pathTracerSnapshotValid ? "frozen snapshot" : (rs.pathTracerCapturePending ? "frozen capture awaiting completion" : "live fallback (no snapshot)")) :
                "live scene (freeze off)",
            rs.pathTracerFreezeRequested ? "yes" : "no", rs.pathTracerCapturePending ? "yes" : "no",
            rs.pathTracerSnapshotValid ? "yes" : "no", rs.pathTracerSnapshotFallback ? "yes" : "no",
            rs.pathTracerFrozen ? "yes" : "no", rs.pathTracerCdfActive ? "active" : "inactive");
        text += line;
        xr_sprintf(line, sizeof(line), "PT scene contents: %u lights | %u emitters (%s)\n",
            rs.pathTracerLightCount, rs.pathTracerEmissiveCount, rs.pathTracerFrozen ? "frozen snapshot" : "live scene");
        text += line;
        if (rs.pathTracerFrozen)
            xr_sprintf(line, sizeof(line), "PT frozen revision: scene %llu | textures %llu | lighting %016llx | cloned textures %u (%s) | retained scene %s\n",
                static_cast<unsigned long long>(rs.pathTracerFrozenSceneRevision),
                static_cast<unsigned long long>(rs.pathTracerFrozenTextureRevision),
                static_cast<unsigned long long>(rs.pathTracerLightingSignature),
                rs.pathTracerCapturedTextures, FormatBytes(rs.pathTracerCapturedTextureBytes, 0),
                FormatBytes(rs.pathTracerRetainedSceneBytes, 1));
        else
            xr_sprintf(line, sizeof(line), "PT frozen revision: none (live scene)\n");
        text += line;
        xr_sprintf(line, sizeof(line), "PT scope: finite depth (RR from depth 3, ray reach 10000 m), per-ray null-event budget with 64x candidate bound, unreconstructed, uncached, unclamped | supported: admitted/resident RT geometry + bounded grass | deferred: dielectric refraction, volumetric absorption, particle/volume transport | live raster effect overlays excluded\n");
        text += line;
    }
    xr_sprintf(line, sizeof(line), "clusters: %u/%u visible (terrain %u/%u) | tris %u+%u | occl cand %u rec %u | residual S%u T%u D%u X%u | vsm %s\n",
        rs.clusterVisible, rs.clusterStaticEntries, rs.clusterTerrainVisible, rs.clusterTerrainEntries,
        rs.clusterTrianglesDrawn, rs.clusterTerrainTrianglesDrawn, rs.clusterOcclusionCandidates, rs.clusterOcclusionRecovered,
        rs.residualStatic, rs.residualTerrain, rs.residualDynamic, rs.residualTransparent,
        rs.vsmActive ? (rs.vsmSunMoving ? "moving" : "active") : "off");
    text += line;

    xr_sprintf(line, sizeof(line), "hierarchy: %u instances, %u nodes, %u leaf refs | deferred %u inst %u nodes | overflow %u\n",
        rs.clusterInstanceVisits, rs.clusterNodeVisits, rs.clusterLeafVisits,
        rs.clusterDeferredInstances, rs.clusterDeferredNodes, rs.clusterOverflow);
    text += line;
    xr_sprintf(line, sizeof(line), "geometry allocation MiB: shared %.2f | instance/ref/BVH %.2f | payload %.2f | vertices %.2f | retained forward %.2f | pages %u\n",
        rs.geometrySharedBytes / 1048576.0, rs.geometryInstanceBytes / 1048576.0,
        rs.geometryPayloadBytes / 1048576.0, rs.geometryVertexBytes / 1048576.0,
        rs.geometryRetainedBytes / 1048576.0, rs.geometryPages);
    text += line;
    xr_sprintf(line, sizeof(line), "geometry ownership MiB: forward draws %.2f | retiring forward %.2f | retiring arenas %.2f | source staging %.2f | source hosts %.2f | forward leases %u | shadow snapshots %.2f readback %.2f host\n",
        rs.geometryForwardDrawBytes / 1048576.0, rs.geometryRetiringSourceBytes / 1048576.0,
        rs.geometryRetiringArenaBytes / 1048576.0, rs.geometrySourceStagingBytes / 1048576.0,
        rs.geometryHostSourceBytes / 1048576.0, rs.geometryForwardUploadLeases,
        rs.geometryShadowSnapshotBytes / 1048576.0, rs.geometryShadowHostBytes / 1048576.0);
    text += line;
    xr_sprintf(line, sizeof(line), "exact RT MiB: source %.2f | generations %.2f | AS %.2f (%s) | compacted %u/%u | generations %u | leases %u\n",
        rs.geometryRTSourceBytes / 1048576.0, rs.geometryRTGenerationBytes / 1048576.0,
        rs.geometryRTAccelerationBytes / 1048576.0, rs.geometryRTAccelerationKnown ? "known" : "partial/unknown",
        rs.geometryRTCompacted, rs.geometryRTCompactable, rs.geometryRTGenerations, rs.geometryRTLeases);
    text += line;
    xr_sprintf(line, sizeof(line), "paging level policy: requested %u | fine budget %u MiB | reload pending %u\n",
        u32(rs.geometryPagingRequested), rs.geometryPageBudgetMiB, u32(rs.geometryPolicyPending));
    text += line;
    xr_sprintf(line, sizeof(line), "geometry vertices: page %llu | unique %llu | cluster %llu | maxima %u vertices %u triangles | recluster %u\n",
        static_cast<unsigned long long>(rs.geometryPageVertexSlots),
        static_cast<unsigned long long>(rs.geometryUniqueVertices),
        static_cast<unsigned long long>(rs.geometryClusterVertexReferences),
        rs.geometryMaxClusterVertices, rs.geometryMaxClusterTriangles, rs.geometryReclusterSplits);
    text += line;
    xr_sprintf(line, sizeof(line), "geometry residency [%s]: arena %.2f/%.2f MiB | pinned %.2f MiB | staging %.2f MiB | cut revision %u\n",
        rs.geometryStreaming ? "demand paged" : "fully resident",
        rs.geometryResidencyUsedBytes / 1048576.0, rs.geometryResidencyArenaBytes / 1048576.0,
        rs.geometryResidencyPinnedBytes / 1048576.0, rs.geometryResidencyStagingBytes / 1048576.0,
        rs.geometryCutRevision);
    text += line;
    xr_sprintf(line, sizeof(line), "geometry groups: %u resident (%u pinned) | %u desired | %u activating | %u blocked | %u evicted\n",
        rs.geometryResidentGroups, rs.geometryPinnedGroups, rs.geometryDesiredGroups,
        rs.geometryActivatingGroups, rs.geometryBlockedGroups, rs.geometryEvictions);
    text += line;
    xr_sprintf(line, sizeof(line), "geometry pages: %u resident (%u pinned) | %u reading | %u uploading | %u retiring | uploads %u (%u KiB) | reads %u (%u failed) | discarded %u | deferrals %u alloc %u budget | snapshots %u live %u failed\n",
        rs.geometryResidentPages, rs.geometryPinnedPages, rs.geometryReadingPages,
        rs.geometryUploadingPages, rs.geometryRetiringPages, rs.geometryUploadsRecorded,
        rs.geometryUploadKiB, rs.geometryReadsIssued, rs.geometryReadsFailed,
        rs.geometryUploadsDiscarded, rs.geometryAllocationDeferrals, rs.geometryBudgetDeferrals,
        rs.geometryLiveSnapshots, rs.geometryFailedSnapshots);
    text += line;

    if (m_gpuProfiler && m_gpuProfiler->IsInitialized())
    {
        const auto& passes = m_gpuProfiler->GetPassTimings();
        const float totalGPU = m_gpuProfiler->GetTotalGPUTimeMs();
        float asyncTotal = 0.0f, graphicsTotal = 0.0f;
        for (const auto& pass : passes)
        {
            if (strchr(pass.name.c_str(), '.'))
                continue;
            (pass.isAsync ? asyncTotal : graphicsTotal) += pass.timeMs;
        }
        xr_sprintf(line, sizeof(line), "\nGPU total %.3f ms | async %.3f ms | graphics %.3f ms | sample %llu\n",
            totalGPU, asyncTotal, graphicsTotal, (unsigned long long)m_gpuProfiler->GetCompletedSampleId());
        text += line;
        if (HasGPUPassNamed(passes, "RTGI Raw Transport [diagnostic]"))
        {
            text += "RTGI Raw Transport [diagnostic]: staged leaf timings include scratch traffic and synchronization\n";
            text += "  rows nest under their name prefix | repeated tile and sample scopes are summed per name and queue\n";
            text += "  only top-level rows contribute to the frame total; do not add parents and children\n";
            text += "  Primary/Step N light + shadows are inclusive groups: Sun, Local lights, Environment, Emissive + accumulate\n";
            text += "  emitter rows include scratch, serialization and timer overhead\n";
            text += "  Emissive + accumulate checks direct-light finiteness and applies all four groups, even with zero emissive emitters\n";
            text += "  group and child timers have separate boundaries, so their sums need not match exactly\n";
            text += "  Step N = one ray iteration, water may not advance a bounce | Tail + resolve finishes remaining water paths and writes guides\n";
            text += "  not phase attribution for the normal single-dispatch kernel\n";
        }
        if (FormatQueueTimingsLine(line, sizeof(line)))
        {
            text += line;
            text += "\n";
        }
        for (int asyncOnly = 1; asyncOnly >= 0; --asyncOnly)
        {
            text += asyncOnly ? "[async]\n" : "[graphics]\n";

            xr_vector<GPUPassTiming> visiblePasses;
            for (const auto& pass : passes)
            {
                if (pass.isAsync != (asyncOnly != 0) || pass.pending)
                    continue;
                visiblePasses.push_back(pass);
            }

            xr_vector<GPUPassNode> nodes;
            xr_vector<u32> roots;
            BuildGPUPassTree(visiblePasses, nodes, roots, asyncOnly != 0, true);

            for (u32 root : roots)
                AppendGPUPassNode(text, nodes, root, totalGPU, nodes[root].name.find('.') == xr_string::npos ? 0 : 1);
        }
    }

    CPUProfiler& profiler = GetCPUProfiler();
    xr_sprintf(line, sizeof(line), "\nCPU total %.3f ms\n", profiler.GetFrameTimeMs());
    text += line;
    const auto& zones = profiler.GetZones();
    for (u32 rootId : profiler.GetRootZones())
        AppendZoneText(text, rootId, zones, 1);
    char submitLine[320];
    if (FormatSubmitThreadLine(submitLine, sizeof(submitLine)))
    {
        text += submitLine;
        text += "\n";
    }

    string_path path;
    FS.update_path(path, "$logs$", "gpu_profile.txt");
    if (IWriter* writer = FS.w_open(path))
    {
        writer->w(text.data(), static_cast<u32>(text.size()));
        FS.w_close(writer);
    }
}

void StatsOverlay::RenderAllocationsSection()
{
    ImGui::SetNextItemOpen(m_allocExpanded, ImGuiCond_Once);
    if (!ImGui::CollapsingHeader("Allocations"))
    {
        m_allocExpanded = false;
        return;
    }
    m_allocExpanded = true;

    const auto& rep = memstats::Report();
    if (!rep.valid)
    {
        ImGui::TextDisabled("No allocation data yet (records in-game frames only)");
        return;
    }

    ImGui::Text("Main thread: %s allocs (%s)",
        FormatNumber((u32)rep.mainCalls), FormatBytes(rep.mainBytes, 0));
    {
        const bool pos = rep.mainBytes >= rep.mainFreeBytes;
        const u64 net = pos ? rep.mainBytes - rep.mainFreeBytes : rep.mainFreeBytes - rep.mainBytes;
        ImGui::TextDisabled("             %s frees (%s), net %s%s",
            FormatNumber((u32)rep.mainFrees), FormatBytes(rep.mainFreeBytes, 1),
            pos ? "+" : "-", FormatBytes(net, 2));
    }

    ImGui::Text("All threads: %s allocs (%s)",
        FormatNumber((u32)rep.allCalls), FormatBytes(rep.allBytes, 0));
    {
        const bool pos = rep.allBytes >= rep.allFreeBytes;
        const u64 net = pos ? rep.allBytes - rep.allFreeBytes : rep.allFreeBytes - rep.allBytes;
        ImGui::TextDisabled("             %s frees (%s), net %s%s",
            FormatNumber((u32)rep.allFrees), FormatBytes(rep.allFreeBytes, 1),
            pos ? "+" : "-", FormatBytes(net, 2));
    }

    char avgCallsStr[32];
    xr_strcpy(avgCallsStr, sizeof(avgCallsStr), FormatNumber((u32)rep.avgMainCalls));
    ImGui::TextDisabled("avg %s / %s    peak %s / %s    (main, 120-frame)",
        avgCallsStr, FormatBytes(rep.avgMainBytes, 0),
        FormatNumber((u32)rep.peakMainCalls), FormatBytes(rep.peakMainBytes, 1));

    if (m_renderStats.fgArenaCapacity > 0)
    {
        ImGui::Text("FG arena: %s / %s (peak %s)",
            FormatBytes(m_renderStats.fgArenaUsed, 0),
            FormatBytes(m_renderStats.fgArenaCapacity, 1),
            FormatBytes(m_renderStats.fgArenaPeak, 2));
        if (m_renderStats.fgArenaFallbacks > 0)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 90, 90, 255));
            ImGui::Text("%u heap fallbacks", m_renderStats.fgArenaFallbacks);
            ImGui::PopStyleColor();
        }
    }

    if (rep.disallowViolations > 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 90, 90, 255));
        ImGui::Text("DisallowHeapAlloc violations: %llu (see log)",
            (unsigned long long)rep.disallowViolations);
        ImGui::PopStyleColor();
    }

    bool hist = memstats::HistogramEnabled();
    if (ImGui::Checkbox("Size histogram", &hist))
        memstats::SetHistogramEnabled(hist);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("pow2 buckets, all threads\nadds two atomic adds to every allocation while enabled");

    if (hist)
    {
        if (ImGui::BeginTable("alloc_hist", 3,
            ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Allocs");
            ImGui::TableSetupColumn("Bytes");
            ImGui::TableHeadersRow();

            u64 threshold = 16;
            for (int b = 0; b < memstats::histBucketCount; ++b)
            {
                if (rep.histCalls[b] != 0)
                {
                    char sizeLabel[40];
                    if (b == memstats::histBucketCount - 1)
                        xr_sprintf(sizeLabel, sizeof(sizeLabel), "> %s", FormatBytes(threshold / 2, 3));
                    else
                        xr_sprintf(sizeLabel, sizeof(sizeLabel), "<= %s", FormatBytes(threshold, 3));

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(sizeLabel);
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(FormatNumber((u32)rep.histCalls[b]));
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(FormatBytes(rep.histBytes[b], 4));
                }
                threshold <<= 1;
            }
            ImGui::EndTable();
        }
    }

    if (memstats::BacktraceCaptureSupported())
    {
        ImGui::Separator();

        if (memstats::BacktraceCaptureArmed())
        {
            const char* armedName = memstats::ArmedZoneName();
            ImGui::TextDisabled("Backtrace capture armed: %s (waiting for a sampled frame)...",
                armedName ? armedName : "?");
            ImGui::SameLine();
            if (ImGui::SmallButton("disarm"))
                memstats::DisarmBacktraceCapture();
        }
        else
        {
            ImGui::TextDisabled("Right-click a CPU zone row to capture alloc backtraces");
            ImGui::SameLine();
            if (ImGui::SmallButton("capture unattributed"))
                memstats::ArmBacktraceCapture(memstats::noZone, "outside any zone");
        }

        static char s_armZoneName[64] = "";
        static u32 s_armMatches[64];
        static int s_armMatchCount = 0;
        ImGui::SetNextItemWidth(200.0f);
        bool armRequested = ImGui::InputText("##btarmname", s_armZoneName, sizeof(s_armZoneName),
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        armRequested |= ImGui::SmallButton("arm by name");
        if (armRequested && s_armZoneName[0])
        {
            const auto& zones = GetCPUProfiler().GetZones();
            s_armMatchCount = 0;
            for (u32 i = 0; i < zones.size() && s_armMatchCount < 64; ++i)
            {
                if (zones[i].info && 0 == std::strcmp(zones[i].info->name, s_armZoneName))
                    s_armMatches[s_armMatchCount++] = i;
            }
            std::sort(s_armMatches, s_armMatches + s_armMatchCount, [&](u32 a, u32 b) {
                if (zones[a].timing.allocCalls != zones[b].timing.allocCalls)
                    return zones[a].timing.allocCalls > zones[b].timing.allocCalls;
                return a < b;
            });
            if (s_armMatchCount == 1)
            {
                memstats::ArmBacktraceCapture(zones[s_armMatches[0]].zoneId, zones[s_armMatches[0]].info->name);
                s_armMatchCount = 0;
            }
            else if (s_armMatchCount > 1)
            {
                ImGui::OpenPopup("##btarmpick");
            }
        }
        if (ImGui::BeginPopup("##btarmpick"))
        {
            const auto& zones = GetCPUProfiler().GetZones();
            for (int m = 0; m < s_armMatchCount; ++m)
            {
                const u32 id = s_armMatches[m];
                if (id >= zones.size() || !zones[id].info)
                    continue;
                const char* file = zones[id].info->file ? zones[id].info->file : "?";
                const char* base = file;
                for (const char* p = file; *p; ++p)
                    if (*p == '/' || *p == '\\')
                        base = p + 1;
                char label[160];
                xr_sprintf(label, sizeof(label), "%s - %s:%u  (%u allocs)##armpick%d",
                    zones[id].info->name, base, zones[id].info->line,
                    (u32)zones[id].timing.allocCalls, m);
                if (ImGui::MenuItem(label))
                {
                    memstats::ArmBacktraceCapture(zones[id].zoneId, zones[id].info->name);
                    s_armMatchCount = 0;
                }
            }
            ImGui::EndPopup();
        }

        const auto& bt = memstats::GetBacktraceReport();
        if (bt.ready)
        {
            if (bt.automatic)
            {
                ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f),
                    "Allocation spike: %llu baseline -> %llu observed (trigger > %llu)",
                    static_cast<unsigned long long>(bt.baselineCalls),
                    static_cast<unsigned long long>(bt.observedCalls),
                    static_cast<unsigned long long>(bt.thresholdCalls));
                ImGui::TextDisabled("Sample %llu: %llu post-threshold stacks captured%s",
                    static_cast<unsigned long long>(bt.sample),
                    static_cast<unsigned long long>(bt.capturedCalls),
                    bt.captureLimitReached ? " (capture limit reached)" : "");
            }
            char header[128];
            xr_sprintf(header, sizeof(header), "Backtraces: %s (%s allocs, %d sites)###bt",
                bt.zoneName ? bt.zoneName : "?", FormatNumber((u32)bt.totalCalls), bt.siteCount);
            const bool open = ImGui::TreeNode(header);
            if (ImGui::BeginPopupContextItem("backtraces_ctx"))
            {
                if (ImGui::MenuItem("Copy all to clipboard"))
                {
                    u64 totalBytes = 0;
                    for (int i = 0; i < bt.siteCount; ++i)
                        totalBytes += bt.sites[i].bytes;

                    xr_string text;
                    text.reserve(16384);
                    text += "Backtraces: ";
                    text += bt.zoneName ? bt.zoneName : "?";
                    char line[256];
                    xr_sprintf(line, sizeof(line), "\nTotal: %llu allocs, %llu bytes, %d sites\n",
                        static_cast<unsigned long long>(bt.totalCalls),
                        static_cast<unsigned long long>(totalBytes), bt.siteCount);
                    text += line;
                    if (bt.automatic)
                    {
                        xr_sprintf(line, sizeof(line),
                            "Automatic allocation spike (tail capture)\nSample: %llu\nBaseline: %llu allocs\n"
                            "Trigger: >%llu allocs\nObserved: %llu allocs\nCaptured: %llu stacks\n",
                            static_cast<unsigned long long>(bt.sample),
                            static_cast<unsigned long long>(bt.baselineCalls),
                            static_cast<unsigned long long>(bt.thresholdCalls),
                            static_cast<unsigned long long>(bt.observedCalls),
                            static_cast<unsigned long long>(bt.capturedCalls));
                        text += line;
                        if (bt.captureLimitReached)
                            text += "Trace capture limit reached.\n";
                    }
                    for (int i = 0; i < bt.siteCount; ++i)
                    {
                        const auto& site = bt.sites[i];
                        xr_sprintf(line, sizeof(line), "\nSite %d: %llu allocs, %llu bytes\n",
                            i + 1, static_cast<unsigned long long>(site.count),
                            static_cast<unsigned long long>(site.bytes));
                        text += line;
                        for (int f = 0; f < site.depth; ++f)
                        {
                            xr_sprintf(line, sizeof(line), "  %d: ", f);
                            text += line;
                            text += site.frames[f];
                            text += "\n";
                        }
                    }
                    ImGui::SetClipboardText(text.c_str());
                }
                ImGui::EndPopup();
            }
            if (open)
            {
                for (int i = 0; i < bt.siteCount; ++i)
                {
                    const auto& site = bt.sites[i];
                    char label[224];
                    const char* top = site.depth > 0 ? site.frames[0] : "?";
                    xr_sprintf(label, sizeof(label), "%s x  %.140s  (%s)###btsite%d",
                        FormatNumber((u32)site.count), top, FormatBytes(site.bytes, 0), i);
                    if (ImGui::TreeNode(label))
                    {
                        for (int f = 0; f < site.depth; ++f)
                            ImGui::TextUnformatted(site.frames[f]);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    ImGui::Separator();
    bool objProfiling = m_allocObjectClassProfiling;
    if (ImGui::Checkbox("Profile per-object-class allocs (UpdateCL)", &objProfiling))
    {
        m_allocObjectClassProfiling = objProfiling;
        memstats::SetObjectClassProfiling(objProfiling);
    }

    if (m_allocObjectClassProfiling && ImGui::TreeNode("Object classes (this frame)"))
    {
        const int t = static_cast<int>(memstats::Table::ObjectClass);
        const int used = rep.namedUsed[t];

        int order[memstats::namedCapacity];
        for (int i = 0; i < used; ++i)
            order[i] = i;
        std::sort(order, order + used, [&](int a, int b) {
            return rep.named[t][a].bytes > rep.named[t][b].bytes;
        });

        const int show = used < 25 ? used : 25;
        if (ImGui::BeginTable("alloc_objclass", 3,
            ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Class");
            ImGui::TableSetupColumn("Allocs");
            ImGui::TableSetupColumn("Bytes");
            ImGui::TableHeadersRow();

            for (int i = 0; i < show; ++i)
            {
                const auto& e = rep.named[t][order[i]];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(e.key ? e.key : "?");
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatNumber((u32)e.calls));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatBytes(e.bytes, 0));
            }
            ImGui::EndTable();
        }
        if (used > show)
            ImGui::TextDisabled("... and %d more classes", used - show);
        ImGui::TreePop();
    }
}

} // namespace xray::profiler
