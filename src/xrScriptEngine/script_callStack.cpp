#include "pch.hpp"
#include "script_callStack.hpp"
#include "script_debugger.hpp"

CScriptCallStack::CScriptCallStack(CScriptDebugger *d)
{
    m_debugger = d;
    m_nCurrentLevel = -1;
}

CScriptCallStack::~CScriptCallStack() {}

void CScriptCallStack::Clear()
{
    m_nCurrentLevel = -1;
    m_lines.clear();
    m_files.clear();
}

void CScriptCallStack::Add(const char *szDesc, const char *szFile, int nLine)
{
    m_lines.push_back(nLine);
    SPath sp;
    sp.path[0] = 0;
    m_files.push_back(sp);
    xr_strcat(m_files.back().path, szFile);
}

void CScriptCallStack::SetStackTraceLevel(int nLevel)
{
    m_nCurrentLevel = nLevel;
    VERIFY(nLevel>=0 || (u32)nLevel<m_files.size());
}

void CScriptCallStack::GotoStackTraceLevel(int nLevel)
{
    if (nLevel<0 || (u32)nLevel>=m_files.size())
        return;
    m_nCurrentLevel = nLevel;
    char *ppath = m_files[nLevel].path;
    m_debugger->_SendMessage(DMSG_GOTO_FILELINE, (WPARAM)ppath, (LPARAM)m_lines[nLevel]);
}
