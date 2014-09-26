// multiTrack.h : main header file for the MULTITRACK application
//

#if !defined(AFX_MULTITRACK_H__6EF40CA1_83C8_448A_883A_63AF15457C5E__INCLUDED_)
#define AFX_MULTITRACK_H__6EF40CA1_83C8_448A_883A_63AF15457C5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMultiTrackApp:
// See multiTrack.cpp for the implementation of this class
//

class CMultiTrackApp : public CWinApp
{
public:
	CMultiTrackApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiTrackApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMultiTrackApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MULTITRACK_H__6EF40CA1_83C8_448A_883A_63AF15457C5E__INCLUDED_)
