#pragma once

#include "pch.hpp"
#include "script_profiler_portions.hpp"
#include "xrCommon/xr_unordered_map.h"

struct lua_State;
struct lua_Debug;

enum class CScriptProfilerType : u32
{
    None = 0,
    Hook = 1,
    Sampling = 2,
};

class XRSCRIPTENGINE_API CScriptProfiler
{
// todo: Can we make some global module to store all the arguments as experessions?
public:
    // List of commnad line args for startup profiler attach:
    constexpr static cpcstr ARGUMENT_PROFILER_DEFAULT = "-lua_profiler";
    constexpr static cpcstr ARGUMENT_PROFILER_HOOK = "-lua_hook_profiler";
    constexpr static cpcstr ARGUMENT_PROFILER_SAMPLING = "-lua_sampling_profiler";

    static const CScriptProfilerType PROFILE_TYPE_DEFAULT = CScriptProfilerType::Hook;
    static const u32 PROFILE_ENTRIES_LOG_LIMIT_DEFAULT = 128;
    static const u32 PROFILE_SAMPLING_INTERVAL_DEFAULT = 10;
    static const u32 PROFILE_SAMPLING_INTERVAL_MAX = 1000;

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

    bool isActive() const { return m_active; };
    CScriptProfilerType getType() const { return m_profiler_type; };
    shared_str getTypeString() const;
    u32 getRecordsCount() const;

    void start(CScriptProfilerType profiler_type = PROFILE_TYPE_DEFAULT);
    void startHookMode();
    void startSamplingMode(u32 sampling_interval = PROFILE_SAMPLING_INTERVAL_DEFAULT);
    void stop();
    void reset();
    void logReport(u32 entries_limit = PROFILE_ENTRIES_LOG_LIMIT_DEFAULT);
    void logHookReport(u32 entries_limit = PROFILE_ENTRIES_LOG_LIMIT_DEFAULT);
    void logSamplingReport(u32 entries_limit = PROFILE_ENTRIES_LOG_LIMIT_DEFAULT);
    void saveReport();
    void saveHookReport(shared_str filename);
    void saveSamplingReport(shared_str filename);
    shared_str getHookReportFilename();
    shared_str getSamplingReportFilename();

    bool attachLuaHook();
    void onReinit(lua_State* L);
    void onDispose(lua_State* L);
    void onLuaHookCall(lua_State* L, lua_Debug* dbg);

private:
    lua_State* lua() const;

    static int luaMemoryUsed(lua_State* L);
    static bool luaIsJitProfilerDefined();
    static void luaJitSamplingProfilerAttach(CScriptProfiler* profiler, u32 interval);
    static void luaJitProfilerStart(lua_State* L, cpcstr mode, luaJIT_profile_callback callback, void* data);
    static void luaJitProfilerStop(lua_State* L);
    static shared_str luaJitProfilerDumpToString(lua_State* L, cpcstr format, int depth);
    static std::pair<cpcstr, size_t> luaJitProfilerDump(lua_State* L, cpcstr format, int depth);
    static std::pair<lua_Debug, bool> luaDebugStackInfo(lua_State* L, int level, cpcstr what);
};
