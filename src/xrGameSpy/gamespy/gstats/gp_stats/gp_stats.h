// gp_stats.h : main header file for the GP_STATS application
//

#if !defined(AFX_GP_STATS_H__90B1DDCE_A86E_49A8_B136_0B29EA43F6B4__INCLUDED_)
#define AFX_GP_STATS_H__90B1DDCE_A86E_49A8_B136_0B29EA43F6B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGp_statsApp:
// See gp_stats.cpp for the implementation of this class
//

class CGp_statsApp : public CWinApp
{
public:
	CGp_statsApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGp_statsApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGp_statsApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GP_STATS_H__90B1DDCE_A86E_49A8_B136_0B29EA43F6B4__INCLUDED_)
