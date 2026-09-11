////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_script.cpp
//	Created 	: 25.12.2002
//  Modified 	: 13.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator script engine export
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "ScriptEngineScript.hpp"
#include "script_engine.hpp"
#include "script_profiler.hpp"
#include "script_debugger.hpp"

void LuaLog(pcstr caMessage)
{
#ifndef MASTER_GOLD
    GEnv.ScriptEngine->script_log(LuaMessageType::Message, "%s", caMessage);
#endif
#if defined(USE_DEBUGGER)
    if (GEnv.ScriptEngine->debugger())
        GEnv.ScriptEngine->debugger()->Write(caMessage);
#endif
}

void ErrorLog(pcstr caMessage)
{
    GEnv.ScriptEngine->error_log("%s", caMessage);
    GEnv.ScriptEngine->print_stack();
#if defined(USE_DEBUGGER)
    if (GEnv.ScriptEngine->debugger())
        GEnv.ScriptEngine->debugger()->Write(caMessage);
#endif
    R_ASSERT2(0, caMessage);
}

void FlushLogs()
{
#ifdef DEBUG
    FlushLog();
    GEnv.ScriptEngine->flush_log();
#endif
}

void CScriptEngine::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        def("log", &LuaLog),
        def("log1", +[](pcstr message) // X-Ray Extensions
        {
            Msg("%s", message);
        }),
        def("error_log", &ErrorLog),
        def("flush", &FlushLogs),
        def("flush1", +[] // X-Ray Extensions
        {
            FlushLog();
        }),
        def("print_stack", +[]()
        {
            GEnv.ScriptEngine->print_stack();
        }),
        def("prefetch", +[](pcstr file_name)
        {
            GEnv.ScriptEngine->process_file(file_name);
        }),
        def("verify_if_thread_is_running", +[]()
        {
            THROW2(GEnv.ScriptEngine->current_thread(), "coroutine.yield() is called outside the LUA thread!");
        }),
        def("bit_and", +[](const int i, const int j)
        {
            return i & j;
        }),
        def("bit_or", +[](const int i, const int j)
        {
            return i | j;
        }),
        def("bit_xor", +[](const int i, const int j)
        {
            return i ^ j;
        }),
        def("bit_not", +[](const int i)
        {
            return ~i;
        }),
        def("editor", +[]()
        {
            return GEnv.ScriptEngine->is_editor();
        }),
        def("user_name", +[]()
        {
            return Core.UserName;
        })
    ];
}

struct profile_timer_script
{
    using Clock = std::chrono::high_resolution_clock;
    using Time = Clock::time_point;
    using Duration = Clock::duration;

    Time start_time;
    Duration accumulator;
    u64 count = 0;
    int recurse_mark = 0;

    profile_timer_script() : start_time(), accumulator(), count(0), recurse_mark(0) {}

    bool operator<(const profile_timer_script& profile_timer) const
    {
        return accumulator < profile_timer.accumulator;
    }

    void start()
    {
        if (recurse_mark)
        {
            ++recurse_mark;
            return;
        }

        ++recurse_mark;
        ++count;
        start_time = Clock::now();
    }

    void stop()
    {
        if (!recurse_mark)
            return;

        --recurse_mark;

        if (recurse_mark)
            return;

        const auto finish = Clock::now();
        if (finish > start_time)
            accumulator += finish - start_time;
    }

    float time() const
    {
        using namespace std::chrono;
        return float(duration_cast<microseconds>(accumulator).count());
    }
};

inline profile_timer_script operator+(const profile_timer_script& portion0, const profile_timer_script& portion1)
{
    profile_timer_script result;
    result.accumulator = portion0.accumulator + portion1.accumulator;
    result.count = portion0.count + portion1.count;
    return result;
}

std::ostream& operator<<(std::ostream& os, const profile_timer_script& pt)
{
    return os << pt.time();
}

void CScriptProfiler::script_register(lua_State* luaState)
{
    using namespace luabind;

    globals(luaState)["PROFILER_TYPE_NONE"]     = (u32)CScriptProfilerType::None;
    globals(luaState)["PROFILER_TYPE_HOOK"]     = (u32)CScriptProfilerType::Hook;
    globals(luaState)["PROFILER_TYPE_SAMPLING"] = (u32)CScriptProfilerType::Sampling;

    module(luaState)
    [
        class_<profile_timer_script>("profile_timer")
            .def(constructor<>())
            .def(constructor<profile_timer_script&>())
            .def(const_self + profile_timer_script())
            .def(const_self < profile_timer_script())
            .def(tostring(self))
            .def("start", &profile_timer_script::start)
            .def("stop", &profile_timer_script::stop)
            .def("time", &profile_timer_script::time)
    ];

    module(luaState, "profiler")
    [
        def("is_active", +[]() -> bool
        {
            return GEnv.ScriptEngine->m_profiler->IsActive();
        }),
        def("get_type", +[]()-> u32
        {
            return static_cast<u32>(GEnv.ScriptEngine->m_profiler->GetType());
        }),
        def("start", +[]()
        {
            GEnv.ScriptEngine->m_profiler->Start();
        }),
        def("start", +[](CScriptProfilerType profiler_type)
        {
            GEnv.ScriptEngine->m_profiler->Start(profiler_type);
        }),
        def("start_hook_mode", +[]()
        {
            GEnv.ScriptEngine->m_profiler->StartHookMode();
        }),
        def("start_sampling_mode", +[]()
        {
            GEnv.ScriptEngine->m_profiler->StartSamplingMode();
        }),
        def("start_sampling_mode", +[](u32 sampling_interval = CScriptProfiler::PROFILE_SAMPLING_INTERVAL_DEFAULT)
        {
            GEnv.ScriptEngine->m_profiler->StartSamplingMode(sampling_interval);
        }),
        def("stop", +[]()
        {
            GEnv.ScriptEngine->m_profiler->Stop();
        }),
        def("reset", +[]()
        {
            GEnv.ScriptEngine->m_profiler->Reset();
        }),
        def("log_report", +[]()
        {
            GEnv.ScriptEngine->m_profiler->LogReport();
        }),
        def("log_report", +[](u32 entries_limit)
        {
            GEnv.ScriptEngine->m_profiler->LogReport(entries_limit);
        }),
        def("save_report", +[]()
        {
            GEnv.ScriptEngine->m_profiler->SaveReport();
        })
    ];
}
