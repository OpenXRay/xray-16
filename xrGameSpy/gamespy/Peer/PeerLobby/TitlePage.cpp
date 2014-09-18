// TitlePage.cpp : implementation file
//

#include "stdafx.h"
#include "PeerLobby.h"
#include "TitlePage.h"
#include "LobbyWizard.h"
#include "ConnectPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTitlePage * TitlePage;

#define COL_NAME          0
#define COL_NUM_WAITING   1
#define COL_MAX_WAITING   2
#define COL_NUM_GAMES     3
#define COL_NUM_PLAYING   4

/////////////////////////////////////////////////////////////////////////////
// CTitlePage property page

IMPLEMENT_DYNCREATE(CTitlePage, CPropertyPage)

CTitlePage::CTitlePage() : CPropertyPage(CTitlePage::IDD)
{
	//{{AFX_DATA_INIT(CTitlePage)
	m_message = _T("");
	//}}AFX_DATA_INIT
}

CTitlePage::~CTitlePage()
{
}

void CTitlePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTitlePage)
	DDX_Control(pDX, IDC_GROUPS, m_groups);
	DDX_Control(pDX, IDC_PLAYERS, m_players);
	DDX_Control(pDX, IDC_CHAT_WINDOW, m_chatWindow);
	DDX_Text(pDX, IDC_MESSAGE, m_message);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTitlePage, CPropertyPage)
	//{{AFX_MSG_MAP(CTitlePage)
	ON_NOTIFY(NM_CLICK, IDC_GROUPS, OnClickGroups)
	ON_NOTIFY(NM_DBLCLK, IDC_GROUPS, OnDblclkGroups)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_GROUPS, OnBegindragGroups)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTitlePage message handlers

// Used to list the players in the title room.
//////////////////////////////////////////////
static void EnumTitlePlayersCallback
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
		Wizard->MessageBox("Error listing title-room players.");
		return;
	}

	if(index == -1)
		return;

	TitlePage->UpdatePlayerPing(nick, 9999);
	GSI_UNUSED(peer);
	GSI_UNUSED(flags);
	GSI_UNUSED(roomType);
	GSI_UNUSED(param);
}

// Called when the join is complete (successful or unsuccessful).
/////////////////////////////////////////////////////////////////
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
		peerEnumPlayers(Wizard->m_peer, TitleRoom, EnumTitlePlayersCallback, NULL);
//PEERSTOP
	}
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
	GSI_UNUSED(roomType);
	GSI_UNUSED(result);
}

static void ListGroupRoomsCallback
(
	PEER peer,
	PEERBool success,
	int groupID,
	SBServer server,
	const char * name,
	int numWaiting,
	int maxWaiting,
	int numGames,
	int numPlaying,
	void * param
)
{
	// Check for failure.
	/////////////////////
	if(!success)
	{
		Wizard->MessageBox("Listing groups failed!");
		Wizard->EndDialog(IDOK);
		return;
	}

	// Check for done.
	//////////////////
	if(groupID == 0)
		return;

	CListCtrl * groups = &TitlePage->m_groups;
	int nIndex = groups->InsertItem(LVIF_TEXT | LVIF_PARAM,
		0,
		name,
		0, 0,
		0,
		(LPARAM)groupID);
	if(nIndex == -1)
		return;

	LV_ITEM item;
	CString str;
	item.mask = LVIF_TEXT;
	item.iItem = nIndex;
	item.iSubItem = COL_NUM_WAITING;
	str.Format("%d", numWaiting);
	item.pszText = (LPSTR)(LPCSTR)str;
	groups->SetItem(&item);
	item.iSubItem = COL_MAX_WAITING;
	str.Format("%d", maxWaiting);
	item.pszText = (LPSTR)(LPCSTR)str;
	groups->SetItem(&item);
	item.iSubItem = COL_NUM_GAMES;
	str.Format("%d", numGames);
	item.pszText = (LPSTR)(LPCSTR)str;
	groups->SetItem(&item);
	item.iSubItem = COL_NUM_PLAYING;
	str.Format("%d", numPlaying);
	item.pszText = (LPSTR)(LPCSTR)str;
	groups->SetItem(&item);
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
	GSI_UNUSED(server);
}

BOOL CTitlePage::OnSetActive() 
{
	// Show the back and next buttons.
	//////////////////////////////////
	Wizard->SetWizardButtons(PSWIZB_BACK);

	// Rename the next button.
	//////////////////////////
	::SetWindowText(Wizard->m_nextButtonWnd, "&Join >");

	// Clear the groups list.
	/////////////////////////
	m_groups.DeleteAllItems();

	// Clear the chat log.
	//////////////////////
	m_chatWindow.ResetContent();

//PEERSTART
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

	// Start listing group rooms.
	/////////////////////////////
	peerListGroupRooms(Wizard->m_peer, "", ListGroupRoomsCallback, NULL, PEERFalse);
//PEERSTOP

	return CPropertyPage::OnSetActive();
}

BOOL CTitlePage::OnKillActive() 
{
	// Reset the "next" button's name.
	//////////////////////////////////
	::SetWindowText(Wizard->m_nextButtonWnd, "&Next >");

	// Clear the player's.
	/////////////////////
	m_players.DeleteAllItems();

//PEERSTART
	// Leave the title room.
	////////////////////////
	if(Wizard->m_peer)
		peerLeaveRoom(Wizard->m_peer, TitleRoom, NULL);
//PEERSTOP

	return CPropertyPage::OnKillActive();
}

LRESULT CTitlePage::OnWizardNext() 
{
	// Update vars.
	///////////////
	UpdateData();

	// Make sure something was selected.
	////////////////////////////////////
	int nIndex = m_groups.GetSelectionMark();
	if(nIndex == -1)
	{
		MessageBox("You must have a group selected.");
		return -1;
	}

	// Join the group.
	//////////////////
	JoinGroup(nIndex);

	return (LRESULT)GROUP_PAGE;
}

BOOL CTitlePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Set the groups columns.
	//////////////////////////
	m_groups.InsertColumn(COL_NAME, "Group", LVCFMT_LEFT, 90);
	m_groups.InsertColumn(COL_NUM_WAITING, "Num Waiting", LVCFMT_LEFT, 75);
	m_groups.InsertColumn(COL_MAX_WAITING, "Max Waiting", LVCFMT_LEFT, 75);
	m_groups.InsertColumn(COL_NUM_GAMES, "Num Games", LVCFMT_LEFT, 75);
	m_groups.InsertColumn(COL_NUM_PLAYING, "Num Playing", LVCFMT_LEFT, 75);
	ListView_SetExtendedListViewStyle(m_groups.m_hWnd,LVS_EX_FULLROWSELECT);

	// Set the players columns.
	///////////////////////////
	m_players.InsertColumn(COL_NAME, "Player", LVCFMT_LEFT, 120);

	return TRUE;
}

// Find the index of a player in the list by its nick.
//////////////////////////////////////////////////////
int CTitlePage::FindPlayer(const char *nick)
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
void CTitlePage::UpdatePlayerPing(const char *nick, int ping)
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

#if 0
	// Add the ping.
	////////////////
	char intValue[16];
	sprintf(intValue, "%d", ping); 
	item.iItem = nIndex;
	item.iSubItem = 1;
	item.mask = LVIF_TEXT;
	item.pszText = intValue;
	m_players.SetItem(&item);
#endif
}

// Remove the player from the list.
///////////////////////////////////
void CTitlePage::RemovePlayer(const char * nick)
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
void CTitlePage::ChangePlayerNick(const char * oldNick, const char * newNick)
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
void CTitlePage::SendMessage()
{
	UpdateData();

	// Ignore blank message.
	////////////////////////
	if(m_message == "")
		return;

//PEERSTART
	// Send it.
	///////////
	peerMessageRoom(Wizard->m_peer, TitleRoom, m_message, NormalMessage);
//PEERSTOP

	// Clear it.
	////////////
	m_message = "";

	UpdateData(FALSE);
}

// Join a group.
////////////////
void CTitlePage::OnDblclkGroups(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMHEADER * header = (NMHEADER *)pNMHDR;

	// If something was double-clicked, join it.
	////////////////////////////////////////////
	if(header->iItem != -1)
		JoinGroup(header->iItem);

	*pResult = 0;
}

// Handle group list clicks.
////////////////////////////
void CTitlePage::OnClickGroups(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Enable/Disable join based on if something is selected.
	/////////////////////////////////////////////////////////
	BOOL enable = (m_groups.GetSelectedCount() > 0);
	::EnableWindow(Wizard->m_nextButtonWnd, enable);

	*pResult = 0;
	GSI_UNUSED(pNMHDR);
}
void CTitlePage::OnBegindragGroups(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Treat this like a click.
	// (OnClick() isn't called for "drags".
	///////////////////////////////////////
	OnClickGroups(pNMHDR, pResult);
}

// Called when the join is complete (successful or unsuccessful).
/////////////////////////////////////////////////////////////////
static void JoinGroupRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	joinSuccess = success;
	GSI_UNUSED(roomType);
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
	GSI_UNUSED(result);
}

// Join a group based on its index in the groups list.
//////////////////////////////////////////////////////
void CTitlePage::JoinGroup(int nIndex)
{
	// Get the group ID.
	/////////////////////////
	int groupID = (int)m_groups.GetItemData(nIndex);
	ASSERT(groupID);

//PEERSTART
	// Join the group room.
	///////////////////////
	Wizard->StartHourglass();
	peerJoinGroupRoom(Wizard->m_peer, groupID, JoinGroupRoomCallback, NULL, PEERTrue);
	Wizard->StopHourglass();
	if(!joinSuccess)
	{
		MessageBox("Error joining the group room.");
		return;
	}
//PEERSTOP

	// Goto the group room page.
	////////////////////////////
	Wizard->SetActivePage(GROUP_PAGE);
}
