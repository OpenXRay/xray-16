////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_script_lua_debug.cpp
//	Created 	: 19.09.2003
//  Modified 	: 19.09.2003
//	Author		: Dmitriy Iassenev
//	Description : XRay Script debugging system
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_script_space.h"
#include "ai_script_lua_extension.h"

using namespace Script;

bool Script::bfPrintOutput(CLuaVirtualMachine *tpLuaVirtualMachine, LPCSTR caScriptFileName, int iErorCode)
{
	for (int i=-1; ; --i)
		if (lua_isstring(tpLuaVirtualMachine,i)) {
			LPCSTR	S = lua_tostring(tpLuaVirtualMachine,i);
			if (!xr_strcmp(S,"cannot resume dead coroutine")) {
				LuaOut	(Lua::eLuaMessageTypeInfo,"Script %s is finished",caScriptFileName);
				return	(true);
			}
			else {
				if (!i && !iErorCode)
					LuaOut	(Lua::eLuaMessageTypeInfo,"Output from %s",caScriptFileName);
				LuaOut	(iErorCode ? Lua::eLuaMessageTypeError : Lua::eLuaMessageTypeMessage,"%s",S);
			}
		}
		else {
			for ( i=0; ; ++i)
				if (lua_isstring(tpLuaVirtualMachine,i)) {
					LPCSTR	S = lua_tostring(tpLuaVirtualMachine,i);
					if (!xr_strcmp(S,"cannot resume dead coroutine")) {
						LuaOut	(Lua::eLuaMessageTypeInfo,"Script %s is finished",caScriptFileName);
						return	(true);
					}
					else {
						if (!i && !iErorCode)
							LuaOut	(Lua::eLuaMessageTypeInfo,"Output from %s",caScriptFileName);
						LuaOut	(iErorCode ? Lua::eLuaMessageTypeError : Lua::eLuaMessageTypeMessage,"%s",S);
					}
				}
				else
					return		(false);
		}
}

void Script::vfPrintError(CLuaVirtualMachine *tpLuaVirtualMachine, int iErrorCode)
{
	switch (iErrorCode) {
		case LUA_ERRRUN : {
			Msg ("! SCRIPT RUNTIME ERROR");
			break;
		}
		case LUA_ERRMEM : {
			Msg ("! SCRIPT ERROR (memory allocation)");
			break;
		}
		case LUA_ERRERR : {
			Msg ("! SCRIPT ERROR (while running the error handler function)");
			break;
		}
		case LUA_ERRFILE : {
			Msg ("! SCRIPT ERROR (while running file)");
			break;
		}
		case LUA_ERRSYNTAX : {
			Msg ("! SCRIPT SYNTAX ERROR");
			break;
		}
		default : NODEFAULT;
	}
	
	for (int i=0; ; ++i) {
		Msg		("! Stack level %d",i);
		if (!bfListLevelVars(tpLuaVirtualMachine,i))
			return;
	}
}

LPCSTR Script::cafEventToString(int iEventCode)
{
	switch (iEventCode) {
		case LUA_HOOKCALL		: return("hook call");
		case LUA_HOOKRET		: return("hook return");
		case LUA_HOOKLINE		: return("hook line");
		case LUA_HOOKCOUNT		: return("hook count");
		case LUA_HOOKTAILRET	: return("hook tail return");
		default					: NODEFAULT;
	}
#ifdef DEBUG
	return(0);
#endif
}

bool Script::bfListLevelVars(CLuaVirtualMachine *tpLuaVirtualMachine, int iStackLevel)
{
//	lua_Debug	l_tDebugInfo;
//	int			i;
//	const char	*name;
//
//	if (lua_getstack(tpLuaVirtualMachine, iStackLevel, &l_tDebugInfo) == 0)
//		return	(false);  /* failure: no such level in the stack */
//	Msg			("  Event			: %d",cafEventToString(l_tDebugInfo.event));
//	Msg			("  Name			: %s",l_tDebugInfo.name);
//	Msg			("  Name what		: %s",l_tDebugInfo.namewhat);
//	Msg			("  What			: %s",l_tDebugInfo.what);
//	Msg			("  Source			: %s",l_tDebugInfo.source);
//	Msg			("  Source (short)	: %s",l_tDebugInfo.short_src);
//	Msg			("  Current line	: %d",l_tDebugInfo.currentline);
//	Msg			("  Nups			: %d",l_tDebugInfo.nups);
//	Msg			("  Line defined	: %d",l_tDebugInfo.linedefined);
//	i			= 1;
//	while (NULL != (name = lua_getlocal(tpLuaVirtualMachine, &l_tDebugInfo, i++))) {
//		Msg		("    local   %d %s", i-1, name);
//		lua_pop	(tpLuaVirtualMachine, 1);  /* remove variable value */
//	}
//
//	lua_getinfo	(tpLuaVirtualMachine, "f", &l_tDebugInfo);  /* retrieves function */
//	i = 1;
//	while (NULL != (name = lua_getupvalue(tpLuaVirtualMachine, -1, i++))) {
//		Msg		("    upvalue %d %s", i-1, name);
//		lua_pop	(tpLuaVirtualMachine, 1);  /* remove upvalue value */
//	}
//	return		(true);
	return		(false);
}
