#include "stdafx.h"

#ifdef SCRIPT_FILE

#include "script_file.h"
#include "script_lua_helper.h"

CScriptFile::CScriptFile()
{
	RemoveAllDebugLines();
	RemoveAllBreakPoints();

//	GetSystemTime(&m_timeCompiled);
}

CScriptFile::~CScriptFile()
{
}

void CScriptFile::RemoveAllDebugLines()
{
	m_nMinDebugLine = 2147483647;
	m_nMaxDebugLine = 0;

//	m_debugLines.RemoveAll();
	m_debugLines.clear();
}

void CScriptFile::AddDebugLine(int nLine)
{
	m_debugLines[nLine] = 1;
	if ( nLine<m_nMinDebugLine )
		m_nMinDebugLine = nLine;
	if ( nLine>m_nMaxDebugLine )
		m_nMaxDebugLine = nLine;
}

void CScriptFile::RemoveAllBreakPoints()
{
	m_nMinBreakPoint = 2147483647;
	m_nMaxBreakPoint = 0;

//	m_breakPoints.RemoveAll();
	m_breakPoints.clear();

//	CProject* pProject = ((CMainFrame*)AfxGetMainWnd())->GetProject();
//	pProject->SetModifiedFlag(TRUE);
}

void CScriptFile::AddBreakPoint(int nLine)
{
	m_breakPoints[nLine] = 1;
	if ( nLine<m_nMinBreakPoint)
		m_nMinBreakPoint = nLine;
	if ( nLine>m_nMaxBreakPoint )
		m_nMaxBreakPoint = nLine;

//	CProject* pProject = ((CMainFrame*)AfxGetMainWnd())->GetProject();
//	pProject->SetModifiedFlag(TRUE);
}

void CScriptFile::RemoveBreakPoint(int nLine)
{
//	m_breakPoints.RemoveKey(nLine);
	m_breakPoints.erase(nLine);

	m_nMinBreakPoint = 2147483647;
	m_nMaxBreakPoint = 0;

	uniIt It = m_breakPoints.begin();
	for(;It!=m_breakPoints.end();++It)
	{
		if ( It->first<m_nMinBreakPoint)
			m_nMinBreakPoint = It->first;
		if ( It->first>m_nMaxBreakPoint )
			m_nMaxBreakPoint = It->first;

	}

/*	POSITION pos = m_breakPoints.GetStartPosition();
	int nTemp;
	while (pos != NULL)
	{
		m_breakPoints.GetNextAssoc( pos, nLine, nTemp );
		if ( nLine<m_nMinBreakPoint)
			m_nMinBreakPoint = nLine;
		if ( nLine>m_nMaxBreakPoint )
			m_nMaxBreakPoint = nLine;
	}
*/
//	CProject* pProject = ((CMainFrame*)AfxGetMainWnd())->GetProject();
//	pProject->SetModifiedFlag(TRUE);
}

int CScriptFile::GetNextDebugLine(int nLine)
{
	++nLine;

	while ( nLine<=m_nMaxDebugLine )
		if ( m_debugLines.find(nLine)!= m_debugLines.end() )
			return nLine;
		else
			++nLine;

	return 0;
}

int CScriptFile::GetPreviousDebugLine(int nLine)
{
	--nLine;

	while ( nLine>=m_nMinDebugLine )
		if ( m_debugLines.find(nLine)!= m_debugLines.end() )
			return nLine;
		else
			--nLine;

	return 0;
}

int CScriptFile::GetNearestDebugLine(int nLine)
{
	int nNearest;
	if ( m_debugLines.find(nLine) != m_debugLines.end() )
		return nLine;

	if ( (nNearest=GetNextDebugLine(nLine)) > 0 )
		return nNearest;

	if ( (nNearest=GetPreviousDebugLine(nLine)) > 0 )
		return nNearest;

	return 0;
}

BOOL CScriptFile::PositionBreakPoints()
{
	if ( !CDbgLuaHelper::LoadDebugLines(this) )
		return FALSE;

/*	BOOL bModified = FALSE;
	POSITION pos = m_breakPoints.GetStartPosition();
	int nLine, nTemp, nNearest;
	while (pos != NULL)
	{
		m_breakPoints.GetNextAssoc( pos, nLine, nTemp );
		nNearest = GetNearestDebugLine(nLine);
		if ( nNearest == 0 )
		{
			m_breakPoints.erase(nLine);
			bModified = TRUE;
		}
		else if ( nLine != nNearest )
		{
			m_breakPoints.erase(nLine);
			m_breakPoints[nNearest] = TRUE;
			bModified = TRUE;
		}
	}

	return bModified;
*/
	return TRUE;
}

const char* CScriptFile::GetName()
{
	return m_strPathName;
}

/*
CString CProjectFile::GetName()
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath( m_strPathName, drive, dir, fname, ext );
	return CString(fname);
}

CString CProjectFile::GetNameExt()
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath( m_strPathName, drive, dir, fname, ext );
	return CString(fname)+ext;
}
*/

BOOL CScriptFile::HasBreakPoint(int nLine)
{
	return m_breakPoints.find(nLine) != m_breakPoints.end();
}

void CScriptFile::SetBreakPointsIn(CLuaEditor *pEditor)
{
/*	pEditor->ClearAllBreakpoints();

	POSITION pos = m_breakPoints.GetStartPosition();
	int nLine, nTemp;
	while (pos != NULL)
	{
		m_breakPoints.GetNextAssoc( pos, nLine, nTemp );
		pEditor->SetBreakpoint(nLine);
	}
*/
}
/*
BOOL CScriptFile::HasFile(CString strPathName)
{
	if(!m_strPathName.CompareNoCase(strPathName))
		return TRUE;

	//should actually search using the luasearch path
	DWORD n=MAX_PATH;
	CString sFullPath;	
	::GetFullPathName(strPathName,n,sFullPath.GetBuffer(n),NULL);
	sFullPath.ReleaseBuffer();

	if(!m_strPathName.CompareNoCase(sFullPath))
		return TRUE;
	return FALSE;
}

*/
/*
BOOL CProjectFile::Load(CArchive &ar)
{
	RemoveAllDebugLines();
	RemoveAllBreakPoints();

	ar >> m_strRelPathName;

	CProject* pProject = ((CMainFrame*)AfxGetMainWnd())->GetProject();
	m_strPathName = pProject->GetProjectDir();
	PathAppend(m_strPathName.GetBuffer(MAX_PATH), m_strRelPathName);

	ar >> m_nMinBreakPoint;
	ar >> m_nMaxBreakPoint;

	int nBreakPoints;
	ar >> nBreakPoints;

	for ( int i=0; i<nBreakPoints; ++i )
	{
		int nLine;
		ar >> nLine;

		m_breakPoints[nLine] = 1;
	}

	return TRUE;
}

BOOL CProjectFile::Save(CArchive &ar)
{
	ar << m_strRelPathName;
	ar << m_nMinBreakPoint;
	ar << m_nMaxBreakPoint;

	int nBreakPoints = m_breakPoints.GetCount();
	ar << nBreakPoints;

	POSITION pos = m_breakPoints.GetStartPosition();
	int nLine, nTemp;
	while (pos != NULL)
	{
		m_breakPoints.GetNextAssoc( pos, nLine, nTemp );
		ar << nLine;
	}

	return TRUE;
}


BOOL CProjectFile::IsModified()
{
	WIN32_FILE_ATTRIBUTE_DATA sourceFile, compiledFile;

	if (! ::GetFileAttributesEx(m_strPathName, GetFileExInfoStandard, &sourceFile) )
		return TRUE;

	if (! ::GetFileAttributesEx(GetOutputPathNameExt(), GetFileExInfoStandard, &compiledFile) )
		return TRUE;

	ULARGE_INTEGER sourceTime, compiledTime;
	sourceTime.LowPart = sourceFile.ftLastWriteTime.dwLowDateTime;
	sourceTime.HighPart = sourceFile.ftLastWriteTime.dwHighDateTime;
	compiledTime.LowPart = compiledFile.ftLastWriteTime.dwLowDateTime;
	compiledTime.HighPart = compiledFile.ftLastWriteTime.dwHighDateTime;

	return ( sourceTime.QuadPart > compiledTime.QuadPart );
}

BOOL CProjectFile::Compile()
{
	CExecutor m_exe;

	COutputWnd* pWnd = ((CMainFrame*)AfxGetMainWnd())->GetOutputWnd();
	CScintillaView* pOutput = pWnd->GetOutput(COutputWnd::outputBuild);

	pOutput->Write(GetNameExt() + "\n");

	CString strCmdLine;
	strCmdLine.Format("\"%s\" -o \"%s\" \"%s\"", 
		theApp.GetModuleDir() + "\\" + "luac.exe", GetOutputPathNameExt(), GetPathName());

	m_exe.Execute(strCmdLine);
	CString strOutput = m_exe.GetOutputString();
	if ( !strOutput.IsEmpty() )
	{
		pOutput->Write(strOutput);
		return FALSE;
	}

	return TRUE;
}

void CProjectFile::DeleteIntermediateFiles()
{
	DeleteFile(GetOutputPathNameExt());
}

CString CProjectFile::GetOutputPathNameExt()
{
	CProject* pProject = ((CMainFrame*)AfxGetMainWnd())->GetProject();	
	return pProject->GetIntermediateDir() + "\\" + GetOutputNameExt();
}

void CProjectFile::UpdateRelPathName()
{
	CProject* pProject = ((CMainFrame*)AfxGetMainWnd())->GetProject();

	PathRelativePathTo(m_strRelPathName.GetBuffer(MAX_PATH), 
		pProject->GetProjectDir(), FILE_ATTRIBUTE_DIRECTORY,
		m_strPathName, 0);
	m_strRelPathName.ReleaseBuffer();
}


*/
#endif