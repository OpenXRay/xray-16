////////////////////////////////////////////////////////////////////////////
//	Module 		: script_stack_tracker.h
//	Created 	: 21.04.2004
//  Modified 	: 21.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Script stack tracker
////////////////////////////////////////////////////////////////////////////

#pragma once

struct lua_Debug;
struct lua_State;

class CScriptStackTracker {
protected:
	enum consts {
		max_stack_size = u32(256),
	};

protected:
	lua_Debug					*m_stack[max_stack_size];
	int							m_current_stack_level;

public:
								CScriptStackTracker		();
	virtual						~CScriptStackTracker	();
			void				script_hook				(lua_State *L, lua_Debug *dbg);
			void				print_stack				(lua_State *L);
};

#include "script_stack_tracker_inline.h"