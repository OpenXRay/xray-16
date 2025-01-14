#include "pch.hpp"
#include "script_profiler.hpp"

CScriptProfiler::CScriptProfiler()
{
    m_active = false;
    m_profile_level = 1;

    // todo: Configuration of profile levels.
    // todo: Configuration of profile levels.
    // todo: Configuration of profile levels.

    if (strstr(Core.Params, "-lua_profiler"))
    {
        start();
    }
}

CScriptProfiler::~CScriptProfiler()
{
}

void CScriptProfiler::start()
{
    if (m_active)
    {
        Msg("Tried to start active profiler, operation ignored");
        return;
    }

    Msg("Starting lua scripts profiler");

    m_active = true;

    // todo: Reset?
    // todo: Reset?
    // todo: Reset?

    // todo: Check JIT and warn? Allow turn it off with parameter?
    // todo: Check JIT and warn? Allow turn it off with parameter?
    // todo: Check JIT and warn? Allow turn it off with parameter?

    // todo: Attach hook?
    // todo: Attach hook?
    // todo: Attach hook?
}

void CScriptProfiler::stop()
{
    if (!m_active)
    {
        Msg("Tried to stop inactive profiler, operation ignored");
        return;
    }

    Msg("Stopping lua scripts profiler");

    // todo: Reset?
    // todo: Reset?
    // todo: Reset?

    // todo: Detach hook?
    // todo: Detach hook?
    // todo: Detach hook?
}

void CScriptProfiler::reset() 
{ 
    Msg("Reset profiler");

    // todo;
    // todo;
    // todo;
}

void CScriptProfiler::log()
{

    u64 total_count = 0;
    u64 total_duration = 0;

    std::vector<decltype(m_profiling_portions)::iterator> entries;
    entries.reserve(m_profiling_portions.size());

    for (auto it = m_profiling_portions.begin(); it != m_profiling_portions.end(); it++)
    {
        entries.push_back(it);

        total_count += it->second.count();
        total_duration += it->second.duration();
    }


     Msg("==================================================================");
     Msg("= Log profiler report, %d entries", entries.size());
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

void CScriptProfiler::save()
{
    Log("Save profiler report");

    // todo;
    // todo;
    // todo;
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

// todo: Add util to get frame info
// todo: Add util to get frame info
// todo: Add util to get frame info
