#include "stdafx.h"

#include "script_CallStack.h"
#include "script_debugger.h"

CScriptCallStack::CScriptCallStack(CScriptDebugger*d)
:m_debugger(d)
{}

CScriptCallStack::~CScriptCallStack()
{}

/*
int CCallStack::OnSci(CScintillaView* pView, SCNotification* pNotify)
{
	CLuaEditor* pEditor = ((CScintillaView*)GetView(0))->GetEditor();

	CPoint pt;
	int nLine;
	CString strLine;
	switch (pNotify->nmhdr.code)
	{
	case SCN_DOUBLECLICK:
		GetCursorPos(&pt);
		pEditor->ScreenToClient(&pt);
		nLine = pEditor->LineFromPoint(pt);
		GotoStackTraceLevel(nLine-1);
		break;
	};

	return 0;
}
*/

void CScriptCallStack::Clear()
{
	m_nCurrentLevel = -1;
	m_lines.clear();
	m_files.clear();
}

void CScriptCallStack::Add(const char *szDesc, const char *szFile, int nLine)
{
	m_lines.push_back(nLine);

	SPath	sp;
	sp.path[0] = 0;
	m_files.push_back(sp);
	xr_strcat(m_files.back().path, szFile );
}

void CScriptCallStack::SetStackTraceLevel(int nLevel)
{
	m_nCurrentLevel = nLevel;
	VERIFY( nLevel>=0 || (u32)nLevel < m_files.size() );
}

void CScriptCallStack::GotoStackTraceLevel(int nLevel)
{
	if ( nLevel<0 || (u32)nLevel >= m_files.size() )
		return;

	m_nCurrentLevel = nLevel;

	char * ppath = m_files[nLevel].path;
	m_debugger->_SendMessage(	DMSG_GOTO_FILELINE,
									(WPARAM)ppath,
									(LPARAM)m_lines[nLevel]);

}


