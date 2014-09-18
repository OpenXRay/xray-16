// sbmfcsample.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "sbmfcsample.h"
#include "sbmfcsampleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSbmfcsampleApp

BEGIN_MESSAGE_MAP(CSbmfcsampleApp, CWinApp)
	//{{AFX_MSG_MAP(CSbmfcsampleApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSbmfcsampleApp construction

CSbmfcsampleApp::CSbmfcsampleApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSbmfcsampleApp object

CSbmfcsampleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSbmfcsampleApp initialization

BOOL CSbmfcsampleApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
/*
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
*/
	CSbmfcsampleDlg dlg;
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
