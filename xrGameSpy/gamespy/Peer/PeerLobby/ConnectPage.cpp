// ConnectPage.cpp : implementation file
//

#include "stdafx.h"
#include "PeerLobby.h"
#include "ConnectPage.h"
#include "TitlePage.h"
#include "LobbyWizard.h"
#include "GroupPage.h"
#include "StagingPage.h"
#include "CreatePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CConnectPage * ConnectPage;

/////////////////////////////////////////////////////////////////////////////
// CConnectPage property page

IMPLEMENT_DYNCREATE(CConnectPage, CPropertyPage)

// Set page defaults.
/////////////////////
CConnectPage::CConnectPage() : CPropertyPage(CConnectPage::IDD)
{
	//{{AFX_DATA_INIT(CConnectPage)
	m_nick = _T("PeerPlayer");
	m_title = _T("gmtest");
	m_groupRooms = FALSE;
	m_key = _T("HA6zkS");
	//}}AFX_DATA_INIT
}

CConnectPage::~CConnectPage()
{
}

void CConnectPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectPage)
	DDX_Text(pDX, IDC_NICK, m_nick);
	DDX_Text(pDX, IDC_TITLE, m_title);
	DDX_Check(pDX, IDC_GROUP_ROOMS, m_groupRooms);
	DDX_Text(pDX, IDC_KEY, m_key);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConnectPage, CPropertyPage)
	//{{AFX_MSG_MAP(CConnectPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Used for printing callback info to the debug window.
///////////////////////////////////////////////////////
static void PrintCallback(const char * callback)
{
	CString buffer;
	buffer = callback;
	buffer += " (callback)\n";
	OutputDebugString(buffer);
}
static void PrintStringParam(const char * param, const char * value)
{
	CString buffer;
	buffer += "   ";
	buffer += param;
	buffer += " = \"";
	buffer += value;
	buffer += "\"\n";
	OutputDebugString(buffer);
}
static void PrintIntParam(const char * param, int value)
{
	char string[16];
	sprintf(string, "%d", value);
	PrintStringParam(param, string);
}
static void PrintBoolParam(const char * param, PEERBool value)
{
	if(value)
		PrintStringParam(param, "True");
	else
		PrintStringParam(param, "False");
}
static void PrintRoomParam(const char * param, RoomType roomType)
{
	if(roomType == TitleRoom)
		PrintStringParam("roomType", "Title Room");
	else if(roomType == GroupRoom)
		PrintStringParam("roomType", "Group Room");
	else if(roomType == StagingRoom)
		PrintStringParam("roomType", "Staging Room");
	else
		PrintStringParam("roomType", "<unknown>");

	GSI_UNUSED(param);
}

///////////////////////////
// Peer Global Callbacks //
///////////////////////////

// Called if peer gets disconnected from chat.
//////////////////////////////////////////////
static void DisconnectedCallback
(
	PEER peer,
	const char * reason,
	void * param
)
{
	PrintCallback("Disconnected");
	PrintStringParam("reason", reason);

	CString text = "You were disconnected from the server: ";
	if(reason)
		text += reason;
	Wizard->MessageBox(text);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called when a message arrives in a room.
///////////////////////////////////////////
static void RoomMessageCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * message,
	MessageType messageType,
	void * param
)
{
	PrintCallback("RoomMessage");
	PrintStringParam("nick", nick);
	PrintStringParam("message", message);

	// Form the message.
	////////////////////
	CString buffer;
	if(messageType == NormalMessage)
		buffer.Format("%s: %s", nick, message);
	else if(messageType == ActionMessage)
		buffer.Format("%s %s", nick, message);
	else
		buffer.Format("*%s* %s", nick, message);

	// Send it to the right place.
	//////////////////////////////
	if(roomType == StagingRoom)
	{
		StagingPage->m_chatWindow.InsertString(-1, buffer);
		StagingPage->m_chatWindow.SetTopIndex(StagingPage->m_chatWindow.GetCount() - 1);
	}
	else if((roomType == GroupRoom) || !Wizard->m_groupRooms)
	{
		GroupPage->m_chatWindow.InsertString(-1, buffer);
		GroupPage->m_chatWindow.SetTopIndex(GroupPage->m_chatWindow.GetCount() - 1);
	}
	else
	{
		TitlePage->m_chatWindow.InsertString(-1, buffer);
		TitlePage->m_chatWindow.SetTopIndex(TitlePage->m_chatWindow.GetCount() - 1);
	}

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called when a staging room player's ready state changes.
///////////////////////////////////////////////////////////
static void ReadyChangedCallback
(
	PEER peer,
	const char * nick,
	PEERBool ready,
	void * param
)
{
	PrintCallback("ReadyChanged");
	PrintStringParam("nick", nick);
	PrintBoolParam("ready", ready);

	// Update his ready state.
	//////////////////////////
	StagingPage->UpdatePlayerReady(nick, (BOOL)ready);

	// Check if we should enable the finish button.
	///////////////////////////////////////////////
	if(Wizard->m_hosting)
		StagingPage->CheckEnableFinish();

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called when the game launches in a staging room we've joined.
////////////////////////////////////////////////////////////////
static void GameStartedCallback
(
	PEER peer,
	SBServer server,
	const char * message,
	void * param
)
{
	const char * address = SBServerGetPublicAddress(server);

	PrintCallback("GameStarted");
	PrintStringParam("IP", address);
	PrintStringParam("message", message);

	char buffer[256];
	sprintf(buffer,
		"The game has been started.\n"
		"The host is at %s.\n"
		"Message: %s\n"
		"Hit OK to return to the staging room.",
		address,
		message);
	StagingPage->MessageBox(buffer);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called when a player joins a room we're in.
//////////////////////////////////////////////
static void PlayerJoinedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	void * param
)
{
	PrintCallback("PlayerJoined");
	PrintRoomParam("roomType", roomType);
	PrintStringParam("nick", nick);

	if(roomType == StagingRoom)
	{
		StagingPage->UpdatePlayerPing(nick, 9999);

		if(Wizard->m_hosting)
			Wizard->SetWizardButtons(PSWIZB_BACK | PSWIZB_DISABLEDFINISH);
	}
	else if((roomType == GroupRoom) || !Wizard->m_groupRooms)
		GroupPage->UpdatePlayerPing(nick, 9999);
	else
		TitlePage->UpdatePlayerPing(nick, 9999);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called when a player leaves a room we're in.
///////////////////////////////////////////////
static void PlayerLeftCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * reason,
	void * param
)
{
	PrintCallback("PlayerLeft");
	PrintRoomParam("roomType", roomType);
	PrintStringParam("nick", nick);

	if(roomType == StagingRoom)
	{
		StagingPage->RemovePlayer(nick);

		if(Wizard->m_hosting)
			StagingPage->CheckEnableFinish();
	}
	else if((roomType == GroupRoom) || !Wizard->m_groupRooms)
		GroupPage->RemovePlayer(nick);
	else
		TitlePage->RemovePlayer(nick);

	GSI_UNUSED(param);
	GSI_UNUSED(reason);
	GSI_UNUSED(peer);
}

// Called when a player's nickname changes.
///////////////////////////////////////////
static void PlayerChangedNickCallback
(
	PEER peer,
	RoomType roomType,
	const char * oldNick,
	const char * newNick,
	void * param
)
{
	PrintCallback("PlayerChangedNick");
	PrintRoomParam("roomType", roomType);
	PrintStringParam("oldNick", oldNick);
	PrintStringParam("newNick", newNick);

	if(roomType == StagingRoom)
		StagingPage->ChangePlayerNick(oldNick, newNick);
	else if((roomType == GroupRoom) || !Wizard->m_groupRooms)
		GroupPage->ChangePlayerNick(oldNick, newNick);
	else
		TitlePage->ChangePlayerNick(oldNick, newNick);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called whenever a new ping time is available.
////////////////////////////////////////////////
static void PingCallback
(
	PEER peer,
	const char * nick,
	int ping,
	void * param
)
{
	PrintCallback("Ping");
	PrintStringParam("nick", nick);
	PrintIntParam("ping", ping);

	if(Wizard->m_groupRooms && TitlePage->m_hWnd && TitlePage->FindPlayer(nick) != -1)
		TitlePage->UpdatePlayerPing(nick, ping);
	if(GroupPage->m_hWnd && GroupPage->FindPlayer(nick) != -1)
		GroupPage->UpdatePlayerPing(nick, ping);
	if(StagingPage->m_hWnd && StagingPage->FindPlayer(nick) != -1)
		StagingPage->UpdatePlayerPing(nick, ping);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called whenever a crossping is available.
////////////////////////////////////////////
static void CrossPingCallback
(
	PEER peer,
	const char * nick1,
	const char * nick2,
	int crossPing,
	void * param
)
{
	PrintCallback("CrossPing");
	PrintStringParam("nick1", nick1);
	PrintStringParam("nick2", nick2);
	PrintIntParam("crossPing", crossPing);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Player info for server reporting.
////////////////////////////////////
const int NumPlayers = 2;
const char * Players[] = { "Bob", "Joe" };

// Converts a key index to a string.
////////////////////////////////////
static const char * KeyToString(int key)
{
	return qr2_registered_key_list[key];
}

// Converts a key type to a string.
///////////////////////////////////
static const char * KeyTypeToString(qr2_key_type type)
{
	switch(type)
	{
	case key_server:
		return "server";
	case key_player:
		return "player";
	case key_team:
		return "team";
	}

	ASSERT(0);
	return "Unkown key type";
}

// Converts an error code to a string.
//////////////////////////////////////
static const char * ErrorTypeToString(qr2_error_t error)
{
	switch(error)
	{
	case e_qrnoerror:
		return "noerror";
	case e_qrwsockerror:
		return "wsockerror";
	case e_qrbinderror:
		return "rbinderror";
	case e_qrdnserror:
		return "dnserror";
	case e_qrconnerror:
		return "connerror";
	}

	ASSERT(0);
	return "Unknown error type";
}

// Reports server keys.
///////////////////////
static void QRServerKeyCallback
(
	PEER peer,
	int key,
	qr2_buffer_t buffer,
	void * param
)
{
	PrintCallback("QRServerKey");
	PrintStringParam("key", KeyToString(key));

	switch(key)
	{
	case GAMEVER_KEY:
		qr2_buffer_add(buffer, "1.01");
		break;
	case HOSTNAME_KEY:
		qr2_buffer_add(buffer, (LPCSTR)CreatePage->m_name);
		break;
	case NUMPLAYERS_KEY:
		qr2_buffer_add_int(buffer, NumPlayers);
		break;
	case MAXPLAYERS_KEY:
		qr2_buffer_add_int(buffer, NumPlayers + 2);
		break;
	default:
		qr2_buffer_add(buffer, "");
		break;
	}
	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Reports player keys.
///////////////////////
static void QRPlayerKeyCallback
(
	PEER peer,
	int key,
	int index,
	qr2_buffer_t buffer,
	void * param
)
{
	PrintCallback("QRPlayerKey");
	PrintStringParam("key", KeyToString(key));
	PrintIntParam("index", index);

	switch(key)
	{
	case PLAYER__KEY:
		qr2_buffer_add(buffer, Players[index]);
		break;
	case PING__KEY:
		qr2_buffer_add_int(buffer, rand() % 100);
		break;
	default:
		qr2_buffer_add(buffer, "");
		break;
	}
	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Reports team keys.
/////////////////////
static void QRTeamKeyCallback
(
	PEER peer,
	int key,
	int index,
	qr2_buffer_t buffer,
	void * param
)
{
	PrintCallback("QRTeamKey");
	PrintStringParam("key", KeyToString(key));
	PrintIntParam("index", index);

	// we don't report teams, so this shouldn't get called
	qr2_buffer_add(buffer, "");

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Reports supported keys.
//////////////////////////
static void QRKeyListCallback
(
	PEER peer,
	qr2_key_type type,
	qr2_keybuffer_t keyBuffer,
	void * param
)
{
	PrintCallback("QRKeyList");
	PrintStringParam("type", KeyTypeToString(type));

	// register the keys we use
	switch(type)
	{
	case key_server:
		qr2_keybuffer_add(keyBuffer, GAMEVER_KEY);
		break;
	case key_player:
		// no custom player keys
		break;
	case key_team:
		// no custom team keys
		break;
	}
	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Reports the players and team counts.
///////////////////////////////////////
static int QRCountCallback
(
	PEER peer,
	qr2_key_type type,
	void * param
)
{
	PrintCallback("QRCount");
	PrintStringParam("type", KeyTypeToString(type));

	if(type == key_player)
		return NumPlayers;
	else if(type == key_team)
		return 0;

	return 0;

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Called when there is a server reporting error.
/////////////////////////////////////////////////
static void QRAddErrorCallback
(
	PEER peer,
	qr2_error_t error,
	char * errorString,
	void * param
)
{
	PrintCallback("QRKeyList");
	PrintStringParam("type", ErrorTypeToString(error));
	PrintStringParam("errorString", errorString);

	CString str;
	str.Format("Peer: Server Reporting error: %s (%s)", ErrorTypeToString(error), errorString);
	Wizard->MessageBox(str);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

// Switching to this page.
//////////////////////////
BOOL CConnectPage::OnSetActive() 
{
	// Set which buttons the wizard shows.
	//////////////////////////////////////
	Wizard->SetWizardButtons(PSWIZB_NEXT);

//PEERSTART
	if(Wizard->m_peer)
	{
		// Shutdown peer.
		/////////////////
		peerShutdown(Wizard->m_peer);
		Wizard->m_peer = NULL;
	}
//PEERSTOP

	return CPropertyPage::OnSetActive();
}

// Called when peerConnect completes.
/////////////////////////////////////
static PEERBool connectSuccess;
static void ConnectCallback
(
	PEER peer,
	PEERBool success,
	int failureReason,
	void * param
)
{
	connectSuccess = success;

	if(!success)
		ConnectPage->MessageBox("Failed to connect.");

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
	GSI_UNUSED(failureReason);
}

// Called if there's an error with the nick.
////////////////////////////////////////////
static void NickErrorCallback
(
	PEER peer,
	int type,
	const char * nick,
	int numSuggestedNicks,
	const char ** suggestedNicks,
	void * param
)
{
	connectSuccess = PEERFalse;

	// Let the user know.
	/////////////////////
	if(type == PEER_IN_USE)
		ConnectPage->MessageBox("That nickname is already taken, please choose another one.");
	else
		ConnectPage->MessageBox("That nickname contains at least 1 invalid character, please choose another one.");

//PEERSTART
	// Cancel the connect.
	// Could display a window here asking for an alternate nick.
	////////////////////////////////////////////////////////////
	peerRetryWithNick(peer, NULL);
//PEERSTOP

	GSI_UNUSED(param);
	GSI_UNUSED(suggestedNicks);
	GSI_UNUSED(numSuggestedNicks);
	GSI_UNUSED(nick);
}

// Going to the next page.
//////////////////////////
LRESULT CConnectPage::OnWizardNext() 
{
	// Check data.
	//////////////
	UpdateData();
	Wizard->m_groupRooms = m_groupRooms;
	if(m_nick == "")
	{
		MessageBox("You must enter a nickname.");
		return -1;
	}
	if(m_title == "")
	{
		MessageBox("You must enter a title.");
		return -1;
	}

//PEERSTART
	// Check that the game's backend is available.
	//////////////////////////////////////////////
	GSIACResult result;
	GSIStartAvailableCheck(m_title);
	while((result = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(result != GSIACAvailable)
	{
		MessageBox("The backend is not available\n");
		return -1;
	}

	// Setup the callbacks.
	///////////////////////
	PEERCallbacks callbacks;
	memset(&callbacks, 0, sizeof(PEERCallbacks));
	callbacks.disconnected = DisconnectedCallback;
	callbacks.readyChanged = ReadyChangedCallback;
	callbacks.roomMessage = RoomMessageCallback;
	callbacks.gameStarted = GameStartedCallback;
	callbacks.playerJoined = PlayerJoinedCallback;
	callbacks.playerLeft = PlayerLeftCallback;
	callbacks.playerChangedNick = PlayerChangedNickCallback;
	callbacks.ping = PingCallback;
	callbacks.crossPing = CrossPingCallback;
	callbacks.qrServerKey = QRServerKeyCallback;
	callbacks.qrPlayerKey = QRPlayerKeyCallback;
	callbacks.qrTeamKey = QRTeamKeyCallback;
	callbacks.qrKeyList = QRKeyListCallback;
	callbacks.qrCount = QRCountCallback;
	callbacks.qrAddError = QRAddErrorCallback;
	callbacks.param = NULL;

	// Init peer.
	/////////////
	Wizard->m_peer = peerInitialize(&callbacks);
	if(!Wizard->m_peer)
	{
		MessageBox("Error initializing peer.");
		return -1;
	}

	// Setup which rooms to do pings and cross-pings in.
	////////////////////////////////////////////////////
	PEERBool pingRooms[NumRooms];
	PEERBool crossPingRooms[NumRooms];
	pingRooms[TitleRoom] = PEERFalse;
	pingRooms[GroupRoom] = PEERTrue;
	pingRooms[StagingRoom] = PEERTrue;
	crossPingRooms[TitleRoom] = PEERFalse;
	crossPingRooms[GroupRoom] = PEERFalse;
	crossPingRooms[StagingRoom] = PEERTrue;

	// Set the title.
	/////////////////
	if(!peerSetTitle(Wizard->m_peer, m_title, m_key, m_title, m_key, 0, 30, PEERFalse, pingRooms, crossPingRooms))
	{
		MessageBox("Error setting title.");
		peerShutdown(Wizard->m_peer);
		Wizard->m_peer = NULL;
		return -1;
	}

	// Connect to chat.
	///////////////////
	Wizard->StartHourglass();
	peerConnect(Wizard->m_peer, m_nick, 0, NickErrorCallback, ConnectCallback, NULL, PEERTrue);
	Wizard->StopHourglass();
	if(!connectSuccess)
	{
		MessageBox("Error connecting.");
		peerShutdown(Wizard->m_peer);
		Wizard->m_peer = NULL;
		return -1;
	}
//PEERSTOP

	if(!m_groupRooms)
		return IDD_GROUP_PAGE;

	return CPropertyPage::OnWizardNext();
}
