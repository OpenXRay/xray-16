// StagingPage.cpp : implementation file
//

#include "stdafx.h"
#include "PeerLobby.h"
#include "StagingPage.h"
#include "LobbyWizard.h"
#include "GroupPage.h"
#include "ConnectPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CStagingPage * StagingPage;

#define COL_NAME          0
#define COL_PING          1

/////////////////////////////////////////////////////////////////////////////
// CStagingPage property page

IMPLEMENT_DYNCREATE(CStagingPage, CPropertyPage)

// Set page defaults.
/////////////////////
CStagingPage::CStagingPage() : CPropertyPage(CStagingPage::IDD)
{
	//{{AFX_DATA_INIT(CStagingPage)
	m_message = _T("");
	m_ready = 0;
	//}}AFX_DATA_INIT
}

CStagingPage::~CStagingPage()
{
}

void CStagingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStagingPage)
	DDX_Control(pDX, IDC_PLAYERS, m_players);
	DDX_Control(pDX, IDC_CHAT_WINDOW, m_chatWindow);
	DDX_Text(pDX, IDC_MESSAGE, m_message);
	DDX_Radio(pDX, IDC_NOT_READY, m_ready);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStagingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CStagingPage)
	ON_BN_CLICKED(IDC_READY, OnReady)
	ON_BN_CLICKED(IDC_NOT_READY, OnNotReady)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Used to list staging room players.
/////////////////////////////////////
static void EnumStagingPlayersCallback
(
	PEER peer,
	PEERBool success,
	RoomType roomType,
	int index,
	const char * nick,
	int flags,
	void * param
)
{
	// Add the player to the list.
	//////////////////////////////
	if(nick)
		StagingPage->UpdatePlayerPing(nick, 9999);
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
	GSI_UNUSED(flags);
	GSI_UNUSED(index);
	GSI_UNUSED(roomType);
	GSI_UNUSED(success);
}

// Called when the join attempt has completed (successfully or unsuccessfully).
///////////////////////////////////////////////////////////////////////////////
PEERBool joinStagingSuccess;
void JoinStagingRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	joinStagingSuccess = success;
	GSI_UNUSED(peer);
	GSI_UNUSED(roomType);
	GSI_UNUSED(param);
	GSI_UNUSED(result);
}

// Switching to this page.
//////////////////////////
BOOL CStagingPage::OnSetActive() 
{
	// Show back and a disabled-finish.
	///////////////////////////////////
	Wizard->SetWizardButtons(PSWIZB_BACK | PSWIZB_DISABLEDFINISH);

	// Clear the chat log.
	//////////////////////
	m_chatWindow.ResetContent();

	// Are we joining?
	//////////////////
	if(!Wizard->m_hosting)
	{
		// Make sure something was selected.
		////////////////////////////////////
		int nIndex = GroupPage->m_games.GetSelectionMark();
		if(nIndex == -1)
		{
			MessageBox("You must have a game selected.");
			return 1;
		}

		// Get the data.
		////////////////
		CListedGame * game = (CListedGame *)GroupPage->m_games.GetItemData(nIndex);

//PEERSTART
		// Join the room.
		/////////////////
		Wizard->StartHourglass();
		peerJoinStagingRoom(Wizard->m_peer, game->server, NULL, JoinStagingRoomCallback, Wizard, PEERTrue);
		Wizard->StopHourglass();
		if(!joinStagingSuccess)
		{
			MessageBox("Error joining the staging room.");
			return FALSE;
		}
		
		// Stop listing.
		////////////////
		peerStopListingGames(Wizard->m_peer);
	}

	// Fill the player list.
	////////////////////////
	peerEnumPlayers(Wizard->m_peer, StagingRoom, EnumStagingPlayersCallback, Wizard);
//PEERSTOP

	// Default to not ready.
	////////////////////////
	m_ready = 0;

	return CPropertyPage::OnSetActive();
}

// Leaving this page.
/////////////////////
BOOL CStagingPage::OnKillActive() 
{
	// Clear the players.
	/////////////////////
	m_players.DeleteAllItems();

	return CPropertyPage::OnKillActive();
}

// Going to the previous page.
//////////////////////////////
LRESULT CStagingPage::OnWizardBack() 
{
//PEERSTART
	// Leave the room.
	//////////////////
	peerLeaveRoom(Wizard->m_peer, StagingRoom, NULL);
//PEERSTOP

	// Do stuff based on hosting.
	/////////////////////////////
	if(Wizard->m_hosting)
	{
		// Goto the create page.
		////////////////////////
		return IDD_CREATE_PAGE;
	}
	else
	{
		// Goto the lobby room.
		///////////////////////
		return IDD_GROUP_PAGE;
	}

	//return CPropertyPage::OnWizardBack();
}

// The game is being launched.
//////////////////////////////
BOOL CStagingPage::OnWizardFinish() 
{
//PEERSTART
	// Start the game.
	//////////////////
	peerStartGame(Wizard->m_peer, "", PEER_KEEP_REPORTING);

	// This is where the host would start up.
	/////////////////////////////////////////
	MessageBox("You are now playing!\nHit enter when you are done.");

	// Stop the game.
	// This should be called when the host is ready to return to the staging room.
	//////////////////////////////////////////////////////////////////////////////
	peerStopGame(Wizard->m_peer);
//PEERSTOP

	return FALSE;
}

// Init the page.
/////////////////
BOOL CStagingPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Change "Finish".
	///////////////////
	HWND hWnd = ::FindWindowEx(GetParent()->m_hWnd, NULL, "Button", "Finish");
	if(hWnd)
		::SetWindowText(hWnd, "&Start Playing");

	// Set the players columns.
	///////////////////////////
	m_players.InsertColumn(COL_NAME, "Player", LVCFMT_LEFT, 70);
	m_players.InsertColumn(COL_PING, "Ping", LVCFMT_LEFT, 50);

	// Image list setup.
	////////////////////
	m_players.SetImageList(&Wizard->m_imageList, LVSIL_SMALL);

	return TRUE;
}

// The player's ready.
//////////////////////
void CStagingPage::OnReady() 
{
//PEERSTART
	// Set ready to true.
	/////////////////////
	peerSetReady(Wizard->m_peer, PEERTrue);
//PEERSTOP
}

// The player's not ready.
//////////////////////////
void CStagingPage::OnNotReady() 
{
//PEERSTART
	// Set ready to false.
	//////////////////////
	peerSetReady(Wizard->m_peer, PEERFalse);
//PEERSTOP
}

// Find the index of a player in the list ny its nick.
//////////////////////////////////////////////////////
int CStagingPage::FindPlayer(const char * nick)
{
	// Always deal in lower-case.
	/////////////////////////////
	CString loweredNick = nick;
	loweredNick.MakeLower();
	nick = loweredNick;

	// Look for this player.
	////////////////////////
	LVFINDINFO findInfo;
	findInfo.flags = LVFI_STRING;
	findInfo.psz = nick;

	// Find the player.
	///////////////////
	int nIndex = m_players.FindItem(&findInfo);

	return nIndex;
}

// Update this player's ping in the list, and add the player if
// not already on the list.
///////////////////////////////////////////////////////////////
void CStagingPage::UpdatePlayerPing(const char * nick, int ping)
{
	LVITEM item;

	// Is this us?
	//////////////
	if(strcasecmp(nick, ConnectPage->m_nick) == 0)
		ping = 0;

	// Always deal in lower-case.
	/////////////////////////////
	CString loweredNick = nick;
	loweredNick.MakeLower();
	nick = loweredNick;

	// Find the player.
	///////////////////
	int nIndex = FindPlayer(nick);

	// Check for a new nick.
	////////////////////////
	if(nIndex == -1)
	{
		item.iItem = 0;
		item.iSubItem = 0;
		item.mask = LVIF_TEXT | LVIF_IMAGE;
		item.pszText = (char *)nick;
		item.iImage = Wizard->m_redSmileyIndex;

		nIndex = m_players.InsertItem(&item);
		if(nIndex == -1)
			return;
	}

	// Add the ping.
	////////////////
	char intValue[16];
	sprintf(intValue, "%d", ping); 
	item.iItem = nIndex;
	item.iSubItem = 1;
	item.mask = LVIF_TEXT;
	item.pszText = intValue;
	m_players.SetItem(&item);
}

// Update the player's ready state, and add the player if 
// not alreayd on the list.
/////////////////////////////////////////////////////////
void CStagingPage::UpdatePlayerReady(const char * nick, BOOL ready)
{
	LVITEM item;

	// Always deal in lower-case.
	/////////////////////////////
	CString loweredNick = nick;
	loweredNick.MakeLower();
	nick = loweredNick;

	// Find the player.
	///////////////////
	int nIndex = FindPlayer(nick);

	// Check for a new nick.
	////////////////////////
	if(nIndex != -1)
	{
		// Update the image.
		////////////////////
		item.iItem = nIndex;
		item.iSubItem = 0;
		item.mask = LVIF_IMAGE;
		if(ready)
			item.iImage = Wizard->m_greenSmileyIndex;
		else
			item.iImage = Wizard->m_redSmileyIndex;
		m_players.SetItem(&item);
	}
	else
	{
		// Add it.
		//////////
		item.iItem = 0;
		item.iSubItem = 0;
		item.mask = LVIF_TEXT | LVIF_IMAGE;
		item.pszText = (char *)nick;
		if(ready)
			item.iImage = Wizard->m_greenSmileyIndex;
		else
			item.iImage = Wizard->m_redSmileyIndex;

		nIndex = m_players.InsertItem(&item);
		if(nIndex == -1)
			return;

		// Set a ping.
		//////////////
		item.iItem = nIndex;
		item.iSubItem = 1;
		item.mask = LVIF_TEXT;
		item.pszText = "9999";
		m_players.SetItem(&item);
	}
}

// Remove a player from the list.
/////////////////////////////////
void CStagingPage::RemovePlayer(const char * nick)
{
	// Always deal in lower-case.
	/////////////////////////////
	CString loweredNick = nick;
	loweredNick.MakeLower();
	nick = loweredNick;

	// Find the player.
	///////////////////
	int nIndex = FindPlayer(nick);

	// Remove it.
	/////////////
	m_players.DeleteItem(nIndex);
}

// Change a player's nick on the list.
//////////////////////////////////////
void CStagingPage::ChangePlayerNick(const char * oldNick, const char * newNick)
{
	// Always deal in lower-case.
	/////////////////////////////
	CString loweredNick = oldNick;
	loweredNick.MakeLower();
	oldNick = loweredNick;
	loweredNick = newNick;
	loweredNick.MakeLower();
	newNick = loweredNick;

	// Find the player.
	///////////////////
	int nIndex = FindPlayer(oldNick);

	// Update the nick.
	///////////////////
	LVITEM item;
	item.iItem = nIndex;
	item.iSubItem = 0;
	item.mask = LVIF_TEXT;
	item.pszText = (char *)newNick;
	m_players.SetItem(&item);
}

// Does the actual chat message sending.
////////////////////////////////////////
void CStagingPage::SendMessage()
{
	UpdateData();

	// Ignore blank message.
	////////////////////////
	if(m_message == "")
		return;

//PEERSTART
	// Send it.
	///////////
	peerMessageRoom(Wizard->m_peer, StagingRoom, m_message, NormalMessage);
//PEERSTOP

	// Clear it.
	////////////
	m_message = "";

	UpdateData(FALSE);
}

// Either enables or disables the finish button based on if
// all the players in the room are ready or not.
///////////////////////////////////////////////////////////
void CStagingPage::CheckEnableFinish()
{
//PEERSTART
	// Check if all the player's in the room are ready.
	///////////////////////////////////////////////////
	PEERBool allReady = peerAreAllReady(Wizard->m_peer);
//PEERSTOP

	// Only enable the finish button if we're hosting and everyone's ready.
	///////////////////////////////////////////////////////////////////////
	if(Wizard->m_hosting && allReady)
		Wizard->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	else
		Wizard->SetWizardButtons(PSWIZB_BACK | PSWIZB_DISABLEDFINISH);
}