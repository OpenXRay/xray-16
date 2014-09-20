// PeerLobby.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PeerLobby.h"
#include "ConnectPage.h"
#include "TitlePage.h"
#include "GroupPage.h"
#include "CreatePage.h"
#include "StagingPage.h"
#include "LobbyWizard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPeerLobbyApp

BEGIN_MESSAGE_MAP(CPeerLobbyApp, CWinApp)
	//{{AFX_MSG_MAP(CPeerLobbyApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPeerLobbyApp construction

CPeerLobbyApp::CPeerLobbyApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPeerLobbyApp object

CPeerLobbyApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPeerLobbyApp initialization

BOOL CPeerLobbyApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
/*
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
*/
	// Setup the wizard.
	////////////////////
	CLobbyWizard wizard("Peer Lobby");
	Wizard = &wizard;
	CConnectPage connectPage;
	ConnectPage = &connectPage;
	CTitlePage titlePage;
	TitlePage = &titlePage;
	CGroupPage groupPage;
	GroupPage = &groupPage;
	CCreatePage createPage;
	CreatePage = &createPage;
	CStagingPage stagingPage;
	StagingPage = &stagingPage;
	wizard.AddPage(ConnectPage);
	wizard.AddPage(TitlePage);
	wizard.AddPage(GroupPage);
	wizard.AddPage(CreatePage);
	wizard.AddPage(StagingPage);
	wizard.SetWizardMode();

	// Main loop.
	/////////////
	wizard.DoModal();

//PEERSTART
	// Shutdown peer.
	/////////////////
	if(wizard.m_peer)
		peerShutdown(wizard.m_peer);
//PEERSTOP

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
