#include "stdafx.h"
#include "PBRConversionUI.h"
#include "xrEngine/device.h"
#include <imgui.h>
#include <chrono>

namespace xray::render::pbr {

static constexpr float UI_LINGER_SECONDS = 6.0f;

static double NowSeconds()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

ConversionProgress& ConversionProgress::Get()
{
    static ConversionProgress instance;
    return instance;
}

void ConversionProgress::BeginJob()
{
    m_total.store(0, std::memory_order_relaxed);
    m_converted.store(0, std::memory_order_relaxed);
    m_skipped.store(0, std::memory_order_relaxed);
    m_failed.store(0, std::memory_order_relaxed);
    {
        ScopeLock lock{ &m_string_lock };
        m_phase = "Starting...";
        m_current_item.clear();
        m_job_start_time = NowSeconds();
        m_job_end_time = -1.0;
    }
    m_job_active.store(true, std::memory_order_release);
}

void ConversionProgress::EndJob()
{
    if (!m_job_active.load(std::memory_order_acquire))
        return;
    {
        ScopeLock lock{ &m_string_lock };
        m_phase = "Done";
        m_current_item.clear();
        m_job_end_time = NowSeconds();
    }
    m_job_active.store(false, std::memory_order_release);
}

void ConversionProgress::BeginPhase(pcstr phase_name, u32 total)
{
    m_total.store(total, std::memory_order_relaxed);
    m_converted.store(0, std::memory_order_relaxed);
    m_skipped.store(0, std::memory_order_relaxed);
    m_failed.store(0, std::memory_order_relaxed);

    ScopeLock lock{ &m_string_lock };
    m_phase = phase_name ? phase_name : "";
    m_current_item.clear();
}

void ConversionProgress::SetCurrentItem(pcstr name)
{
    ScopeLock lock{ &m_string_lock };
    m_current_item = name ? name : "";
}

ConversionProgress::Snapshot ConversionProgress::GetSnapshot()
{
    Snapshot snap;
    snap.job_active = m_job_active.load(std::memory_order_acquire);
    snap.total = m_total.load(std::memory_order_relaxed);
    snap.converted = m_converted.load(std::memory_order_relaxed);
    snap.skipped = m_skipped.load(std::memory_order_relaxed);
    snap.failed = m_failed.load(std::memory_order_relaxed);

    ScopeLock lock{ &m_string_lock };
    snap.phase = m_phase;
    snap.current_item = m_current_item;

    const double now = NowSeconds();
    if (snap.job_active)
        snap.job_elapsed_seconds = static_cast<float>(now - m_job_start_time);
    else if (m_job_end_time >= 0.0)
    {
        snap.job_elapsed_seconds = static_cast<float>(m_job_end_time - m_job_start_time);
        snap.seconds_since_job_end = static_cast<float>(now - m_job_end_time);
    }

    return snap;
}

void RenderConversionProgressUI()
{
    auto snap = ConversionProgress::Get().GetSnapshot();

    const bool lingering = !snap.job_active &&
        snap.seconds_since_job_end >= 0.f &&
        snap.seconds_since_job_end < UI_LINGER_SECONDS;

    if (!snap.job_active && !lingering)
        return;

    ImGui::SetCurrentContext(Device.GetImGuiContext());

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 10.f, viewport->WorkPos.y + 10.f),
        ImGuiCond_FirstUseEver, ImVec2(1.f, 0.f));
    ImGui::SetNextWindowSize(ImVec2(420.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.85f);

    const bool ideActive = Device.editor().IsActiveState();

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (!ideActive)
        flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing;

    if (!ImGui::Begin("PBR Texture Conversion", nullptr, flags))
    {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted(snap.phase.c_str());

    const u32 done = snap.converted + snap.skipped + snap.failed;

    if (snap.total > 0)
    {
        const float fraction = static_cast<float>(done) / static_cast<float>(snap.total);
        char overlay[64];
        xr_sprintf(overlay, sizeof(overlay), "%u / %u", done, snap.total);
        ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0.f), overlay);
    }
    else if (snap.job_active)
    {
        ImGui::ProgressBar(-1.f * static_cast<float>(ImGui::GetTime()), ImVec2(-FLT_MIN, 0.f), "...");
    }

    if (!snap.current_item.empty())
        ImGui::Text("Current: %s", snap.current_item.c_str());

    ImGui::Text("Converted: %u   Skipped: %u", snap.converted, snap.skipped);
    if (snap.failed > 0)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "  Failed: %u", snap.failed);
    }

    const u32 elapsed = static_cast<u32>(snap.job_elapsed_seconds);
    if (snap.job_active && snap.total > 0 && snap.converted > 0 && done > 0)
    {
        const float remaining = snap.job_elapsed_seconds / done * (snap.total - done);
        ImGui::TextDisabled("Elapsed: %um %02us   ETA: ~%um %02us",
            elapsed / 60, elapsed % 60,
            static_cast<u32>(remaining) / 60, static_cast<u32>(remaining) % 60);
    }
    else
    {
        ImGui::TextDisabled("Elapsed: %um %02us", elapsed / 60, elapsed % 60);
    }

    ImGui::End();
}

}
