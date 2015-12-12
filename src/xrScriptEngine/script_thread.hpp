////////////////////////////////////////////////////////////////////////////
//	Module 		: script_thread.h
//	Created 	: 19.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script thread class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrScriptEngine/xrScriptEngine.hpp"
#include "xrCore/xrCore.h"
#ifdef DEBUG
#include "script_stack_tracker.hpp"
#endif

#define LUABIND_HAS_BUGS_WITH_LUA_THREADS

struct lua_State;

#ifdef DEBUG
class XRSCRIPTENGINE_API CScriptThread : public CScriptStackTracker
#else
class XRSCRIPTENGINE_API CScriptThread
#endif
{
    friend class CScriptEngine;
private:
    CScriptEngine *scriptEngine;
    shared_str m_script_name;
    int m_thread_reference;
    bool m_active;
    lua_State *m_virtual_machine;

private:
    CScriptThread(CScriptEngine *scriptEngine, LPCSTR caNamespaceName, bool do_string = false, bool reload = false);
public:
    virtual ~CScriptThread();
    bool update();
    bool active() const { return m_active; }
    shared_str script_name() const { return m_script_name; }
    int thread_reference() const { return m_thread_reference; }
    lua_State *lua() const { return m_virtual_machine; }
};
