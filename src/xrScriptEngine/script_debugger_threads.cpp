#include "pch.hpp"
#include "script_debugger_threads.hpp"
#include "script_process.hpp"
#include "script_engine.hpp"
#include "script_thread.hpp"
#include "script_debugger.hpp"
#include "Include/xrAPI/xrAPI.h"

u32 CDbgScriptThreads::Fill()
{
    u32 res = 0;
    if (!scriptEngine)
        return res;
    CScriptProcess* sp = scriptEngine->script_process(ScriptProcessor::Game);
	if (sp)
		res += FillFrom(sp);	
    sp = scriptEngine->script_process(ScriptProcessor::Level);
	if (sp)
		res += FillFrom(sp);
	return res;
}

u32 CDbgScriptThreads::FillFrom(CScriptProcess *sp)
{
    m_threads.clear();
    for (auto &scriptThread : sp->scripts())
    {
        SScriptThread th;
        // th.pScript = scriptThread;
        th.lua = scriptThread->lua();
        th.scriptID = scriptThread->thread_reference();
        th.active = scriptThread->active();
        xr_strcat(th.name, *scriptThread->script_name());
        xr_strcat(th.process, *sp->name());
        m_threads.push_back(th);
    }
    return m_threads.size();
}

lua_State *CDbgScriptThreads::FindScript(int nThreadID)
{
    for (auto &thread : m_threads)
    {
        if (thread.scriptID==nThreadID)
            return thread.lua;
    }
    return nullptr;
}

void CDbgScriptThreads::DrawThreads()
{
    //CScriptDebugger::GetDebugger()->ClearThreads();
    m_debugger->ClearThreads();
    for (auto &scripThread : m_threads)
    {
        SScriptThread th;
        th = scripThread;
        m_debugger->AddThread(th);
    }
}
