#pragma once
#include "xrCore/xrCore.h"
#include "xrScriptEngine/xrScriptEngine.hpp"
#include "script_debugger_messages.hpp"

class CScriptProcess;
class CScriptDebugger;
struct lua_State;

class XRSCRIPTENGINE_API CDbgScriptThreads
{
private:
    xr_vector<SScriptThread> m_threads;
public:
    CScriptDebugger *m_debugger;

    CDbgScriptThreads(CScriptDebugger *d) { m_debugger = nullptr; }
    ~CDbgScriptThreads() {};
    u32 FillFrom(CScriptProcess *);
    u32 Fill();
    lua_State *FindScript(int nthreadID);
    void DrawThreads();
};
