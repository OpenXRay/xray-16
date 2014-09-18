// LobbyWizard.cpp : implementation file
//

#include "stdafx.h"
#include "PeerLobby.h"
#include "LobbyWizard.h"
#include "GroupPage.h"
#include "StagingPage.h"
#include "TitlePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLobbyWizard * Wizard;

#define THINK_TIMER  100

/////////////////////////////////////////////////////////////////////////////
// CLobbyWizard

IMPLEMENT_DYNAMIC(CLobbyWizard, CPropertySheet)

CLobbyWizard::CLobbyWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
//PEERSTART
	// Initialize our peer pointer to NULL.
	///////////////////////////////////////
	m_peer = NULL;
//PEERSTOP
}

CLobbyWizard::CLobbyWizard(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
//PEERSTART
	// Initialize our peer pointer to NULL.
	///////////////////////////////////////
	m_peer = NULL;
//PEERSTOP
}

CLobbyWizard::~CLobbyWizard()
{
}


BEGIN_MESSAGE_MAP(CLobbyWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CLobbyWizard)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLobbyWizard message handlers

// Init the wizard.
///////////////////
BOOL CLobbyWizard::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	// Get the "next" button wnd.
	/////////////////////////////
	m_nextButtonWnd = ::FindWindowEx(this->m_hWnd, NULL, "Button", "&Next >");

	// Rename "Cancel" to "Quit".
	/////////////////////////////
	HWND hWnd = ::FindWindowEx(this->m_hWnd, NULL, "Button", "Cancel");
	if(hWnd)
		::SetWindowText(hWnd, "&Quit");

	// Hide the "Help" button.
	//////////////////////////
	hWnd = ::FindWindowEx(this->m_hWnd, NULL, "Button", "Help");
	if(hWnd)
		::ShowWindow(hWnd, SW_HIDE);

	// Load icons.
	//////////////
	m_greenSmileyIcon = AfxGetApp()->LoadIcon(IDI_GREEN_SMILEY);
	m_yellowSmileyIcon = AfxGetApp()->LoadIcon(IDI_YELLOW_SMILEY);
	m_redSmileyIcon = AfxGetApp()->LoadIcon(IDI_RED_SMILEY);
	m_stagingRoomIcon = AfxGetApp()->LoadIcon(IDI_STAGING_ROOM);
	m_runningGameIcon = AfxGetApp()->LoadIcon(IDI_RUNNING_GAME);
	if(!m_greenSmileyIcon || !m_yellowSmileyIcon || !m_redSmileyIcon || !m_stagingRoomIcon || !m_runningGameIcon)
		return FALSE;

	// Create the image list.
	/////////////////////////
	if(!m_imageList.Create(16, 16, ILC_COLOR, 5, 5))
		return FALSE;
	m_greenSmileyIndex = m_imageList.Add(m_greenSmileyIcon);
	m_yellowSmileyIndex = m_imageList.Add(m_yellowSmileyIcon);
	m_redSmileyIndex = m_imageList.Add(m_redSmileyIcon);
	m_stagingRoomIndex = m_imageList.Add(m_stagingRoomIcon);
	m_runningGameIndex = m_imageList.Add(m_runningGameIcon);
	if((m_yellowSmileyIndex == -1) || (m_redSmileyIndex == -1) || (m_greenSmileyIndex == -1) || (m_runningGameIndex == -1) || (m_stagingRoomIndex == -1))
		return FALSE;

//PEERSTART
	// We're not hosting.
	/////////////////////
	m_hosting = FALSE;

	// Set the timer for every 10 seconds.
	//////////////////////////////////////
	SetTimer(THINK_TIMER, 10, NULL);
//PEERSTOP

	return bResult;
}

// Think every 10ms.
////////////////////
void CLobbyWizard::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == THINK_TIMER)
	{
//PEERSTART
		// Let peer think.
		//////////////////
		if(m_peer)
			peerThink(m_peer);
//PEERSTOP
	}

	CPropertySheet::OnTimer(nIDEvent);
}

// Show the hourglass.
//////////////////////
void CLobbyWizard::StartHourglass()
{
	// Load the hourglass.
	//////////////////////
	HCURSOR hourglass = LoadCursor(NULL, IDC_WAIT);
	if(!hourglass)
		return;

	// Set the cursor.
	//////////////////
	m_lastCursor = SetCursor(hourglass);
}

// Back to normal pointer.
//////////////////////////
void CLobbyWizard::StopHourglass()
{
	// Reset the old cursor.
	////////////////////////
	SetCursor(m_lastCursor);
}

// Catch [Enter] when pressed in a chat message box.
////////////////////////////////////////////////////
BOOL CLobbyWizard::PreTranslateMessage(MSG* pMsg) 
{
	// Check for enter.
	///////////////////
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == 0x0d))
	{
		// Check what page we're on.
		/////////////////////////////
		int page = GetActiveIndex();
		CWnd * focus;
		if(page == TITLE_PAGE)
		{
			focus = TitlePage->GetFocus();
			if(focus->m_hWnd == TitlePage->GetDlgItem(IDC_MESSAGE)->m_hWnd)
			{
				TitlePage->SendMessage();
				return TRUE;
			}
		}
		else if(page == GROUP_PAGE)
		{
			focus = GroupPage->GetFocus();
			if(focus->m_hWnd == GroupPage->GetDlgItem(IDC_MESSAGE)->m_hWnd)
			{
				GroupPage->SendMessage();
				return TRUE;
			}
		}
		else if(page == STAGING_PAGE)
		{
			focus = StagingPage->GetFocus();
			if(focus->m_hWnd == StagingPage->GetDlgItem(IDC_MESSAGE)->m_hWnd)
			{
				StagingPage->SendMessage();
				return TRUE;
			}
		}
	}

	return CPropertySheet::PreTranslateMessage(pMsg);
}
