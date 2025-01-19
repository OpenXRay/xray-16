#include "pch.hpp"
#include "script_profiler.hpp"
#include "xrScriptEngine/script_engine.hpp"

CScriptProfiler::CScriptProfiler(CScriptEngine* engine)
{
    R_ASSERT(engine != NULL);

    m_engine = engine;
    m_active = false;
    m_profiler_type = CScriptProfilerType::None;
    m_sampling_profile_interval = PROFILE_SAMPLING_INTERVAL_DEFAULT;

    if (strstr(Core.Params, ARGUMENT_PROFILER_DEFAULT))
        start();
    else if (strstr(Core.Params, ARGUMENT_PROFILER_HOOK))
        start(CScriptProfilerType::Hook);
    else if (strstr(Core.Params, ARGUMENT_PROFILER_SAMPLING))
        start(CScriptProfilerType::Sampling);
}

CScriptProfiler::~CScriptProfiler()
{
    m_engine = nullptr;
}

/*
 * @returns current hook type as shared string
 */
shared_str CScriptProfiler::getTypeString() const
{
    switch (m_profiler_type)
    {
    case CScriptProfilerType::None:
        return "None";
    case CScriptProfilerType::Hook:
        return "Hook";
    case CScriptProfilerType::Sampling:
        return "Sampling";
    default:
        NODEFAULT;
        return "Unknown";
    }
};

/*
 * @returns count of recorded profiling entries (based on currently active hook type)
 */
u32 CScriptProfiler::getRecordsCount() const
{
    switch (m_profiler_type)
    {
    case CScriptProfilerType::None:
        return 0;
    case CScriptProfilerType::Hook:
        return m_hook_profiling_portions.size();
    case CScriptProfilerType::Sampling:
        return m_sampling_profiling_log.size();
    default:
        NODEFAULT;
        return 0;
    }
};

/*
 * Start profiler with provided type.
 *
 * @param profiler_type - type of the profiler to start
 */
void CScriptProfiler::start(CScriptProfilerType profiler_type)
{
    switch (profiler_type)
    {
    case CScriptProfilerType::Hook:
        startHookMode();
        return;
    case CScriptProfilerType::Sampling:
        startSamplingMode(PROFILE_SAMPLING_INTERVAL_DEFAULT);
        return;
    case CScriptProfilerType::None:
        Msg("[P] Tried to start none type profiler");
        return;
    default:
        Msg("[P] Tried to start unknown type (%d) profiler", profiler_type);
        return;
    }
}

/*
 * Start profiler in hook mode (based on built-in lua tools).
 */
void CScriptProfiler::startHookMode()
{
    if (m_active)
    {
        Msg("[P] Tried to start already active profiler, operation ignored");
        return;
    }

    if (lua())
    {
        if (attachLuaHook())
            Msg("[P] Starting scripts hook profiler");
        else
        {
            Msg("[P] Cannot start scripts hook profiler, hook was not set properly");
            return;
        }
    }
    else
    {
        Msg("[P] Activating hook profiler on lua engine start, waiting init");
    }

    m_hook_profiling_portions.clear();
    m_profiler_type = CScriptProfilerType::Hook;
    m_active = true;
}

/*
 * Start profiler in sampling mode (based on luaJIT built-in profiler).
 *
 * @param sampling_interval - interval for calls sampling and further reporting
 */
void CScriptProfiler::startSamplingMode(u32 sampling_interval)
{
    if (m_active)
    {
        Msg("[P] Tried to start already active profiler, operation ignored");
        return;
    }

    if (!luaIsJitProfilerDefined())
    {
        Msg("[P] Cannot start scripts sampling profiler, jit module is not defined");
        return;
    }

    clamp(sampling_interval, 1u, PROFILE_SAMPLING_INTERVAL_MAX);

    if (lua())
    {
        Msg("[P] Starting scripts sampling profiler, interval: %d", sampling_interval);
        luaJitSamplingProfilerAttach(this, sampling_interval);
    }
    else
        Msg("[P] Activating sampling profiler on lua engine start, waiting init");

    m_sampling_profiling_log.clear();
    m_sampling_profile_interval = sampling_interval;
    m_profiler_type = CScriptProfilerType::Sampling;
    m_active = true;
}

/*
 * Stop profiler and clean up stored data.
 * Clean up attached hooks/jit profilers.
 *
 * Note:
 *  - LUA hook is not detached because we cannot be sure where it was attached, but calls are ignore while profiling stopped
 */
void CScriptProfiler::stop()
{
    if (!m_active)
    {
        Msg("[P] Tried to stop inactive profiler");
        return;
    }

    switch (m_profiler_type)
    {
    case CScriptProfilerType::Hook:
        Msg("[P] Stopping scripts hook profiler");
        // Do not detach hook here, adding it means that it is already test run in the first place.
        break;
    case CScriptProfilerType::Sampling:
    {
        Msg("[P] Stopping scripts sampling profiler");
        // Detach profiler from luajit, stop operation will be ignore anyway if it is stopped/captured by another VM.
        luaJitProfilerStop(lua());
        break;
    }
    default:
        Msg("[P] Tried to stop none type profiler");
        return;
    }

    m_active = false;
    m_profiler_type = CScriptProfilerType::None;
    m_hook_profiling_portions.clear();
    m_sampling_profiling_log.clear();
}

/*
 * Reset profiling data.
 * Does not affect profiler flow (start, stop, reinit etc).
 */
void CScriptProfiler::reset()
{
    Msg("[P] Reset profiler");

    m_hook_profiling_portions.clear();
    m_sampling_profiling_log.clear();
}

/*
 * Log profiler report with brief summary based on current context.
 *
 * @param entries_limit - count of top entries to log
 */
void CScriptProfiler::logReport(u32 entries_limit)
{
    switch (m_profiler_type)
    {
    case CScriptProfilerType::Hook:
        return logHookReport(entries_limit);
    case CScriptProfilerType::Sampling:
        return logSamplingReport(entries_limit);
    default:
        Msg("[P] No active profiling data to report");
        return;
    }
}

/*
 * Log hook profiler report with brief summary.
 *
 * @param entries_limit - count of top entries to log
 */
void CScriptProfiler::logHookReport(u32 entries_limit)
{
    if (m_hook_profiling_portions.empty())
    {
        Msg("[P] Nothing to report for hook profiler, data is missing");
        return;
    }

    u64 total_count = 0;
    u64 total_duration = 0;

    xr_vector<decltype(m_hook_profiling_portions)::iterator> entries;
    entries.reserve(m_hook_profiling_portions.size());

    for (auto it = m_hook_profiling_portions.begin(); it != m_hook_profiling_portions.end(); it++)
    {
        entries.push_back(it);
        total_count += it->second.count();
        total_duration += it->second.duration();
    }

    Msg("[P] ==================================================================");
    Msg("[P] = Hook profiler report, %d entries", entries.size());
    Msg("[P] ==================================================================");
    Msg("[P] = By calls duration:");
    Msg("[P] ==================================================================");
    Msg("[P] [idx]     sum        sum%%        avg    |   calls   | trace");

    u64 index = 0;

    std::sort(entries.begin(), entries.end(),
        [](auto& left, auto& right) { return left->second.duration() > right->second.duration(); });

    for (auto it = entries.begin(); it != entries.end(); it++)
    {
        if (index >= entries_limit)
            break;

        if (total_duration > 0)
            Msg("[P] [%3d] %9.3f ms %5.2f%%  %9.3f ms | %9d | %s", index, (*it)->second.duration() / 1000.0,
                ((f64)(*it)->second.duration() * 100.0) / (f64)total_duration,
                (f64)(*it)->second.duration() / (f64)(*it)->second.count() / 1000.0, (*it)->second.count(),
                (*it)->first.c_str());
        else
            // Small measurements chunks can result in 10-20 calls with ~0 total duration.
            Msg("[P] [%3d] %9.3f ms %5.2f%%  %9.3f ms | %9d | %s", index, 0, 0, 0, (*it)->second.count(),
                (*it)->first.c_str());

        index += 1;
    }

    Msg("[P] ==================================================================");
    Msg("[P] = By calls count:");
    Msg("[P] ==================================================================");
    Msg("[P] [idx]    calls  calls%% | trace");

    index = 0;

    std::sort(entries.begin(), entries.end(),
        [](auto& left, auto& right) { return left->second.count() > right->second.count(); });

    for (auto it = entries.begin(); it != entries.end(); it++)
    {
        if (index >= entries_limit)
            break;

        Msg("[P] [%3d] %9d %5.2f%% | %s", index, (*it)->second.count(),
            ((f64)(*it)->second.count() * 100.0) / (f64)total_count, (*it)->first.c_str());

        index += 1;
    }

    Msg("[P] ==================================================================");
    Msg("[P] = Total function calls count: %d", total_count);
    Msg("[P] = Total function calls duration: %f ms", (f32) total_duration / 1000.0);
    Msg("[P] ==================================================================");

    FlushLog();
}

/*
 * Log sampling profiler report with brief summary.
 *
 * @param entries_limit - count of top entries to log
 */
void CScriptProfiler::logSamplingReport(u32 entries_limit)
{
    if (m_sampling_profiling_log.empty())
    {
        Msg("[P] Nothing to report for sampling profiler, data is missing");
        return;
    }

    u64 total_count = 0;
    xr_unordered_map<shared_str, CScriptProfilerSamplingPortion> sampling_portions;

    for (auto it = m_sampling_profiling_log.begin(); it != m_sampling_profiling_log.end(); it++)
    {
        auto portion = sampling_portions.find(it->m_name);

        if (portion != sampling_portions.end())
            portion->second.m_samples += it->m_samples;
        else
            sampling_portions.emplace(it->m_name, it->cloned());
    }

    xr_vector<decltype(sampling_portions)::iterator> entries;
    entries.reserve(sampling_portions.size());

    for (auto it = sampling_portions.begin(); it != sampling_portions.end(); it++)
    {
        entries.push_back(it);
        total_count += it->second.m_samples;
    }

    std::sort(entries.begin(), entries.end(),
        [](auto& left, auto& right) { return left->second.m_samples > right->second.m_samples; });

    Msg("[P] ==================================================================");
    Msg("[P] = Sampling profiler report, %d entries", entries.size());
    Msg("[P] ==================================================================");
    Msg("[P] [idx]  samples    %%    | trace");

    u64 index = 0;

    for (auto it = entries.begin(); it != entries.end(); it++)
    {
        if (index >= entries_limit)
            break;

        Msg("[P] [%3d] %9d %5.2f%% | %s", index, (*it)->second.m_samples,
            ((f64)(*it)->second.m_samples * 100.0) / (f64)total_count, (*it)->first.c_str());

        index += 1;
    }

    Msg("[P] ==================================================================");
    Msg("[P] = Total samples: %d", total_count);
    Msg("[P] ==================================================================");

    FlushLog();
}

/*
 * Save reported data for hook profiler based on current context.
 */
void CScriptProfiler::saveReport()
{
    switch (m_profiler_type)
    {
    case CScriptProfilerType::Hook:
        return saveHookReport(getHookReportFilename());
    case CScriptProfilerType::Sampling:
        return saveSamplingReport(getSamplingReportFilename());
    default:
        Msg("[P] No active profiling data to save report");
        return;
    }
}

/*
 * Save reported data for hook profiler.
 * Saved data is represented by lines with serialized stack, calls count and duration.
 *
 * @param filename - target file to write report to
 */
void CScriptProfiler::saveHookReport(shared_str filename)
{
    if (m_hook_profiling_portions.empty())
    {
        Msg("[P] Nothing to report for hook profiler, data is missing");
        return;
    }

    Msg("[P] Saving hook report to %s", filename.c_str());
    IWriter* F = FS.w_open(filename.c_str());

    xr_vector<decltype(m_hook_profiling_portions)::iterator> entries;
    entries.reserve(m_hook_profiling_portions.size());

    for (auto it = m_hook_profiling_portions.begin(); it != m_hook_profiling_portions.end(); it++)
        entries.push_back(it);

    std::sort(entries.begin(), entries.end(),
        [](auto& left, auto& right) { return left->second.duration() > right->second.duration(); });

    if (F)
    {
        string2048 buffer;

        for (auto &it : entries)
        {
            xr_sprintf(buffer, "trace:%s calls:%d avg:%.3fms sum:%.3fms ", it->first.c_str(), it->second.count(),
               (f32)it->second.duration() / (f32)it->second.count() / 1000.0, it->second.duration() / 1000.0);
            F->w_string(buffer);
        }

        FS.w_close(F);
    }
}

/*
 * Save reported data for sampling profiler.
 * Saved data is represented by lines with folded stacks and count of samples.
 *
 * @param filename - target file to write report to
 */
void CScriptProfiler::saveSamplingReport(shared_str filename)
{
    if (m_sampling_profiling_log.empty())
    {
        Msg("[P] Nothing to report for sampling profiler, data is missing");
        return;
    }

    Msg("[P] Saving sampling report to %s", filename.c_str());
    IWriter* F = FS.w_open(filename.c_str());

    if (F)
    {
        for (auto &it : m_sampling_profiling_log)
            F->w_string(*it.getFoldedStack());

        FS.w_close(F);
    }
}

/*
 * @returns filename for hook profiler report
 */
shared_str CScriptProfiler::getHookReportFilename()
{
    string_path log_file_name;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_hook_profile.log");
    FS.update_path(log_file_name, "$logs$", log_file_name);

    return log_file_name;
}

/*
 * @returns filename for sampling profiler report
 */
shared_str CScriptProfiler::getSamplingReportFilename()
{
    string_path log_file_name;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_sampling_profile.perf");
    FS.update_path(log_file_name, "$logs$", log_file_name);

    return log_file_name;
}

/*
* @returns whether profiling lua hook was/is attached to current VM context
*/
bool CScriptProfiler::attachLuaHook()
{
    lua_Hook hook = lua_gethook(lua());

    // Do not rewrite active hooks and verify if correct hooks is set.
    // Avoid rewriting of hook since something else took hook place, preferrably we want only 1 hook in script engine.
    if (hook)
        return hook == CScriptEngine::lua_hook_call;
    else
        return lua_sethook(lua(), CScriptEngine::lua_hook_call, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
}

/*
 * Handle profiler dispose event.
 * Make sure we are cleaning up all the data/links.
 *
 * @param L - lua VM active on dispose event
 */
void CScriptProfiler::onDispose(lua_State* L)
{
    // When handling instance disposal (reinit), stop profiling for VM.
    // Otherwise you cannot stop profiling because VM pointer will be destroyed and become inaccessible.
    if (m_active && m_profiler_type == CScriptProfilerType::Sampling)
    {
        Msg("[P] Disposing sampling profiler dependencies");
        luaJitProfilerStop(L);
    }
}

/*
 * Handle engine reinit event within profiler.
 * Required to switch current profiler context from one lua VM to newly initialize lua VM.
 *
 * @param L - new lua VM to initialize on
 */
void CScriptProfiler::onReinit(lua_State* L)
{
    if (!m_active)
        return;

    Msg("[P] Profiler reinit, VM:%d", lua());

    switch (m_profiler_type)
    {
    case CScriptProfilerType::Hook:
        if (!attachLuaHook())
        {
            Msg("[P] Cannot start scripts hook profiler on reinit, hook was not set properly");
            return;
        }

        Msg("[P] Reinit scripts hook profiler");

        return;
    case CScriptProfilerType::Sampling:
    {
        if (!luaIsJitProfilerDefined())
        {
            Msg("[P] Cannot start scripts sampling profiler on reinit, jit.profiler module is not defined");
            return;
        }

        Msg("[P] Re-init scripts sampling profiler - attach handler, interval: %d", m_sampling_profile_interval);
        luaJitSamplingProfilerAttach(this, m_sampling_profile_interval);

        return;
    }

    default: NODEFAULT;
    }
}

/*
 * Callback to handle lua profiling hook.
 * Receiving lua events and collecting data in profiler instance to do further performance measurements and assumptions.
 *
 * @param L - lua VM context of calls
 * @param dbg - lua debug context of hook call (only event data is valid)
 */
void CScriptProfiler::onLuaHookCall(lua_State* L, lua_Debug* dbg)
{
    if (!m_active || m_profiler_type != CScriptProfilerType::Hook || dbg->event == LUA_HOOKLINE)
        return;

    auto [parent_stack_info, has_parent_stack_info] = luaDebugStackInfo(L, 2, "nSl");
    auto [stack_info, has_stack_info] = luaDebugStackInfo(L, 1, "nSl");
    auto [at_stack_info, has_at_stack_info] = luaDebugStackInfo(L, 0, "nSl");

    if (!has_parent_stack_info || !has_stack_info)
        return;

    string512 buffer;

    auto name = stack_info.name ? stack_info.name : at_stack_info.name ;
    auto parent_name = parent_stack_info.name ? parent_stack_info.name : "[C]";
    auto short_src = stack_info.short_src;
    auto parent_short_src = parent_stack_info.short_src;
    auto line_defined = stack_info.linedefined;
    auto parent_line_defined = parent_stack_info.linedefined;

    if (!name)
        name = "?";

    if (!stack_info.name && line_defined == 0)
        name = "script-body";

    if (xr_strcmp(short_src, parent_short_src) == 0)
        xr_sprintf(buffer, "%s:%d;%s@%s:%d", name, line_defined, parent_name, parent_short_src, parent_line_defined);
    else
        xr_sprintf(buffer, "%s@%s:%d;%s@%s:%d", name, short_src, line_defined, parent_name, parent_short_src,
            parent_line_defined);

    auto it = m_hook_profiling_portions.find(buffer);
    bool exists = it != m_hook_profiling_portions.end();

    switch (dbg->event)
    {
    case LUA_HOOKCALL:
    {
        if (exists)
            it->second.start();
        else
        {
            CScriptProfilerHookPortion portion;

            portion.start();

            m_hook_profiling_portions.emplace(buffer, std::move(portion));
        }

        return;
    }
    case LUA_HOOKRET:
    case LUA_HOOKTAILRET:
    {
        if (exists)
            it->second.stop();

        return;
    }
    default: NODEFAULT;
    }
}

/*
 * @returns reference to currently active lua VM state (or null if it was not initialize yet)
 */
lua_State* CScriptProfiler::lua() const
{
    return this->m_engine->lua();
}

/*
 * @returns used memory by lua state in bytes
 */
int CScriptProfiler::luaMemoryUsed(lua_State* L)
{
    return lua_gc(L, LUA_GCCOUNT, 0) * 1024 + lua_gc(L, LUA_GCCOUNTB, 0);
}

/*
 * @returns whether jit is enabled
 */
bool CScriptProfiler::luaIsJitProfilerDefined()
{
    // Safest and least invasive way to check it.
    // Other methods affect lua stack and may add interfere with VS extensions / other hooks / error callbacks.
    // We assume that we do not load JIT libs only if nojit parameter is provided.
    return !strstr(Core.Params, CScriptEngine::ARGUMENT_ENGINE_NOJIT);
}

/*
 * Attach sampling profiling hooks.
 * With provided period report samples and store information in profiler for further reporting.
 *
 * @param interval - sampling interval for built-in luaJIT profiler
*/
void CScriptProfiler::luaJitSamplingProfilerAttach(CScriptProfiler* profiler, u32 interval)
{
    string32 buffer = "fli";
    xr_itoa(interval, buffer + 3, 10);

    luaJitProfilerStart(
        profiler->lua(), buffer,
        [](void* data, lua_State* L, int samples, int vmstate) {
            CScriptProfiler* profiler = static_cast<CScriptProfiler*>(data);

            profiler->m_sampling_profiling_log.push_back(std::move(CScriptProfilerSamplingPortion(
                luaJitProfilerDumpToString(L, "fl", 1),
                luaJitProfilerDumpToString(L, "flZ;", -64),
                samples,
                vmstate,
                luaMemoryUsed(L)
            )));
        },
        profiler);
}

/*
 * Possible modes for profiling:
 * f — profile with precision down to the function level
 * l — profile with precision down to the line level
 * i<number> — sampling interval in milliseconds (default 10ms)
 *
 * Note: The actual sampling precision is OS-dependent.
 *
 * @param mode - jit profiling mode variant
 * @param callback - callback to handle periodic sampling report event
 * @param data - any pointer to receive in sampling report callback to return feedback
 * @returns whether jit profiler start call was successful
 */
void CScriptProfiler::luaJitProfilerStart(lua_State* L, cpcstr mode, luaJIT_profile_callback callback, void* data)
{
    // Only single JIT profiler can exist and it will not attach with multiple states.
    // Also only VM started profiler can end it, be careful.
    luaJIT_profile_start(L, mode, callback, data);
}

/*
 * Stop JIT built-in sampling profiler.
 *
 * Notes:
 *    - failsafe, can stop already stopped data
 *    - profiler is singleton instance across all lua VM states
 *    - cannot stop profiler with VM reference, if it was started with another instance
 *    - no status / possibility to check if stop was successful without modifying luaJIT
 */
void CScriptProfiler::luaJitProfilerStop(lua_State* L)
{
    luaJIT_profile_stop(L);
}

/*
 * Possible format values for dump:
 * f — dump function name
 * F — dump function name, module:name for F variant
 * p — preserve full path
 * l — dump module:line
 * Z — zap trailing separator
 * i<number> — Sampling interval in milliseconds (default 10ms)
 *
 * @returns jit profiler dump as shared string
 */
shared_str CScriptProfiler::luaJitProfilerDumpToString(lua_State* L, cpcstr format, int depth)
{
    string2048 buffer;
    size_t length;
    cpcstr dump = luaJIT_profile_dumpstack(L, format, depth, &length);

    R_ASSERT2(length < sizeof(buffer), "Profiling dump buffer overflow");
    strncpy_s(buffer, sizeof(buffer), dump, length);
    buffer[length] = 0;

    return shared_str(buffer);
}

/*
 * Possible format values for dump:
 * f — dump function name
 * F — dump function name, module:name for F variant
 * p — preserve full path
 * l — dump module:line
 * Z — zap trailing separator
 * i<number> — Sampling interval in milliseconds (default 10ms)
 *
 * @returns pair with dump char buffer and length of valid dump data in it
 */
std::pair<cpcstr, size_t> CScriptProfiler::luaJitProfilerDump(lua_State* L, cpcstr format, int depth)
{
    size_t length;
    cpcstr dump = luaJIT_profile_dumpstack(L, format, depth, &length);

    return std::make_pair(dump, length);
}

/*
 * @returns pair with debug information and status of debug information (whether was able to get info from stack)
 */
std::pair<lua_Debug, bool> CScriptProfiler::luaDebugStackInfo(lua_State* L, int level, cpcstr what)
{
    lua_Debug info;
    bool has_stack = lua_getstack(L, level, &info);

    if (has_stack)
    {
        lua_getinfo(L, what, &info);
    }

    return std::make_pair(std::move(info), has_stack);
}
