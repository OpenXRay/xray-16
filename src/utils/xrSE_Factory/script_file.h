#pragma once

#ifdef SCRIPT_FILE

class CLuaEditor;

class CScriptFile
{

public:
	void		UpdateRelPathName		();
//	void		DeleteIntermediateFiles	();
//	BOOL		Compile					();
//	BOOL		IsModified				();
//	BOOL		Save					(CArchive& ar);
//	BOOL		Load					(CArchive& ar);
//	BOOL		HasFile					(CString strPathName);
	void		RemoveBreakPoint		(int nLine);
	void		SetBreakPointsIn		(CLuaEditor* pEditor);
	BOOL		HasBreakPoint			(int nLine);
	CScriptFile							();
	~CScriptFile						();

	void		AddDebugLine			(int nLine);
	void		RemoveAllDebugLines		();
	void		AddBreakPoint			(int nLine);
	void		RemoveAllBreakPoints	();

	BOOL		PositionBreakPoints		();
	int			GetNearestDebugLine		(int nLine);
	int			GetPreviousDebugLine	(int nLine);
	int			GetNextDebugLine		(int nLine);

	const char* GetName();

/*	void SetPathName(CString strPathName) { m_strPathName=strPathName; UpdateRelPathName(); };
	CString GetPathName() { return m_strPathName; };
	CString GetName();
	CString GetNameExt();
	CString GetOutputNameExt() { return GetName()+".out"; }
	CString GetOutputPathNameExt();
*/
protected:
//	CString m_strPathName, m_strRelPathName;
	string_path						m_strPathName;
	string_path						m_strRelPathName;
//	CMap<int, int, BOOL, BOOL> m_breakPoints;
	xr_map<int,BOOL>				m_breakPoints;
	int								m_nMinBreakPoint;
	int								m_nMaxBreakPoint;
//	CMap<int, int, BOOL, BOOL> m_debugLines;
	xr_map<int,BOOL>				m_debugLines;
	int								m_nMinDebugLine;
	int								m_nMaxDebugLine;
//	SYSTEMTIME	m_timeCompiled;

	typedef xr_map<int,BOOL>::iterator	uniIt;
};

#endif
