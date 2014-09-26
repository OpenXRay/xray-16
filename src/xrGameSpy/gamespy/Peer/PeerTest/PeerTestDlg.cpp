// PeerTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PeerTest.h"
#include "PeerTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define VERBOSE 0

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

CPeerTestDlg * dlg;

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPeerTestDlg dialog

CPeerTestDlg::CPeerTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPeerTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPeerTestDlg)
	m_blocking = FALSE;
	m_nick = _T("PeerPants");
	m_title = _T("gmtest");
	m_ready = FALSE;
	m_message = _T("");
	m_password = _T("");
	m_name = _T("My Game");
	m_quiet = FALSE;
	m_key = _T("");
	m_player = _T("");
	m_keyType = 0;
	m_keyRoom = 0;
	m_value = _T("");
	m_away = FALSE;
	m_awayReason = _T("");
	m_raw = _T("");
	m_filter = _T("");
	m_secretKey = _T("HA6zkS");
	m_cdKey = _T("");
	m_autoMatchStatus = _T("");
	m_maxPlayers = 2;
	m_email = _T("");
	m_loginPassword = _T("");
	m_namespace = _T("1");
	m_authToken = _T("");
	m_partnerChallenge = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Check for cfg's.
	///////////////////
	FILE * file;
	gsi_char buffer[256];
	int rcode;
	char dummy;
	file = _tfopen(_T("peertest.cfg"), _T("rt"));
	if(file)
	{
		rcode = _ftscanf(file, _T("%[^\n]%c"), buffer, &dummy);
		if((rcode >= 1) && buffer[0])
			m_title = buffer;
		rcode = _ftscanf(file, _T("%[^\n]%c"), buffer, &dummy);
		if((rcode >= 1) && buffer[0])
			m_secretKey = buffer;
		rcode = _ftscanf(file, _T("%[^\n]%c"), buffer, &dummy);
		if((rcode >= 1) && buffer[0])
			m_nick = buffer;
		rcode = _ftscanf(file, _T("%[^\n]%c"), buffer, &dummy);
		if((rcode >= 1) && buffer[0])
			m_email = buffer;
		rcode = _ftscanf(file, _T("%[^\n]%c"), buffer, &dummy);
		if((rcode >= 1) && buffer[0])
			m_loginPassword = buffer;
		rcode = _ftscanf(file, _T("%[^\n]%c"), buffer, &dummy);
		if((rcode >= 1) && buffer[0])
			m_namespace = buffer;
		fclose(file);
	}

	// Set the nick if it was passed on the command line.
	/////////////////////////////////////////////////////
	if(__argc == 2)
		m_nick = __argv[1];

	m_selectedNick = _T("PleaseSelectANick");
	m_selectedRoom = TitleRoom;
}

void CPeerTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPeerTestDlg)
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Control(pDX, IDC_TITLE_PLAYERS, m_playersT);
	DDX_Control(pDX, IDC_GROUP_PLAYERS, m_playersG);
	DDX_Control(pDX, IDC_STAGING_PLAYERS, m_playersS);
	DDX_Control(pDX, IDC_PLAYERS_BOX3, m_playersBoxT);
	DDX_Control(pDX, IDC_PLAYERS_BOX2, m_playersBoxG);
	DDX_Control(pDX, IDC_PLAYERS_BOX, m_playersBoxS);
	DDX_Control(pDX, IDC_CHAT_LIST, m_chatList);
	DDX_Control(pDX, IDC_ROOMS, m_rooms);
	DDX_Control(pDX, IDC_ROOMS_BOX, m_roomsBox);
	DDX_Control(pDX, IDC_CHECK1, m_readyCtl);
	DDX_Check(pDX, IDC_BLOCKING, m_blocking);
	DDX_Text(pDX, IDC_NICK, m_nick);
	DDV_MaxChars(pDX, m_nick, 127);
	DDX_Text(pDX, IDC_TITLE, m_title);
	DDX_Check(pDX, IDC_CHECK1, m_ready);
	DDX_Text(pDX, IDC_MESSAGE, m_message);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDV_MaxChars(pDX, m_password, 23);
	DDX_Text(pDX, IDC_NAME, m_name);
	DDX_Check(pDX, IDC_QUIET, m_quiet);
	DDX_Text(pDX, IDC_KEY, m_key);
	DDX_Text(pDX, IDC_PLAYER, m_player);
	DDX_Radio(pDX, IDC_KEY_TYPE, m_keyType);
	DDX_Radio(pDX, IDC_KEY_ROOM, m_keyRoom);
	DDX_Text(pDX, IDC_VALUE, m_value);
	DDX_Check(pDX, IDC_AWAY, m_away);
	DDX_Text(pDX, IDC_AWAY_REASON, m_awayReason);
	DDX_Text(pDX, IDC_RAW, m_raw);
	DDX_Text(pDX, IDC_FILTER, m_filter);
	DDX_Text(pDX, IDC_SECRET_KEY, m_secretKey);
	DDX_Text(pDX, IDC_CDKEY, m_cdKey);
	DDX_Text(pDX, IDC_AUTO_MATCH_STATUS, m_autoMatchStatus);
	DDX_Text(pDX, IDC_MAX_PLAYERS, m_maxPlayers);
	DDX_Text(pDX, IDC_EMAIL, m_email);
	DDX_Text(pDX, IDC_LOGIN_PASSWORD, m_loginPassword);
	DDX_Text(pDX, IDC_NAMESPACE, m_namespace);
	DDX_Text(pDX, IDC_AUTH_TOKEN, m_authToken);
	DDX_Text(pDX, IDC_PARTNER_CHALLENGE, m_partnerChallenge);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPeerTestDlg, CDialog)
	//{{AFX_MSG_MAP(CPeerTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BUTTON5, OnButton5)
	ON_BN_CLICKED(IDC_BUTTON8, OnButton8)
	ON_BN_CLICKED(IDC_BUTTON9, OnButton9)
	ON_BN_CLICKED(IDC_BUTTON10, OnButton10)
	ON_BN_CLICKED(IDC_BUTTON12, OnButton12)
	ON_BN_CLICKED(IDC_BUTTON14, OnButton14)
	ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
	ON_BN_CLICKED(IDC_BUTTON15, OnButton15)
	ON_BN_CLICKED(IDC_BUTTON13, OnButton13)
	ON_BN_CLICKED(IDC_BUTTON20, OnButton20)
	ON_BN_CLICKED(IDC_BUTTON25, OnButton25)
	ON_BN_CLICKED(IDC_BUTTON17, OnButton17)
	ON_BN_CLICKED(IDC_BUTTON22, OnButton22)
	ON_BN_CLICKED(IDC_BUTTON18, OnButton18)
	ON_BN_CLICKED(IDC_BUTTON23, OnButton23)
	ON_BN_CLICKED(IDC_BUTTON26, OnButton26)
	ON_BN_CLICKED(IDC_BUTTON27, OnButton27)
	ON_BN_CLICKED(IDC_BUTTON28, OnButton28)
	ON_BN_CLICKED(IDC_BUTTON16, OnButton16)
	ON_BN_CLICKED(IDC_BUTTON29, OnButton29)
	ON_BN_CLICKED(IDC_BUTTON30, OnButton30)
	ON_LBN_SELCHANGE(IDC_TITLE_PLAYERS, OnSelchangeTitlePlayers)
	ON_LBN_SELCHANGE(IDC_GROUP_PLAYERS, OnSelchangeGroupPlayers)
	ON_LBN_SELCHANGE(IDC_STAGING_PLAYERS, OnSelchangeStagingPlayers)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_BUTTON7, OnButton7)
	ON_BN_CLICKED(IDC_BUTTON21, OnButton21)
	ON_BN_CLICKED(IDC_BUTTON24, OnButton24)
	ON_BN_CLICKED(IDC_BUTTON31, OnButton31)
	ON_BN_CLICKED(IDC_BUTTON32, OnButton32)
	ON_BN_CLICKED(IDC_BUTTON33, OnButton33)
	ON_BN_CLICKED(IDC_BUTTON34, OnButton34)
	ON_BN_CLICKED(IDC_BUTTON35, OnButton35)
	ON_BN_CLICKED(IDC_CHANGE_NICK, OnChangeNick)
	ON_BN_CLICKED(IDC_QUIET, OnQuiet)
	ON_BN_CLICKED(IDC_BUTTON37, OnButton37)
	ON_BN_CLICKED(IDC_BUTTON38, OnButton38)
	ON_LBN_SETFOCUS(IDC_TITLE_PLAYERS, OnSetfocusTitlePlayers)
	ON_LBN_SETFOCUS(IDC_GROUP_PLAYERS, OnSetfocusGroupPlayers)
	ON_LBN_SETFOCUS(IDC_STAGING_PLAYERS, OnSetfocusStagingPlayers)
	ON_BN_CLICKED(IDC_AWAY, OnAway)
	ON_BN_CLICKED(IDC_BUTTON39, OnButton39)
	ON_BN_CLICKED(IDC_BUTTON40, OnButton40)
	ON_BN_CLICKED(IDC_FIX_NICK, OnFixNick)
	ON_BN_CLICKED(IDC_BUTTON11, OnButton11)
	ON_BN_CLICKED(IDC_BUTTON42, OnButton42)
	ON_BN_CLICKED(IDC_BUTTON43, OnButton43)
	ON_BN_CLICKED(IDC_BUTTON44, OnButton44)
	ON_BN_CLICKED(IDC_BUTTON45, OnButton45)
	ON_BN_CLICKED(IDC_BUTTON46, OnButton46)
	ON_LBN_DBLCLK(IDC_ROOMS, OnDblclkRooms)
	ON_BN_CLICKED(IDC_BUTTON47, OnButton47)
	ON_BN_CLICKED(IDC_BUTTON48, OnButton48)
	ON_BN_CLICKED(IDC_START_AUTO_MATCH, OnStartAutoMatch)
	ON_BN_CLICKED(IDC_STOP_AUTO_MATCH, OnStopAutoMatch)
	ON_BN_CLICKED(IDC_BUTTON36, OnButton36)
	ON_BN_CLICKED(IDC_BUTTON49, OnButton49)
	ON_BN_CLICKED(IDC_BUTTON50, OnButton50)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPeerTestDlg message handlers

BOOL CPeerTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	dlg = this;
	SetTimer(100, 10, NULL);
	m_peer = NULL;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPeerTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPeerTestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPeerTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

static const gsi_char * RoomToString
(
	RoomType roomType
)
{
	if(roomType == TitleRoom)
		return _T("Title");

	if(roomType == GroupRoom)
		return _T("Group");

	if(roomType == StagingRoom)
		return _T("Staging");

	return _T("?");
}

static void ClearPlayersList(CListBox & list)
{
	CString * str;
	for(int i = 0 ; i < list.GetCount() ; i++)
	{
		str = (CString *)list.GetItemData(i);
		if(str)
			delete str;
	}
	list.ResetContent();
}

static void ClearRoomsList(CListBox & list)
{
	list.ResetContent();
}

static CListBox & m_players(RoomType roomType)
{
	switch(roomType)
	{
	case TitleRoom:
		return dlg->m_playersT;
	case GroupRoom:
		return dlg->m_playersG;
	case StagingRoom:
		return dlg->m_playersS;
	default:
		ASSERT(0);
	}

	static CListBox dummy;
	return dummy;
}

static CButton & m_playersBox(RoomType roomType)
{
	switch(roomType)
	{
	case TitleRoom:
		return dlg->m_playersBoxT;
	case GroupRoom:
		return dlg->m_playersBoxG;
	case StagingRoom:
		return dlg->m_playersBoxS;
	default:
		ASSERT(0);
	}

	static CButton dummy;
	return dummy;
}

static void DisconnectedCallback
(
	PEER peer,
	const gsi_char * reason,
	void * param
)
{
	CString message = "Disconnected: ";
	message += reason;
	dlg->MessageBox(message);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void RoomMessageCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	const gsi_char * message,
	MessageType messageType,
	void * param
)
{
	int c;

	c = *RoomToString(roomType);

	gsi_char buffer[2048];
	if(messageType == NormalMessage)
	{
		_stprintf(buffer, _T("%c]%s: %s"),
			c,
			nick,
			message);
	}
	else if(messageType == ActionMessage)
	{
		_stprintf(buffer, _T("%c]%s %s"),
			c,
			nick,
			message);
	}
	else
	{
		_stprintf(buffer, _T("%c]*%s*: %s"),
			c,
			nick,
			message);
	}
	int index = dlg->m_chatList.InsertString(-1, buffer);

#if VERBOSE
	OutputDebugString(buffer);
	OutputDebugString("\n");
#endif

	dlg->m_chatList.SetCurSel(index);
	dlg->m_chatList.SetCurSel(-1);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void RoomUTMCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	const gsi_char * command,
	const gsi_char * parameters,
	PEERBool authenticated,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T(" UTM %s| %s | %s | %s | %s\n"),
		authenticated?"(authenticated) ":"",
		RoomToString(roomType),
		nick,
		command,
		parameters);
	OutputDebugString(buffer);
#endif
	
	GSI_UNUSED(param);
	GSI_UNUSED(authenticated);
	GSI_UNUSED(parameters);
	GSI_UNUSED(command);
	GSI_UNUSED(nick);
	GSI_UNUSED(roomType);
	GSI_UNUSED(peer);
}

static void RoomNameChangedCallback
(
	PEER peer,
	RoomType roomType,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T(" NAME | %s | %s\n"),
		RoomToString(roomType),
		peerGetRoomName(peer, roomType));

	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
	GSI_UNUSED(roomType);
}

const gsi_char * ModeToString
(
	CHATChannelMode * mode
)
{
	static gsi_char buffer[256];
	buffer[0] = '\0';

	if(mode->InviteOnly)
		_tcscat(buffer,_T("InviteOnly+"));
	if(mode->Private)
		_tcscat(buffer,_T("Private+"));
	if(mode->Secret)
		_tcscat(buffer,_T("Secret+"));
	if(mode->Moderated)
		_tcscat(buffer,_T("Moderated+"));
	if(mode->NoExternalMessages)
		_tcscat(buffer,_T("NoExternalMessages+"));
	if(mode->OnlyOpsChangeTopic)
		_tcscat(buffer,_T("OnlyOpsChangeTopic+"));

	if(buffer[0])
		buffer[_tcslen(buffer) - 1] = '\0';

	return buffer;
}

static void RoomModeChangedCallback
(
	PEER peer,
	RoomType roomType,
	CHATChannelMode * mode,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T(" MODE | %s | %s\n"),
		RoomToString(roomType),
		ModeToString(mode));

	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(mode);
	GSI_UNUSED(roomType);
	GSI_UNUSED(peer);
}

static void PlayerMessageCallback
(
	PEER peer,
	const gsi_char * nick,
	const gsi_char * message,
	MessageType messageType,
	void * param
)
{
	gsi_char buffer[2048];
	if(messageType == NormalMessage)
	{
		_stprintf(buffer, _T("%s: %s"),
			nick,
			message);
	}
	else if(messageType == ActionMessage)
	{
		_stprintf(buffer, _T("%s %s"),
			nick,
			message);
	}
	else
	{
		_stprintf(buffer, _T("*%s*: %s"),
			nick,
			message);
	}
	int index = dlg->m_chatList.InsertString(-1, buffer);

#if VERBOSE
	OutputDebugString(buffer);
	OutputDebugString("\n");
#endif

	dlg->m_chatList.SetCurSel(index);
	dlg->m_chatList.SetCurSel(-1);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void PlayerUTMCallback
(
	PEER peer,
	const gsi_char * nick,
	const gsi_char * command,
	const gsi_char * parameters,
	PEERBool authenticated,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T(" UTM %s| %s | %s | %s\n"),
		authenticated?"(authenticated) ":"",
		nick,
		command,
		parameters);
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(authenticated);
	GSI_UNUSED(parameters);
	GSI_UNUSED(command);
	GSI_UNUSED(nick);
	GSI_UNUSED(peer);
}

static void ReadyChangedCallback
(
	PEER peer,
	const gsi_char * nick,
	PEERBool ready,
	void * param
)
{
	gsi_char buffer[256];
	if(ready)
		_stprintf(buffer, _T("%s is ready\n"), nick);
	else
		_stprintf(buffer, _T("%s is not ready\n"), nick);
	OutputDebugString(buffer);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void GameStartedCallback
(
	PEER peer,
	SBServer server,
	const gsi_char * message,
	void * param
)
{
	gsi_char buffer[256];
	_stprintf(buffer, _T("The game is starting: %s:%d %s"),
		SBServerGetPublicAddress(server), SBServerGetIntValue(server, _T("hostport"), 0), message);
	dlg->MessageBox(buffer);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void PlayerJoinedCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	void * param
)
{
	dlg->FillPlayerList(roomType);

	GSI_UNUSED(param);
	GSI_UNUSED(nick);
	GSI_UNUSED(peer);
}

static void PlayerLeftCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	const gsi_char * reason,
	void * param
)
{
	dlg->FillPlayerList(roomType);

	GSI_UNUSED(param);
	GSI_UNUSED(reason);
	GSI_UNUSED(nick);
	GSI_UNUSED(peer);
}

static void KickedCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	const gsi_char * reason,
	void * param
)
{
	CString str;
	str.Format(_T("Kicked from %s by %s: \"%s\""), RoomToString(roomType), nick, reason);
	dlg->MessageBox(str);
	ClearPlayersList(m_players(roomType));

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void NewPlayerListCallback
(
	PEER peer,
	RoomType roomType,
	void * param
)
{
	dlg->FillPlayerList(roomType);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void PlayerChangedNickCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * oldNick,
	const gsi_char * newNick,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T(" NICK | %s | %s\n"), oldNick, newNick);
	OutputDebugString(buffer);
#endif

	// Update the player list for this room.
	////////////////////////////////////////
	dlg->FillPlayerList(roomType);

	GSI_UNUSED(param);
	GSI_UNUSED(newNick);
	GSI_UNUSED(oldNick);
	GSI_UNUSED(peer);
}

static void PlayerInfoCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	unsigned int IP,
	int profileID,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	IN_ADDR addr;
	addr.s_addr = IP;
	_stprintf(buffer, _T(" INFO | %s | %s | %s | %d\n"), RoomToString(roomType), nick?nick:"(END)", inet_ntoa(addr), profileID);
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(profileID);
	GSI_UNUSED(IP);
	GSI_UNUSED(nick);
	GSI_UNUSED(roomType);
	GSI_UNUSED(peer);
}

static void FlagsToString
(
	int flags,
	CString & str
)
{
	if(!flags)
	{
		str = "(none)";
		return;
	}

	str = "";

	if(flags & PEER_FLAG_STAGING)
		str += "staging+";
	if(flags & PEER_FLAG_READY)
		str += "ready+";
	if(flags & PEER_FLAG_PLAYING)
		str += "playing+";
	if(flags & PEER_FLAG_AWAY)
		str += "away+";
	if(flags & PEER_FLAG_HOST)
		str += "host+";
	if(flags & PEER_FLAG_OP)
		str += "op+";
	if(flags & PEER_FLAG_VOICE)
		str += "voice+";

	if(!str.IsEmpty())
		str.Delete(str.GetLength() - 1);
}

static void PlayerFlagsChangedCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	int oldFlags,
	int newFlags,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	CString strOldFlags;
	CString strNewFlags;

	FlagsToString(oldFlags, strOldFlags);
	FlagsToString(newFlags, strNewFlags);

	_stprintf(buffer, _T(" FLAGS | %s | %s | %s=>%s\n"), RoomToString(roomType), nick, strOldFlags, strNewFlags);
	OutputDebugString(buffer);
#endif

	// If the host flag changed, refresh the room.
	//////////////////////////////////////////////
	if((oldFlags ^ newFlags) & PEER_FLAG_HOST)
		dlg->FillPlayerList(roomType);

	GSI_UNUSED(param);
	GSI_UNUSED(nick);
	GSI_UNUSED(peer);
}

static int FindPingPlayersNick
(
	const gsi_char * nick
)
{
	CListBox & list = dlg->m_pingPlayers;
	int count;
	int i;
	CString text;
	int start;

	count = list.GetCount();
	for(i = 0 ; i < count ; i++)
	{
		list.GetText(i, text);

		start = text.Find('-');
		ASSERT(start != -1);

		if(text.Mid(start + 1).CompareNoCase(nick) == 0)
			return i;
	}

	return -1;
}

static void PingCallback
(
	PEER peer,
	const gsi_char * nick,
	int ping,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T(" PING | %s | %d\n"), nick, ping);
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(ping);
	GSI_UNUSED(nick);
	GSI_UNUSED(peer);
}

static void CrossPingCallback
(
	PEER peer,
	const gsi_char * nick1,
	const gsi_char * nick2,
	int crossPing,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T("XPING | %s<->%s | %d\n"), nick1, nick2, crossPing);
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(crossPing);
	GSI_UNUSED(nick2);
	GSI_UNUSED(nick1);
	GSI_UNUSED(peer);
}

static void GlobalKeyChangedCallback
(
	PEER peer,
	const gsi_char * nick,
	const gsi_char * key,
	const gsi_char * value,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T("KEY | %s | %s = %s\n"), nick, key, value);
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(value);
	GSI_UNUSED(key);
	GSI_UNUSED(nick);
	GSI_UNUSED(peer);
}

static void RoomKeyChangedCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	const gsi_char * key,
	const gsi_char * value,
	void * param
)
{
#if VERBOSE
	gsi_char buffer[256];
	_stprintf(buffer, _T("KEY | %s | %s | %s = %s\n"), nick, RoomToString(roomType), key, value);
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(value);
	GSI_UNUSED(key);
	GSI_UNUSED(nick);
	GSI_UNUSED(roomType);
	GSI_UNUSED(peer);
}

static const gsi_char * KeyTypeToString(qr2_key_type type)
{
	switch(type)
	{
	case key_server:
		return _T("server");
	case key_player:
		return _T("player");
	case key_team:
		return _T("team");
	}

	ASSERT(0);
	return _T("Unkown key type");
}

static const gsi_char * ErrorTypeToString(qr2_error_t error)
{
	switch(error)
	{
	case e_qrnoerror:
		return _T("noerror");
	case e_qrwsockerror:
		return _T("wsockerror");
	case e_qrbinderror:
		return _T("rbinderror");
	case e_qrdnserror:
		return _T("dnserror");
	case e_qrconnerror:
		return _T("connerror");
	case e_qrnochallengeerror:
		return _T("nochallengeerror");
	}

	ASSERT(0);
	return _T("Unknown error type");
}

static void QRServerKeyCallback
(
	PEER peer,
	int key,
	qr2_buffer_t buffer,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, "QR_SERVER_KEY | %d\n", key);
	OutputDebugString(verbose);
#endif

	switch(key)
	{
	case HOSTNAME_KEY:
		qr2_buffer_add(buffer, _T("My Game"));
		break;
	case NUMPLAYERS_KEY:
		qr2_buffer_add_int(buffer, 1);
		break;
	case MAXPLAYERS_KEY:
		qr2_buffer_add_int(buffer, 4);
		break;
	case GAMEMODE_KEY:
		qr2_buffer_add(buffer, _T("openplaying"));
		break;
	case HOSTPORT_KEY:
		qr2_buffer_add_int(buffer, 15151);
		break;
	case MAPNAME_KEY:
		qr2_buffer_add(buffer, _T("Big Crate Room"));
		break;
	case GAMETYPE_KEY:
		qr2_buffer_add(buffer, _T("Friendly"));
		break;
	case TIMELIMIT_KEY:
		qr2_buffer_add_int(buffer, 100);
		break;
	case FRAGLIMIT_KEY:
		qr2_buffer_add_int(buffer, 0);
		break;
	case TEAMPLAY_KEY:
		qr2_buffer_add_int(buffer, 0);
		break;
	default:
		qr2_buffer_add(buffer, _T(""));
		break;
	}

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void QRPlayerKeyCallback
(
	PEER peer,
	int key,
	int index,
	qr2_buffer_t buffer,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, "QR_PLAYER_KEY | %d | %d\n", key, index);
	OutputDebugString(verbose);
#endif

	switch(key)
	{
	case PLAYER__KEY:
		qr2_buffer_add(buffer, _T("Johnny McJohnson"));
		break;
	case PING__KEY:
		qr2_buffer_add_int(buffer, 17);
		break;
	default:
		qr2_buffer_add(buffer, _T(""));
		break;
	}

	GSI_UNUSED(param);
	GSI_UNUSED(index);
	GSI_UNUSED(peer);
}

static void QRTeamKeyCallback
(
	PEER peer,
	int key,
	int index,
	qr2_buffer_t buffer,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, "QR_TEAM_KEY | %d | %d\n", key, index);
	OutputDebugString(verbose);
#endif

	// we don't report teams, so this shouldn't get called
	qr2_buffer_add(buffer, _T(""));

	GSI_UNUSED(param);
	GSI_UNUSED(key);
	GSI_UNUSED(index);
	GSI_UNUSED(peer);
}

static void QRKeyListCallback
(
	PEER peer,
	qr2_key_type type,
	qr2_keybuffer_t keyBuffer,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, "QR_KEY_LIST | %s\n", KeyTypeToString(type));
	OutputDebugString(verbose);
#endif

	// register the keys we use
	switch(type)
	{
	case key_server:
		if(!peerIsAutoMatching(peer))
		{
			qr2_keybuffer_add(keyBuffer, HOSTPORT_KEY);
			qr2_keybuffer_add(keyBuffer, MAPNAME_KEY);
			qr2_keybuffer_add(keyBuffer, GAMETYPE_KEY);
			qr2_keybuffer_add(keyBuffer, TIMELIMIT_KEY);
			qr2_keybuffer_add(keyBuffer, FRAGLIMIT_KEY);
			qr2_keybuffer_add(keyBuffer, TEAMPLAY_KEY);
		}
		break;
	case key_player:
		// no custom player keys
		break;
	case key_team:
		// no custom team keys
		break;
	}

	GSI_UNUSED(param);
}

static int QRCountCallback
(
	PEER peer,
	qr2_key_type type,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, "QR_COUNT | %s\n", KeyTypeToString(type));
	OutputDebugString(verbose);
#endif

	if(type == key_player)
		return 1;
	else if(type == key_team)
		return 0;

	return 0;

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void QRAddErrorCallback
(
	PEER peer,
	qr2_error_t error,
	gsi_char * errorString,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, "QR_ADD_ERROR | %s | %s\n", ErrorTypeToString(error), errorString);
	OutputDebugString(verbose);
#endif

	CString str;
	str.Format(_T("Reporting error: %s (%s)"), ErrorTypeToString(error), errorString);
	dlg->MessageBox(str);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void QRNatNegotiateCallback
(
	PEER peer,
	int cookie,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, _T("QR_NAT_NEGOTIATE | 0x%08X\n"), cookie);
	OutputDebugString(verbose);
#endif

	CString str;
	str.Format(_T("Received Nat Negotiate Cookie: 0x%08X"), cookie);
	dlg->MessageBox(str);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void QRPublicAddressCallback
(
	PEER peer,
	unsigned int ip,
	unsigned short port,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	struct in_addr addr;
	addr.s_addr = ip;
	_stprintf(verbose, _T("QR_PUBLIC_ADDRESS | %s:%d\n"), inet_ntoa(addr), port);
	OutputDebugString(verbose);
#endif

	GSI_UNUSED(param);
	GSI_UNUSED(port);
	GSI_UNUSED(ip);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnButton7() 
{
	if(!m_peer)
	{
		GSIStartAvailableCheck("gmtest");
		GSIACResult result;
		while((result = GSIAvailableCheckThink()) == GSIACWaiting)
			msleep(10);

		if(result == GSIACUnavailable)
			MessageBox("This game's backend services are unavailable.");
		else if(result == GSIACTemporarilyUnavailable)
			MessageBox("This game's backend services are temporarily unavailable.\nPlease try again later.");
		else
		{
			PEERCallbacks callbacks;
			memset(&callbacks, 0, sizeof(PEERCallbacks));
			callbacks.disconnected = DisconnectedCallback;
			callbacks.readyChanged = ReadyChangedCallback;
			callbacks.roomMessage = RoomMessageCallback;
			callbacks.roomUTM = RoomUTMCallback;
			callbacks.roomNameChanged = RoomNameChangedCallback;
			callbacks.roomModeChanged = RoomModeChangedCallback;
			callbacks.playerMessage = PlayerMessageCallback;
			callbacks.playerUTM = PlayerUTMCallback;
			callbacks.gameStarted = GameStartedCallback;
			callbacks.playerJoined = PlayerJoinedCallback;
			callbacks.playerLeft = PlayerLeftCallback;
			callbacks.kicked = KickedCallback;
			callbacks.newPlayerList = NewPlayerListCallback;
			callbacks.playerChangedNick = PlayerChangedNickCallback;
			callbacks.playerInfo = PlayerInfoCallback;
			callbacks.playerFlagsChanged = PlayerFlagsChangedCallback;
			callbacks.ping = PingCallback;
			callbacks.crossPing = CrossPingCallback;
			callbacks.globalKeyChanged = GlobalKeyChangedCallback;
			callbacks.roomKeyChanged = RoomKeyChangedCallback;
			callbacks.qrServerKey = QRServerKeyCallback;
			callbacks.qrPlayerKey = QRPlayerKeyCallback;
			callbacks.qrTeamKey = QRTeamKeyCallback;
			callbacks.qrKeyList = QRKeyListCallback;
			callbacks.qrCount = QRCountCallback;
			callbacks.qrAddError = QRAddErrorCallback;
			callbacks.qrNatNegotiateCallback = QRNatNegotiateCallback;
			callbacks.qrPublicAddressCallback = QRPublicAddressCallback;
			callbacks.param = NULL;

			m_peer = peerInitialize(&callbacks);
			if(!m_peer)
				dlg->MessageBox(_T("Init: failure"));
		}
	}
}

static void ConnectCallback
(
	PEER peer,
	PEERBool success,
	int failureReason,
	void * param
)
{
	CString str;
	if(success)
		str.Format(_T("Connect: success"));
	else
	{
		CString reason;
		if(failureReason == PEER_DISCONNECTED)
			reason = "disconnected";
		else if(failureReason == PEER_NICK_ERROR)
			reason = "nick error";
		else if(failureReason == PEER_LOGIN_FAILED)
			reason = "login failed";
		else
			reason = "unknown";
		str.Format(_T("Connect: failure (%s)"), reason);
	}
	dlg->MessageBox(str);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void NickErrorCallback
(
	PEER peer,
	int type,
	const gsi_char * nick,
	int numSuggestedNicks,
	const char ** suggestedNicks,
	void * param
)
{
	CString str;
	gsi_char retryNick[64];
	int rcode;

	if(type == PEER_IN_USE)
	{
		_stprintf(retryNick, "PeerTest%d", rand() % 100);
		str.Format("Nick '%s' in use.\nRetry with random nick '%s'?", nick, retryNick);
		rcode = dlg->MessageBox(str, NULL, MB_YESNO);
		peerRetryWithNick(peer, (rcode == IDYES)?retryNick:NULL);
		return;
	}

	if(type == PEER_NICK_TOO_LONG)
	{
		_stprintf(retryNick, "PeerTest%d", rand() % 100);
		str.Format("Nick '%s' too long.\nRetry with random nick '%s'?", nick, retryNick);
		rcode = dlg->MessageBox(str, NULL, MB_YESNO);
		peerRetryWithNick(peer, (rcode == IDYES)?retryNick:NULL);
		return;
	}
	if(type == PEER_INVALID)
	{
		peerFixNick(retryNick, nick);
		str.Format(_T("Invalid nick '%s'.\nRetry with fixed nick '%s'?"), nick, retryNick);
		rcode = dlg->MessageBox(str, NULL, MB_YESNO);
		peerRetryWithNick(peer, (rcode == IDYES)?retryNick:NULL);
		return;
	}

	if(type == PEER_UNIQUENICK_EXPIRED)
	{
		dlg->UpdateData();
		str.Format(_T("This account's unique nick has expired.\nRegister '%s'?"), dlg->m_nick);
		rcode = dlg->MessageBox(str, NULL, MB_YESNO);
		peerRegisterUniqueNick(peer, atoi(dlg->m_namespace), (rcode == IDYES)?((LPCSTR)dlg->m_nick):NULL, NULL);
		return;
	}

	if(type == PEER_NO_UNIQUENICK)
	{
		dlg->UpdateData();
		str.Format(_T("This account has no unique nick.\nRegister '%s'?"), dlg->m_nick);
		rcode = dlg->MessageBox(str, NULL, MB_YESNO);
		peerRegisterUniqueNick(peer, atoi(dlg->m_namespace), (rcode == IDYES)?((LPCSTR)dlg->m_nick):NULL, NULL);
		return;
	}

	if(type == PEER_INVALID_UNIQUENICK)
	{
		if(numSuggestedNicks <= 0)
		{
			dlg->MessageBox(_T("Invalid unique nick with no suggestions!"));
			peerRegisterUniqueNick(peer, 0, NULL, NULL);
			return;
		}

		dlg->UpdateData();

		for(int i = 0 ; i < numSuggestedNicks ; i++)
		{
			str.Format(_T("The unique nick you attempted to register is either invalid or already in use.\nWould you like to register '%s'?"), suggestedNicks[i]);
			rcode = dlg->MessageBox(str, NULL, MB_YESNOCANCEL);
			if(rcode == IDCANCEL)
				break;
			if(rcode == IDYES)
			{
				peerRegisterUniqueNick(peer, atoi(dlg->m_namespace), suggestedNicks[i], NULL);
				return;
			}
		}
		peerRegisterUniqueNick(peer, 0, NULL, NULL);
		return;
	}

	GSI_UNUSED(param);
}

void CPeerTestDlg::OnButton1() 
{
	if(m_peer)
	{
		if(peerIsConnected(m_peer))
		{
			MessageBox("Already connected");
			return;
		}

		UpdateData();

		peerConnect(
		m_peer,
		m_nick,
		0,
		NickErrorCallback,
		ConnectCallback, 
		this,
		(PEERBool)m_blocking);
	}
}

void CPeerTestDlg::OnButton36() 
{
	if(m_peer)
	{
		if(peerIsConnected(m_peer))
		{
			MessageBox("Already connected");
			return;
		}

		UpdateData();

		if(m_namespace.IsEmpty() || !isdigit(m_namespace.GetAt(0)))
		{
			MessageBox("Enter a namespace of 0 or greater");
			return;
		}

		peerConnectLogin(
		m_peer,
		atoi(m_namespace),
		m_email,
		m_nick,
		NULL,
		m_loginPassword,
		NickErrorCallback,
		ConnectCallback, 
		this,
		(PEERBool)m_blocking);
	}
}

void CPeerTestDlg::OnButton50() 
{
	if(m_peer)
	{
		if(peerIsConnected(m_peer))
		{
			MessageBox("Already connected");
			return;
		}

		UpdateData();

		if(m_namespace.IsEmpty() || !isdigit(m_namespace.GetAt(0)))
		{
			MessageBox("Enter a namespace of 0 or greater");
			return;
		}

		peerConnectLogin(
		m_peer,
		atoi(m_namespace),
		NULL,
		NULL,
		m_nick,
		m_loginPassword,
		NickErrorCallback,
		ConnectCallback, 
		this,
		(PEERBool)m_blocking);
	}
}

void CPeerTestDlg::OnButton49() 
{
	if(m_peer)
	{
		if(peerIsConnected(m_peer))
		{
			MessageBox("Already connected");
			return;
		}

		UpdateData();

		peerConnectPreAuth(
		m_peer,
		m_authToken,
		m_partnerChallenge,
		NickErrorCallback,
		ConnectCallback, 
		this,
		(PEERBool)m_blocking);
	}
}

void CPeerTestDlg::OnButton21() 
{
	if(m_peer)
	{
		peerShutdown(m_peer);
		m_peer = NULL;
	}
}

void CPeerTestDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == 100)
	{
		if(m_peer)
		{
			peerThink(m_peer);
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}

void EnumPlayersCallback
(
	PEER peer,
	PEERBool success,
	RoomType roomType,
	int index,
	const gsi_char * nick,
	int flags,
	void * param
)
{
	if(!success)
	{
		dlg->MessageBox(_T("EnumPlayers failed"));
		return;
	}

	if(index == -1)
		return;

	dlg->m_count++;
	if(flags & PEER_FLAG_HOST)
	{
		gsi_char buffer[128];
		_stprintf(buffer, _T("HOST: %s"), nick);
		m_players(roomType).AddString(buffer);
	}
	else
		m_players(roomType).AddString(nick);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::FillPlayerList
(
	RoomType roomType
)
{
	UpdateData();
	ClearPlayersList(m_players(roomType));
	m_count = 0;
	if(peerInRoom(m_peer, roomType))
		peerEnumPlayers(m_peer, roomType, EnumPlayersCallback, NULL);

	gsi_char buffer[128];
	_stprintf(buffer, _T("Players: %d"), dlg->m_count);
	m_playersBox(roomType).SetWindowText(buffer);
}

static const gsi_char * ResultToString
(
	PEERJoinResult result
)
{
	switch(result)
	{
	case PEERJoinSuccess:
		return _T("Success");
	case PEERFullRoom:
		return _T("Full");
	case PEERInviteOnlyRoom:
		return _T("Invite Only");
	case PEERBannedFromRoom:
		return _T("Banned");
	case PEERBadPassword:
		return _T("Bad Password");
	case PEERAlreadyInRoom:
		return _T("Already in room");
	case PEERNoTitleSet:
		return _T("No Title");
	case PEERNoConnection:
		return _T("No Connection");
	case PEERAutoMatching:
		return _T("Auto Matching");
	case PEERJoinFailed:
		return _T("Join Failed");
	}

	ASSERT(0);
	return _T("<UNKNOWN RESULT>");
}

static void JoinRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	if(!success)
	{
		CString msg;
		msg.Format(_T("Join failure: %s"), ResultToString(result));
		dlg->MessageBox(msg);
	}
	else
	{
		dlg->FillPlayerList(roomType);
		if(roomType == StagingRoom)
		{
			dlg->m_name = peerGetRoomName(peer, StagingRoom);
			dlg->UpdateData(FALSE);
		}
	}

	GSI_UNUSED(param);
}

void CPeerTestDlg::OnButton2() 
{
	if(m_peer)
	{
		UpdateData();
		peerCreateStagingRoom(m_peer, m_name, 6, m_password, JoinRoomCallback, NULL, (PEERBool)m_blocking);
	}
}

void CPeerTestDlg::OnButton3() 
{
	if(m_peer)
	{
		UpdateData();
		SBServer server = GetCurrentServer();
		if(server)
		{
			if(SBServerGetIntValue(server, _T("password"), 0) && !m_password.GetLength())
				dlg->MessageBox(_T("This server requires a password"));
			peerJoinStagingRoom(m_peer, server, m_password, JoinRoomCallback, NULL, (PEERBool)m_blocking);
		}
	}
}

void CPeerTestDlg::OnButton17() 
{
	if(m_peer)
	{
		SBServer server = GetCurrentServer();
		if(server)
		{
			int groupID = ntohl(SBServerGetPublicInetAddress(server));
			if(groupID != 0)
				peerJoinGroupRoom(m_peer, groupID, JoinRoomCallback, NULL, (PEERBool)m_blocking);
		}
	}
}

void CPeerTestDlg::OnButton22() 
{
	if(m_peer)
	{
		peerJoinTitleRoom(m_peer, NULL, JoinRoomCallback, NULL, (PEERBool)m_blocking);
	}
}

void CPeerTestDlg::PostNcDestroy() 
{
	if(m_peer)
	{
		peerShutdown(m_peer);
		m_peer = NULL;
	}
	
	CDialog::PostNcDestroy();
}

void CPeerTestDlg::OnButton4() 
{
	if(m_peer)
	{
		UpdateData();
		m_ready = FALSE;
		UpdateData(FALSE);
		peerLeaveRoom(m_peer, StagingRoom, NULL);
		ClearPlayersList(m_players(StagingRoom));
	}
}

void CPeerTestDlg::OnButton18() 
{
	if(m_peer)
	{
		peerLeaveRoom(m_peer, GroupRoom, NULL);
		ClearPlayersList(m_players(GroupRoom));
	}
}

void CPeerTestDlg::OnButton23() 
{
	if(m_peer)
	{
		peerLeaveRoom(m_peer, TitleRoom, NULL);
		ClearPlayersList(m_players(TitleRoom));
	}
}

void CPeerTestDlg::OnButton5() 
{
	if(m_peer)
	{
		peerDisconnect(m_peer);
	}
}

void ListGroupRoomsCallback
(
	PEER peer,
	PEERBool success,
	int groupID,
	SBServer server,
	const gsi_char * name,
	int numWaiting,
	int maxWaiting,
	int numGames,
	int numPlaying,
	void * param
)
{
	if(!success)
	{
		dlg->MessageBox(_T("Failed to list the group rooms"));
		return;
	}

	if(groupID == 0)
	{
		gsi_char buffer[128];
		_stprintf(buffer, _T("Rooms: %d"), dlg->m_count);
		dlg->m_roomsBox.SetWindowText(buffer);
		return;
	}

	int nIndex = dlg->m_rooms.AddString(name);
	if(nIndex >= 0)
	{
		dlg->m_rooms.SetItemDataPtr(nIndex, server);
		dlg->m_count++;
		gsi_char buffer[32];
		_stprintf(buffer, _T("Rooms: %d"), dlg->m_count);
		dlg->m_roomsBox.SetWindowText(buffer);
	}

	GSI_UNUSED(param);
	GSI_UNUSED(numPlaying);
	GSI_UNUSED(numGames);
	GSI_UNUSED(maxWaiting);
	GSI_UNUSED(numWaiting);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnButton6() 
{
	if(m_peer)
	{
		UpdateData();

		ClearRoomsList(m_rooms);
		m_count = 0;
		peerListGroupRooms(m_peer, _T(""), ListGroupRoomsCallback, NULL, (PEERBool)m_blocking);
	}	
}

void CPeerTestDlg::OnButton8() 
{
	if(m_peer)
	{
		FillPlayerList(TitleRoom);
	}
}

void CPeerTestDlg::OnButton9() 
{
	if(m_peer)
	{
		FillPlayerList(GroupRoom);
	}
}

void CPeerTestDlg::OnButton10() 
{
	if(m_peer)
	{
		FillPlayerList(StagingRoom);
	}
}

void GetPlayerIPCallback
(
	PEER peer,
	PEERBool success,
	const gsi_char * nick,
	unsigned int IP,
	void * param
)
{
	gsi_char buffer[128];

	if(success)
	{
		IN_ADDR inAddr;
		inAddr.S_un.S_addr = IP;
		_stprintf(buffer, _T("%s's IP is %s"), nick, inet_ntoa(inAddr));
	}
	else
		_stprintf(buffer, _T("failed to get %s's IP"), nick);

	dlg->MessageBox(buffer);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnButton12() 
{
	if(m_peer)
	{
		gsi_char buffer[512];
		unsigned int IP;
		int profileID;

		if(!peerGetPlayerInfoNoWait(m_peer, m_selectedNick, &IP, &profileID))
			_stprintf(buffer, _T("Info Not Available"));
		else
		{
			IN_ADDR addr;
			addr.s_addr = IP;

			_stprintf(buffer, _T("IP = %s\npid = %d"), inet_ntoa(addr), profileID);

			CString strFlags;
			int nFlags;
			RoomType roomType;
			int i;
			for(i = 0 ; i < (int)NumRooms ; i++)
			{
				roomType = (RoomType)i;

				if(peerInRoom(m_peer, roomType))
				{
					if(peerGetPlayerFlags(m_peer, m_selectedNick, roomType, &nFlags))
					{
						if(roomType == TitleRoom)
							_tcscat(buffer,_T("\ntitle room flags = "));
						else if(roomType == GroupRoom)
							_tcscat(buffer,_T("\ngroup room flags = "));
						else if(roomType == StagingRoom)
							_tcscat(buffer,_T("\nstaging room flags = "));

						if(!nFlags)
							_tcscat(buffer,_T("N/A"));
						else
						{
							if(nFlags & PEER_FLAG_STAGING)
								_tcscat(buffer,_T("staging+"));
							if(nFlags & PEER_FLAG_READY)
								_tcscat(buffer,_T("ready+"));
							if(nFlags & PEER_FLAG_PLAYING)
								_tcscat(buffer,_T("playing+"));
							if(nFlags & PEER_FLAG_AWAY)
								_tcscat(buffer,_T("away+"));
							if(nFlags & PEER_FLAG_HOST)
								_tcscat(buffer, _T("host+"));
							if(nFlags & PEER_FLAG_OP)
								_tcscat(buffer,_T("op+"));
							if(nFlags & PEER_FLAG_VOICE)
								_tcscat(buffer,_T("voice+"));
							buffer[_tcslen(buffer) - 1] = '\0';
						}
					}
				}
			}
		}

		CString caption;
		caption.Format(_T("Info for %s"), m_selectedNick);
		MessageBox(buffer, caption);
	}
}

void CPeerTestDlg::OnButton14() 
{
	if(m_peer)
	{
		UpdateData();

		PEERBool ready;
		if(peerGetReady(m_peer, m_selectedNick, &ready))
		{
			if(ready)
				MessageBox(_T("Ready"));
			else
				MessageBox(_T("Not Ready"));
		}
		else
		{
			MessageBox(_T("Ready failed"));
		}
	}
}

void CPeerTestDlg::OnCheck1() 
{
	if(m_peer)
	{
		UpdateData();

		peerSetReady(m_peer, (PEERBool)m_ready);

		if(m_ready)
			m_readyCtl.SetWindowText(_T("Ready"));
		else
			m_readyCtl.SetWindowText(_T("Not Ready"));
	}
}

void CPeerTestDlg::OnButton15() 
{
	if(m_peer)
	{
		UpdateData();
		peerStartGame(m_peer, NULL, PEER_KEEP_REPORTING);
	}
}

void CPeerTestDlg::OnButton13() 
{
	if(m_peer)
	{
		UpdateData();
		peerMessageRoom(m_peer, StagingRoom, m_message, NormalMessage);
		m_message = _T("");
		UpdateData(FALSE);
	}
}

void CPeerTestDlg::OnButton20() 
{
	if(m_peer)
	{
		UpdateData();
		peerMessageRoom(m_peer, GroupRoom, m_message, NormalMessage);
		m_message = _T("");
		UpdateData(FALSE);
	}
}

void CPeerTestDlg::OnButton25() 
{
	if(m_peer)
	{
		UpdateData();
		peerMessageRoom(m_peer, TitleRoom, m_message, NormalMessage);
		m_message = _T("");
		UpdateData(FALSE);
	}
}

void CPeerTestDlg::OnButton26() 
{
	if(m_peer)
	{
		const gsi_char * name = peerGetRoomName(m_peer, TitleRoom);
		const gsi_char * channel = peerGetRoomChannel(m_peer, TitleRoom);
		if(!name || !channel)
			dlg->MessageBox(_T("Failed to get room info."));
		else
		{
			gsi_char buffer[256];
			_stprintf(buffer, _T("Name: %s\nChannel: %s\nUsers: %d"), name, channel, chatGetChannelNumUsers(peerGetChat(m_peer), channel));
			dlg->MessageBox(buffer);
		}
	}
}

void CPeerTestDlg::OnButton27() 
{
	if(m_peer)
	{
		const gsi_char * name = peerGetRoomName(m_peer, GroupRoom);
		const gsi_char * channel = peerGetRoomChannel(m_peer, GroupRoom);
		if(!name)
			dlg->MessageBox(_T("Failed to get room info."));
		else
		{
			gsi_char buffer[256];
			_stprintf(buffer, _T("Name: %s\nChannel: %s\nUsers: %d"), name, channel, chatGetChannelNumUsers(peerGetChat(m_peer), channel));
			dlg->MessageBox(buffer);
		}
	}
}

void CPeerTestDlg::OnButton28() 
{
	if(m_peer)
	{
		const gsi_char * name = peerGetRoomName(m_peer, StagingRoom);
		const gsi_char * channel = peerGetRoomChannel(m_peer, StagingRoom);
		if(!name)
			dlg->MessageBox(_T("Failed to get room info."));
		else
		{
			gsi_char buffer[256];
			_stprintf(buffer, _T("Name: %s\nChannel: %s\nUsers: %d"), name, channel, chatGetChannelNumUsers(peerGetChat(m_peer), channel));
			dlg->MessageBox(buffer);
		}
	}
}

BOOL CPeerTestDlg::DestroyWindow() 
{
	ClearRoomsList(m_rooms);

	// Save some info.
	//////////////////
	FILE * file;
	file = _tfopen(_T("peertest.cfg"), _T("wt"));
	if(file)
	{
		_ftprintf(file, _T("%s\n%s\n%s\n%s\n%s\n%s\n"),
			m_title, m_secretKey, m_nick, m_email, m_loginPassword, m_namespace);
		fclose(file);
	}
	
	return CDialog::DestroyWindow();
}


void CPeerTestDlg::OnButton16() 
{
	if(m_peer)
	{
		int ping;
		gsi_char buffer[256];

		if(peerGetPlayerPing(m_peer, m_selectedNick, &ping))
			_stprintf(buffer, _T("%s has a %dms ping"), m_selectedNick, ping);
		else
			_stprintf(buffer, _T("Failed to get a ping for %s"), m_selectedNick);

		MessageBox(buffer);
	}
}

void OnSelchangePlayers(RoomType roomType)
{
	dlg->UpdateData();
	dlg->m_selectedRoom = roomType;
	int cur = m_players(roomType).GetCurSel();
	if(cur == -1)
		return;
	m_players(roomType).GetText(cur, dlg->m_selectedNick);
	if(_tcscmp(dlg->m_selectedNick.Left(6), _T("HOST: ")) == 0)
		dlg->m_selectedNick = dlg->m_selectedNick.Mid(6);
	dlg->m_player = dlg->m_selectedNick;
	dlg->UpdateData(FALSE);
}

void CPeerTestDlg::OnSelchangeTitlePlayers() 
{
	if(m_peer)
	{
		OnSelchangePlayers(TitleRoom);
	}
}

void CPeerTestDlg::OnSelchangeGroupPlayers() 
{
	if(m_peer)
	{
		OnSelchangePlayers(GroupRoom);
	}
}

void CPeerTestDlg::OnSelchangeStagingPlayers() 
{
	if(m_peer)
	{
		OnSelchangePlayers(StagingRoom);
	}
}

void ListingGamesCallback
(
	PEER peer,
	PEERBool success,
	const gsi_char * name,
	SBServer server,
	PEERBool staging,
	int msg,
	int progress,
	void * param
)
{
	int nIndex;

	if(!success)
	{
		dlg->MessageBox(_T("ListingGames failed"));
		return;
	}

	dlg->m_progress.SetPos(progress);

	if(msg == PEER_CLEAR)
	{
		ClearRoomsList(dlg->m_rooms);
		return;
	}

	// If updating or removing, find then remove it.
	////////////////////////////////////////////////
	if((msg == PEER_UPDATE) || (msg == PEER_REMOVE))
	{
		int count = dlg->m_rooms.GetCount();
		for(nIndex = 0 ; nIndex < count ; nIndex++)
		{
			if((SBServer)dlg->m_rooms.GetItemDataPtr(nIndex) == server)
			{
				dlg->m_rooms.DeleteString(nIndex);
				break;
			}
		}
		ASSERT(nIndex < count);
	}

	if((msg == PEER_ADD) || (msg == PEER_UPDATE))
	{
		CString str;
		if(staging)
			str = _T("Staging: ");
		else
			str = _T("Playing: ");
		str += name;

		// Add it.
		//////////
		nIndex = dlg->m_rooms.AddString(str);
		if(nIndex >= 0)
			dlg->m_rooms.SetItemDataPtr(nIndex, server);
	}

	gsi_char buffer[32];
	_stprintf(buffer, _T("Rooms: %d"), dlg->m_rooms.GetCount());
	dlg->m_roomsBox.SetWindowText(buffer);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnButton29() 
{
	if(m_peer)
	{
		UpdateData();

		m_roomsBox.SetWindowText(_T("Rooms:"));
		ClearRoomsList(m_rooms);
		m_progress.SetPos(0);

		const unsigned char keys[] =
		{
			NUMPLAYERS_KEY,
			MAXPLAYERS_KEY,
			HOSTPORT_KEY,
			MAPNAME_KEY,
			PASSWORD_KEY
		};
		peerStartListingGames(m_peer, keys, sizeof(keys), m_filter, ListingGamesCallback, NULL);
	}
}

void CPeerTestDlg::OnButton30() 
{
	if(m_peer)
	{
		peerStopListingGames(m_peer);
	}
}

void CPeerTestDlg::OnButton24() 
{
	if(m_peer)
	{
		UpdateData();

		PEERBool pingRooms[NumRooms];
		PEERBool crossPingRooms[NumRooms];
		pingRooms[TitleRoom] = PEERTrue;
		pingRooms[GroupRoom] = PEERTrue;
		pingRooms[StagingRoom] = PEERTrue;
		crossPingRooms[TitleRoom] = PEERFalse;
		crossPingRooms[GroupRoom] = PEERFalse;
		crossPingRooms[StagingRoom] = PEERTrue;

		PEERBool result = peerSetTitle(m_peer,
		m_title,
		m_secretKey,
		m_title,
		m_secretKey,
		0,
		30,
		PEERTrue,
		pingRooms,
		crossPingRooms);

		if(!result)
			MessageBox(_T("Failed to set the title"));
	}
}

void CPeerTestDlg::OnButton31() 
{
	if(m_peer)
	{
		peerStopGame(m_peer);
	}
}

void CPeerTestDlg::OnButton32() 
{
	if(m_peer)
	{
		peerStartReporting(m_peer);
	}
}

void CPeerTestDlg::OnButton33() 
{
	if(m_peer)
	{
		peerStateChanged(m_peer);
	}
}

void CPeerTestDlg::OnButton34() 
{
	if(m_peer)
	{
		UpdateData();
		peerSetPassword(m_peer, StagingRoom, m_password);
	}
}

void CPeerTestDlg::OnButton35() 
{
	if(m_peer)
	{
		UpdateData();
		peerSetRoomName(m_peer, StagingRoom, m_name);
	}
}

BOOL CPeerTestDlg::PreTranslateMessage(MSG* pMsg) 
{
	// Check for enter.
	///////////////////
	if(m_peer)
	{
		if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == 0x0d))
		{
			CWnd * focus;
			focus = GetFocus();
			if(focus->m_hWnd == GetDlgItem(IDC_MESSAGE)->m_hWnd)
			{
				UpdateData();
				if(m_message)
				{
					int i;
					for(i = 0 ; i < NumRooms ; i++)
						if(peerInRoom(m_peer, (RoomType)i))
							peerMessageRoom(m_peer, (RoomType)i, m_message, NormalMessage);

					m_message = _T("");
					UpdateData(FALSE);
				}

				return TRUE;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

static void ChangeNickCallback
(
	PEER peer,
	PEERBool success,
	const gsi_char * oldNick,
	const gsi_char * newNick,
	void * param
)
{
	gsi_char buffer[128];

	if(success)
		_stprintf(buffer, _T("Nick changed from %s to %s\n"), oldNick, newNick);
	else
		_stprintf(buffer, _T("Failed to change nick from %s to %s\n"), oldNick, newNick);

	dlg->MessageBox(buffer);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnChangeNick() 
{
	if(m_peer)
	{
		UpdateData();
		if(m_nick)
		{
			peerChangeNick(m_peer, m_nick, ChangeNickCallback, NULL, (PEERBool)m_blocking);
		}
	}
}

void CPeerTestDlg::OnQuiet() 
{
	if(m_peer)
	{
		UpdateData();
		peerSetQuietMode(m_peer, (PEERBool)m_quiet);
	}
}

void CPeerTestDlg::OnButton37() 
{
	const gsi_char * key;
	const gsi_char * value;
	if(m_peer)
	{
		UpdateData();
		if(m_key.IsEmpty())
			return;

		key = m_key;
		value = m_value;

		if(m_keyType == 0)
			peerSetGlobalKeys(m_peer, 1, &key, &value);
		else
			peerSetRoomKeys(m_peer, (RoomType)m_keyRoom, m_player, 1, &key, &value);
	}
}

static void GetGlobalKeysCallback
(
	PEER peer,
	PEERBool success,
	const gsi_char * nick,
	int num,
	const gsi_char ** keys,
	const gsi_char ** values,
	void * param
)
{
	int i;
	if(success && nick)
	{
		OutputDebugString(nick);
		OutputDebugString(_T("[Global]: "));
		for(i = 0 ; i < num ; i++)
		{
			OutputDebugString(_T("\\"));
			OutputDebugString(keys[i]);
			OutputDebugString(_T("\\"));
			OutputDebugString(values[i]);
		}
		OutputDebugString(_T("\n"));
	}

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

static void GetRoomKeysCallback
(
	PEER peer,
	PEERBool success,
	RoomType roomType,
	const gsi_char * nick,
	int num,
	gsi_char ** keys,
	gsi_char ** values,
	void * param
)
{
	int i;
	if(success)
	{
		if(nick)
			OutputDebugString(nick);
		if(roomType == TitleRoom)
			OutputDebugString(_T("[Title]: "));
		else if(roomType == GroupRoom)
			OutputDebugString(_T("[Group]: "));
		else
			OutputDebugString(_T("[Staging]: "));
		for(i = 0 ; i < num ; i++)
		{
			OutputDebugString(_T("\\"));
			OutputDebugString(keys[i]);
			OutputDebugString(_T("\\"));
			if(values)
				OutputDebugString(values[i]);
		}
		OutputDebugString(_T("\n"));
	}

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnButton38() 
{
	const gsi_char * key;
	if(m_peer)
	{
		UpdateData();

		key = m_key;

		if(m_keyType == 0)
		{
			if(m_key.IsEmpty())
				return;

			if(m_player.IsEmpty())
				peerGetRoomGlobalKeys(m_peer, (RoomType)m_keyRoom, 1, &key, GetGlobalKeysCallback, NULL, (PEERBool)m_blocking);
			else
				peerGetPlayerGlobalKeys(m_peer, m_player, 1, &key, GetGlobalKeysCallback, NULL, (PEERBool)m_blocking);
		}
		else
		{
			peerGetRoomKeys(m_peer, (RoomType)m_keyRoom, m_player, key[0]?1:0, &key, GetRoomKeysCallback, NULL, (PEERBool)m_blocking);
		}
	}
}

void OnSetfocusRoom(RoomType roomType)
{
	dlg->UpdateData();
	dlg->m_keyRoom = roomType;
	dlg->UpdateData(FALSE);
}

void CPeerTestDlg::OnSetfocusTitlePlayers() 
{
	if(m_peer)
	{
		OnSetfocusRoom(TitleRoom);
	}
}

void CPeerTestDlg::OnSetfocusGroupPlayers() 
{
	if(m_peer)
	{
		OnSetfocusRoom(GroupRoom);
	}
}

void CPeerTestDlg::OnSetfocusStagingPlayers() 
{
	if(m_peer)
	{
		OnSetfocusRoom(StagingRoom);
	}
}

void CPeerTestDlg::OnAway() 
{
	if(m_peer)
	{
		UpdateData();

		if(m_away)
			peerSetAwayMode(m_peer, m_awayReason);
		else
			peerSetAwayMode(m_peer, NULL);
	}
}

void CPeerTestDlg::OnButton39() 
{
	if(m_peer)
	{
		UpdateData();
		chatSendRaw(peerGetChat(m_peer), m_raw);
	}
}

void CPeerTestDlg::OnButton40() 
{
	if(m_peer)
	{
		peerStayInRoom(m_peer, TitleRoom);
		MessageBox(_T("Will stay in the title room when the title is cleared/changed"));
	}
}

void CPeerTestDlg::OnFixNick() 
{
	gsi_char newNick[128];

	UpdateData();

	chatFixNick(newNick, m_nick);

	m_nick = newNick;

	UpdateData(FALSE);
}

void CPeerTestDlg::OnButton11() 
{
	m_pingPlayers.ResetContent();
}

void CPeerTestDlg::OnButton42() 
{
	if(m_peer)
	{
		peerStartPlaying(m_peer);
	}
}

void CPeerTestDlg::OnButton43() 
{
	if(m_peer)
	{
		CString message;

		if(peerIsPlaying(m_peer))
			message = _T("Yes");
		else
			message = _T("No");

		MessageBox(message);
	}
}

void ServerEnumKeysCallback(gsi_char * key, gsi_char * value, void * instance)
{
	CString * keyValues = (CString *)instance;
	*keyValues += key;
	*keyValues += _T(" = ");
	*keyValues += value;
	*keyValues += '\n';
}

void CPeerTestDlg::OnButton44() 
{
	if(m_peer)
	{
		UpdateData();
		SBServer server = GetCurrentServer();
		if(server)
		{
			CString keyValues;
			CString info;

			// get the flags
			int flags = SBServerGetFlags(server);

			// get the state
			int state = SBServerGetState(server);

			// add the public address
			info.Format(_T("public address = %s:%d\n"), SBServerGetPublicAddress(server), SBServerGetPublicQueryPort(server));
			keyValues += info;

			// add the private address
			if(flags & PRIVATE_IP_FLAG)
			{
				info.Format(_T("private address = %s:%d\n"), SBServerGetPrivateAddress(server), SBServerGetPrivateQueryPort(server));
				keyValues += info;
			}

			// add the ping
			info.Format(_T("ping = %d\n"), SBServerGetPing(server));
			keyValues += info;

			// add the flags
			info = _T("flags = ");
			if(flags & UNSOLICITED_UDP_FLAG)
				info += _T("UNSOLICITED_UDP_FLAG|");
			if(flags & PRIVATE_IP_FLAG)
				info += _T("PRIVATE_IP_FLAG|");
			if(flags & CONNECT_NEGOTIATE_FLAG)
				info += _T("CONNECT_NEGOTIATE_FLAG|");
			if(flags & ICMP_IP_FLAG)
				info += _T("ICMP_IP_FLAG|");
			if(flags & NONSTANDARD_PORT_FLAG)
				info += _T("NONSTANDARD_PORT_FLAG|");
			if(flags & NONSTANDARD_PRIVATE_PORT_FLAG)
				info += _T("NONSTANDARD_PRIVATE_PORT_FLAG|");
			if(flags & HAS_KEYS_FLAG)
				info += _T("HAS_KEYS_FLAG|");
			if(flags & HAS_FULL_RULES_FLAG)
				info += _T("HAS_FULL_RULES_FLAG|");
			info.SetAt(info.GetLength() - 1, '\n');
			keyValues += info;

			// add the state
			info = _T("state = ");
			if(state & STATE_BASICKEYS)
				info += _T("STATE_BASICKEYS|");
			if(state & STATE_FULLKEYS)
				info += _T("STATE_FULLKEYS|");
			if(state & STATE_PENDINGBASICQUERY)
				info += _T("STATE_PENDINGBASICQUERY|");
			if(state & STATE_PENDINGFULLQUERY)
				info += _T("STATE_PENDINGFULLQUERY|");
			if(state & STATE_QUERYFAILED)
				info += _T("STATE_QUERYFAILED|");
			info.SetAt(info.GetLength() - 1, '\n');
			keyValues += info;

			SBServerEnumKeys(server, ServerEnumKeysCallback, &keyValues);
			MessageBox(keyValues);
		}
	}
}

void CPeerTestDlg::OnButton45() 
{
	UpdateData();

	m_title = _T("gmtest");
	m_secretKey = _T("HA6zkS");

	UpdateData(FALSE);
}

void AuthenticateCDKeyCallback
(
	PEER peer,
	int result,
	const gsi_char * message,
	void * param
)
{
	CString str;
	str.Format(_T("CD Key Result: %s (%d)"), message, result);
	dlg->MessageBox(str);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnButton46() 
{
	if(m_peer)
	{
		UpdateData();
		peerAuthenticateCDKey(m_peer, m_cdKey, AuthenticateCDKeyCallback, NULL, (PEERBool)m_blocking);
	}
}

void CPeerTestDlg::OnDblclkRooms() 
{
	if(m_peer)
	{
		UpdateData();
		SBServer server = GetCurrentServer();
		if(server)
		{
		}
	}
}

SBServer CPeerTestDlg::GetCurrentServer()
{
	int index = m_rooms.GetCurSel();
	if(index == -1)
		return NULL;
	return (SBServer)m_rooms.GetItemDataPtr(index);
}

void CPeerTestDlg::OnButton47() 
{
	if(m_peer)
	{
		peerKickPlayer(m_peer, m_selectedRoom, m_selectedNick, _T("You've been kicked"));
	}
}

void CPeerTestDlg::OnButton48() 
{
	if(m_peer)
	{
		UpdateData();
		SBServer server = GetCurrentServer();
		if(server)
		{
			peerUpdateGame(m_peer, server, PEERTrue);
		}
	}
}

void AutoMatchStatusCallback
(
	PEER peer,
	PEERAutoMatchStatus status,
	void * param
)
{
	const gsi_char * StatusNames[] = 
	{
		_T("Failed"),
		_T("Searching"),
		_T("Waiting"),
		_T("Staging"),
		_T("Ready"),
		_T("Complete")
	};

	CString str;
	str.Format(_T("AM Status: %-10s\n"), StatusNames[status]);
	OutputDebugString(str);

	dlg->UpdateData();

	if(peerInRoom(peer, StagingRoom))
		dlg->FillPlayerList(StagingRoom);
	else
		ClearPlayersList(m_players(StagingRoom));

	switch(status)
	{
	case PEERFailed:
		dlg->MessageBox(_T("The AutoMatch attempt has failed"));
		dlg->m_autoMatchStatus = _T("");
		break;

	case PEERSearching:
	case PEERWaiting:
	case PEERStaging:
	case PEERReady:
	case PEERComplete:
		dlg->m_autoMatchStatus = StatusNames[status];
		break;
	}

	dlg->UpdateData(FALSE);

	GSI_UNUSED(param);
}

int AutoMatchRateCallback
(
	PEER peer,
	SBServer match,
	void * param
)
{
	dlg->UpdateData();

	// we only want exact maxplayers matches
	if(SBServerGetIntValue(match, _T("maxplayers"), 0) != dlg->m_maxPlayers)
		return 0;

	// this match is acceptable
	return 1;

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
}

void CPeerTestDlg::OnStartAutoMatch() 
{
	if(m_peer)
	{
		UpdateData();
		if(m_maxPlayers < 2)
		{
			MessageBox(_T("Max Players must be at least 2"));
			return;
		}

		OutputDebugString(_T("AM Starting\n"));

		peerStartAutoMatch(m_peer, m_maxPlayers, _T(""), AutoMatchStatusCallback, AutoMatchRateCallback, NULL, (PEERBool)m_blocking);
	}
}

void CPeerTestDlg::OnStopAutoMatch() 
{
	if(m_peer)
	{
		peerStopAutoMatch(m_peer);

		OutputDebugString(_T("AM Stopped\n"));

		UpdateData();
		m_autoMatchStatus = _T("");
		UpdateData(FALSE);
	}
}
