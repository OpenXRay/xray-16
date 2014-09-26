// PeerTest.h : main header file for the PEERTEST application
//

#if !defined(AFX_PEERTEST_H__2F625A3C_93C3_4689_9749_3B9FCBE53594__INCLUDED_)
#define AFX_PEERTEST_H__2F625A3C_93C3_4689_9749_3B9FCBE53594__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CPeerTestApp:
// See PeerTest.cpp for the implementation of this class
//

class CPeerTestApp : public CWinApp
{
public:
	CPeerTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPeerTestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPeerTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PEERTEST_H__2F625A3C_93C3_4689_9749_3B9FCBE53594__INCLUDED_)
