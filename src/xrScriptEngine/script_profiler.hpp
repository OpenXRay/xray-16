#pragma once

#include "pch.hpp"
#include "xrCommon/xr_unordered_map.h"

struct lua_State;
struct lua_Debug;

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
    // todo: Can we make some global module to store all the arguments as experessions?
    // List of commnad line args for startup profuler attach:
    constexpr static cpcstr ARGUMENT_PROFILER_DEFAULT = "-lua_profiler";
    constexpr static cpcstr ARGUMENT_PROFILER_HOOK = "-lua_hook_profiler";
    constexpr static cpcstr ARGUMENT_PROFILER_SAMPLING = "-lua_sampling_profiler";

    static const u32 PROFILE_ENTRIES_LOG_LIMIT = 128;
    static const u32 PROFILE_HOOK_DEPTH_DEFAULT = 1;
    static const u32 PROFILE_HOOK_DEPTH_MAX = 1;
    static const u32 PROFILE_SAMPLING_INTERVAL_DEFAULT = 10;
    static const u32 PROFILE_SAMPLING_INTERVAL_MAX = 1000;

    CScriptEngine* m_engine;
    CScriptProfilerType m_profiler_type;
    bool m_active;

    /*
     * Profiling depth - number of stacks to summarise function call trace.
     * Helps validating results of same functions called from different places vs totals by specific function.
     */
	u32 m_hook_profile_depth;
    /*
     * Sampling interval for JIT based profiler.
     * Value should be set in ms and defaults to 10ms.
     */
	u32 m_sampling_profile_interval;
    xr_unordered_map<shared_str, CScriptProfilerHookPortion> m_hook_profiling_portions;
    xr_vector<shared_str> m_sampling_profiling_log;

public:
    CScriptProfiler(CScriptEngine* engine);
    virtual ~CScriptProfiler();

    bool isActive() const { return m_active; };

	void start(CScriptProfilerType profiler_type = CScriptProfilerType::Hook);
    void startSamplingMode(u32 sampling_interval);
    void startHookMode(u32 stack_depth);
    void stop();
    void reset();

    void logReport();
    void logHookReport();
    void logSamplingReport();
    void saveReport();
    void saveHookReport();
    void saveSamplingReport();

   	void onReinit(lua_State* L);
	void onDispose(lua_State* L);
    void onLuaHookCall(lua_State* L, lua_Debug* dbg);

private:
    lua_State* lua() const;
    bool attachLuaHook();

   	static int luaMemoryUsed(lua_State* L);
    static bool luaIsJitProfilerDefined(lua_State* L);
    static void luaJitSamplingProfilerAttach(CScriptProfiler* profiler, u32 interval);
    static void luaJitProfilerStart(lua_State* L, cpcstr mode, luaJIT_profile_callback callback, void* data);
    static void luaJitProfilerStop(lua_State* L);
    static std::pair<cpcstr, size_t> luaJitProfilerDump(lua_State* L, cpcstr format, int depth);
};
