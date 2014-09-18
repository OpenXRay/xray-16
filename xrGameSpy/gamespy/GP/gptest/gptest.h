// gptest.h : main header file for the GPTEST application
//

#if !defined(AFX_GPTEST_H__FFE82BD6_F31E_4222_818F_A8EE211E15E4__INCLUDED_)
#define AFX_GPTEST_H__FFE82BD6_F31E_4222_818F_A8EE211E15E4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGptestApp:
// See gptest.cpp for the implementation of this class
//

class CGptestApp : public CWinApp
{
public:
	CGptestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGptestApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGptestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GPTEST_H__FFE82BD6_F31E_4222_818F_A8EE211E15E4__INCLUDED_)
