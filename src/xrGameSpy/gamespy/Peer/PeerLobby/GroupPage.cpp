// GroupPage.cpp : implementation file
//

#include "stdafx.h"
#include "PeerLobby.h"
#include "GroupPage.h"
#include "LobbyWizard.h"
#include "ConnectPage.h"
#include "TitlePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CGroupPage * GroupPage;

#define COL_NAME          0
#define COL_PING          1
#define COL_RUNNING       2
#define COL_NUM_PLAYERS   3

/////////////////////////////////////////////////////////////////////////////
// CGroupPage property page

IMPLEMENT_DYNCREATE(CGroupPage, CPropertyPage)

// Set page defaults.
/////////////////////
CGroupPage::CGroupPage() : CPropertyPage(CGroupPage::IDD)
{
	//{{AFX_DATA_INIT(CGroupPage)
	m_message = _T("");
	//}}AFX_DATA_INIT
}

CGroupPage::~CGroupPage()
{
}

void CGroupPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupPage)
	DDX_Control(pDX, IDC_PLAYERS, m_players);
	DDX_Control(pDX, IDC_GAMES, m_games);
	DDX_Control(pDX, IDC_CHAT_WINDOW, m_chatWindow);
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Text(pDX, IDC_MESSAGE, m_message);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGroupPage)
	ON_BN_CLICKED(IDC_CREATE, OnCreate)
	ON_NOTIFY(NM_CLICK, IDC_GAMES, OnClickGames)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_GAMES, OnBegindragGames)
	ON_NOTIFY(NM_DBLCLK, IDC_GAMES, OnDblclkGames)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Gets called to maintain the game list.
/////////////////////////////////////////
static void ListingGamesCallback
(
	PEER peer,
	PEERBool success,
	const char * name,
	SBServer server,
	PEERBool staging,
	int msg,
	int progress,
	void * param
)
{
	LVITEM item;

	// Check for failure.
	/////////////////////
	if(!success)
	{
		Wizard->MessageBox("Listing games failed!");
		Wizard->EndDialog(IDOK);
		return;
	}

	// Cache pointers.
	//////////////////
	CListCtrl * games = &GroupPage->m_games;
	CListedGame * game = NULL;
	BOOL doUpdate = FALSE;
	int nIndex = -1;

	// Set the progress.
	////////////////////
	GroupPage->m_progress.SetPos(progress);

	// Handle the message based on its type.
	////////////////////////////////////////
	if(msg == PEER_CLEAR)
	{
		GroupPage->ClearGames();
	}
	else if(msg == PEER_ADD)
	{
		// Add this to the list.
		////////////////////////
		game = new CListedGame;
		game->server = server;
		game->name = name;
		game->staging = staging;
		game->ping = SBServerGetPing(server);
		game->numPlayers = SBServerGetIntValue(server, "numplayers", 0);
		game->maxPlayers = SBServerGetIntValue(server, "maxplayers", 0);

		nIndex = games->InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE,
			0,
			(LPCTSTR)name,
			0, 0,
			staging ? Wizard->m_stagingRoomIndex : Wizard->m_runningGameIndex,
			(LPARAM)game);
		if(nIndex == -1)
		{
			delete game;
			return;
		}

		doUpdate = TRUE;
	}
	else if(msg == PEER_UPDATE)
	{
		nIndex = GroupPage->FindListedGame(server);
		if(nIndex != -1)
		{
			// Update values.
			/////////////////
			game = (CListedGame *)games->GetItemData(nIndex);
			game->name = name;
			game->staging = staging;
			game->ping = SBServerGetPing(server);
			game->numPlayers = SBServerGetIntValue(server, "numplayers", 0);
			game->maxPlayers = SBServerGetIntValue(server, "maxplayers", 0);

			// Update the list.
			///////////////////
			item.mask = LVIF_IMAGE;
			item.iItem = nIndex;
			item.iSubItem = 0;
			item.iImage = staging ? Wizard->m_stagingRoomIndex : Wizard->m_runningGameIndex;
			games->SetItem(&item);

			doUpdate = TRUE;
		}
	}
	else if(msg == PEER_REMOVE)
	{
		nIndex = GroupPage->FindListedGame(server);
		if(nIndex != -1)
		{
			delete (CListedGame *)games->GetItemData(nIndex);
			games->DeleteItem(nIndex);
		}
	}

	if(doUpdate)
	{
		item.mask = LVIF_TEXT;
		item.iItem = nIndex;
		item.iSubItem = COL_PING;
		char buffer[32];
		sprintf(buffer, "%d", game->ping);
		item.pszText = buffer;
		games->SetItem(&item);
		item.iSubItem = COL_RUNNING;
		if(staging)
			item.pszText = "No";
		else
			item.pszText = "Yes";
		games->SetItem(&item);
		item.iSubItem = COL_NUM_PLAYERS;
		sprintf(buffer, "%d/%d", game->numPlayers, game->maxPlayers);
		item.pszText = buffer;
		games->SetItem(&item);
	}

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Used to list the players in the room.
////////////////////////////////////////
static void EnumPlayersCallback
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
	if(!success)
	{
		Wizard->MessageBox("Error listing players.");
		return;
	}

	if(index == -1)
		return;

	GroupPage->UpdatePlayerPing(nick, 9999);

	GSI_UNUSED(param);
	GSI_UNUSED(flags);
	GSI_UNUSED(roomType);
	GSI_UNUSED(peer);
}

static PEERBool joinSuccess;
static void JoinTitleRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	joinSuccess = success;

	if(success)
	{
//PEERSTART
		// List the players in the room.
		////////////////////////////////
		peerEnumPlayers(Wizard->m_peer, TitleRoom, EnumPlayersCallback, NULL);
//PEERSTOP
	}

	GSI_UNUSED(param);
	GSI_UNUSED(roomType);
	GSI_UNUSED(result);
	GSI_UNUSED(peer);
}

// Switching to this page.
//////////////////////////
BOOL CGroupPage::OnSetActive() 
{
	// Start off with only a back button.
	/////////////////////////////////////
	Wizard->SetWizardButtons(PSWIZB_BACK);

	// Rename the next button.
	//////////////////////////
	::SetWindowText(Wizard->m_nextButtonWnd, "&Join >");

	// Clear the progress bar.
	//////////////////////////
	m_progress.SetPos(0);

	// Clear the game list.
	///////////////////////
	ClearGames();

	// Clear the chat log.
	//////////////////////
	m_chatWindow.ResetContent();

//PEERSTART
	if(!Wizard->m_groupRooms)
	{
		// Join the title room.
		///////////////////////
		Wizard->StartHourglass();
		peerJoinTitleRoom(Wizard->m_peer, NULL, JoinTitleRoomCallback, NULL, PEERTrue);
		Wizard->StopHourglass();
		if(!joinSuccess)
		{
			MessageBox("Error joining the title room.");
			return FALSE;
		}
	}
	else
	{
		// List the players in the room.
		////////////////////////////////
		peerEnumPlayers(Wizard->m_peer, GroupRoom, EnumPlayersCallback, NULL);
	}

	// Start listing games.
	////////////////////////
	unsigned char fields[] = { GAMEVER_KEY, NUMPLAYERS_KEY, MAXPLAYERS_KEY };
	peerStartListingGames(Wizard->m_peer, fields, sizeof(fields), NULL, ListingGamesCallback, NULL);
//PEERSTOP

	// Setup the player's box columns.
	//////////////////////////////////
	if(Wizard->m_groupRooms)
	{
		m_players.InsertColumn(COL_NAME, "Player", LVCFMT_LEFT, 70);
		m_players.InsertColumn(COL_PING, "Ping", LVCFMT_LEFT, 50);
	}
	else
		m_players.InsertColumn(COL_NAME, "Player", LVCFMT_LEFT, 120);

	return CPropertyPage::OnSetActive();
}

// Leaving this page.
/////////////////////
BOOL CGroupPage::OnKillActive() 
{
//PEERSTART
	if(!Wizard->m_groupRooms)
		peerLeaveRoom(Wizard->m_peer, TitleRoom, NULL);
//PEERSTOP

	// Delete player columns.
	/////////////////////////
	m_players.DeleteColumn(0);
	if(Wizard->m_groupRooms)
		m_players.DeleteColumn(1);

	// Reset the "next" button's name.
	//////////////////////////////////
	::SetWindowText(Wizard->m_nextButtonWnd, "&Next >");

	// Clear the players.
	/////////////////////
	m_players.DeleteAllItems();

	return CPropertyPage::OnKillActive();
}

// Going to the next page.
//////////////////////////
LRESULT CGroupPage::OnWizardNext() 
{
	// Update vars.
	///////////////
	UpdateData();

	// Make sure something was selected.
	////////////////////////////////////
	int nIndex = m_games.GetSelectionMark();
	if(nIndex == -1)
	{
		MessageBox("You must have a game selected.");
		return -1;
	}

	// Join the game.
	/////////////////
	JoinGame(nIndex);

	return (LRESULT)1;
}

LRESULT CGroupPage::OnWizardBack() 
{
//PEERSTART
	// Leave the group room.
	////////////////////////
	if(Wizard->m_groupRooms)
		peerLeaveRoom(Wizard->m_peer, GroupRoom, NULL);
	else
		return IDD_CONNECT_PAGE;
//PEERSTOP

	return CPropertyPage::OnWizardBack();
}

// Init the page.
/////////////////
BOOL CGroupPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Set the games columns.
	/////////////////////////
	m_games.InsertColumn(COL_NAME, "Server Name", LVCFMT_LEFT, 170);
	m_games.InsertColumn(COL_PING, "Ping", LVCFMT_LEFT, 50);
	m_games.InsertColumn(COL_RUNNING, "Running", LVCFMT_LEFT, 60);
	m_games.InsertColumn(COL_NUM_PLAYERS, "Num Players", LVCFMT_LEFT, 75);
	ListView_SetExtendedListViewStyle(m_games.m_hWnd,LVS_EX_FULLROWSELECT);

	// Image list setup.
	////////////////////
	m_games.SetImageList(&Wizard->m_imageList, LVSIL_SMALL);

	return TRUE;
}

// Cleanup the page.
////////////////////
void CGroupPage::OnDestroy() 
{
	CPropertyPage::OnDestroy();

	// Clear the game list.
	///////////////////////
	ClearGames();
}

// Clear the list of games.
///////////////////////////
void CGroupPage::ClearGames()
{
	// Free the data first.
	///////////////////////
	int count = m_games.GetItemCount();
	int i;
	for(i = 0 ; i < count ; i++)
	{
		LVITEM item;
		item.mask = LVIF_PARAM;
		item.iItem = i;
		item.iSubItem = 0;
		if(m_games.GetItem(&item) && item.lParam)
			delete (CListedGame *)item.lParam;
	}

	// Clear the list.
	//////////////////
	m_games.DeleteAllItems();
}

// Create a staging room.
/////////////////////////
void CGroupPage::OnCreate() 
{
//PEERSTART
	// Stop listing games.
	//////////////////////
	peerStopListingGames(Wizard->m_peer);
//PEERSTOP

	// Goto the create page.
	////////////////////////
	Wizard->SetActivePage(CREATE_PAGE);
}

// Join a game.
///////////////
void CGroupPage::OnDblclkGames(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMHEADER * header = (NMHEADER *)pNMHDR;

	// If something was double-clicked, join it.
	////////////////////////////////////////////
	if(header->iItem != -1)
		JoinGame(header->iItem);

	*pResult = 0;
}

// Handle game list clicks.
///////////////////////////
void CGroupPage::OnClickGames(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Enable/Disable join based on if something is selected.
	/////////////////////////////////////////////////////////
	BOOL enable = (m_games.GetSelectedCount() > 0);
	::EnableWindow(Wizard->m_nextButtonWnd, enable);

	*pResult = 0;

	GSI_UNUSED(pNMHDR);
}
void CGroupPage::OnBegindragGames(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Treat this like a click.
	// (OnClick() isn't called for "drags".
	///////////////////////////////////////
	OnClickGames(pNMHDR, pResult);
}

// Find the index of a game in the list by its SBServer object.
///////////////////////////////////////////////////////////////
int CGroupPage::FindListedGame(SBServer server)
{
	int count = m_games.GetItemCount();
	int i;
	CListedGame * game;
	for(i = 0 ; i < count ; i++)
	{
		game = (CListedGame *)m_games.GetItemData(i);
		if(game && game->server == server)
			return i;
	}
	return -1;
}

// Find the index of a player in the list by its nick.
//////////////////////////////////////////////////////
int CGroupPage::FindPlayer(const char * nick)
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

// Updates the player's ping in the player list, and adds the player
// if its not on the list.
////////////////////////////////////////////////////////////////////
void CGroupPage::UpdatePlayerPing(const char * nick, int ping)
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
		item.mask = LVIF_TEXT;
		item.pszText = (char *)nick;

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

// Remove the player from the list.
///////////////////////////////////
void CGroupPage::RemovePlayer(const char * nick)
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

// Change a nick in the player list.
////////////////////////////////////
void CGroupPage::ChangePlayerNick(const char * oldNick, const char * newNick)
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

// Join a game based on its index in the game list.
///////////////////////////////////////////////////
void CGroupPage::JoinGame(int nIndex)
{
	// Get the data.
	////////////////
	CListedGame * game = (CListedGame *)m_games.GetItemData(nIndex);
	ASSERT(game);

	// Is it staging?
	/////////////////
	if(game->staging)
	{
		// Goto the staging room page.
		//////////////////////////////
		Wizard->SetActivePage(STAGING_PAGE);
	}
	else
	{
//PEERSTART
		// Because there's no staging room for this game, it can just be joined.
		// You can get info on the server using the GOA ServerGet*() functions.
		////////////////////////////////////////////////////////////////////////
		CString buffer = "You are now playing at ";
		buffer += SBServerGetPublicAddress(game->server);
		buffer += ".\nHit enter when you are done.";
		MessageBox(buffer);
//PEERSTOP
	}
}

// Does the actual chat message sending.
////////////////////////////////////////
void CGroupPage::SendMessage()
{
	UpdateData();

	// Ignore blank message.
	////////////////////////
	if(m_message == "")
		return;

//PEERSTART
	// Send it.
	///////////
	peerMessageRoom(Wizard->m_peer, Wizard->m_groupRooms?GroupRoom:TitleRoom, m_message, NormalMessage);
//PEERSTOP

	// Clear it.
	////////////
	m_message = "";

	UpdateData(FALSE);
}
