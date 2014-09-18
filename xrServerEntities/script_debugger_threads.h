#pragma once
#include "script_debugger_messages.h"

class CScriptProcess;
class CScriptDebugger;
struct lua_State;

class CDbgScriptThreads
{
	xr_vector<SScriptThread>				m_threads;
public:
	CScriptDebugger*						m_debugger;				
	CDbgScriptThreads			(CScriptDebugger* d):m_debugger(d){};
				~CDbgScriptThreads			(){};
	u32			FillFrom					(CScriptProcess*);
	u32			Fill						();
	lua_State*	FindScript					(int nthreadID);
	void		DrawThreads					();
};