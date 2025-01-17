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
        Time tracked_at;

    	int memory;
        int samples;
        int state;
        shared_str name;
        shared_str trace;

        CScriptProfilerSamplingPortion(shared_str name, shared_str trace, int samples, int state, int memory)
            : name(name), trace(trace), memory(memory), samples(samples), state(state), tracked_at(Clock::now()) {}

        // Build flamechart folded stack including frames and samples count
        // Example: `C;frame_1_func:24;frame_2_func:45 4`
        shared_str getFoldedStack()
        {
            string2048 buffer;
            xr_sprintf(buffer, "%c;%s %d", state, trace.c_str(), samples);

        	return shared_str(buffer);
        }
};
