#include "pch.hpp"
#include "script_profiler.hpp"
#include "xrScriptEngine/script_engine.hpp"

CScriptProfiler::CScriptProfiler(CScriptEngine* engine)
{
    R_ASSERT(engine != NULL);

    m_engine = engine;

    m_active = false;
    m_profile_level = 1;
    m_profiler_type = CScriptProfilerType::None;

    // todo: Configuration of profile levels for hook profiler.
    // todo: Configuration of profile levels for hook profiler.
    // todo: Configuration of profile levels for hook profiler.

    if (strstr(Core.Params, "-lua_profiler"))
        start();
	else if (strstr(Core.Params, "-lua_hook_profiler"))
    	start(CScriptProfilerType::Hook);
    else if (strstr(Core.Params, "-lua_jit_profiler"))
        start(CScriptProfilerType::Jit);
}

CScriptProfiler::~CScriptProfiler()
{
}

void CScriptProfiler::start(CScriptProfilerType profiler_type)
{
    if (m_active)
    {
        Msg("Tried to start active profiler, operation ignored");
        return;
    }

    if (profiler_type == CScriptProfilerType::None)
    {
        Msg("Tried to start none profiler type");
        return;
    }

    // todo: Check JIT and warn? Allow turn it off with parameter?
    // todo: Check JIT and warn? Allow turn it off with parameter?
    // todo: Check JIT and warn? Allow turn it off with parameter?

    m_profiling_portions.clear();
    m_active = true;
    m_profiler_type = profiler_type;

    switch (profiler_type)
    {
	case CScriptProfilerType::Hook:
    	Msg("Starting lua scripts hook profiler");

    	attachLuaHook();

    	return;
    case CScriptProfilerType::Jit:
    {
    	Msg("Starting lua scripts jit profiler");

    	return;
    }

    default:  NODEFAULT;
    }
}

void CScriptProfiler::stop()
{
    if (!m_active)
    {
        Msg("Tried to stop inactive profiler");
        return;
    }

    if (m_profiler_type == CScriptProfilerType::None)
    {
        Msg("Tried to stop none profiler type");
        return;
    }

    switch (m_profiler_type)
    {
	case CScriptProfilerType::Hook:
        Msg("Stopping lua scripts hook profiler");

        // Do not detach hook here, adding it means that it is already test run in the first place.

    	break;
    case CScriptProfilerType::Jit:
    {
    	Msg("Stopping lua scripts jit profiler");

    	break;
    }

    default:  NODEFAULT;
    }

    m_active = false;
}

void CScriptProfiler::reset()
{
    Msg("Reset profiler");

    m_profiling_portions.clear();
}

void CScriptProfiler::logReport()
{
	switch (m_profiler_type)
    {
        case CScriptProfilerType::Hook:
            return logHookReport();
        case CScriptProfilerType::Jit:
            return logJitReport();
        default:
            Msg("Nothing to report for profiler");
            return;
    }
}

void CScriptProfiler::logHookReport()
{
    if (m_profiling_portions.empty())
    {
        Msg("Nothing to report for hook profiler, data is missing");
        return;
    }

    u64 total_count = 0;
    u64 total_duration = 0;

    xr_vector<decltype(m_profiling_portions)::iterator> entries;
    entries.reserve(m_profiling_portions.size());

    for (auto it = m_profiling_portions.begin(); it != m_profiling_portions.end(); it++)
    {
        entries.push_back(it);

        total_count += it->second.count();
        total_duration += it->second.duration();
    }

     Msg("==================================================================");
     Msg("= Log hook profiler report, %d entries", entries.size());
     Msg("==================================================================");
     Msg("= By calls duration:");
     Msg("====");

     u64 index = 0;
     string512 buffer;

     std::sort(entries.begin(), entries.end(),
         [](auto& left, auto& right) { return left->second.duration() > right->second.duration(); });

     for (auto it = entries.begin(); it != entries.end(); it++)
     {
         if (index >= CScriptProfiler::PROFILE_ENTRIES_LOG_LIMIT)
             break;

         Msg("[%3d] %6.3f ms (%5.2f%%) %6d calls, %6.3f ms avg : %s", index, (*it)->second.duration() / 1000.0,
             ((f64)(*it)->second.duration() * 100.0) / (f64)total_duration, (*it)->second.count(),
             (f64)(*it)->second.duration() / (f64)(*it)->second.count() / 1000.0,
             (*it)->first.c_str());

         index += 1;
     }

     Msg("==================================================================");
     Msg("= By calls count:");
     Msg("====");

     index = 0;

     std::sort(entries.begin(), entries.end(),
         [](auto& left, auto& right) { return left->second.count() > right->second.count(); });

     for (auto it = entries.begin(); it != entries.end(); it++)
     {
         if (index >= CScriptProfiler::PROFILE_ENTRIES_LOG_LIMIT)
             break;

         Msg("[%3d] %6d (%5.2f%%) : %s", index, (*it)->second.count(),
             ((f64)(*it)->second.count() * 100.0) / (f64)total_count, (*it)->first.c_str());

         index += 1;
     }

     Msg("==================================================================");
     Msg("= Total function calls count: %d", total_count);
     Msg("= Total function calls duration: %f ms", (f32) total_duration / 1000.0);
     Msg("==================================================================");

     FlushLog();

    // todo;
    // todo;
    // todo;
}

void CScriptProfiler::logJitReport()
{
	// todo;
	// todo;
	// todo;
}

void CScriptProfiler::saveReport()
{
    Log("Save profiler report");

    // todo;
    // todo;
    // todo;
}

void CScriptProfiler::attachLuaHook()
{
    lua_State* L = lua();
	lua_Hook hook = lua_gethook(L);

    if (hook)
    {
        if (hook != CScriptEngine::lua_hook_call)
        {
            Msg("Warning: hook already defined by something else, cannot take ownership as CScriptEngine");
        }
    }
    else
    {
      	Msg("Attaching lua scripts hook");
		lua_sethook(L, CScriptEngine::lua_hook_call, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
    }
}

void CScriptProfiler::onLuaHookCall(lua_State* L, lua_Debug* dbg)
{
    if (!m_active || dbg->event == LUA_HOOKLINE)
        return;

    lua_Debug parent_stack_info;
    lua_Debug stack_info;

    // Check higher level of stack.
    if (m_profile_level > 0)
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
    auto parent_name = m_profile_level > 0 && parent_stack_info.name ? parent_stack_info.name : "?";
    auto short_src = stack_info.short_src;
    auto line_defined = stack_info.linedefined;

    if (!stack_info.name && line_defined == 0)
        name = "lua-script-body";

    // Include date from higher stack levels.
    if (m_profile_level > 0)
        xr_sprintf(buffer, "%s [%d] - %s @ %s", name, line_defined, parent_name, short_src);
    else
        xr_sprintf(buffer, "%s [%d] @ %s", name, line_defined, parent_name, short_src);

    auto it = m_profiling_portions.find(buffer);
    bool exists = it != m_profiling_portions.end();

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
            CScriptProfilerPortion portion;

            portion.start();

            m_profiling_portions.emplace(buffer, std::move(portion));
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

bool CScriptProfiler::luaIsJitProfilerDefined(lua_State* L)
{
    return true;
}

// todo: Add util to get frame info
// todo: Add util to get frame info
// todo: Add util to get frame info
