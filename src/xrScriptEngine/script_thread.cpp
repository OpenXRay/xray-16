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

// #include "Common/Platform.hpp"

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

CScriptThread::CScriptThread(CScriptEngine *scriptEngine, LPCSTR caNamespaceName, bool do_string, bool reload)
#ifdef DEBUG
    : CScriptStackTracker(scriptEngine)
#endif
{
    this->scriptEngine = scriptEngine;
    m_virtual_machine = nullptr;
    m_active = false;
    lua_State *engineLua = scriptEngine->lua();
    try
    {
        string256 S;
        if (!do_string)
        {
            m_script_name = caNamespaceName;
            scriptEngine->process_file(caNamespaceName, reload);
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
                    CScriptEngine::print_output(engineLua, *m_script_name, l_iErrorCode);
                    CScriptEngine::on_error(engineLua);
                    return;
                }
            }
            else
            {
                CScriptEngine::print_output(engineLua, *m_script_name, l_iErrorCode);
                CScriptEngine::on_error(engineLua);
                return;
            }
        }
        m_virtual_machine = lua_newthread(engineLua);
        VERIFY2 (lua(),"Cannot create new Lua thread");
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
        if (scriptEngine->debugger())
            scriptEngine->debugger()->add(m_virtual_machine);
#endif
#if !defined(USE_LUA_STUDIO) && defined(DEBUG)
#ifdef USE_DEBUGGER
        if (scriptEngine.debugger() && scriptEngine.debugger()->Active())
            lua_sethook(lua(), CDbgLuaHelper::hookLua, LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET, 0);
        else
#endif
            lua_sethook(lua(), CScriptEngine::lua_hook_call, LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET,	0);
#endif
        if (!do_string)
            xr_sprintf(S, "%s.main()", caNamespaceName);
        else
            xr_sprintf(S, "%s()", main_function);
        if (!scriptEngine->load_buffer(lua(), S, xr_strlen(S), "@_thread_main"))
            return;
        m_active = true;
    } catch (...) {
        m_active = false;
    }
}

CScriptThread::~CScriptThread()
{ scriptEngine->DestroyScriptThread(this); }

bool CScriptThread::update()
{
    if (!m_active)
        R_ASSERT2(false,"Cannot resume dead Lua thread!");
    try
    {
        scriptEngine->current_thread(this);
        int l_iErrorCode = lua_resume(lua(), 0);
        if (l_iErrorCode && l_iErrorCode!=LUA_YIELD)
        {
            CScriptEngine::print_output(lua(), *script_name(), l_iErrorCode);
            CScriptEngine::on_error(scriptEngine->lua());
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
                    CScriptEngine::print_output(lua(), *script_name(), l_iErrorCode);
                    CScriptEngine::on_error(scriptEngine->lua());
                    //print_stack(lua());
                }
#endif
                m_active = false;
#ifdef DEBUG
                scriptEngine->script_log(LuaMessageType::Info, "Script %s is finished!", *m_script_name);
#endif

            }
            else
                VERIFY2(!lua_gettop(lua()), "Do not pass any value to coroutine.yield()!");
        }
        scriptEngine->current_thread(nullptr);
    } catch (...) {
        scriptEngine->current_thread(nullptr);
        m_active = false;
    }
    return m_active;
}
