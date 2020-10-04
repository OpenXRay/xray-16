#pragma once

#include "xrScriptEngine/xrScriptEngine.hpp"
#include "script_lua_helper.hpp"
#include "script_debugger_threads.hpp"
#include "script_callStack.hpp"
#include "script_debugger_messages.hpp"

class CMailSlotMsg;
struct lua_State;

#define DMOD_NONE 0
#define DMOD_STEP_INTO 1
#define DMOD_STEP_OVER 2
#define DMOD_STEP_OUT 3
#define DMOD_RUN_TO_CURSOR 4
//#define DMOD_SHOW_STACK_LEVEL 5
#define DMOD_BREAK 10
#define DMOD_STOP 11

struct SBreakPoint
{
    shared_str fileName;
    s32 nLine;

    SBreakPoint() { nLine = 0; }
    SBreakPoint(const SBreakPoint& other) { operator=(other); }
    SBreakPoint& operator=(const SBreakPoint& other)
    {
        fileName = other.fileName;
        nLine = other.nLine;
        return *this;
    }
};

class XRSCRIPTENGINE_API CScriptDebugger
{
public:
    void Connect(LPCSTR mslot_name);
    void Eval(const char* strCode, char* res, int res_sz);
    void AddLocalVariable(const Variable& var);
    void ClearLocalVariables();
    void AddGlobalVariable(const char* name, const char* type, const char* value);
    void ClearGlobalVariables();
    void StackLevelChanged();
    void initiateDebugBreak();
    void DebugBreak(const char* szFile, int nLine);
    void ErrorBreak(const char* szFile = nullptr, int nLine = 0);
    void LineHook(const char* szFile, int nLine);
    void FunctionHook(const char* szFile, int nLine, BOOL bCall);
    void Write(const char* szMsg);
    int PrepareLua(lua_State*);
    void UnPrepareLua(lua_State* l, int idx);
    BOOL PrepareLuaBind();

    CScriptDebugger(CScriptEngine* scriptEngine);
    virtual ~CScriptDebugger();

    void Go();
    void StepInto();
    void StepOver();
    void StepOut();
    void RunToCursor();
    void ClearThreads();
    void AddThread(SScriptThread&);
    void ClearStackTrace();
    void AddStackTrace(const char* strDesc, const char* strFile, int nLine);
    int GetStackTraceLevel();
    BOOL Active();
    // static CScriptDebugger* GetDebugger() { return m_pDebugger; }
    LRESULT _SendMessage(UINT message, WPARAM wParam, LPARAM lParam);

protected:
    void DrawVariableInfo(char* varName);
    void DrawCurrentState();
    void DrawThreadInfo(int nThreadID);
    void GetBreakPointsFromIde();
    void FillBreakPointsIn(CMailSlotMsg* msg);
    bool HasBreakPoint(const char* fileName, s32 lineNum);
    void CheckNewMessages();
    LRESULT DebugMessage(UINT nMsg, WPARAM wParam, LPARAM lParam);
    void WaitForReply(bool bWaitForModalResult);
    bool TranslateIdeMessage(CMailSlotMsg*);
    void SendMessageToIde(CMailSlotMsg&);

    CScriptEngine* scriptEngine;
    CDbgScriptThreads* m_threads;
    CDbgLuaHelper* m_lua;
    CScriptCallStack* m_callStack;
    // static CScriptDebugger *m_pDebugger;
    int m_nMode;
    int m_nLevel; // for step into/over/out
    string_path m_strPathName; // for run_to_line_number
    int m_nLine; // for run_to_line_number
    HANDLE m_mailSlot;
    BOOL m_bIdePresent;
    xr_vector<SBreakPoint> m_breakPoints;
    string_path m_curr_connected_mslot;
};
