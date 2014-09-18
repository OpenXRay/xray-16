// multiTrack.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "multiTrack.h"
#include "multiTrackDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMultiTrackApp

BEGIN_MESSAGE_MAP(CMultiTrackApp, CWinApp)
	//{{AFX_MSG_MAP(CMultiTrackApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMultiTrackApp construction

CMultiTrackApp::CMultiTrackApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMultiTrackApp object

CMultiTrackApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMultiTrackApp initialization

BOOL CMultiTrackApp::InitInstance()
{
	// Standard initialization
/*
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
*/
	CMultiTrackDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
