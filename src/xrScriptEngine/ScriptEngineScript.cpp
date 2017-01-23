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
#include "Include/xrAPI/xrAPI.h"
#include "ScriptExporter.hpp"

void LuaLog(LPCSTR caMessage)
{
#ifndef MASTER_GOLD
    GlobalEnv.ScriptEngine->script_log(LuaMessageType::Message, "%s", caMessage);
#endif
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
    if (GlobalEnv.ScriptEngine->debugger())
        GlobalEnv.ScriptEngine->debugger()->Write(caMessage);
#endif
}

void ErrorLog(LPCSTR caMessage)
{
    GlobalEnv.ScriptEngine->error_log("%s", caMessage);
#ifdef DEBUG
    GlobalEnv.ScriptEngine->print_stack();
#endif
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
    if (GlobalEnv.ScriptEngine->debugger())
        GlobalEnv.ScriptEngine->debugger()->Write(caMessage);
#endif
#ifdef DEBUG
    bool lua_studio_connected = !!GlobalEnv.ScriptEngine->debugger();
    if (!lua_studio_connected)
        R_ASSERT2(0, caMessage);
#else
    R_ASSERT2(0, caMessage);
#endif
}

void FlushLogs()
{
#ifdef DEBUG
    FlushLog();
    GlobalEnv.ScriptEngine->flush_log();
#endif
}

void verify_if_thread_is_running()
{ THROW2(GlobalEnv.ScriptEngine->current_thread(), "coroutine.yield() is called outside the LUA thread!"); }

bool is_editor()
{
#ifdef EDITOR
    return true;
#else
    return false;
#endif
}

int bit_and(int i, int j) { return i&j; }
int bit_or(int i, int j) { return i|j; }
int bit_xor(int i, int j) { return i^j; }
int bit_not(int i) { return ~i; }
const char *user_name() { return Core.UserName; }
void prefetch_module(LPCSTR file_name) { GlobalEnv.ScriptEngine->process_file(file_name); }

struct profile_timer_script
{
    u64 m_start_cpu_tick_count;
    u64 m_accumulator;
    u64 m_count;
    int m_recurse_mark;

    profile_timer_script()
    {
        m_start_cpu_tick_count = 0;
        m_accumulator = 0;
        m_count = 0;
        m_recurse_mark = 0;
    }

    profile_timer_script(const profile_timer_script &profile_timer)
    { *this = profile_timer; }

    profile_timer_script &operator=(const profile_timer_script &profile_timer)
    {
        m_start_cpu_tick_count = profile_timer.m_start_cpu_tick_count;
        m_accumulator = profile_timer.m_accumulator;
        m_count = profile_timer.m_count;
        m_recurse_mark = profile_timer.m_recurse_mark;
        return *this;
    }

    bool operator<(const profile_timer_script &profile_timer) const
    { return m_accumulator<profile_timer.m_accumulator; }

    void start()
    {
        if (m_recurse_mark)
        {
            m_recurse_mark++;
            return;
        }
        m_recurse_mark++;
        m_count++;
        m_start_cpu_tick_count = CPU::GetCLK();
    }

    void stop()
    {
        if (!m_recurse_mark)
            return;
        m_recurse_mark--;
        if (m_recurse_mark)
            return;
        u64 finish = CPU::GetCLK();
        if (finish>m_start_cpu_tick_count)
            m_accumulator += finish-m_start_cpu_tick_count;
    }

    float time() const
    {
        FPU::m64r();
        float result = float(double(m_accumulator)/double(CPU::clk_per_second))*1000000.f;
        FPU::m24r();
        return result;
    }
};

IC profile_timer_script operator+(const profile_timer_script &portion0, const profile_timer_script &portion1)
{
    profile_timer_script result;
    result.m_accumulator = portion0.m_accumulator+portion1.m_accumulator;
    result.m_count = portion0.m_count+portion1.m_count;
    return result;
}

std::ostream& operator<<(std::ostream& os, const profile_timer_script& pt)
{
    return os << pt.time();
}

SCRIPT_EXPORT(CScriptEngine, (),
{
    using namespace luabind;
    module(luaState)
    [
        class_<profile_timer_script>("profile_timer")
            .def(constructor<>())
            .def(constructor<profile_timer_script&>())
            .def(const_self+profile_timer_script())
            .def(const_self<profile_timer_script())
            .def(tostring(self))
            .def("start", &profile_timer_script::start)
            .def("stop", &profile_timer_script::stop)
            .def("time", &profile_timer_script::time),
        def("log", &LuaLog),
        def("error_log", &ErrorLog),
        def("flush", &FlushLogs),
        def("prefetch", &prefetch_module),
        def("verify_if_thread_is_running", &verify_if_thread_is_running),
        def("editor", &is_editor),
        def("bit_and", &bit_and),
        def("bit_or", &bit_or),
        def("bit_xor", &bit_xor),
        def("bit_not", &bit_not),
        def("user_name", &user_name)
    ];
});
