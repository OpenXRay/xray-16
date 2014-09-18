// track.h : main header file for the TRACK application
//

#if !defined(AFX_TRACK_H__0DF04935_1FD6_4741_AEC5_5A40455C0C7F__INCLUDED_)
#define AFX_TRACK_H__0DF04935_1FD6_4741_AEC5_5A40455C0C7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTrackApp:
// See track.cpp for the implementation of this class
//

class CTrackApp : public CWinApp
{
public:
	CTrackApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrackApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTrackApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRACK_H__0DF04935_1FD6_4741_AEC5_5A40455C0C7F__INCLUDED_)
