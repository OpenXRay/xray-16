#pragma once

#include "pch.hpp"
#include "xrCommon/xr_unordered_map.h"

struct lua_State;
struct lua_Debug;

class CScriptProfilerPortion {
    using Clock = std::chrono::high_resolution_clock;
    using Time = Clock::time_point;
    using Duration = Clock::duration;

    private:
        u64 m_calls_count;
        u32 m_calls_active;

        Time m_started_at;
        Duration m_duration;

    public:
        CScriptProfilerPortion(): m_calls_count(0), m_calls_active(0), m_duration(0), m_started_at() {}

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

class XRSCRIPTENGINE_API CScriptProfiler
{
private:
    static const u32 PROFILE_ENTRIES_LOG_LIMIT = 128;
    
    // Profiling level - number of stacks to check before each function call.
    // Helps validating results of same functions called from different places vs totals by specific function.
    u8 m_profile_level;
    bool m_active;
    xr_unordered_map<shared_str, CScriptProfilerPortion> m_profiling_portions;

public:
    CScriptProfiler();
    virtual ~CScriptProfiler();

    bool isActive() const { return m_active; };

    void start();
    void stop();
    void reset();
    void log();
    void save();

    void onLuaHookCall(lua_State* L, lua_Debug* dbg);
};
