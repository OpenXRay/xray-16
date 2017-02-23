// ghttpmfc.h : main header file for the GHTTPMFC application
//

#if !defined(AFX_GHTTPMFC_H__30518F92_6CEC_4512_AC74_EF31A5DE2D1C__INCLUDED_)
#define AFX_GHTTPMFC_H__30518F92_6CEC_4512_AC74_EF31A5DE2D1C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGhttpmfcApp:
// See ghttpmfc.cpp for the implementation of this class
//

class CGhttpmfcApp : public CWinApp
{
public:
	CGhttpmfcApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGhttpmfcApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGhttpmfcApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GHTTPMFC_H__30518F92_6CEC_4512_AC74_EF31A5DE2D1C__INCLUDED_)
