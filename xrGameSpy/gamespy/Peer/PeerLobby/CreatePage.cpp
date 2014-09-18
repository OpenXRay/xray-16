// CreatePage.cpp : implementation file
//

#include "stdafx.h"
#include "PeerLobby.h"
#include "CreatePage.h"
#include "LobbyWizard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreatePage * CreatePage;

/////////////////////////////////////////////////////////////////////////////
// CCreatePage property page

IMPLEMENT_DYNCREATE(CCreatePage, CPropertyPage)

// Set page defaults.
/////////////////////
CCreatePage::CCreatePage() : CPropertyPage(CCreatePage::IDD)
{
	//{{AFX_DATA_INIT(CCreatePage)
	m_name = _T("My Server");
	m_maxPlayers = 8;
	//}}AFX_DATA_INIT
}

CCreatePage::~CCreatePage()
{
}

void CCreatePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreatePage)
	DDX_Text(pDX, IDC_NAME, m_name);
	DDX_Text(pDX, IDC_MAX_PLAYERS, m_maxPlayers);
	DDV_MinMaxInt(pDX, m_maxPlayers, 2, 9999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreatePage, CPropertyPage)
	//{{AFX_MSG_MAP(CCreatePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Switching to this page.
//////////////////////////
BOOL CCreatePage::OnSetActive() 
{
	// Show the back and next buttons.
	//////////////////////////////////
	Wizard->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	// We're hosting the room.
	//////////////////////////
	Wizard->m_hosting = TRUE;

	return CPropertyPage::OnSetActive();
}

// Going to the previous page.
//////////////////////////////
LRESULT CCreatePage::OnWizardBack() 
{
	// Not hosting anymore.
	///////////////////////
	Wizard->m_hosting = FALSE;

	return CPropertyPage::OnWizardBack();
}

// Called when the staging room has been created (or the attempt failed).
/////////////////////////////////////////////////////////////////////////
static PEERBool createStagingSuccess;
static void CreateStagingRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	createStagingSuccess = success;

	GSI_UNUSED(param);
	GSI_UNUSED(roomType);
	GSI_UNUSED(result);
	GSI_UNUSED(peer);
}

// Going to the next page.
//////////////////////////
LRESULT CCreatePage::OnWizardNext() 
{
	// Update the data.
	///////////////////
	UpdateData();

//PEERSTART
	// Create the room.
	///////////////////
	Wizard->StartHourglass();
	peerCreateStagingRoom(Wizard->m_peer, m_name, m_maxPlayers, NULL, CreateStagingRoomCallback, NULL, PEERTrue);
	Wizard->StopHourglass();
//PEERSTOP

	return CPropertyPage::OnWizardNext();
}
