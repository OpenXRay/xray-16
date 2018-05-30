#include "pch.hpp"
#include "script_debugger.hpp"
#include "script_lua_helper.hpp"
#include "mslotutils.h"

// CScriptDebugger* CScriptDebugger::m_pDebugger = NULL;

void CScriptDebugger::SendMessageToIde(CMailSlotMsg& msg)
{
    if (CheckExisting(IDE_MAIL_SLOT))
    {
        SendMailslotMessage(IDE_MAIL_SLOT, msg);
        m_bIdePresent = true;
    }
    else
        m_bIdePresent = false;
}

LRESULT CScriptDebugger::_SendMessage(u32 message, WPARAM wParam, LPARAM lParam)
{
    //	if ( (m_pDebugger)&&(m_pDebugger->Active())&&(message >= _DMSG_FIRST_MSG && message <= _DMSG_LAST_MSG) )
    //		return m_pDebugger->DebugMessage(message, wParam, lParam);
    if (Active() && message >= _DMSG_FIRST_MSG && message <= _DMSG_LAST_MSG)
        return DebugMessage(message, wParam, lParam);
    return 0;
}

LRESULT CScriptDebugger::DebugMessage(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    CMailSlotMsg msg;
    switch (nMsg)
    {
    case DMSG_NEW_CONNECTION:
        msg.w_int(DMSG_NEW_CONNECTION);
        SendMessageToIde(msg);
        break;

    case DMSG_CLOSE_CONNECTION:
        msg.w_int(DMSG_CLOSE_CONNECTION);
        SendMessageToIde(msg);
        break;

    case DMSG_WRITE_DEBUG:
        msg.w_int(DMSG_WRITE_DEBUG);
        msg.w_string((char*)wParam);
        SendMessageToIde(msg);
        break;

    case DMSG_GOTO_FILELINE:
        msg.w_int(DMSG_GOTO_FILELINE);
        msg.w_string((char*)wParam);
        msg.w_int((int)lParam);
        SendMessageToIde(msg);
        break;
    case DMSG_DEBUG_BREAK:
        msg.w_int(DMSG_ACTIVATE_IDE);
        SendMessageToIde(msg);
        WaitForReply(true);
        break;
    case DMSG_CLEAR_STACKTRACE:
        m_callStack->Clear();
        msg.w_int(DMSG_CLEAR_STACKTRACE);
        SendMessageToIde(msg);
        break;
    case DMSG_ADD_STACKTRACE:
    {
        auto stackTrace = (StackTrace*)wParam;
        m_callStack->Add(stackTrace->szDesc, stackTrace->szFile, stackTrace->nLine);
        msg.w_int(DMSG_ADD_STACKTRACE);
        msg.w_buff(stackTrace, sizeof(StackTrace));
        SendMessageToIde(msg);
        break;
    }
    case DMSG_GOTO_STACKTRACE_LEVEL:
        m_callStack->GotoStackTraceLevel((int)wParam);
        StackLevelChanged();
        break;
    case DMSG_CLEAR_LOCALVARIABLES:
        msg.w_int(DMSG_CLEAR_LOCALVARIABLES);
        SendMessageToIde(msg);
        break;
    case DMSG_ADD_LOCALVARIABLE:
        msg.w_int(DMSG_ADD_LOCALVARIABLE);
        msg.w_buff((void*)wParam, sizeof(Variable));
        SendMessageToIde(msg);
        break;
    case DMSG_CLEAR_THREADS:
        msg.w_int(DMSG_CLEAR_THREADS);
        SendMessageToIde(msg);
        break;
    case DMSG_ADD_THREAD:
        msg.w_int(DMSG_ADD_THREAD);
        msg.w_buff((void*)wParam, sizeof(SScriptThread));
        SendMessageToIde(msg);
        break;
    case DMSG_THREAD_CHANGED: DrawThreadInfo((int)wParam); break;
    case DMSG_GET_VAR_TABLE: DrawVariableInfo((char*)wParam); break;
    case DMSG_EVAL_WATCH:
    {
        string2048 res;
        res[0] = 0;
        Eval((const char*)wParam, res, sizeof(res));
        msg.w_int(DMSG_EVAL_WATCH);
        msg.w_string(res);
        msg.w_string((const char*)wParam);
        SendMessageToIde(msg);
        break;
    }
    }
    return 0;
}

BOOL CScriptDebugger::Active() { return m_bIdePresent; }
CScriptDebugger::CScriptDebugger(CScriptEngine* scriptEngine)
{
    this->scriptEngine = scriptEngine;
    m_threads = new CDbgScriptThreads(scriptEngine, this);
    m_callStack = new CScriptCallStack(this);
    m_lua = new CDbgLuaHelper(this);
    ZeroMemory(m_curr_connected_mslot, sizeof(m_curr_connected_mslot));
    //	m_pDebugger					= this;
    m_nLevel = 0;
    m_mailSlot = CreateMailSlotByName(DEBUGGER_MAIL_SLOT);
#if defined(WINDOWS)
    if (m_mailSlot == INVALID_HANDLE_VALUE)
    {
        m_bIdePresent = false;
        return;
    }
#endif
    Connect(IDE_MAIL_SLOT);
}

void CScriptDebugger::Connect(LPCSTR mslot_name)
{
    m_bIdePresent = CheckExisting(IDE_MAIL_SLOT);
    ZeroMemory(m_curr_connected_mslot, sizeof(m_curr_connected_mslot));
    if (Active())
    {
        _SendMessage(DMSG_NEW_CONNECTION, 0, 0);
        CMailSlotMsg msg;
        msg.w_int(DMSG_GET_BREAKPOINTS);
        SendMessageToIde(msg);
        WaitForReply(false);
        xr_strcat(m_curr_connected_mslot, mslot_name);
    }
}

CScriptDebugger::~CScriptDebugger()
{
    if (Active())
        _SendMessage(DMSG_CLOSE_CONNECTION, 0, 0);
#if defined(WINDOWS)
    CloseHandle(m_mailSlot);
#endif
    xr_delete(m_threads);
    xr_delete(m_callStack);
    xr_delete(m_lua);
}

void CScriptDebugger::UnPrepareLua(lua_State* l, int idx)
{
    if (idx == -1)
        return; // !Active()
    m_lua->UnPrepareLua(l, idx);
}

int CScriptDebugger::PrepareLua(lua_State* l)
{
    // call this function immediatly before calling lua_pcall.
    // returns index in stack for errorFunc
    if (!Active())
        return -1;
    m_nMode = DMOD_NONE;
    return m_lua->PrepareLua(l);
}

BOOL CScriptDebugger::PrepareLuaBind()
{
    if (!Active())
        return FALSE;
    m_lua->PrepareLuaBind();
    m_nMode = DMOD_NONE;
    return TRUE;
}

void CScriptDebugger::initiateDebugBreak() { m_nMode = DMOD_BREAK; }
void CScriptDebugger::Write(const char* szMsg) { _SendMessage(DMSG_WRITE_DEBUG, (WPARAM)szMsg, 0); }
void CScriptDebugger::LineHook(const char* szFile, int nLine)
{
    CheckNewMessages();
    if (m_nMode == DMOD_STOP)
    {
        // Console->Execute("quit");
        return;
    }
    if (HasBreakPoint(szFile, nLine) || m_nMode == DMOD_STEP_INTO || m_nMode == DMOD_BREAK ||
        m_nMode == DMOD_STEP_OVER && m_nLevel <= 0 || m_nMode == DMOD_STEP_OUT && m_nLevel < 0 ||
        m_nMode == DMOD_RUN_TO_CURSOR && xr_strcmp(m_strPathName, szFile) && m_nLine == nLine)
    {
        DebugBreak(szFile, nLine);
        GetBreakPointsFromIde();
    }
}

void CScriptDebugger::FunctionHook(const char* szFile, int nLine, BOOL bCall)
{
    if (m_nMode == DMOD_STOP)
        return;
    m_nLevel += bCall ? 1 : -1;
}

void CScriptDebugger::DrawThreadInfo(int nThreadID)
{
    // find corresponding lua_state
    lua_State* ls = m_threads->FindScript(nThreadID);
    if (!ls)
        return;
    m_lua->set_lua(ls);
    DrawCurrentState();
}

void CScriptDebugger::DrawCurrentState()
{
    m_lua->DrawStackTrace();
    m_callStack->SetStackTraceLevel(0);
    m_lua->DrawGlobalVariables();
    _SendMessage(DMSG_GOTO_STACKTRACE_LEVEL, GetStackTraceLevel(), 0);
}

void CScriptDebugger::DebugBreak(const char* szFile, int nLine)
{
    m_nMode = DMOD_NONE;
    m_threads->Fill();
    m_threads->DrawThreads();
    DrawCurrentState();
    _SendMessage(DMSG_DEBUG_BREAK, 0, 0);
}

void CScriptDebugger::GetBreakPointsFromIde()
{
    CMailSlotMsg msg;
    msg.w_int(DMSG_GET_BREAKPOINTS);
    SendMessageToIde(msg);
    WaitForReply(false);
}

void CScriptDebugger::ErrorBreak(const char* szFile, int nLine)
{
    if (Active())
        DebugBreak(szFile, nLine);
}

void CScriptDebugger::ClearStackTrace() { _SendMessage(DMSG_CLEAR_STACKTRACE, 0, 0); }
void CScriptDebugger::AddStackTrace(const char* szDesc, const char* szFile, int nLine)
{
    StackTrace st;
    xr_strcat(st.szDesc, szDesc);
    xr_strcat(st.szFile, szFile);
    st.nLine = nLine;
    _SendMessage(DMSG_ADD_STACKTRACE, (WPARAM)&st, 0);
}

int CScriptDebugger::GetStackTraceLevel() { return m_callStack->GetLevel(); }
void CScriptDebugger::StackLevelChanged() { m_lua->DrawLocalVariables(); }
void CScriptDebugger::DrawVariableInfo(char* varName) { m_lua->DrawVariableInfo(varName); }
void CScriptDebugger::ClearLocalVariables() { _SendMessage(DMSG_CLEAR_LOCALVARIABLES, 0, 0); }
void CScriptDebugger::AddLocalVariable(const Variable& var) { _SendMessage(DMSG_ADD_LOCALVARIABLE, (WPARAM)&var, 0); }
void CScriptDebugger::ClearGlobalVariables() { _SendMessage(DMSG_CLEAR_GLOBALVARIABLES, 0, 0); }
void CScriptDebugger::AddGlobalVariable(const char* name, const char* type, const char* value)
{
    Variable var;
    xr_strcat(var.szName, name);
    xr_strcat(var.szType, type);
    xr_strcat(var.szValue, value);
    _SendMessage(DMSG_ADD_GLOBALVARIABLE, (WPARAM)&var, 0);
}

void CScriptDebugger::Eval(const char* strCode, char* res, int res_sz)
{
    string1024 strCodeFull;
    strCodeFull[0] = 0;
    const char* r = "return  ";
    strconcat(sizeof(strCodeFull), strCodeFull, r, strCode);
    m_lua->Eval(strCodeFull, res, res_sz);
}

void CScriptDebugger::CheckNewMessages()
{
    CMailSlotMsg msg;
    while (CheckMailslotMessage(m_mailSlot, msg))
    {
        TranslateIdeMessage(&msg);
    }
}

void CScriptDebugger::WaitForReply(bool bWaitForModalResult) // UINT nMsg)
{
    bool mr = false;
    do
    {
        CMailSlotMsg msg;
        while (true)
        {
            if (CheckMailslotMessage(m_mailSlot, msg))
                break;
            Sleep(10);
        }
        R_ASSERT(msg.GetLen());
        mr = TranslateIdeMessage(&msg); // mr--is this an ide modalResult ?
    } while (bWaitForModalResult && !mr);
}

bool CScriptDebugger::TranslateIdeMessage(CMailSlotMsg* msg)
{
    int nType;
    msg->r_int(nType);
    switch (nType)
    {
    case DMSG_DEBUG_GO: m_nMode = DMOD_NONE; return true;
    case DMSG_DEBUG_BREAK: m_nMode = DMOD_BREAK; return true;
    case DMSG_DEBUG_STEP_INTO: m_nMode = DMOD_STEP_INTO; return true;
    case DMSG_DEBUG_STEP_OVER:
        m_nLevel = 0;
        m_nMode = DMOD_STEP_OVER;
        return true;
    case DMSG_DEBUG_STEP_OUT:
        m_nLevel = 0;
        m_nMode = DMOD_STEP_OUT;
        return true;
    case DMSG_DEBUG_RUN_TO_CURSOR:
        // DMOD_RUN_TO_CURSOR;
        return true;
    case DMSG_STOP_DEBUGGING:
        m_nMode = DMOD_STOP;
        // Console->Execute("quit");
        return true;
    case DMSG_GOTO_STACKTRACE_LEVEL:
    {
        int nLevel;
        msg->r_int(nLevel);
        _SendMessage(DMSG_GOTO_STACKTRACE_LEVEL, nLevel, 0);
        return false;
    }
    case DMSG_GET_BREAKPOINTS: FillBreakPointsIn(msg); return false;
    case DMSG_THREAD_CHANGED:
    {
        int nThreadID;
        msg->r_int(nThreadID);
        _SendMessage(DMSG_THREAD_CHANGED, nThreadID, 0);
        return false;
    }
    case DMSG_GET_VAR_TABLE:
    {
        string512 varName;
        varName[0] = 0;
        msg->r_string(varName);
        _SendMessage(DMSG_GET_VAR_TABLE, (WPARAM)varName, 0);
        return false;
    }
    case DMSG_EVAL_WATCH:
    {
        string2048 watch;
        watch[0] = 0;
        int iItem;
        msg->r_string(watch);
        msg->r_int(iItem);
        _SendMessage(DMSG_EVAL_WATCH, (WPARAM)watch, (LPARAM)iItem);
        return false;
    }
    default: return false;
    }
}

bool CScriptDebugger::HasBreakPoint(const char* fileName, s32 lineNum)
{
    string256 sFileName;
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char ext[_MAX_EXT];
    _splitpath(fileName, drive, dir, sFileName, ext);
    for (u32 i = 0; i < m_breakPoints.size(); i++)
    {
        SBreakPoint bp(m_breakPoints[i]);
        if (bp.nLine == lineNum && xr_strlen(bp.fileName) == xr_strlen(sFileName) && !xr_stricmp(*bp.fileName, sFileName))
            return true;
    }
    return false;
}

void CScriptDebugger::FillBreakPointsIn(CMailSlotMsg* msg)
{
    m_breakPoints.clear();
    s32 nCount = 0;
    msg->r_int(nCount);
    for (s32 i = 0; i < nCount; i++)
    {
        SBreakPoint bp;
        string256 fn;
        msg->r_string(fn);
        bp.fileName = fn;
        s32 bpCount = 0;
        msg->r_int(bpCount);
        for (s32 j = 0; j < bpCount; j++)
        {
            msg->r_int(bp.nLine);
            m_breakPoints.push_back(bp);
        }
    }
}

void CScriptDebugger::ClearThreads() { _SendMessage(DMSG_CLEAR_THREADS, 0, 0); }
void CScriptDebugger::AddThread(SScriptThread& th) { _SendMessage(DMSG_ADD_THREAD, (WPARAM)(&th), 0); }
