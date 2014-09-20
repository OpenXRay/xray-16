////////////////////////////////////////////////////////////////////////////
//	Module 		: script_thread.cpp
//	Created 	: 19.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script thread class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
extern "C" {
	#include "lua/lua.h"
};
#include "script_engine.h"
#include "script_thread.h"
#include "ai_space.h"

#define LUABIND_HAS_BUGS_WITH_LUA_THREADS

#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
#		include "script_debugger.h"
#	else // #ifndef USE_LUA_STUDIO
#		include "lua_studio.h"
#	endif // #ifndef USE_LUA_STUDIO
#endif

const LPCSTR main_function = "console_command_run_string_main_thread_function";

//void print_stack_(lua_State *L)
//{
//	Msg(" ");
//	for (int i=0; lua_type(L, -i-1); i++)
//		Msg("%2d : %s",-i-1,lua_typename(L, lua_type(L, -i-1)));
//}

//extern "C" __declspec(dllimport) lua_State *lua_newcthread(lua_State *OL, int cstacksize);

CScriptThread::CScriptThread(LPCSTR caNamespaceName, bool do_string, bool reload)
{
	m_virtual_machine		= 0;
	m_active				= false;

	try {
		string256			S;
		if (!do_string) {
			m_script_name	= caNamespaceName;
			ai().script_engine().process_file(caNamespaceName,reload);
		}
		else {
			m_script_name	= "console command";
			xr_sprintf			(S,"function %s()\n%s\nend\n",main_function,caNamespaceName);
			int				l_iErrorCode = luaL_loadbuffer(ai().script_engine().lua(),S,xr_strlen(S),"@console_command");
			if (!l_iErrorCode) {
				l_iErrorCode = lua_pcall(ai().script_engine().lua(),0,0,0);
				if (l_iErrorCode) {
					ai().script_engine().print_output	(ai().script_engine().lua(),*m_script_name,l_iErrorCode);
					ai().script_engine().on_error		(ai().script_engine().lua());
					return;
				}
			}
			else {
				ai().script_engine().print_output		(ai().script_engine().lua(),*m_script_name,l_iErrorCode);
				ai().script_engine().on_error			(ai().script_engine().lua());
				return;
			}
		}

//		print_stack_		(ai().script_engine().lua());
		m_virtual_machine	= lua_newthread(ai().script_engine().lua());
//		m_virtual_machine	= lua_newcthread(ai().script_engine().lua(),0);
		VERIFY2				(lua(),"Cannot create new Lua thread");
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
		if ( ai().script_engine().debugger() )
			ai().script_engine().debugger()->add	( m_virtual_machine );
#endif // #if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
//		print_stack_		(ai().script_engine().lua());
//		m_thread_reference	= luaL_ref(ai().script_engine().lua(),LUA_REGISTRYINDEX);
//		print_stack_		(ai().script_engine().lua());
		
//		if (g_ca_stdout[0]) {
//			fputc							(0,stderr);
//			ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeInfo,"%s",g_ca_stdout);
//			fflush							(stderr);
//		}
//		Msg					("lua get top %d",lua_gettop(ai().script_engine().lua()));
//		print_stack_		(ai().script_engine().lua());
		
#ifndef USE_LUA_STUDIO
#	ifdef DEBUG
#		ifdef USE_DEBUGGER
			if (ai().script_engine().debugger() && ai().script_engine().debugger()->Active())
				lua_sethook		(lua(), CDbgLuaHelper::hookLua,			LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET, 0);
			else
#		endif // #ifdef USE_DEBUGGER
				lua_sethook		(lua(),CScriptEngine::lua_hook_call,	LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET,	0);
#	endif // #ifdef DEBUG
#endif // #ifndef USE_LUA_STUDIO

		if (!do_string)
			xr_sprintf			(S,"%s.main()",caNamespaceName);
		else
			xr_sprintf			(S,"%s()",main_function);

		if (!ai().script_engine().load_buffer(lua(),S,xr_strlen(S),"@_thread_main"))
			return;

		m_active			= true;
	}
	catch(...) {
		m_active			= false;
	}
}

CScriptThread::~CScriptThread()
{
#ifdef DEBUG
	Msg						("* Destroying script thread %s",*m_script_name);
#endif
	try {
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
		if (ai().script_engine().debugger())
			ai().script_engine().debugger()->remove	( m_virtual_machine );
#endif // #if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
#ifndef LUABIND_HAS_BUGS_WITH_LUA_THREADS
		luaL_unref			(ai().script_engine().lua(),LUA_REGISTRYINDEX,m_thread_reference);
#endif
	}
	catch(...) {
	}
}

bool CScriptThread::update()
{
	if (!m_active)
		R_ASSERT2		(false,"Cannot resume dead Lua thread!");

	try {
		ai().script_engine().current_thread	(this);
		
		int					l_iErrorCode = lua_resume(lua(),0);
		
		if (l_iErrorCode && (l_iErrorCode != LUA_YIELD)) {
			ai().script_engine().print_output	(lua(),*script_name(),l_iErrorCode);
			ai().script_engine().on_error		(ai().script_engine().lua());
#ifdef DEBUG
			print_stack		(lua());
#endif
			m_active		= false;
		}
		else {
			if (l_iErrorCode != LUA_YIELD) {
#ifdef DEBUG
				if (m_current_stack_level) {
					ai().script_engine().print_output	(lua(),*script_name(),l_iErrorCode);
					ai().script_engine().on_error		(ai().script_engine().lua());
//					print_stack		(lua());
				}
#endif // DEBUG
				m_active	= false;
#ifdef DEBUG
				ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeInfo,"Script %s is finished!",*m_script_name);
#endif // DEBUG
			}
			else {
				VERIFY2		(!lua_gettop(lua()),"Do not pass any value to coroutine.yield()!");
			}
		}
		
		ai().script_engine().current_thread	(0);
	}
	catch(...) {
		ai().script_engine().current_thread	(0);
		m_active		= false;
	}
	return				(m_active);
}
