////////////////////////////////////////////////////////////////////////////
//	Module 		: script_thread.h
//	Created 	: 19.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script thread class
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef DEBUG
#	include "script_stack_tracker.h"
#endif

struct lua_State;

#ifdef DEBUG
	class CScriptThread : public CScriptStackTracker
#else
	class CScriptThread
#endif
{
private:
	shared_str				m_script_name;
	int						m_thread_reference;
	bool					m_active;
	lua_State				*m_virtual_machine;

#ifdef DEBUG
protected:
	static	void			lua_hook_call		(lua_State *L, lua_Debug *dbg);
#endif

public:
							CScriptThread		(LPCSTR caNamespaceName, bool do_string = false, bool reload = false);
	virtual					~CScriptThread		();
			bool			update				();
	IC		bool			active				() const;
	IC		shared_str		script_name			() const;
	IC		int				thread_reference	() const;
	IC		lua_State		*lua				() const;
};

#include "script_thread_inline.h"