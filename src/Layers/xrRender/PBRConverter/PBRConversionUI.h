#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/Threading/Lock.hpp"
#include <atomic>

namespace xray::render::pbr {

class ConversionProgress
{
public:
    static ConversionProgress& Get();

    void BeginJob();
    void EndJob();

    void BeginPhase(pcstr phase_name, u32 total);

    void SetCurrentItem(pcstr name);

    void OnConverted() { m_converted.fetch_add(1, std::memory_order_relaxed); }
    void OnSkipped()   { m_skipped.fetch_add(1, std::memory_order_relaxed); }
    void OnFailed()    { m_failed.fetch_add(1, std::memory_order_relaxed); }

    struct Snapshot
    {
        bool job_active = false;
        float seconds_since_job_end = -1.f;
        float job_elapsed_seconds = 0.f;
        xr_string phase;
        xr_string current_item;
        u32 total = 0;
        u32 converted = 0;
        u32 skipped = 0;
        u32 failed = 0;
    };
    Snapshot GetSnapshot();

private:
    std::atomic<bool> m_job_active{ false };
    std::atomic<u32> m_total{ 0 };
    std::atomic<u32> m_converted{ 0 };
    std::atomic<u32> m_skipped{ 0 };
    std::atomic<u32> m_failed{ 0 };

    double m_job_start_time = 0.0;
    double m_job_end_time = -1.0;

    Lock m_string_lock;
    xr_string m_phase;
    xr_string m_current_item;
};

void RenderConversionProgressUI();

}
