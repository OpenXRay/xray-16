#pragma once

#include "xrCommon/xr_unordered_map.h"

#include "script_profiler_portions.hpp"

enum class CScriptProfilerType : u32
{
    None = 0,
    Hook = 1,
    Sampling = 2,
};

class XRSCRIPTENGINE_API CScriptProfiler
{
public:
    // List of command line args for startup profiler attach:
    // XXX: Can we make some global module to store all the arguments as expressions?
    static constexpr cpcstr ARGUMENT_PROFILER_DEFAULT = "-lua_profiler";
    static constexpr cpcstr ARGUMENT_PROFILER_HOOK = "-lua_hook_profiler";
    static constexpr cpcstr ARGUMENT_PROFILER_SAMPLING = "-lua_sampling_profiler";

    static constexpr CScriptProfilerType PROFILE_TYPE_DEFAULT = CScriptProfilerType::Hook;
    static constexpr u32 PROFILE_ENTRIES_LOG_LIMIT_DEFAULT = 128;
    static constexpr u32 PROFILE_SAMPLING_INTERVAL_DEFAULT = 10;
    static constexpr u32 PROFILE_SAMPLING_INTERVAL_MAX = 1000;

private:
    CScriptEngine* m_engine;
    CScriptProfilerType m_profiler_type;
    bool m_active;

    xr_unordered_map<shared_str, CScriptProfilerHookPortion> m_hook_profiling_portions;
    xr_vector<CScriptProfilerSamplingPortion> m_sampling_profiling_log;
    /*
     * Sampling interval for JIT based profiler.
     * Value should be set in ms and defaults to 10ms.
     */
    u32 m_sampling_profile_interval;

public:
    CScriptProfiler(CScriptEngine* engine);
    virtual ~CScriptProfiler();

    bool IsActive() const { return m_active; }
    CScriptProfilerType GetType() const { return m_profiler_type; }
    shared_str GetTypeString() const;
    size_t GetRecordsCount() const;

    void Start(CScriptProfilerType profiler_type = PROFILE_TYPE_DEFAULT);
    void StartHookMode();
    void StartSamplingMode(u32 sampling_interval = PROFILE_SAMPLING_INTERVAL_DEFAULT);
    void Stop();
    void Reset();
    void LogReport(u32 entries_limit = PROFILE_ENTRIES_LOG_LIMIT_DEFAULT);
    void LogHookReport(u32 entries_limit = PROFILE_ENTRIES_LOG_LIMIT_DEFAULT);
    void LogSamplingReport(u32 entries_limit = PROFILE_ENTRIES_LOG_LIMIT_DEFAULT);
    void SaveReport();
    void SaveHookReport(shared_str filename);
    void SaveSamplingReport(shared_str filename);
    shared_str GetHookReportFilename();
    shared_str GetSamplingReportFilename();

    bool AttachLuaHook();
    void OnReinit(lua_State* L);
    void OnDispose(lua_State* L);
    void OnLuaHookCall(lua_State* L, lua_Debug* dbg);

private:
    lua_State* lua() const;

    static int LuaMemoryUsed(lua_State* L);
    static bool LuaIsJitProfilerDefined();
    static void LuaJitSamplingProfilerAttach(CScriptProfiler* profiler, u32 interval);
    static void LuaJitProfilerStart(lua_State* L, cpcstr mode, luaJIT_profile_callback callback, void* data);
    static void LuaJitProfilerStop(lua_State* L);
    static shared_str LuaJitProfilerDumpToString(lua_State* L, cpcstr format, int depth);
    static std::pair<cpcstr, size_t> LuaJitProfilerDump(lua_State* L, cpcstr format, int depth);
    static std::pair<lua_Debug, bool> LuaDebugStackInfo(lua_State* L, int level, cpcstr what);
};
