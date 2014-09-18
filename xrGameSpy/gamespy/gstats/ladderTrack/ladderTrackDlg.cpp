// ladderTrackDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ladderTrack.h"
#include "ladderTrackDlg.h"
#include "waitingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLadderTrackDlg dialog

CLadderTrackDlg::CLadderTrackDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLadderTrackDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLadderTrackDlg)
	m_localPosition = _T("");
	m_remotePosition = _T("");
	m_info = _T("Ready");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLadderTrackDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLadderTrackDlg)
	DDX_Control(pDX, IDC_START_RACE, m_startRace);
	DDX_Control(pDX, IDC_REMOTE_PROGRESS, m_remoteProgress);
	DDX_Control(pDX, IDC_LOCAL_PROGRESS, m_localProgress);
	DDX_Text(pDX, IDC_LOCAL_POSITION, m_localPosition);
	DDX_Text(pDX, IDC_REMOTE_POSITION, m_remotePosition);
	DDX_Text(pDX, IDC_INFO, m_info);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLadderTrackDlg, CDialog)
	//{{AFX_MSG_MAP(CLadderTrackDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START_RACE, OnStart)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_LOGOUT, OnLogout)
	ON_BN_CLICKED(IDC_UPDATE_POSITIONS, OnUpdatePositions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLadderTrackDlg message handlers

#define MSG_CONNECT              "is" // client-pid, nick
#define MSG_CHALLENGE            "is" // host-pid, challenge
#define MSG_CHALLENGE_TYPE       1
#define MSG_RESPONSE             "s"  // response
#define MSG_RESPONSE_TYPE        2
#define MSG_COUNTDOWN            "i"  // count
#define MSG_COUNTDOWN_TYPE       3
#define MSG_START_RACE           ""
#define MSG_START_RACE_TYPE      5
#define MSG_PROGRESS             "i"  // progress
#define MSG_PROGRESS_TYPE        6
#define MSG_END_RACE             "i"  // time
#define MSG_END_RACE_TYPE        7
#define MSG_CHAT                 "s"  // message
#define MSG_CHAT_TYPE            8

//#define HOST_PORT                38466
#define HOST_PORT_STRING         ":38466"
#define CLIENT_PORT_STRING       ":38467" // so you can run both
#define COUNTDOWN_START          5

#define TIMER_THINK              100
#define TIMER_COUNTDOWN          101

CLadderTrackDlg * Dlg;

void ConnectedCallback
(
	GT2Connection connection,
	GT2Result result,
	GT2Byte * message,
	int len
)
{
	if(result == GT2Success)
		Dlg->m_state = JOIN_WAITING;
	else
	{
		Dlg->m_state = JOIN_ERROR;
		Dlg->m_GT2Connection = NULL;
	}
}

void ReceivedCallback
(
	GT2Connection connection,
	GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	if(!message || !len)
		return;

	GTMessageType type;

	type = gtEncodedMessageType((char*)message);

	if(type == MSG_CHALLENGE_TYPE)
	{
		ASSERT(!Dlg->m_hosting && (Dlg->m_state == JOIN_WAITING));

		int pid;
		char challenge[64];
		char response[33];
		if(gtDecode(MSG_CHALLENGE, (char*)message, len, &pid, challenge) == -1)
		{
			Dlg->m_state = JOIN_ERROR;
			return;
		}

		GenerateAuth(challenge, (char *)(LPCSTR)Dlg->m_loginDlg.m_password, response);

		// Send the response.
		/////////////////////
		char message[64];
		int rcode;
		rcode = gtEncode(MSG_RESPONSE_TYPE, MSG_RESPONSE, message, sizeof(message), response);
		ASSERT(rcode != -1);
		gt2Send(connection, (const GT2Byte *)message, rcode, GT2True);

		Dlg->m_remoteProfile = pid;
		Dlg->m_state = JOIN_CONNECTED;
	}
	else if(type == MSG_RESPONSE_TYPE)
	{
		ASSERT(Dlg->m_hosting && (Dlg->m_state == HOST_CHALLENGING));

		char response[33];
		if(gtDecode(MSG_RESPONSE, (char*)message, len, response) == -1)
		{
			Dlg->m_state = HOST_ERROR;
			return;
		}
		Dlg->m_remoteResponse = response;
		Dlg->m_state = HOST_CONNECTED;
	}
	else if(type == MSG_COUNTDOWN_TYPE)
	{
		ASSERT(!Dlg->m_hosting);

		if(gtDecode(MSG_COUNTDOWN, (char*)message, len, &Dlg->m_countdown) == -1)
		{
			Dlg->m_state = JOIN_ERROR;
			return;
		}

		Dlg->Countdown();
	}
	else if(type == MSG_START_RACE_TYPE)
	{
		ASSERT(!Dlg->m_hosting);

		Dlg->StartRace();
	}
	else if(type == MSG_PROGRESS_TYPE)
	{
		if(Dlg->m_racing)
		{
			int progress;
			if(gtDecode(MSG_PROGRESS, (char*)message, len, &progress) != -1)
				Dlg->m_remoteProgress.SetPos(progress);
		}
	}
	else if(type == MSG_END_RACE_TYPE)
	{
		if(Dlg->m_racing)
		{
			gtDecode(MSG_END_RACE, (char*)message, len, &Dlg->m_remoteTime);
		}
	}
}

void ClosedCallback
(
	GT2Connection connection,
	GT2CloseReason reason
)
{
	// Logout triggers this function when it's a local close
	// so we need to make sure we don't loop
	if (reason != GT2LocalClose)
	{
		Dlg->MessageBox("Connection closed");
		Dlg->Logout();
	}
	else
		Dlg->m_GT2Connection = NULL;
}

void ConnectAttemptCallback
(
	GT2Socket listener,
	GT2Connection connection,
	unsigned int ip,
	unsigned short port,
	int latency,
	GT2Byte * message,
	int len
)
{
	int pid = 0;
	char nick[128];

	// Only allow one connection.
	/////////////////////////////
	if(Dlg->m_GT2Connection)
	{
		gt2Reject(connection, NULL, 0);
		return;
	}

	// Decode the pid.
	//////////////////
	if(message && len)
	{
		if(gtDecodeNoType(MSG_CONNECT, (char*)message, len, &pid, nick) == -1)
			pid = 0;
	}

	// If we didn't/couldn't get the pid, reject the attempt.
	/////////////////////////////////////////////////////////
	if(!pid)
	{
		gt2Reject(connection, NULL, 0);
		return;
	}

	// Accept the connection.
	/////////////////////////
	GT2ConnectionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(GT2ConnectionCallbacks));
	callbacks.received = ReceivedCallback;
	callbacks.closed = ClosedCallback;
	if(!gt2Accept(connection, &callbacks))
		return;

	// Set some states.
	///////////////////
	Dlg->m_remoteProfile = pid;
	Dlg->m_GT2Connection = connection;
	Dlg->m_state = HOST_CHALLENGING;
	Dlg->m_challenged = FALSE;
	Dlg->m_remoteNick = nick;
}

/*void StoppedCallback
(
	GT2Socket listener,
	GTStopReason reason
)
{
	if(reason != GTStopped)
	{
		Dlg->MessageBox("Listener stopped");
		Dlg->Logout();
	}
}*/

void SocketErrorCallback
(
	GT2Socket socket
)
{
	Dlg->MessageBox("Socket Error!");
	Dlg->Logout();
}


BOOL CLadderTrackDlg::SetupHosting()
{
	int rcode;
	CString str;

	if(!IsStatsConnected())
	{
		// Use the development system.
		//////////////////////////////
		strcpy(StatsServerHostname, "sdkdev.gamespy.com");

		// Set the gamename and secret key.
		///////////////////////////////////
		strcpy(gcd_gamename, "st_ladder");
		gcd_secret_key[0] = 'K';
		gcd_secret_key[1] = 'w';
		gcd_secret_key[2] = 'F';
		gcd_secret_key[3] = 'J';
		gcd_secret_key[4] = '2';
		gcd_secret_key[5] = 'X';

		// Init the connection to the backend.
		//////////////////////////////////////
		rcode = InitStatsConnection(0);
		if(rcode != GE_NOERROR)
		{
			str.Format("Failed to connect to the stats server (%d).", rcode);
			MessageBox(str);
			PostQuitMessage(1);
			return TRUE;
		}

		// Get the challenge.
		/////////////////////
		m_challenge = GetChallenge(NULL);

		//FakeStats();
	}

	/*
	// Setup the listener's callbacks.
	//////////////////////////////////
	GTListenerCallbacks callbacks;
	memset(&callbacks, 0, sizeof(GTListenerCallbacks));
	callbacks.connectAttempt = ConnectAttemptCallback;
	callbacks.stopped = StoppedCallback;

	// Create the listener.
	///////////////////////

	m_listener = gtListen(gtAddressToString(0, HOST_PORT, NULL), &callbacks);
	if(!m_listener)
		return FALSE;*/

	GT2ConnectionCallbacks connectionCallbacks;
	memset(&connectionCallbacks, 0, sizeof(GT2ConnectionCallbacks));
	connectionCallbacks.received = ReceivedCallback;
	connectionCallbacks.closed = ClosedCallback;

	GT2Result aResult = gt2CreateSocket(&m_GT2Socket, HOST_PORT_STRING, 0, 0, SocketErrorCallback);
	if (GT2Success != aResult)
		return FALSE;

	gt2Listen(m_GT2Socket, ConnectAttemptCallback);
	m_state = HOST_LISTENING;

	// Bring up the "waiting" dialog.
	/////////////////////////////////
	rcode = m_waitingDlg.DoModal();

	// If it was cancelled, try again.
	//////////////////////////////////
	if(rcode != IDOK)
		Logout();

	return TRUE;
}

BOOL CLadderTrackDlg::SetupJoining()
{
	int rcode;

	// Setup the address to connect to.
	///////////////////////////////////
	CString remoteAddress;
	remoteAddress.Format("%s%s", m_hostOrJoinDlg.m_joinAddress, HOST_PORT_STRING);

	// Encode the profile id.
	/////////////////////////
	char buffer[64];
	rcode = gtEncodeNoType(MSG_CONNECT, buffer, sizeof(buffer), m_loginDlg.m_profile, m_loginDlg.m_nick);
	ASSERT(rcode != -1);

	// Setup the callbacks.
	///////////////////////
	GT2ConnectionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(GT2ConnectionCallbacks));
	callbacks.connected = ConnectedCallback;
	callbacks.received = ReceivedCallback;
	callbacks.closed = ClosedCallback;

	// Create the socket
	GT2Result aResult = gt2CreateSocket(&m_GT2Socket, CLIENT_PORT_STRING, 0, 0, SocketErrorCallback);
	if (aResult != GT2Success)
	{
		MessageBox("Failed to create socket!");
		return FALSE;
	}

	// Connect.
	///////////
	m_state = JOIN_CONNECTING;
	GT2Result result;
	result = gt2Connect(m_GT2Socket, &m_GT2Connection, remoteAddress, (const GT2Byte *)buffer, sizeof(buffer), -1, &callbacks, GT2False);
	if(!m_GT2Connection)
		return FALSE;

	// Bring up the "waiting" dialog.
	/////////////////////////////////
	rcode = m_waitingDlg.DoModal();

	// If it was cancelled, try again.
	//////////////////////////////////
	if(rcode != IDOK)
		Logout();

	return TRUE;
}

GHTTPBool PlayerPositionPageCompleted
(
	GHTTPRequest request,
	GHTTPResult result,
	char * buffer,
	__int64 bufferLen,
	void * param
)
{
	if(result == GHTTPSuccess)
	{
		BOOL localPlayer;
		int position;

		Dlg->UpdateData();

		localPlayer = (BOOL)param;
		position = atoi(buffer);

		if(localPlayer)
			Dlg->m_localPosition.Format("%d", position);
		else
			Dlg->m_remotePosition.Format("%d", position);

		Dlg->UpdateData(FALSE);
	}

	return GHTTPTrue;
}

void CLadderTrackDlg::UpdatePlayerPositions()
{
	CString url;
	url.Format("http://sdkdev.gamespy.com/games/st_ladder/web/playerposition.asp?pid=%d", m_loginDlg.m_profile);
	ghttpGet(url, GHTTPFalse, PlayerPositionPageCompleted, (void *)TRUE);
	url.Format("http://sdkdev.gamespy.com/games/st_ladder/web/playerposition.asp?pid=%d", m_remoteProfile);
	ghttpGet(url, GHTTPFalse, PlayerPositionPageCompleted, (void *)FALSE);
}

BOOL CLadderTrackDlg::SetupMatch()
{
	int rcode;
	BOOL result;

	m_state = SETTING_UP;

	ASSERT(!m_GT2Connection);
	ASSERT(!m_GT2Socket);

	m_state = 0;
	m_remoteResponse.Empty();
	m_remoteProfile = 0;
	m_GT2Connection = NULL;
	m_startRace.EnableWindow(FALSE);
	m_start = 0;
	m_countdown = 0;
	m_racing = FALSE;
	m_numSteps = 0;
	m_step = NONE;

	do
	{
		// Login the user (actually just verifying the login).
		//////////////////////////////////////////////////////
		rcode = m_loginDlg.DoModal();
		if(rcode != IDOK)
		{
			PostQuitMessage(1);
			return FALSE;
		}

		// See if they want to host or join.
		////////////////////////////////////
		rcode = m_hostOrJoinDlg.DoModal();
	}
	while(rcode != IDOK);

	m_GT2Connection = NULL;
	m_hosting = (m_hostOrJoinDlg.m_hostOrJoin == HOSTORJOIN_HOST);

	CString str;
	str.Format("ladderTrack%s", m_hosting?" (hosting)":"");
	SetWindowText(str);

	if(m_hosting)
		result = SetupHosting();
	else
		result = SetupJoining();

	if(result && m_hosting)
	{
		m_startRace.EnableWindow();
	}

	UpdatePlayerPositions();

	return result;
}

BOOL CLadderTrackDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// Basic initialization.
	////////////////////////
	Dlg = this;
	m_GT2Connection = NULL;
	m_GT2Socket		= NULL;
	m_state			= LOGGED_OUT;
	m_countdown		= 0;
	m_racing		= FALSE;

	// Init gt.
	///////////
	//gtStartup();

	// Set a think timer.
	/////////////////////
	SetTimer(TIMER_THINK, 50, NULL);
	
	return TRUE;
}

void CLadderTrackDlg::OnPaint() 
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

HCURSOR CLadderTrackDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CLadderTrackDlg::OnLogout() 
{
	// Clean stuff up.
	//////////////////
	if(m_GT2Connection)
	{
		gt2CloseConnection(m_GT2Connection);
		m_GT2Connection = NULL;
	}
	if(m_GT2Socket)
	{
		gt2CloseSocket(m_GT2Socket);
		m_GT2Socket = NULL;
	}
	if(m_waitingDlg.m_hWnd && m_waitingDlg.IsWindowEnabled())
	{
		m_waitingDlg.EndDialog(IDCANCEL);
	}

	m_state = LOGGED_OUT;
}

void CLadderTrackDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	if(IsStatsConnected())
		CloseStatsConnection();

	ghttpCleanup();

	//gt2Cleanup(GTFalse);
	if (m_GT2Connection)
	{
		gt2CloseConnection(m_GT2Connection);
		m_GT2Connection = NULL;
	}
	if (m_GT2Socket)
	{
		gt2CloseSocket(m_GT2Socket);
		m_GT2Socket = NULL;
	}
}

void CLadderTrackDlg::OnTimer(UINT nIDEvent) 
{
	char buffer[64];
	int rcode;

	if(nIDEvent == TIMER_THINK)
	{
		static BOOL thinking;

		if(!thinking)
		{
			thinking = TRUE;

			if (m_GT2Socket)
				gt2Think(m_GT2Socket);
			ghttpThink();

			if(m_state == HOST_CHALLENGING)
			{
				if(!m_challenged)
				{
					// Send the challenge string and our profile.
					/////////////////////////////////////////////
					rcode = gtEncode(MSG_CHALLENGE_TYPE, MSG_CHALLENGE, buffer, sizeof(buffer), m_loginDlg.m_profile, (LPCSTR)m_challenge);
					ASSERT(rcode != -1);
					gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2True);
					m_challenged = TRUE;
				}
			}
			else if(m_state == HOST_CONNECTED)
			{
				m_waitingDlg.EndDialog(IDOK);
				m_state = RACING;
			}
			else if(m_state == HOST_ERROR)
			{
				MessageBox("Error setting up hosting");
				m_waitingDlg.EndDialog(IDCANCEL);
			}
			else if(m_state == JOIN_CONNECTED)
			{
				if(m_waitingDlg.m_hWnd && m_waitingDlg.IsWindowEnabled())
					m_waitingDlg.EndDialog(IDOK);
			}
			else if(m_state == JOIN_ERROR)
			{
				MessageBox("Error joining a game");
				m_waitingDlg.EndDialog(IDCANCEL);
			}

			thinking = FALSE;
		}

		if(m_state == LOGGED_OUT)
		{
			if(!Dlg->SetupMatch())
			{
				MessageBox("Error setting up the match");
				Logout();
			}
		}

		// Are we racing?
		/////////////////
		if(m_racing)
		{
			// Did we finish?
			/////////////////
			if(m_localTime)
			{
				// Did we both finish?
				//////////////////////
				if(m_remoteTime)
				{
					// Done racing.
					///////////////
					m_racing = FALSE;

					m_info = "Race Complete";
					UpdateData(FALSE);

					// Report the stats.
					////////////////////
					if(m_hosting)
						ReportStats();

					// Show the times.
					//////////////////
					CString message;
					if(m_localTime < m_remoteTime)
						message.Format("You won!\n%0.3fs to %0.3fs", m_localTime / 1000.0, m_remoteTime / 1000.0);
					else if(m_remoteTime < m_localTime)
						message.Format("You lost!\n%0.3fs to %0.3fs", m_localTime / 1000.0, m_remoteTime / 1000.0);
					else
						message.Format("You tied!\n%0.3fs", m_localTime / 1000.0);
					MessageBox(message);

					m_localProgress.SetPos(0);
					m_remoteProgress.SetPos(0);
				}
			}
			else
			{
				// Let our opponent know how far we are.
				////////////////////////////////////////
				rcode = gtEncode(MSG_PROGRESS_TYPE, MSG_PROGRESS, buffer, sizeof(buffer), m_numSteps);
				ASSERT(rcode != -1);
				gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2False);
			}
		}
	}
	else if(nIDEvent == TIMER_COUNTDOWN)
	{
		m_countdown--;
		if(m_countdown <= 0)
			KillTimer(TIMER_COUNTDOWN);

		if(m_countdown < 0)
			return;

		Countdown();
		if(!m_countdown)
			StartRace();
	}

	CDialog::OnTimer(nIDEvent);
}

void CLadderTrackDlg::Logout()
{
	OnLogout();
}

void CLadderTrackDlg::Countdown()
{
	if(m_hosting)
	{
		int rcode;
		char message[32];
		rcode = gtEncode(MSG_COUNTDOWN_TYPE, MSG_COUNTDOWN, message, sizeof(message), m_countdown);
		ASSERT(rcode != -1);
		gt2Send(m_GT2Connection, (const unsigned char*)message, rcode, GT2True);
	}

	if(m_countdown)
	{
		UpdateData();

		m_info.Format("Race starts in %ds", m_countdown);

		UpdateData(FALSE);
	}
}

void CLadderTrackDlg::OnStart()
{
	// Start the countdown.
	///////////////////////
	m_countdown = COUNTDOWN_START;
	SetTimer(TIMER_COUNTDOWN, 1000, NULL);
	Countdown();
}

BOOL CLadderTrackDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN)
	{
		int nChar = pMsg->wParam;
		if((nChar == 'Z') || (nChar == 'X'))
		{
			if((pMsg->lParam & 0xFFFF) == 1)
			{
				CString str;
				BOOL stepped = FALSE;

				if((nChar == 'Z') && (m_step != LEFT))
				{
					m_step = LEFT;
					m_numSteps++;
					stepped = TRUE;
				}
				else if ((nChar == 'X') && (m_step != RIGHT))
				{
					m_step = RIGHT;
					m_numSteps++;
					stepped = TRUE;
				}

				if(stepped && m_racing)
				{
					m_localProgress.SetPos(m_numSteps);
					if(m_numSteps == m_totalSteps)
					{
						m_localTime = (GetTickCount() - m_start);
						str.Format("%0.3fs\n", m_localTime / 1000.0);
						OutputDebugString(str);
						//MessageBox(str);

						if(!m_remoteTime)
						{
							UpdateData();
							m_info = "Waiting for opponent";
							UpdateData(FALSE);
						}

						// Let them know we finished.
						/////////////////////////////
						char buffer[32];
						int rcode;
						rcode = gtEncode(MSG_END_RACE_TYPE, MSG_END_RACE, buffer, sizeof(buffer), m_localTime);
						ASSERT(rcode != -1);
						gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2True);
					}
				}
			}

			return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CLadderTrackDlg::StartRace()
{
	if(m_hosting)
	{
		int rcode;
		char buffer[32];
		rcode = gtEncode(MSG_START_RACE_TYPE, MSG_START_RACE, buffer, sizeof(buffer));
		ASSERT(rcode != -1);
		gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2True);
	}

	m_localTime = 0;
	m_remoteTime = 0;
	m_racing = TRUE;
	m_numSteps = 0;
	m_step = NONE;
	m_racing = TRUE;
	m_start = GetTickCount();
	m_totalSteps = RACE_STEPS;
	m_localProgress.SetRange(0, m_totalSteps);
	m_localProgress.SetPos(0);
	m_remoteProgress.SetRange(0, m_totalSteps);
	m_remoteProgress.SetPos(0);

	UpdateData();
	m_info.Format("GO!");
	UpdateData(FALSE);
}

void CLadderTrackDlg::ReportStats()
{
	char response[33];

	// Game.
	////////
	NewGame(1);
	BucketStringOp(NULL, "hostname", bo_set, (char *)(LPCSTR)m_loginDlg.m_nick, bl_server, 0);

	// Local player (host).
	///////////////////////
	NewPlayer(NULL, 0, (char *)(LPCSTR)m_loginDlg.m_nick);
	BucketIntOp(NULL, "time", bo_set, m_localTime, bl_player, 0);
	BucketIntOp(NULL, "pid", bo_set, m_loginDlg.m_profile, bl_player, 0);
	GenerateAuth((char *)(LPCSTR)m_challenge, (char *)(LPCSTR)m_loginDlg.m_password, response);
	BucketStringOp(NULL, "auth", bo_set, response, bl_player, 0);

	// Remote player (joined).
	//////////////////////////
	NewPlayer(NULL, 1, (char *)(LPCSTR)m_remoteNick);
	BucketIntOp(NULL, "time", bo_set, m_remoteTime, bl_player, 1);
	BucketIntOp(NULL, "pid", bo_set, m_remoteProfile, bl_player, 1);
	BucketStringOp(NULL, "auth", bo_set, (char *)(LPCSTR)m_remoteResponse, bl_player, 1);

	SendGameSnapShot(NULL, NULL, SNAP_FINAL);
	FreeGame(NULL);
}

struct FakeProfile
{
	int pid;
	char password[32];
	char nick[32];
	DWORD minTime;
	DWORD maxTime;
};

FakeProfile FakeProfiles[] = 
{
	{ 4933555, "mrpants", "ABCDEF",     4500, 6000},
	{ 4903509, "mrpants", "BigFatty",   5500, 7000},
	{ 4933502, "mrpants", "Colonel Op", 5500, 7000},
	{ 4933634, "mrpants", "GHIJKL",     4500, 7000},
	{ 3017545, "mrpants", "test1",      4500, 8000},
	{ 3017574, "mrpants", "test2",      4500, 6000},
	{ 3360407, "mrpants", "test3",      6500, 7000},
	{ 4933752, "mrpants", "tester",     5500, 7000},
	{ 2840651, "mrpants", "a,b,c",      6500, 8000},
	{ 3410517, "mrpants", "abcabc",     4500, 6000},
	{ 3652959, "mrpants", "bane",       6500, 8000},
	{ 3654054, "mrpants", "cccccc",     6500, 7000},
	{ 8696884, "mrpants", "Jimmy Page", 5500, 6000},
	{ 100001, "mrpants", "Mr. Pants",   5500, 8000},
	{ 2881317, "mrpants", "mrpants",    4500, 7000},
	{ 3651362, "mrpants", "mrpantsz",   6500, 7000},
	{ 3671851, "mrpants", "mrwasabi",   5500, 6000},
	{ 3652930, "mrpants", "pants",      6500, 8000},
	{ 2833041, "mrpants", "pants3",     6500, 7000},
	{ 2833042, "mrpants", "pants4",     5500, 7000},
	{ 2833043, "mrpants", "pants5",     6500, 7000},
	{ 2833045, "mrpants", "pants6",     4500, 7000},
	{ 2833048, "mrpants", "pants7",     6500, 7000},
	{ 2833049, "mrpants", "pants8",     4500, 6000},
	{ 2833050, "mrpants", "pants9",     6500, 8000},
	{ 2833074, "mrpants", "pants16",    7000, 8000},
	{ 2833075, "mrpants", "pants17",    4500, 8000},
	{ 2833076, "mrpants", "pants18",    6500, 7000},
	{ 2833077, "mrpants", "pants19",    5500, 7000},
	{ 2833079, "mrpants", "pants20",    6500, 7000},
	{ 2833080, "mrpants", "pants21",    6500, 8000},
	{ 2833081, "mrpants", "pants22",    4500, 6000},
	{ 3654074, "mrpants", "sdfrrr",     4500, 6000},
	{ 3733688, "mrpants", "Skeletor",   4500, 7000},
	{ 2977286, "mrpants", "testtesttest", 6500, 8000},
	{ 3654019, "mrpants", "tttttest",   4500, 6000}
};

void CLadderTrackDlg::FakeStats()
{
	int num = (sizeof(FakeProfiles) / sizeof(FakeProfile));
	int total = 1000;
	int p1;
	int p2;
	int i;
	char response[33];
	CString msg;

	srand(time(NULL));

	for(i = 0 ; i < total ; i++)
	{
		p1 = (rand() % num);
		p2 = (rand() % num);

		if(p1 == p2)
			continue;

		m_loginDlg.m_nick = FakeProfiles[p1].nick;
		m_loginDlg.m_profile = FakeProfiles[p1].pid;
		m_loginDlg.m_password = FakeProfiles[p1].password;
		m_localTime = (DWORD)(FakeProfiles[p1].minTime + ((rand() / (float)RAND_MAX) * (FakeProfiles[p1].maxTime - FakeProfiles[p1].minTime)));

		m_remoteNick = FakeProfiles[p2].nick;
		m_remoteProfile = FakeProfiles[p2].pid;
		GenerateAuth((char *)(LPCSTR)m_challenge, FakeProfiles[p2].password, response);
		m_remoteResponse = response;
		m_remoteTime = (DWORD)(FakeProfiles[p2].minTime + ((rand() / (float)RAND_MAX) * (FakeProfiles[p2].maxTime - FakeProfiles[p2].minTime)));

		ReportStats();

		msg.Format("game %d reported\n", i);
		OutputDebugString(msg);

		Sleep(1200);
	}

	exit(1);
}

void CLadderTrackDlg::OnUpdatePositions() 
{
	UpdatePlayerPositions();
}
