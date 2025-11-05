#pragma once

#include "xrCore/xrCore.h"
#include <chrono>

namespace AnimationECS {

//-----------------------------------------------------------------------------
// PerformanceTimer
// High-resolution timer for measuring animation performance
//-----------------------------------------------------------------------------
class PerformanceTimer
{
public:
    using clock = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock>;
    using duration = std::chrono::duration<double, std::milli>;

    PerformanceTimer() { Reset(); }

    void Reset()
    {
        start_time = clock::now();
    }

    double ElapsedMs() const
    {
        auto end_time = clock::now();
        duration elapsed = end_time - start_time;
        return elapsed.count();
    }

private:
    time_point start_time;
};

//-----------------------------------------------------------------------------
// AnimationPerformanceStats
// Collects and reports animation system performance metrics
//-----------------------------------------------------------------------------
struct AnimationPerformanceStats
{
    // Timing data (milliseconds)
    double total_update_time{0.0};
    double sampling_time{0.0};
    double blending_time{0.0};
    double local_to_model_time{0.0};
    double callback_time{0.0};

    // Entity counts
    size_t total_entities{0};
    size_t active_entities{0};
    size_t blending_entities{0};

    // Frame counter
    u32 frame_count{0};

    // Execution mode
    bool parallel_mode{false};

    void Reset()
    {
        total_update_time = 0.0;
        sampling_time = 0.0;
        blending_time = 0.0;
        local_to_model_time = 0.0;
        callback_time = 0.0;
        total_entities = 0;
        active_entities = 0;
        blending_entities = 0;
        frame_count = 0;
    }

    void PrintStats() const
    {
        if (frame_count == 0)
        {
            Msg("[AnimationECS] No performance data collected");
            return;
        }

        const double avg_total = total_update_time / frame_count;
        const double avg_sampling = sampling_time / frame_count;
        const double avg_blending = blending_time / frame_count;
        const double avg_ltm = local_to_model_time / frame_count;
        const double avg_callback = callback_time / frame_count;
        const double avg_entities = static_cast<double>(total_entities) / frame_count;

        Msg("[AnimationECS] Performance Stats (%s mode)", parallel_mode ? "PARALLEL" : "SEQUENTIAL");
        Msg("  Frames: %u", frame_count);
        Msg("  Avg Entities: %.1f (%.1f active, %.1f blending)",
            avg_entities,
            static_cast<double>(active_entities) / frame_count,
            static_cast<double>(blending_entities) / frame_count);
        Msg("  Avg Total Time: %.3f ms", avg_total);
        Msg("  Avg Sampling Time: %.3f ms (%.1f%%)", avg_sampling, (avg_sampling / avg_total) * 100.0);
        Msg("  Avg Blending Time: %.3f ms (%.1f%%)", avg_blending, (avg_blending / avg_total) * 100.0);
        Msg("  Avg LocalToModel Time: %.3f ms (%.1f%%)", avg_ltm, (avg_ltm / avg_total) * 100.0);
        Msg("  Avg Callback Time: %.3f ms (%.1f%%)", avg_callback, (avg_callback / avg_total) * 100.0);
    }

    void SaveToFile(const char* filename) const
    {
        IWriter* writer = FS.w_open(filename);
        if (!writer)
        {
            Msg("! [AnimationECS] Failed to open performance log file: %s", filename);
            return;
        }

        const double avg_total = total_update_time / (frame_count > 0 ? frame_count : 1);
        const double avg_entities = static_cast<double>(total_entities) / (frame_count > 0 ? frame_count : 1);

        writer->w_printf("AnimationECS Performance Report\n");
        writer->w_printf("================================\n\n");
        writer->w_printf("Mode: %s\n", parallel_mode ? "PARALLEL" : "SEQUENTIAL");
        writer->w_printf("Frames: %u\n", frame_count);
        writer->w_printf("Avg Entities: %.2f\n", avg_entities);
        writer->w_printf("Avg Update Time: %.3f ms\n", avg_total);
        writer->w_printf("Avg Sampling: %.3f ms\n", sampling_time / (frame_count > 0 ? frame_count : 1));
        writer->w_printf("Avg Blending: %.3f ms\n", blending_time / (frame_count > 0 ? frame_count : 1));
        writer->w_printf("Avg LocalToModel: %.3f ms\n", local_to_model_time / (frame_count > 0 ? frame_count : 1));
        writer->w_printf("Avg Callbacks: %.3f ms\n", callback_time / (frame_count > 0 ? frame_count : 1));

        FS.w_close(writer);
        Msg("[AnimationECS] Performance report saved to: %s", filename);
    }
};

//-----------------------------------------------------------------------------
// PerformanceProfiler
// Global singleton for tracking animation performance
//-----------------------------------------------------------------------------
class PerformanceProfiler
{
private:
    AnimationPerformanceStats m_stats;
    bool m_enabled{false};
    u32 m_log_interval{300};  // Log every 300 frames (~5 seconds at 60fps)

    PerformanceProfiler() = default;
    ~PerformanceProfiler() = default;

    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

public:
    static PerformanceProfiler& Instance()
    {
        static PerformanceProfiler instance;
        return instance;
    }

    void Enable() { m_enabled = true; }
    void Disable() { m_enabled = false; }
    bool IsEnabled() const { return m_enabled; }

    void SetLogInterval(u32 frames) { m_log_interval = frames; }

    AnimationPerformanceStats& GetStats() { return m_stats; }
    const AnimationPerformanceStats& GetStats() const { return m_stats; }

    void Reset()
    {
        m_stats.Reset();
    }

    void EndFrame()
    {
        if (!m_enabled)
            return;

        m_stats.frame_count++;

        // Periodically log stats
        if (m_log_interval > 0 && (m_stats.frame_count % m_log_interval) == 0)
        {
            m_stats.PrintStats();
        }
    }

    void PrintFinalStats() const
    {
        if (m_enabled)
        {
            m_stats.PrintStats();
        }
    }

    void SaveReport(const char* filename) const
    {
        if (m_enabled)
        {
            m_stats.SaveToFile(filename);
        }
    }
};

//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------
inline PerformanceProfiler& GetPerformanceProfiler()
{
    return PerformanceProfiler::Instance();
}

} // namespace AnimationECS
