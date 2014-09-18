////////////////////////////////////////////////////////////////////////////
//	Module 		: script_stack_tracker.cpp
//	Created 	: 21.04.2004
//  Modified 	: 21.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Script stack tracker
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_stack_tracker.h"
#include "script_storage_space.h"
#include "ai_space.h"
#include "script_engine.h"

CScriptStackTracker::CScriptStackTracker	()
{
	m_current_stack_level	= 0;
	for (int i=0; i<max_stack_size; ++i)
		m_stack[i]			= xr_new<lua_Debug>();
}

CScriptStackTracker::~CScriptStackTracker	()
{
	for (int i=0; i<max_stack_size; ++i)
		xr_delete			(m_stack[i]);
}

void CScriptStackTracker::script_hook	(lua_State *L, lua_Debug *dbg)
{
	VERIFY				(L);// && (m_virtual_machine == L));

	switch	(dbg->event) {
		case LUA_HOOKCALL : {
			if (m_current_stack_level >= max_stack_size)
				return;
			if (!lua_getstack(L,0,m_stack[m_current_stack_level]))
				break;
			lua_getinfo	(L,"nSlu",m_stack[m_current_stack_level]);
			if (m_current_stack_level && lua_getstack(L,1,m_stack[m_current_stack_level - 1]))
				lua_getinfo	(L,"nSlu",m_stack[m_current_stack_level - 1]);
			++m_current_stack_level;
			break;
		}
		case LUA_HOOKRET : {
			if (m_current_stack_level > 0)
				--m_current_stack_level;
			break;
		}
		case LUA_HOOKTAILRET : {
			if (m_current_stack_level > 0)
				--m_current_stack_level;
			break;
		}
		case LUA_HOOKLINE : {
			lua_getinfo	(L,"l",dbg);
			m_stack[m_current_stack_level]->currentline = dbg->currentline;
			break;
		}
		case LUA_HOOKCOUNT : {
			lua_getinfo	(L,"l",dbg);
			m_stack[m_current_stack_level]->currentline = dbg->currentline;
			break;
		}
		default : NODEFAULT;
	}
}

void CScriptStackTracker::print_stack	(lua_State *L)
{
	VERIFY					(L);// && (m_virtual_machine == L));

	for (int j=m_current_stack_level - 1, k=0; j>=0; --j, ++k) {
		lua_Debug			l_tDebugInfo = *m_stack[j];
		if (!l_tDebugInfo.name)
			ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"%2d : [%s] %s(%d) : %s",k,l_tDebugInfo.what,l_tDebugInfo.short_src,l_tDebugInfo.currentline,"");
		else
			if (!xr_strcmp(l_tDebugInfo.what,"C"))
				ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"%2d : [C  ] %s",k,l_tDebugInfo.name);
			else
				ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"%2d : [%s] %s(%d) : %s",k,l_tDebugInfo.what,l_tDebugInfo.short_src,l_tDebugInfo.currentline,l_tDebugInfo.name);
	}
	m_current_stack_level	= 0;
}
