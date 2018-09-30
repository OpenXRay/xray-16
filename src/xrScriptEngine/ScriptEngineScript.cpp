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
#include "script_debugger.hpp"
#include "DebugMacros.hpp"
#include "ScriptExporter.hpp"

void LuaLog(pcstr caMessage)
{
#ifndef MASTER_GOLD
    GEnv.ScriptEngine->script_log(LuaMessageType::Message, "%s", caMessage);
#endif
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
    if (GEnv.ScriptEngine->debugger())
        GEnv.ScriptEngine->debugger()->Write(caMessage);
#endif
}

void ErrorLog(pcstr caMessage)
{
    GEnv.ScriptEngine->error_log("%s", caMessage);
    GEnv.ScriptEngine->print_stack();
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
    if (GEnv.ScriptEngine->debugger())
        GEnv.ScriptEngine->debugger()->Write(caMessage);
#endif
#ifdef DEBUG
    bool lua_studio_connected = !!GEnv.ScriptEngine->debugger();
    if (!lua_studio_connected)
        R_ASSERT2(0, caMessage);
#else
    R_ASSERT2(0, caMessage);
#endif
}

//AVO:
void PrintStack()
{
    GEnv.ScriptEngine->print_stack();
}
//-AVO

void FlushLogs()
{
#ifdef DEBUG
    FlushLog();
    GEnv.ScriptEngine->flush_log();
#endif
}

void verify_if_thread_is_running()
{
    THROW2(GEnv.ScriptEngine->current_thread(), "coroutine.yield() is called outside the LUA thread!");
}

bool is_editor()
{
    return GEnv.ScriptEngine->is_editor();
}

inline int bit_and(const int i, const int j) { return i & j; }
inline int bit_or(const int i, const int j) { return i | j; }
inline int bit_xor(const int i, const int j) { return i ^ j; }
inline int bit_not(const int i) { return ~i; }
inline const char* user_name() { return Core.UserName; }

void prefetch_module(pcstr file_name) { GEnv.ScriptEngine->process_file(file_name); }

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

std::ostream& operator<<(std::ostream& os, const profile_timer_script& pt) { return os << pt.time(); }
SCRIPT_EXPORT(CScriptEngine, (),
{
    using namespace luabind;
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
            .def("time", &profile_timer_script::time),

        def("log", &LuaLog),
        def("error_log", &ErrorLog),
        def("flush", &FlushLogs),
        def("print_stack", &PrintStack),
        def("prefetch", &prefetch_module),
        def("verify_if_thread_is_running", &verify_if_thread_is_running),
        def("bit_and", &bit_and),
        def("bit_or", &bit_or),
        def("bit_xor", &bit_xor),
        def("bit_not", &bit_not),
        def("editor", &is_editor),
        def("user_name", &user_name)
    ];
});
