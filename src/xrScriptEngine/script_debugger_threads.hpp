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
    CScriptEngine *scriptEngine;
    xr_vector<SScriptThread> m_threads;
public:
    CScriptDebugger *m_debugger;

    CDbgScriptThreads(CScriptEngine *scriptEngine, CScriptDebugger *debugger)
    {
        this->scriptEngine = scriptEngine;
        m_debugger = debugger;
    }
    ~CDbgScriptThreads() {};
    u32 FillFrom(CScriptProcess *);
    u32 Fill();
    lua_State *FindScript(int nthreadID);
    void DrawThreads();
};
