// multiTrackDlg.cpp : implementation file
//

#include "stdafx.h"
#include "multiTrack.h"
#include "multiTrackDlg.h"
#include "waitingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMultiTrackDlg dialog

CMultiTrackDlg::CMultiTrackDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMultiTrackDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMultiTrackDlg)
	m_info = _T("Ready");
	m_localInfo100 = _T("");
	m_localInfo200 = _T("");
	m_localInfo50 = _T("");
	m_localInfoOverall = _T("");
	m_remoteInfo100 = _T("");
	m_remoteInfo200 = _T("");
	m_remoteInfo50 = _T("");
	m_remoteInfoOverall = _T("");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMultiTrackDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMultiTrackDlg)
	DDX_Control(pDX, IDC_START_100, m_start100);
	DDX_Control(pDX, IDC_START_200, m_start200);
	DDX_Control(pDX, IDC_REMOTE_PROGRESS, m_remoteProgress);
	DDX_Control(pDX, IDC_LOCAL_PROGRESS, m_localProgress);
	DDX_Control(pDX, IDC_START_50, m_start50);
	DDX_Text(pDX, IDC_INFO, m_info);
	DDX_Text(pDX, IDC_LOCAL_INFO_100, m_localInfo100);
	DDX_Text(pDX, IDC_LOCAL_INFO_200, m_localInfo200);
	DDX_Text(pDX, IDC_LOCAL_INFO_50, m_localInfo50);
	DDX_Text(pDX, IDC_LOCAL_INFO_OVERALL, m_localInfoOverall);
	DDX_Text(pDX, IDC_REMOTE_INFO_100, m_remoteInfo100);
	DDX_Text(pDX, IDC_REMOTE_INFO_200, m_remoteInfo200);
	DDX_Text(pDX, IDC_REMOTE_INFO_50, m_remoteInfo50);
	DDX_Text(pDX, IDC_REMOTE_INFO_OVERALL, m_remoteInfoOverall);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMultiTrackDlg, CDialog)
	//{{AFX_MSG_MAP(CMultiTrackDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOGOUT, OnLogout)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_START_50, OnStart50)
	ON_BN_CLICKED(IDC_START_100, OnStart100)
	ON_BN_CLICKED(IDC_START_200, OnStart200)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMultiTrackDlg message handlers

#define MSG_CONNECT              "is" // client-pid, nick
#define MSG_CHALLENGE            "is" // host-pid, challenge
#define MSG_CHALLENGE_TYPE       1
#define MSG_RESPONSE             "s"  // response
#define MSG_RESPONSE_TYPE        2
#define MSG_COUNTDOWN            "ii" // event, count
#define MSG_COUNTDOWN_TYPE       3
#define MSG_START_RACE           ""
#define MSG_START_RACE_TYPE      5
#define MSG_PROGRESS             "i"  // progress
#define MSG_PROGRESS_TYPE        6
#define MSG_END_RACE             "i"  // time
#define MSG_END_RACE_TYPE        7
#define MSG_CHAT                 "s"  // message
#define MSG_CHAT_TYPE            8

#define HOST_PORT                38465
#define HOST_PORT_STRING         ":38465"
#define CLIENT_PORT_STRING       ":38446"
#define COUNTDOWN_START          5

#define TIMER_THINK              100
#define TIMER_COUNTDOWN          101

#define DEFRATING                1400
#define POINTSWIN                16
#define POINTSLOSS               -16

CMultiTrackDlg * Dlg;

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
		Dlg->m_connection = NULL;
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
		gt2Send(connection, (const GT2Byte*)message, rcode, GT2True);

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

		if(gtDecode(MSG_COUNTDOWN, (char*)message, len, &Dlg->m_event, &Dlg->m_countdown) == -1)
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
	if(reason != GT2LocalClose)
	{
		Dlg->MessageBox("Connection closed");
		Dlg->Logout();
	}
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
	if(Dlg->m_connection)
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
	Dlg->m_connection = connection;
	Dlg->m_state = HOST_CHALLENGING;
	Dlg->m_challenged = FALSE;
	Dlg->m_remoteNick = nick;
}

/*void StoppedCallback
(
	GTListener listener,
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

BOOL CMultiTrackDlg::SetupHosting()
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
		strcpy(gcd_gamename, "st_rank");
		gcd_secret_key[0] = '5';
		gcd_secret_key[1] = '3';
		gcd_secret_key[2] = 'J';
		gcd_secret_key[3] = 'x';
		gcd_secret_key[4] = '7';
		gcd_secret_key[5] = 'W';

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
		return FALSE;
		*/

	GT2ConnectionCallbacks connectionCallbacks;
	memset(&connectionCallbacks, 0, sizeof(GT2ConnectionCallbacks));
	connectionCallbacks.received = ReceivedCallback;
	connectionCallbacks.closed = ClosedCallback;

	GT2Result aResult = gt2CreateSocket(&m_socket, HOST_PORT_STRING, 0, 0, SocketErrorCallback);
	if (GT2Success != aResult)
		return FALSE;

	gt2Listen(m_socket, ConnectAttemptCallback);

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

BOOL CMultiTrackDlg::SetupJoining()
{
	int rcode;

	// Setup the address to connect to.
	///////////////////////////////////
	CString remoteAddress;
	remoteAddress.Format("%s:%d", m_hostOrJoinDlg.m_joinAddress, HOST_PORT);

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
	GT2Result aResult = gt2CreateSocket(&m_socket, CLIENT_PORT_STRING, 0, 0, SocketErrorCallback);
	if (aResult != GT2Success)
	{
		MessageBox("Failed to create socket!");
		return FALSE;
	}

	// Connect.
	///////////
	m_state = JOIN_CONNECTING;
	aResult = gt2Connect(m_socket, &m_connection, remoteAddress, (const GT2Byte*)buffer, sizeof(buffer), -1, &callbacks, GT2False);
	if(!m_connection)
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

void CMultiTrackDlg::UpdateRatingsDisplay()
{
	Dlg->UpdateData();

	m_localInfoOverall.Format("%d", m_localRatings[0]);
	m_localInfo50.Format("%d", m_localRatings[1]);
	m_localInfo100.Format("%d", m_localRatings[2]);
	m_localInfo200.Format("%d", m_localRatings[3]);

	m_remoteInfoOverall.Format("%d", m_remoteRatings[0]);
	m_remoteInfo50.Format("%d", m_remoteRatings[1]);
	m_remoteInfo100.Format("%d", m_remoteRatings[2]);
	m_remoteInfo200.Format("%d", m_remoteRatings[3]);

	Dlg->UpdateData(FALSE);
}

GHTTPBool PlayerRatingsPageCompleted
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
		BOOL localPlayer = (BOOL)param;
		int ratings[4];
		int rcode;

		// Scan out ratings.
		////////////////////
		rcode = sscanf(buffer, "%d %d %d %d", &ratings[0], &ratings[1], &ratings[2], &ratings[3]);
		if(rcode == 4)
		{
			if(localPlayer)
				memcpy(Dlg->m_localRatings, ratings, sizeof(int) * 4);
			else
				memcpy(Dlg->m_remoteRatings, ratings, sizeof(int) * 4);

			Dlg->UpdateRatingsDisplay();
		}
	}

	return GHTTPTrue;
}

BOOL CMultiTrackDlg::SetupMatch()
{
	int rcode;
	BOOL result;

	m_state = SETTING_UP;

	ASSERT(!m_connection);
	ASSERT(!m_socket);

	m_state = 0;
	m_remoteResponse.Empty();
	m_remoteProfile = 0;
	m_connection = NULL;
	m_start50.EnableWindow(FALSE);
	m_start100.EnableWindow(FALSE);
	m_start200.EnableWindow(FALSE);
	m_start = 0;
	m_countdown = 0;
	m_racing = FALSE;
	m_event = EVENT_NONE;
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

	m_connection = NULL;
	m_hosting = (m_hostOrJoinDlg.m_hostOrJoin == HOSTORJOIN_HOST);

	CString str;
	str.Format("multiTrack%s", m_hosting?" (hosting)":"");
	SetWindowText(str);

	if(m_hosting)
		result = SetupHosting();
	else
		result = SetupJoining();

	if(result && m_hosting)
	{
		m_start50.EnableWindow();
		m_start100.EnableWindow();
		m_start200.EnableWindow();
	}

	CString url;
	url.Format("http://sdkdev.gamespy.com/games/st_rank/web/playerratings.asp?pid=%d", m_loginDlg.m_profile);
	ghttpGet(url, GHTTPFalse, PlayerRatingsPageCompleted, (void *)TRUE);
	url.Format("http://sdkdev.gamespy.com/games/st_rank/web/playerratings.asp?pid=%d", m_remoteProfile);
	ghttpGet(url, GHTTPFalse, PlayerRatingsPageCompleted, (void *)FALSE);

	return result;
}

BOOL CMultiTrackDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// Basic initialization.
	////////////////////////
	Dlg = this;
	m_connection = NULL;
	m_socket = NULL;
	m_state = LOGGED_OUT;
	m_countdown = 0;
	m_racing = FALSE;

	// Init gt.
	///////////
	//gt2Startup();

	// Set a think timer.
	/////////////////////
	SetTimer(TIMER_THINK, 50, NULL);

	return TRUE;
}

void CMultiTrackDlg::OnPaint() 
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

HCURSOR CMultiTrackDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMultiTrackDlg::OnLogout() 
{
	m_state = LOGGED_OUT;

	// Clean stuff up.
	//////////////////
	if(m_connection)
	{
		gt2CloseConnection(m_connection);
		m_connection = NULL;
	}
	if(m_socket)
	{
		gt2CloseSocket(m_socket);
		m_socket = NULL;
	}
	if(m_waitingDlg.m_hWnd && m_waitingDlg.IsWindowEnabled())
	{
		m_waitingDlg.EndDialog(IDCANCEL);
	}
}

void CMultiTrackDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	if(IsStatsConnected())
		CloseStatsConnection();

	ghttpCleanup();

	//gt2Cleanup(GT2False);
	if (m_connection)
	{
		gt2CloseConnection(m_connection);
		m_connection = NULL;
	}
	if (m_socket)
	{
		gt2CloseSocket(m_socket);
		m_socket = NULL;
	}

}

int CalculateHandicap(int rating1, int rating2)
{
	static int htable[] = { 12, 34, 56, 78, 101, 126, 151, 177, 206, 239, 273, 315, 366, 446, 471, 715 };
	int diff;
	int i;
	
	diff = abs(rating1 - rating2);

	for(i = 0 ; i < 16 ; i++)
	{
		if(diff < htable[i])
			return i;
	}

	return 16;
}

void CMultiTrackDlg::UpdateStats()
{
	int handicap;

	handicap = CalculateHandicap(m_localRatings[0], m_remoteRatings[0]);
	if(handicap)
	{
		if(m_localRatings[0] > m_remoteRatings[0])
		{
			m_localRatings[0] -= handicap;
			m_remoteRatings[0] += handicap;
		}
		else
		{
			m_localRatings[0] += handicap;
			m_remoteRatings[0] -= handicap;
		}
	}

	handicap = CalculateHandicap(m_localRatings[m_event], m_remoteRatings[m_event]);
	if(handicap)
	{
		if(m_localRatings[m_event] > m_remoteRatings[m_event])
		{
			m_localRatings[m_event] -= handicap;
			m_remoteRatings[m_event] += handicap;
		}
		else
		{
			m_localRatings[m_event] += handicap;
			m_remoteRatings[m_event] -= handicap;
		}
	}

	if(m_localTime < m_remoteTime)
	{
		m_localRatings[0] += POINTSWIN;
		m_localRatings[m_event] += POINTSWIN;
		m_remoteRatings[0] += POINTSLOSS;
		m_remoteRatings[m_event] += POINTSLOSS;
	}
	else if(m_remoteTime < m_localTime)
	{
		m_remoteRatings[0] += POINTSWIN;
		m_remoteRatings[m_event] += POINTSWIN;
		m_localRatings[0] += POINTSLOSS;
		m_localRatings[m_event] += POINTSLOSS;
	}

	UpdateRatingsDisplay();
}

void CMultiTrackDlg::OnTimer(UINT nIDEvent) 
{
	char buffer[64];
	int rcode;

	if(nIDEvent == TIMER_THINK)
	{
		static BOOL thinking;

		if(!thinking)
		{
			thinking = TRUE;

			if (m_socket)
				gt2Think(m_socket);
			ghttpThink();

			if(m_state == HOST_CHALLENGING)
			{
				if(!m_challenged)
				{
					// Send the challenge string and our profile.
					/////////////////////////////////////////////
					rcode = gtEncode(MSG_CHALLENGE_TYPE, MSG_CHALLENGE, buffer, sizeof(buffer), m_loginDlg.m_profile, (LPCSTR)m_challenge);
					ASSERT(rcode != -1);
					gt2Send(m_connection, (const unsigned char*)buffer, rcode, GT2True);
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

					// Report the stats.
					////////////////////
					if(m_hosting)
						ReportStats();

					m_event = EVENT_NONE;

					UpdateStats();
				}
			}
			else
			{
				// Let our opponent know how far we are.
				////////////////////////////////////////
				rcode = gtEncode(MSG_PROGRESS_TYPE, MSG_PROGRESS, buffer, sizeof(buffer), m_numSteps);
				ASSERT(rcode != -1);
				gt2Send(m_connection, (const GT2Byte*)buffer, rcode, GT2False);
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

void CMultiTrackDlg::Logout()
{
	OnLogout();
}

void CMultiTrackDlg::Countdown()
{
	if(m_hosting)
	{
		int rcode;
		char message[32];
		rcode = gtEncode(MSG_COUNTDOWN_TYPE, MSG_COUNTDOWN, message, sizeof(message), m_event, m_countdown);
		ASSERT(rcode != -1);
		gt2Send(m_connection, (const GT2Byte*)message, rcode, GT2True);
	}

	if(m_countdown)
	{
		UpdateData();

		CString strEvent;
		if(m_event == EVENT_50)
			strEvent = "50m";
		else if(m_event == EVENT_100)
			strEvent = "100m";
		else if(m_event == EVENT_200)
			strEvent = "200m";
		m_info.Format("%s starts in %ds", strEvent, m_countdown);

		UpdateData(FALSE);
	}
}

void CMultiTrackDlg::OnStart50()
{
	// Start the countdown.
	///////////////////////
	m_countdown = COUNTDOWN_START;
	SetTimer(TIMER_COUNTDOWN, 1000, NULL);
	m_event = EVENT_50;
	Countdown();
}

void CMultiTrackDlg::OnStart100() 
{
	// Start the countdown.
	///////////////////////
	m_countdown = COUNTDOWN_START;
	SetTimer(TIMER_COUNTDOWN, 1000, NULL);
	m_event = EVENT_100;
	Countdown();
}

void CMultiTrackDlg::OnStart200() 
{
	// Start the countdown.
	///////////////////////
	m_countdown = COUNTDOWN_START;
	SetTimer(TIMER_COUNTDOWN, 1000, NULL);
	m_event = EVENT_200;
	Countdown();
}

BOOL CMultiTrackDlg::PreTranslateMessage(MSG* pMsg) 
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

						UpdateData();

						m_info = "Race Complete";

						// Let them know we finished.
						/////////////////////////////
						char buffer[32];
						int rcode;
						rcode = gtEncode(MSG_END_RACE_TYPE, MSG_END_RACE, buffer, sizeof(buffer), m_localTime);
						ASSERT(rcode != -1);
						gt2Send(m_connection, (const GT2Byte*)buffer, rcode, GT2True);

						UpdateData(FALSE);
					}
				}
			}

			return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CMultiTrackDlg::StartRace()
{
	if(m_hosting)
	{
		int rcode;
		char buffer[32];
		rcode = gtEncode(MSG_START_RACE_TYPE, MSG_START_RACE, buffer, sizeof(buffer));
		ASSERT(rcode != -1);
		gt2Send(m_connection, (const GT2Byte*)buffer, rcode, GT2True);
	}

	m_localTime = 0;
	m_remoteTime = 0;
	m_racing = TRUE;
	m_numSteps = 0;
	m_step = NONE;
	m_racing = TRUE;
	m_start = GetTickCount();
	if(m_event == EVENT_50)
		m_totalSteps = RACE_STEPS_50;
	else if(m_event == EVENT_100)
		m_totalSteps = RACE_STEPS_100;
	else if(m_event == EVENT_200)
		m_totalSteps = RACE_STEPS_200;
	m_localProgress.SetRange(0, m_totalSteps);
	m_localProgress.SetPos(0);
	m_remoteProgress.SetRange(0, m_totalSteps);
	m_remoteProgress.SetPos(0);

	UpdateData();
	m_info.Format("GO!");
	UpdateData(FALSE);
}

void CMultiTrackDlg::ReportStats()
{
	char response[33];

	// Game.
	////////
	NewGame(1);
	BucketStringOp(NULL, "hostname", bo_set, (char *)(LPCSTR)m_loginDlg.m_nick, bl_server, 0);
	BucketIntOp(NULL, "event", bo_set, m_event, bl_server, 0);

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
