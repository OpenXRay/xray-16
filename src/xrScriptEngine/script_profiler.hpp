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

enum class CScriptProfilerType : u32
{
    None = 0,
    Hook = 1,
    Sampling = 2,
};

class XRSCRIPTENGINE_API CScriptProfiler
{
    using Clock = std::chrono::high_resolution_clock;
    using Time = Clock::time_point;
    using Duration = Clock::duration;

private:
    static const u32 PROFILE_ENTRIES_LOG_LIMIT = 128;

    CScriptEngine* m_engine;

    bool m_active;

    // todo: Add period parameter for sampling profiler.

    // Profiling level - number of stacks to check before each function call.
    // Helps validating results of same functions called from different places vs totals by specific function.
 	// todo: Handle level / provide level properly.
	u8 m_hook_profile_level;
    CScriptProfilerType m_profiler_type;
    // todo: Profiler-type specific naming for containers.
    // todo: Profiler-type specific naming for containers.
    // todo: Profiler-type specific naming for containers.
    xr_unordered_map<shared_str, CScriptProfilerPortion> m_profiling_portions;
    xr_vector<shared_str> m_profiling_log;

public:
    CScriptProfiler(CScriptEngine* engine);
    virtual ~CScriptProfiler();

    bool isActive() const { return m_active; };

	void start(CScriptProfilerType profiler_type = CScriptProfilerType::Hook);
    void stop();
    void reset();

    void logReport();
    void logHookReport();
    void logSamplingReport();
    void saveReport();
    // todo: Save sampling report.
    // todo: Save hook.

    bool attachLuaHook();
   	void onReinit(lua_State* L);
	void onDispose(lua_State* L);
    void onLuaHookCall(lua_State* L, lua_Debug* dbg);

private:
    lua_State* lua() const;

    static int luaMemoryUsed(lua_State* L);
    static bool luaIsJitProfilerDefined(lua_State* L);
    static void luaJitSamplingProfilerAttach(CScriptProfiler* profiler);
    static void luaJitProfilerStart(lua_State* L, cpcstr mode, luaJIT_profile_callback callback, void* data);
    static void luaJitProfilerStop(lua_State* L);
    static std::pair<cpcstr, size_t> luaJitProfilerDump(lua_State* L, cpcstr format, int depth);
};
