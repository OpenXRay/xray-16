#pragma once
#include "pch.hpp"

class CScriptProfilerHookPortion {
    using Clock = std::chrono::high_resolution_clock;
    using Time = Clock::time_point;
    using Duration = Clock::duration;

    private:
        u64 m_calls_count;
        u32 m_calls_active;

        Time m_started_at;
        Duration m_duration;

    public:
        CScriptProfilerHookPortion(): m_calls_count(0), m_calls_active(0), m_duration(0), m_started_at() {}

    void start()
    {
        m_calls_count += 1;

        if (m_calls_active)
        {
            m_calls_active += 1;
            return;
        }
        else
        {
            m_started_at = Clock::now();
            m_calls_active += 1;
        }
    }

    void stop()
    {
        if (!m_calls_active)
            return;

        m_calls_active -= 1;

        if (m_calls_active)
            return;

        const auto now = Clock::now();

        if (now > m_started_at)
            m_duration += now - m_started_at;
    }

    u64 count() const { return m_calls_count; }

    u64 duration() const
    {
        using namespace std::chrono;
        return u64(duration_cast<microseconds>(m_duration).count());
    }
};

class CScriptProfilerSamplingPortion {
    using Clock = std::chrono::high_resolution_clock;
    using Time = Clock::time_point;

     public:
        Time m_recoreded_at;

        int m_memory;
        int m_samples;
        int m_state;
        shared_str m_name;
        shared_str m_trace;

        CScriptProfilerSamplingPortion(shared_str name, shared_str trace, int samples, int state, int memory)
            : m_name(name), m_trace(trace), m_memory(memory), m_samples(samples), m_state(state),
              m_recoreded_at(Clock::now()) {}

        CScriptProfilerSamplingPortion(const CScriptProfilerSamplingPortion& rhs)
            : m_name(rhs.m_name), m_trace(rhs.m_trace), m_memory(rhs.m_memory), m_samples(rhs.m_samples),
              m_state(rhs.m_state), m_recoreded_at(rhs.m_recoreded_at) {}

        CScriptProfilerSamplingPortion cloned() const { return CScriptProfilerSamplingPortion(*this); }

        // Build flamechart folded stack including frames and samples count
        // Example: `C;frame_1_func:24;frame_2_func:45 4`
        shared_str getFoldedStack()
        {
            string2048 buffer;
            xr_sprintf(buffer, "%c;%s %d", m_state, m_trace.c_str(), m_samples);

            return shared_str(buffer);
        }
};
