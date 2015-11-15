////////////////////////////////////////////////////////////////////////////
//	Module 		: script_stack_tracker.h
//	Created 	: 21.04.2004
//  Modified 	: 21.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Script stack tracker
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCore/xrCore.h"
#include "xrScriptEngine/xrScriptEngine.hpp"

struct lua_Debug;
struct lua_State;
class CScriptEngine;

class XRSCRIPTENGINE_API CScriptStackTracker
{
protected:
    enum consts
    {
        max_stack_size = u32(256),
    };

protected:
    CScriptEngine *scriptEngine;
    lua_Debug *m_stack[max_stack_size];
    int m_current_stack_level;

public:
    CScriptStackTracker(CScriptEngine *scriptEngine);
    virtual ~CScriptStackTracker();
    void script_hook(lua_State *L, lua_Debug *dbg);
    void print_stack(lua_State *L);
};
