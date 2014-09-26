// PeerLobby.h : main header file for the PEERLOBBY application
//

#if !defined(AFX_PEERLOBBY_H__A167413C_51DD_4C60_BA40_97CCA3876DDB__INCLUDED_)
#define AFX_PEERLOBBY_H__A167413C_51DD_4C60_BA40_97CCA3876DDB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CPeerLobbyApp:
// See PeerLobby.cpp for the implementation of this class
//

class CPeerLobbyApp : public CWinApp
{
public:
	CPeerLobbyApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPeerLobbyApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPeerLobbyApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PEERLOBBY_H__A167413C_51DD_4C60_BA40_97CCA3876DDB__INCLUDED_)
