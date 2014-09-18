// ladderTrack.h : main header file for the LADDERTRACK application
//

#if !defined(AFX_LADDERTRACK_H__E9856F44_580A_48C0_ABFF_6FFA9BA944A3__INCLUDED_)
#define AFX_LADDERTRACK_H__E9856F44_580A_48C0_ABFF_6FFA9BA944A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CLadderTrackApp:
// See ladderTrack.cpp for the implementation of this class
//

class CLadderTrackApp : public CWinApp
{
public:
	CLadderTrackApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLadderTrackApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CLadderTrackApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LADDERTRACK_H__E9856F44_580A_48C0_ABFF_6FFA9BA944A3__INCLUDED_)
