////////////////////////////////////////////////////////////////////////////
//	Module 		: script_thread.cpp
//	Created 	: 19.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script thread class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "script_thread.hpp"
#include "script_engine.hpp"
#include "Include/xrAPI/xrAPI.h"

// #include "xrCore/Platform.h"

extern "C"
{
#include "lua/lua.h"
};

#define LUABIND_HAS_BUGS_WITH_LUA_THREADS

#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
#include "script_debugger.h"
#else
#include "LuaStudio/LuaStudio.hpp"
#endif
#endif

const LPCSTR main_function = "console_command_run_string_main_thread_function";

//void print_stack_(lua_State *L)
//{
//	Msg(" ");
//	for (int i=0; lua_type(L, -i-1); i++)
//		Msg("%2d : %s",-i-1,lua_typename(L, lua_type(L, -i-1)));
//}

//extern "C" XR_IMPORT lua_State *lua_newcthread(lua_State *OL, int cstacksize);

CScriptThread::CScriptThread(LPCSTR caNamespaceName, bool do_string, bool reload)
{
    m_virtual_machine = nullptr;
    m_active = false;
    lua_State *engineLua = GlobalEnv.ScriptEngine->lua();
    try
    {
        string256 S;
        if (!do_string)
        {
            m_script_name = caNamespaceName;
            GlobalEnv.ScriptEngine->process_file(caNamespaceName, reload);
        }
        else
        {
            m_script_name = "console command";
            xr_sprintf(S, "function %s()\n%s\nend\n", main_function, caNamespaceName);
            int l_iErrorCode = luaL_loadbuffer(engineLua, S, xr_strlen(S), "@console_command");
            if (!l_iErrorCode)
            {
                l_iErrorCode = lua_pcall(engineLua, 0, 0, 0);
                if (l_iErrorCode)
                {
                    GlobalEnv.ScriptEngine->print_output(engineLua, *m_script_name, l_iErrorCode);
                    GlobalEnv.ScriptEngine->on_error(engineLua);
                    return;
                }
            }
            else
            {
                GlobalEnv.ScriptEngine->print_output(engineLua, *m_script_name, l_iErrorCode);
                GlobalEnv.ScriptEngine->on_error(engineLua);
                return;
            }
        }
        m_virtual_machine = lua_newthread(engineLua);
        VERIFY2 (lua(),"Cannot create new Lua thread");
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
        if (GlobalEnv.ScriptEngine->debugger())
            GlobalEnv.ScriptEngine->debugger()->add(m_virtual_machine);
#endif
#if !defined(USE_LUA_STUDIO) && defined(DEBUG)
#ifdef USE_DEBUGGER
        if (GlobalEnv.ScriptEngine->debugger() && GlobalEnv.ScriptEngine->debugger()->Active())
            lua_sethook(lua(), CDbgLuaHelper::hookLua, LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET, 0);
        else
#endif
            lua_sethook(lua(), CScriptEngine::lua_hook_call, LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET,	0);
#endif
        if (!do_string)
            xr_sprintf(S, "%s.main()", caNamespaceName);
        else
            xr_sprintf(S, "%s()", main_function);
        if (!GlobalEnv.ScriptEngine->load_buffer(lua(), S, xr_strlen(S), "@_thread_main"))
            return;
        m_active = true;
    } catch (...) {
        m_active = false;
    }
}

CScriptThread::~CScriptThread()
{
#ifdef DEBUG
    Msg("* Destroying script thread %s", *m_script_name);
#endif
    try
    {
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
        if (GlobalEnv.ScriptEngine->debugger())
            GlobalEnv.ScriptEngine->debugger()->remove(m_virtual_machine);
#endif
#ifndef LUABIND_HAS_BUGS_WITH_LUA_THREADS
        luaL_unref(GlobalEnv.ScriptEngine->lua(), LUA_REGISTRYINDEX, m_thread_reference);
#endif
    } catch (...) {
    }
}

bool CScriptThread::update()
{
    if (!m_active)
        R_ASSERT2(false,"Cannot resume dead Lua thread!");
    try
    {
        GlobalEnv.ScriptEngine->current_thread(this);
        int l_iErrorCode = lua_resume(lua(), 0);
        if (l_iErrorCode && l_iErrorCode!=LUA_YIELD)
        {
            GlobalEnv.ScriptEngine->print_output(lua(), *script_name(), l_iErrorCode);
            GlobalEnv.ScriptEngine->on_error(GlobalEnv.ScriptEngine->lua());
#ifdef DEBUG
            print_stack(lua());
#endif
            m_active = false;
        }
        else
        {
            if (l_iErrorCode!=LUA_YIELD)
            {
#ifdef DEBUG
                if (m_current_stack_level)
                {
                    GlobalEnv.ScriptEngine->print_output(lua(), *script_name(), l_iErrorCode);
                    GlobalEnv.ScriptEngine->on_error(GlobalEnv.ScriptEngine->lua());
                    //print_stack(lua());
                }
#endif
                m_active = false;
#ifdef DEBUG
                GlobalEnv.ScriptEngine->script_log(LuaMessageType::Info, "Script %s is finished!", *m_script_name);
#endif

            }
            else
                VERIFY2(!lua_gettop(lua()), "Do not pass any value to coroutine.yield()!");
        }
        GlobalEnv.ScriptEngine->current_thread(nullptr);
    } catch (...) {
        GlobalEnv.ScriptEngine->current_thread(nullptr);
        m_active = false;
    }
    return m_active;
}
