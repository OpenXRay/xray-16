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
    m_hook_profile_depth = PROFILE_HOOK_DEPTH_DEFAULT;

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

void CScriptProfiler::start(CScriptProfilerType profiler_type)
{
    switch (profiler_type)
    {
	case CScriptProfilerType::Hook:
		startHookMode(PROFILE_HOOK_DEPTH_DEFAULT);
    	return;
    case CScriptProfilerType::Sampling:
		startSamplingMode(PROFILE_SAMPLING_INTERVAL_DEFAULT);
	    return;
    case CScriptProfilerType::None:
		Msg("[P] Tried to start none type profiler");
	    return;
    default: NODEFAULT;
    }
}

void CScriptProfiler::startHookMode(u32 stack_depth)
{
    if (m_active)
    {
        Msg("[P] Tried to start already active profiler, operation ignored");
        return;
    }

    if (!lua())
    {
    	Msg("[P] Activating hook profiler on lua engine start, waiting init");

		m_profiler_type = CScriptProfilerType::Hook;
		m_active = true;

        return;
    }

    clamp(stack_depth, 0u, PROFILE_SAMPLING_INTERVAL_MAX);

    if (!attachLuaHook())
    {
        Msg("[P] Cannot start scripts hook profiler, hook was not set properly");
		return;
    }

    Msg("[P] Starting scripts hook profiler, depth: %d", stack_depth);

    m_hook_profile_depth = stack_depth;
    m_hook_profiling_portions.clear();
	m_profiler_type = CScriptProfilerType::Hook;
	m_active = true;
}

void CScriptProfiler::startSamplingMode(u32 sampling_interval)
{
    if (m_active)
    {
        Msg("[P] Tried to start already active profiler, operation ignored");
        return;
    }

    if (!luaIsJitProfilerDefined(lua()))
    {
        Msg("[P] Cannot start scripts sampling profiler, jit module is not defined");
		return;
    }

    if (!lua())
    {
    	Msg("[P] Activating sampling profiler on lua engine start, waiting init");

		m_profiler_type = CScriptProfilerType::Sampling;
		m_active = true;

        return;
    }

    clamp(sampling_interval, 1u, PROFILE_SAMPLING_INTERVAL_MAX);

    Msg("[P] Starting scripts sampling profiler, interval: %d", sampling_interval);

    luaJitSamplingProfilerAttach(this, sampling_interval);

    m_sampling_profile_interval = sampling_interval;
	m_profiler_type = CScriptProfilerType::Sampling;
	m_active = true;
}

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
    m_hook_profiling_portions.clear();
    m_sampling_profiling_log.clear();
}

void CScriptProfiler::reset()
{
    Msg("[P] Reset profiler");

    m_hook_profiling_portions.clear();
    m_sampling_profiling_log.clear();
}

void CScriptProfiler::logReport()
{
	switch (m_profiler_type)
    {
        case CScriptProfilerType::Hook:
            return logHookReport();
        case CScriptProfilerType::Sampling:
            return logSamplingReport();
        default:
            Msg("[P] No active profiling data to report");
            return;
    }
}

void CScriptProfiler::logHookReport()
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
     Msg("[P] ====");

     u64 index = 0;
     string512 buffer;

     std::sort(entries.begin(), entries.end(),
         [](auto& left, auto& right) { return left->second.duration() > right->second.duration(); });

     for (auto it = entries.begin(); it != entries.end(); it++)
     {
         if (index >= CScriptProfiler::PROFILE_ENTRIES_LOG_LIMIT)
             break;

         Msg("[P] [%3d] %6.3f ms (%5.2f%%) %6d calls, %6.3f ms avg : %s", index, (*it)->second.duration() / 1000.0,
             ((f64)(*it)->second.duration() * 100.0) / (f64)total_duration, (*it)->second.count(),
             (f64)(*it)->second.duration() / (f64)(*it)->second.count() / 1000.0,
             (*it)->first.c_str());

         index += 1;
     }

     Msg("[P] ==================================================================");
     Msg("[P] = By calls count:");
     Msg("[P] ====");

     index = 0;

     std::sort(entries.begin(), entries.end(),
         [](auto& left, auto& right) { return left->second.count() > right->second.count(); });

     for (auto it = entries.begin(); it != entries.end(); it++)
     {
         if (index >= CScriptProfiler::PROFILE_ENTRIES_LOG_LIMIT)
             break;

         Msg("[P] [%3d] %6d (%5.2f%%) : %s", index, (*it)->second.count(),
             ((f64)(*it)->second.count() * 100.0) / (f64)total_count, (*it)->first.c_str());

         index += 1;
     }

     Msg("[P] ==================================================================");
     Msg("[P] = Total function calls count: %d", total_count);
     Msg("[P] = Total function calls duration: %f ms", (f32) total_duration / 1000.0);
     Msg("[P] ==================================================================");

     FlushLog();

    // todo;
    // todo;
    // todo;
}

void CScriptProfiler::logSamplingReport()
{
    // todo: Separete save method, implement printing function here.
    // todo: Separete save method, implement printing function here.
    // todo: Separete save method, implement printing function here.

    if (m_sampling_profiling_log.empty())
    {
        Msg("[P] Nothing to report for sampling profiler, data is missing");
        return;
    }

    string_path log_file_name;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_sampling_profile.perf");
    FS.update_path(log_file_name, "$logs$", log_file_name);

    Msg("[P] Saving sampling report to %s", log_file_name);

    IWriter* F = FS.w_open(log_file_name);

    if (F)
    {
        for (auto &it : m_sampling_profiling_log)
        {
            F->w_string(*it.getFoldedStack());
        }

        FS.w_close(F);
    }
}

void CScriptProfiler::saveReport()
{
	switch (m_profiler_type)
    {
        case CScriptProfilerType::Hook:
            return saveHookReport();
        case CScriptProfilerType::Sampling:
            return saveSamplingReport();
        default:
            Msg("[P] No active profiling data to save report");
            return;
    }
}

void CScriptProfiler::saveHookReport()
{
    if (m_hook_profiling_portions.empty())
	{
        Msg("[P] Nothing to report for hook profiler, data is missing");
        return;
    }

    Log("[P] Saving hook profiler report");

    // todo;
    // todo;
    // todo;
}

void CScriptProfiler::saveSamplingReport()
{
	if (m_sampling_profiling_log.empty())
	{
        Msg("[P] Nothing to report for sampling profiler, data is missing");
        return;
    }

    string_path log_file_name;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_sampling_profile.perf");
    FS.update_path(log_file_name, "$logs$", log_file_name);

    Msg("[P] Saving sampling report to %s", log_file_name);

    IWriter* F = FS.w_open(log_file_name);

    if (F)
    {
        for (auto &it : m_sampling_profiling_log)
        {
            F->w_string(*it.getFoldedStack());
        }

        FS.w_close(F);
    }
}

/*
* @returns whether profiling lua hook was/is attached to current VM context
*/
bool CScriptProfiler::attachLuaHook()
{
    lua_Hook hook = lua_gethook(lua());

    return hook ? hook == CScriptEngine::lua_hook_call : lua_sethook(lua(), CScriptEngine::lua_hook_call, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
}

void CScriptProfiler::onDispose(lua_State* L)
{
    // When handling instance disposal (reinit), stop profiling for VM.
    // Otherwise you cannot stop profiling because VM pointer will be destroyed and become inaccessible.
    if (m_active && m_profiler_type == CScriptProfilerType::Sampling)
    {
        Msg("[P] Disposing sampling dependencies");
		luaJitProfilerStop(L);
    }
}

void CScriptProfiler::onReinit(lua_State* L)
{
    if (!m_active)
    	return;

	Msg("[P] Profiler reinit %d", lua());

    switch (m_profiler_type)
    {
	case CScriptProfilerType::Hook:
   		if (!attachLuaHook())
        {
        	Msg("[P] Cannot start scripts hook profiler on reinit, hook was not set properly");
			return;
        }

    	Msg("[P] Reinit scripts hook profiler");

        m_hook_profiling_portions.clear();

    	return;
    case CScriptProfilerType::Sampling:
    {
        if (!luaIsJitProfilerDefined(lua()))
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

void CScriptProfiler::onLuaHookCall(lua_State* L, lua_Debug* dbg)
{
    if (!m_active || m_profiler_type != CScriptProfilerType::Hook || dbg->event == LUA_HOOKLINE)
        return;

    lua_Debug parent_stack_info;
    lua_Debug stack_info;

    // todo: Implement dynamic depth.
    // todo: Implement dynamic depth.

    // Check higher level of stack.
    if (m_hook_profile_depth > 0)
    {
        if (!lua_getstack(L, 1, &parent_stack_info))
        {
            return;
        }

        lua_getinfo(L, "nSl", &parent_stack_info);
    }

    if (!lua_getstack(L, 0, &stack_info))
    {
        return;
    }

    lua_getinfo(L, "nSl", &stack_info);

    string512 buffer;

    auto name = stack_info.name ? stack_info.name : "?";
    auto parent_name = m_hook_profile_depth > 0 && parent_stack_info.name ? parent_stack_info.name : "?";
    auto short_src = stack_info.short_src;
    auto line_defined = stack_info.linedefined;

    if (!stack_info.name && line_defined == 0)
        name = "lua-script-body";

    // Include date from higher stack levels.
    if (m_hook_profile_depth > 0)
        xr_sprintf(buffer, "%s [%d] - %s @ %s", name, line_defined, parent_name, short_src);
    else
        xr_sprintf(buffer, "%s [%d] @ %s", name, line_defined, parent_name, short_src);

    auto it = m_hook_profiling_portions.find(buffer);
    bool exists = it != m_hook_profiling_portions.end();

    switch (dbg->event)
    {
    case LUA_HOOKCALL:
    {
        if (exists)
        {
            it->second.start();
        }
        else
        {
            CScriptProfilerHookPortion portion;

            portion.start();

            m_hook_profiling_portions.emplace(buffer, std::move(portion));
        }

        return;
    }
    case LUA_HOOKRET:
    {
        if (exists)
        {
            it->second.stop();
        }

        return;
    }
    case LUA_HOOKTAILRET:
    {
        if (exists)
        {
            it->second.stop();
        }

        return;
    }
    default: NODEFAULT;
    }
}

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
bool CScriptProfiler::luaIsJitProfilerDefined(lua_State* L)
{
    // Safest and least invasive way to check it.
    // Other methods affect lua stack and may add interfere with VS extensions / other hooks / error callbacks.
    // We assume that we do not load JIT libs only if nojit parameter is provided.
    return !strstr(Core.Params, CScriptEngine::ARGUMENT_ENGINE_NOJIT);
}

/**
* Attach sampling profiling hooks.
* With provided period report samples and store information in profiler for further reporting.
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
 * @returns whether jit profiler start call was successful
 */
void CScriptProfiler::luaJitProfilerStart(lua_State* L, cpcstr mode, luaJIT_profile_callback callback, void* data)
{
    // Only single JIT profiler can exist and it will not attach with multiple states.
    // Also only VM started profiler can end it, be careful.

    luaJIT_profile_start(L, mode, callback, data);
}

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
 */
shared_str CScriptProfiler::luaJitProfilerDumpToString(lua_State* L, cpcstr format, int depth)
{
	string2048 buffer;
 	size_t length;
    cpcstr dump = luaJIT_profile_dumpstack(L, format, depth, &length);

    R_ASSERT2(length < 2048, "Profiling dump buffer overflow");

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
 */
std::pair<cpcstr, size_t> CScriptProfiler::luaJitProfilerDump(lua_State* L, cpcstr format, int depth)
{
	size_t length;
    cpcstr dump = luaJIT_profile_dumpstack(L, format, depth, &length);

    return std::make_pair(dump, length);
}
