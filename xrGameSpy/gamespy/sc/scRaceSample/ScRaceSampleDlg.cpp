// ScRaceSampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ScRaceSample.h"
#include "ScRaceSampleDlg.h"
#include "waitingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScRaceSampleDlg dialog

CScRaceSampleDlg::CScRaceSampleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScRaceSampleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScRaceSampleDlg)
	m_info = _T("Ready");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CScRaceSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScRaceSampleDlg)
	DDX_Control(pDX, IDC_START_RACE, m_startRace);
	DDX_Control(pDX, IDC_REMOTE_PROGRESS, m_remoteProgress);
	DDX_Control(pDX, IDC_LOCAL_PROGRESS, m_localProgress);
	DDX_Text(pDX, IDC_INFO, m_info);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CScRaceSampleDlg, CDialog)
	//{{AFX_MSG_MAP(CScRaceSampleDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START_RACE, OnStart)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_LOGOUT, OnLogout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScRaceSampleDlg message handlers

// Pre-game messaging
#define MSG_CONNECT              "is" // client-pid, nick
#define MSG_TO_HOST_CERT		 "ir" // host-pid, certificate
#define MSG_TO_HOST_CERT_TYPE	 1
#define MSG_TO_CLIENT_CERT		 "ir" // join-pid, certificate
#define MSG_TO_CLIENT_CERT_TYPE	 2
#define MSG_TO_HOST_KEYS		 "r"  // encryption keys
#define MSG_TO_HOST_KEYS_TYPE	 3
#define MSG_TO_CLIENT_KEYS		 "r"  // encrpytion keys
#define MSG_TO_CLIENT_KEYS_TYPE	 4

// Game messaging
#define MSG_COUNTDOWN            "i"  // count
#define MSG_COUNTDOWN_TYPE       20
#define MSG_SESSION_ID           "rr" // Host session ID, connID exchange
#define MSG_SESSION_ID_TYPE      21
#define MSG_CONNECTION_ID        "r"  // connection ID exhcnage
#define MSG_CONNECTION_ID_TYPE   22
#define MSG_START_RACE           ""	  // race start
#define MSG_START_RACE_TYPE      23
#define MSG_PROGRESS             "i"  // progress
#define MSG_PROGRESS_TYPE        24
#define MSG_END_RACE             "i"  // time
#define MSG_END_RACE_TYPE        25
#define MSG_CHAT                 "s"  // message
#define MSG_CHAT_TYPE            26

//#define HOST_PORT                38466
#define HOST_PORT_STRING         ":38466"
#define CLIENT_PORT_STRING       ":38467" // so you can run both
#define COUNTDOWN_START          5

#define TIMER_THINK              100
#define TIMER_COUNTDOWN          101

// Stats & SDK constants
#define SCRACE_TIMEOUT_MS		 0
#define SCRACE_SLEEP_MS			 100
#define SCRACE_AUTHORITATIVE	 gsi_true
#define SCRACE_COLLABORATIVE	 gsi_false
#define SCRACE_NUM_PLAYERS		 2
#define SCRACE_NUM_TEAMS		 2

#define SCRACE_HOST_TEAM		 7564	// fake team ids
#define SCRACE_CLIENT_TEAM		 7565	// fake team ids


CScRaceSampleDlg * Dlg;
gsi_bool sessionCreated;
gsi_bool connIdSet;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// A utility to block until a request completes
//     Used to simplify the logic flow of the sample
void waitForScCallbacks(SCInterfacePtr theInterface, int howMany)
{
	// wait for the request to complete
	/////////////////////////////////////////////////////////
	gPlayerData.mWaitCount = howMany;
	while (gPlayerData.mWaitCount > 0)
	{
		msleep(SCRACE_SLEEP_MS);
		scThink(theInterface);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Competition Callbacks
void createSessionCallback(const SCInterfacePtr theInterface,
						   GHTTPResult theHttpResult,
                           SCResult theResult,
						   void * theUserData)
{	
	CString debugStr;

	if (theHttpResult == GHTTPSuccess && theResult == SCResult_NO_ERROR)
	{
		// Retrieve the Session/Connection ID to be used later
		/////////////////////////////////////////////////////////		
		
		memcpy(gPlayerData.mSessionId, scGetSessionId(theInterface), SC_SESSION_GUID_SIZE);
		memcpy(gPlayerData.mConnectionId, scGetConnectionId(theInterface), SC_CONNECTION_GUID_SIZE);

		sessionCreated = gsi_true;
		
		debugStr.Format("[createSessionCB] Session ID: %s\n", gPlayerData.mSessionId);
		OutputDebugString(debugStr);
		debugStr.Format("[createSessionCB] Connection ID: [%s]\n", gPlayerData.mConnectionId);
		OutputDebugString(debugStr);
	}
	else
	{
		debugStr.Format("[createSessionCB] Error. HTTPResult: %d, SCResult: %d\r\n", 
			theHttpResult, theResult);
		OutputDebugString(debugStr);
			
		Dlg->MessageBox("Error Creating Stats Session");
		Dlg->Logout();
	}

	gPlayerData.mWaitCount--;

	GSI_UNUSED(theInterface);
	GSI_UNUSED(theUserData);
}

void setReportIntentionCallback(const SCInterfacePtr theInterface,
								GHTTPResult theHttpResult,
                                SCResult theResult,
								void * theUserData)
{
	CString debugStr;
	
	if (theHttpResult == GHTTPSuccess && theResult == SCResult_NO_ERROR)
	{		
		// Retrieve the connection ID to be used later
		/////////////////////////////////////////////////////////
		const char * connectionId = scGetConnectionId(theInterface);
		memcpy(gPlayerData.mConnectionId, connectionId, SC_CONNECTION_GUID_SIZE);
		connIdSet = gsi_true;

		debugStr.Format("[setIntentionCB] Connection ID: [%s]\n", gPlayerData.mConnectionId);
		OutputDebugString(debugStr);
	}
	else
	{
		debugStr.Format("[setIntentionCB] Error. HTTPResult: %d, SCResult: %d\r\n",
			theHttpResult, theResult);
		OutputDebugString(debugStr);
		
		Dlg->MessageBox("Error Initializing Stats System");
		Dlg->Logout();
	}

	gPlayerData.mWaitCount--; // one less request to wait for

	GSI_UNUSED(theInterface);
	GSI_UNUSED(theUserData);
}

void submitReportCallback(const SCInterfacePtr theInterface, 
						  GHTTPResult theHttpResult,
						  SCResult theResult,
						  void * theUserData)
{
	CString debugStr;

	if (theHttpResult != GHTTPSuccess || theResult != SCResult_NO_ERROR)
	{			
		debugStr.Format("[submitReportCB] Error. HTTPResult: %d, SCResult: %d\r\n",
			theHttpResult, theResult);
		OutputDebugString(debugStr);	
		
		Dlg->MessageBox("Error Submitting Stats Report");
	}	
	
	gPlayerData.mWaitCount--; // one less request to wait for

	GSI_UNUSED(theInterface);
	GSI_UNUSED(theUserData);
}

///////////////////////////////////////////////////////////////////////////////
// GT2 Callbacks
void ConnectedCallback
(
	GT2Connection connection,
	GT2Result result,
	GT2Byte * message,
	int len
)
{
	if(result == GT2Success)
		Dlg->m_state = JOIN_EXCHANGE_CERT;
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
	CString debugStr;

	type = gtEncodedMessageType((char*)message);

	if(type == MSG_TO_CLIENT_CERT_TYPE)
	{
		char remoteCert[512];
		int remoteCertLen = 512;
		int pid;

		if(gtDecode(MSG_TO_CLIENT_CERT, (char*)message, len, &pid, remoteCert, &remoteCertLen) == -1)
		{
			Dlg->m_state = JOIN_ERROR;
			return;
		}
			
		// Parse the certificate
		/////////////////////////////////////////////////////////
		wsLoginCertReadBinary(&Dlg->m_remoteCertificate, remoteCert, remoteCertLen);

		Dlg->m_remoteProfile = pid;

		// Build up and send the certificate response
		/////////////////////////////////////////////////////////
		char buffer[520];
		char cert[512];
		int rcode;
		gsi_u32 certLen;

		wsLoginCertWriteBinary(&gPlayerData.mCertificate, cert, sizeof(cert), &certLen);

		rcode = gtEncode(MSG_TO_HOST_CERT_TYPE, MSG_TO_HOST_CERT, buffer, sizeof(buffer), gPlayerData.mProfileId, cert, certLen);
		ASSERT(rcode != -1);
		gt2Send(connection, (const unsigned char*)buffer, rcode, GT2True);

		Dlg->m_state = JOIN_VERIFY_CERT;
	}
	else if(type == MSG_TO_HOST_CERT_TYPE)
	{
		char remoteCert[512];
		int remoteCertLen = 512;
		int pid;

		if(gtDecode(MSG_TO_HOST_CERT, (char*)message, len, &pid, remoteCert, &remoteCertLen) == -1)
		{
			Dlg->m_state = HOST_ERROR;
			return;
		}

		// Parse the certificate
		/////////////////////////////////////////////////////////
		wsLoginCertReadBinary(&Dlg->m_remoteCertificate, remoteCert, remoteCertLen);

		Dlg->m_remoteProfile = pid;
		Dlg->m_state = HOST_VERIFY_CERT;
	}
	else if(type == MSG_TO_CLIENT_KEYS_TYPE)
	{
		SCPeerKeyExchangeMsg recvMsg;
		int recvMsgLen = GS_CRYPT_RSA_BYTE_SIZE;

		if(gtDecode(MSG_TO_CLIENT_KEYS, (char*)message, len, recvMsg, &recvMsgLen) == -1)
		{
			Dlg->m_state = JOIN_ERROR;
			return;
		}

		// Receiving player should parse the cipher key out of it.
		//   - decrypting the msg requires the local player's private data
		/////////////////////////////////////////////////////////
		scPeerCipherParseKeyExchangeMsg(&gPlayerData.mCertificate, &gPlayerData.mPrivateData, 
			recvMsg, &gPlayerData.mPeerSendCipher);


		// Send response to host
		/////////////////////////////////////////////////////////
		char buffer[512];
		int rcode;
		SCPeerKeyExchangeMsg exchangeMsg;

		scPeerCipherInit(&gPlayerData.mCertificate, &gPlayerData.mPeerRecvCipher); 	
		scPeerCipherCreateKeyExchangeMsg(&Dlg->m_remoteCertificate, &gPlayerData.mPeerRecvCipher, exchangeMsg);

		// Now send the key to the other player
		/////////////////////////////////////////////////////////
		rcode = gtEncode(MSG_TO_HOST_KEYS_TYPE, MSG_TO_HOST_KEYS, buffer, sizeof(buffer), exchangeMsg, GS_CRYPT_RSA_BYTE_SIZE);
		ASSERT(rcode != -1);
		gt2Send(connection, (const unsigned char*)buffer, rcode, GT2True);

		Dlg->m_state = JOIN_CONNECTED;
	}
	else if(type == MSG_TO_HOST_KEYS_TYPE)
	{
		SCPeerKeyExchangeMsg exchangeMsg;
		int exchangeMsgLen = GS_CRYPT_RSA_BYTE_SIZE;

		if(gtDecode(MSG_TO_HOST_KEYS, (char*)message, len, exchangeMsg, &exchangeMsgLen) == -1)
		{
			Dlg->m_state = HOST_ERROR;
			return;
		}

		// Receiving player should parse the cipher key out of it.
		//   - decrypting the msg requires the local player's private data
		/////////////////////////////////////////////////////////
		scPeerCipherParseKeyExchangeMsg(&gPlayerData.mCertificate, &gPlayerData.mPrivateData, 
			exchangeMsg, &gPlayerData.mPeerSendCipher);


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
	else if(type == MSG_SESSION_ID_TYPE)
	{
		ASSERT(!Dlg->m_hosting);
		char sessionCrypt[SC_SESSION_GUID_SIZE];
		char connCrypt[SC_CONNECTION_GUID_SIZE];
		int sidLen = SC_SESSION_GUID_SIZE;
		int ccidLen = SC_CONNECTION_GUID_SIZE;


		// Client decodes sessionID / remote connID
		/////////////////////////////////////////////////////////
		if(gtDecode(MSG_SESSION_ID, (char*)message, len, sessionCrypt, &sidLen, connCrypt, &ccidLen) == -1)
		{
			Dlg->m_state = JOIN_ERROR;
			return;
		}

		debugStr.Format("[MSG_SESSION_ID_TYPE] sessionCrypt: %.40s\r\n", sessionCrypt);
		OutputDebugString(debugStr);
		debugStr.Format("[MSG_SESSION_ID_TYPE] connCrypt: %.40s\r\n", connCrypt);
		OutputDebugString(debugStr);

		// Decrypt the sessionID / remote connID
		/////////////////////////////////////////////////////////
		scPeerCipherDecryptBufferIV(&gPlayerData.mPeerRecvCipher, 1, (gsi_u8*)sessionCrypt, SC_SESSION_GUID_SIZE);
		memcpy(gPlayerData.mSessionId, sessionCrypt, SC_SESSION_GUID_SIZE);

		scPeerCipherDecryptBufferIV(&gPlayerData.mPeerRecvCipher, 2, (gsi_u8*)connCrypt, SC_CONNECTION_GUID_SIZE);
		memcpy(Dlg->m_remoteConnId, connCrypt, SC_CONNECTION_GUID_SIZE);


		debugStr.Format("[MSG_SESSION_ID_TYPE] mSessionId: %s\r\n", gPlayerData.mSessionId);
		OutputDebugString(debugStr);
		debugStr.Format("[MSG_SESSION_ID_TYPE] m_remoteConnId: %s\r\n", Dlg->m_remoteConnId);
		OutputDebugString(debugStr);


		// Joining player sets the session ID and the report intention
		/////////////////////////////////////////////////////////
		scSetSessionId(gPlayerData.mStatsInterface, gPlayerData.mSessionId);
		scSetReportIntention(gPlayerData.mStatsInterface, gPlayerData.mConnectionId, SCRACE_AUTHORITATIVE, &gPlayerData.mCertificate, &gPlayerData.mPrivateData,
			setReportIntentionCallback, SCRACE_TIMEOUT_MS, NULL);

		Dlg->m_state = JOIN_SEND_CONNID;
	}
	else if(type == MSG_CONNECTION_ID_TYPE)
	{
		ASSERT(Dlg->m_hosting);
		char connCrypt[SC_CONNECTION_GUID_SIZE];
		int ccidLen = SC_CONNECTION_GUID_SIZE;

		// Hosting player decodes the remote conn ID for use in reporting
		/////////////////////////////////////////////////////////
		if(gtDecode(MSG_CONNECTION_ID, (char*)message, len, connCrypt, &ccidLen) == -1)
		{
			Dlg->m_state = HOST_ERROR;
			return;
		}

		debugStr.Format("[MSG_CONNECTION_ID_TYPE] connCrypt: %.40s\r\n", connCrypt);
		OutputDebugString(debugStr);


		// Decrypt the remote conn ID
		/////////////////////////////////////////////////////////
		scPeerCipherDecryptBufferIV(&gPlayerData.mPeerRecvCipher, 1, (gsi_u8*)connCrypt, SC_CONNECTION_GUID_SIZE);
		memcpy(Dlg->m_remoteConnId, connCrypt, SC_CONNECTION_GUID_SIZE);


		debugStr.Format("[MSG_CONNECTION_ID_TYPE] m_remoteConnId: %s\r\n", Dlg->m_remoteConnId);
		OutputDebugString(debugStr);

		// at this point all of our exchanges are complete - ready for game start
		/////////////////////////////////////////////////////////
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
			{
				Dlg->m_remoteProgress.SetPos(progress);
			}
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
	/////////////////////////////////////////////////////////
	if (reason != GT2LocalClose)
	{
		if (Dlg->m_racing && !Dlg->m_reportSent)
		{
			// If the connection was closed remotely, or connection errors occured
			// we should report stats anyways
			//////////////////////////////////////////////////////////////////////
			Dlg->MessageBox("Connection closed - Sending Broken report");

			Dlg->m_disconnect = gsi_true;
			Dlg->ReportStats();

			// Reset progress bars
			//////////////////////
			Dlg->m_localProgress.SetPos(0); 
			Dlg->m_remoteProgress.SetPos(0);

			// Once stats have been reported, we can logout
			///////////////////////////////////////////////
			Dlg->Logout();
		}
		else
		{
			// Reset progress bars
			//////////////////////			
			Dlg->m_localProgress.SetPos(0);
			Dlg->m_remoteProgress.SetPos(0);
			
			Dlg->MessageBox("Connection closed");
			Dlg->Logout();
		}
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
	Dlg->m_state = HOST_EXCHANGE_CERT; //once connected, exchange certifications
	Dlg->m_remoteNick = nick;
}

void SocketErrorCallback
(
	GT2Socket socket
)
{
	Dlg->MessageBox("Socket Error!");
	Dlg->Logout();
}


BOOL CScRaceSampleDlg::SetupHosting()
{
	int rcode;
	CString str;


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

BOOL CScRaceSampleDlg::SetupJoining()
{
	int rcode;

	// Setup the address to connect to.
	///////////////////////////////////
	CString remoteAddress;
	remoteAddress.Format("%s%s", m_hostOrJoinDlg.m_joinAddress, HOST_PORT_STRING);

	// Encode the profile id.
	/////////////////////////
	char buffer[64];
	rcode = gtEncodeNoType(MSG_CONNECT, buffer, sizeof(buffer), gPlayerData.mProfileId, m_loginDlg.m_nick);
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

BOOL CScRaceSampleDlg::SetupMatch()
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
	m_reportSent = gsi_false;

	do
	{
		// Login the user
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
	str.Format("ScRaceSample%s", m_hosting?" (hosting)":"");
	SetWindowText(str);

	if(m_hosting)
		result = SetupHosting();
	else
		result = SetupJoining();

	if(result && m_hosting)
	{
		m_startRace.EnableWindow();
	}

	return result;
}

BOOL CScRaceSampleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// Basic initialization.
	////////////////////////
	Dlg = this;
	sessionCreated  = gsi_false;
	connIdSet		= gsi_false;
	m_GT2Connection = NULL;
	m_GT2Socket		= NULL;
	m_state			= LOGGED_OUT;
	m_countdown		= 0;
	m_racing		= FALSE;


	// Do Availability Check - Make sure backend is available
	/////////////////////////////////////////////////////////
	GSIACResult aResult = GSIACWaiting; 
	GSIStartAvailableCheck(SCRACE_GAMENAME);
	while(aResult == GSIACWaiting)
	{
		aResult = GSIAvailableCheckThink();
		msleep(5);
	}

	if (aResult == GSIACUnavailable)
	{
		MessageBox("Online service for ScRaceSample is no longer available.");
		return FALSE;
	}

	if (aResult == GSIACTemporarilyUnavailable)
	{
		MessageBox("Online service for ScRaceSample is temporarily down for maintenance.");
		return FALSE;
	}


	// Initialize SDK core object - used for both the AuthService and the Competition SDK
	/////////////////////////////////
	gsCoreInitialize();


	// Initialize the Competition SDK - all users submit a snapshot
	/////////////////////////////////
	if (scInitialize(SCRACE_GAMEID, &m_interface) != SCResult_NO_ERROR)
	{
		MessageBox("Out of memory exception - application failed to initialize.");
		return FALSE;
	}
	gPlayerData.mStatsInterface = m_interface;


	// Set a think timer.
	/////////////////////
	SetTimer(TIMER_THINK, 50, NULL);
	
	return TRUE;
}

void CScRaceSampleDlg::OnPaint() 
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

HCURSOR CScRaceSampleDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CScRaceSampleDlg::OnLogout() 
{
	if (m_racing && !m_reportSent)
	{
		// If we close the connection explicitly, we should report our stats
		//////////////////////////////////////////////////////////////////////
		MessageBox("Logging out - Sending Broken report");

		m_disconnect = gsi_true;
		ReportStats();

		// Reset progress bars
		//////////////////////
		m_localProgress.SetPos(0); 
		m_remoteProgress.SetPos(0);
	}		
	
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

void CScRaceSampleDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	if (m_racing && !m_reportSent)
	{
		// If we attempt to kill the app by hard-killing it or exiting while in
		// the middle of a game, report it as a disconnect
		//////////////////////////////////////////////////////////////////////
		MessageBox("Hard Close - Sending Broken report");

		m_disconnect = gsi_true;
		ReportStats();

		// Reset progress bars
		//////////////////////
		m_localProgress.SetPos(0); 
		m_remoteProgress.SetPos(0);
	}	

	scShutdown(m_interface);
	m_interface = NULL;

	gsCoreShutdown();

	// Wait for core shutdown 
	//   (should be instantaneous unless you have multiple cores)
	while(gsCoreIsShutdown() == GSCore_SHUTDOWN_PENDING)
	{
		gsCoreThink(0);
		msleep(5);
	}

	ghttpCleanup();

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

void CScRaceSampleDlg::OnTimer(UINT nIDEvent) 
{
	CString debugStr;
	
	if(nIDEvent == TIMER_THINK)
	{
		static BOOL thinking;

		if(!thinking)
		{
			thinking = TRUE;

			// Think so SDKs can process
			/////////////////////////////////////////////
			if (m_GT2Socket)
				gt2Think(m_GT2Socket);
			ghttpThink();
			scThink(m_interface);

// ********************************************************** //
// ************************* HOST LOGIC ********************* //
// ********************************************************** //

			if(m_state == HOST_EXCHANGE_CERT)
			{
				char buffer[520];
				char cert[512];
				int rcode;
				gsi_u32 certLen;

				// Store cert in a binary buffer for easy exchange
				/////////////////////////////////////////////
				wsLoginCertWriteBinary(&gPlayerData.mCertificate, cert, sizeof(cert), &certLen);

				// Exchange certificates with the other player to validate (step 1)
				/////////////////////////////////////////////
				rcode = gtEncode(MSG_TO_CLIENT_CERT_TYPE, MSG_TO_CLIENT_CERT, buffer, sizeof(buffer), gPlayerData.mProfileId, cert, certLen);
				ASSERT(rcode != -1);
				gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2True);

				// Wait for a reply
				/////////////////////////////////////////////
				m_state = HOST_WAITING;
			}
			else if(m_state == HOST_VERIFY_CERT)
			{
				// Validate authentication certificates (step 2)
				/////////////////////////////////////////////
				Dlg->m_remoteCertificate.mIsValid = wsLoginCertIsValid(&Dlg->m_remoteCertificate);
				if (gsi_is_false(Dlg->m_remoteCertificate.mIsValid))
				{
					MessageBox("Remote player has an invalid certificate, cancelling game.");
					m_waitingDlg.EndDialog(IDCANCEL);
					Logout();
				}
				else
					m_state = HOST_EXCHANGE_KEYS;
			}
			else if(m_state == HOST_EXCHANGE_KEYS)
			{
				char buffer[512];
				int rcode;
				SCPeerKeyExchangeMsg exchangeMsg;

				// P2P encryption exchange keys (step 3)
				/////////////////////////////////////////////

				// Each player should create a key for receiving data from the remote player
				//     For extra security, we use a different encryption key for each channel
				/////////////////////////////////////////////
				scPeerCipherInit(&gPlayerData.mCertificate, &gPlayerData.mPeerRecvCipher); 
			
				// Create a key exchange message for transmitting the key to the other player
				// using the remote player's certificate to encrypt the cipher
				/////////////////////////////////////////////
				scPeerCipherCreateKeyExchangeMsg(&m_remoteCertificate, &gPlayerData.mPeerRecvCipher, exchangeMsg);

				// Now send the key to the other player
				/////////////////////////////////////////////
				rcode = gtEncode(MSG_TO_CLIENT_KEYS_TYPE, MSG_TO_CLIENT_KEYS, buffer, sizeof(buffer), exchangeMsg, GS_CRYPT_RSA_BYTE_SIZE);
				ASSERT(rcode != -1);
				gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2True);

				// Wait for a reply
				/////////////////////////////////////////////
				m_state = HOST_WAITING;
			}
			else if(m_state == HOST_CONNECTED)
			{
				if (m_waitingDlg.m_hWnd && m_waitingDlg.IsWindowEnabled())
					m_waitingDlg.EndDialog(IDOK);
				m_state = RACING;
			}
			else if(m_state == HOST_SEND_SESSID)
			{
				if(sessionCreated)
				{
					int rcode;
					char buffer[256];
					char sessionCrypt[SC_SESSION_GUID_SIZE];
					char connCrypt[SC_CONNECTION_GUID_SIZE];

					// Encrypt the connID/session ID to send using P2P encryption
					/////////////////////////////////////////////////////////
					memcpy(sessionCrypt, gPlayerData.mSessionId, SC_SESSION_GUID_SIZE);
					scPeerCipherEncryptBufferIV(&gPlayerData.mPeerSendCipher, 1, (gsi_u8*)sessionCrypt, SC_SESSION_GUID_SIZE);
					
					memcpy(connCrypt, gPlayerData.mConnectionId, SC_CONNECTION_GUID_SIZE);
					scPeerCipherEncryptBufferIV(&gPlayerData.mPeerSendCipher, 2, (gsi_u8*)connCrypt, SC_CONNECTION_GUID_SIZE);

					debugStr.Format("[HOST_SEND_SESSID] sessionCrypt: %.40s\r\n", sessionCrypt);
					OutputDebugString(debugStr);
					debugStr.Format("[HOST_SEND_SESSID] connCrypt: %.40s\r\n", connCrypt);
					OutputDebugString(debugStr);

							
					// Now the host sends the session ID & his conn ID to the client
					/////////////////////////////////////////////
					rcode = gtEncode(MSG_SESSION_ID_TYPE, MSG_SESSION_ID, buffer, sizeof(buffer), 
						sessionCrypt, SC_SESSION_GUID_SIZE, connCrypt, SC_CONNECTION_GUID_SIZE);
					ASSERT(rcode != -1);
					gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2True);
						
					
					// Once session is created, set the session ID and report intention
					/////////////////////////////////////////////
					scSetSessionId(m_interface, gPlayerData.mSessionId);
					scSetReportIntention(m_interface, gPlayerData.mConnectionId, SCRACE_AUTHORITATIVE, &gPlayerData.mCertificate, &gPlayerData.mPrivateData,
						setReportIntentionCallback, SCRACE_TIMEOUT_MS, NULL);

					sessionCreated = gsi_false;

					// Go back to connected state
					/////////////////////////////////////////////
					m_state = HOST_CONNECTED;
				}
			}
			else if(m_state == HOST_ERROR)
			{
				MessageBox("Error setting up hosting");
				m_waitingDlg.EndDialog(IDCANCEL);
			}

// ********************************************************** //
// **************** JOIN (CLIENT) LOGIC ********************* //
// ********************************************************** //

			else if(m_state == JOIN_EXCHANGE_CERT)
			{
				// Wait for host to send cert first
				/////////////////////////////////////////////
			}
			else if(m_state == JOIN_VERIFY_CERT)
			{
				// Validate authentication certificates (step 2)
				/////////////////////////////////////////////
				Dlg->m_remoteCertificate.mIsValid = wsLoginCertIsValid(&Dlg->m_remoteCertificate);
				if (gsi_is_false(Dlg->m_remoteCertificate.mIsValid))
				{
					MessageBox("Remote player has an invalid certificate, cancelling game.");
					m_waitingDlg.EndDialog(IDCANCEL);
					Logout();
				}
				else
					m_state = JOIN_EXCHANGE_KEYS;
			}
			else if(m_state == JOIN_EXCHANGE_KEYS)
			{
				// Wait for host to send keys first
				/////////////////////////////////////////////
			}
			else if(m_state == JOIN_CONNECTED)
			{				
				if(m_waitingDlg.m_hWnd && m_waitingDlg.IsWindowEnabled())
					m_waitingDlg.EndDialog(IDOK);
			}
			else if(m_state == JOIN_SEND_CONNID)
			{
				// Once connection ID has been set, relay it to the host
				/////////////////////////////////////////////
				if (connIdSet)
				{
					int rcode;
					char buffer[128];		
					char connCrypt[SC_CONNECTION_GUID_SIZE];

					// Encrypt the connection ID to send using P2P encryption
					/////////////////////////////////////////////////////////
					memcpy(connCrypt, gPlayerData.mConnectionId, SC_CONNECTION_GUID_SIZE);
					scPeerCipherEncryptBufferIV(&gPlayerData.mPeerSendCipher, 1, (gsi_u8*)connCrypt, SC_CONNECTION_GUID_SIZE);

					debugStr.Format("[JOIN_SEND_CONNID] connCrypt: %.40s\r\n", connCrypt);
					OutputDebugString(debugStr);


					// Client needs to send the host his/her connection ID
					/////////////////////////////////////////////
					rcode = gtEncode(MSG_CONNECTION_ID_TYPE, MSG_CONNECTION_ID, buffer, sizeof(buffer), connCrypt, SC_CONNECTION_GUID_SIZE);
					ASSERT(rcode != -1);
					gt2Send(m_GT2Connection, (const unsigned char*)buffer, rcode, GT2True);

					connIdSet = gsi_false;

					// Go back to the connected state
					/////////////////////////////////////////////
					m_state = JOIN_CONNECTED;
				}
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

					// Show the times.
					//////////////////
					CString message;
					if(m_localTime < m_remoteTime)
					{
						message.Format("You won!\n%0.3fs to %0.3fs", m_localTime / 1000.0, m_remoteTime / 1000.0);
						m_win = gsi_true;
					}
					else if(m_remoteTime < m_localTime)
					{
						message.Format("You lost!\n%0.3fs to %0.3fs", m_localTime / 1000.0, m_remoteTime / 1000.0);
					}
					else
					{
						message.Format("You tied!\n%0.3fs", m_localTime / 1000.0);
						m_tie = gsi_true;
					}
					MessageBox(message);


					// Report the stats.
					////////////////////
					if (!m_reportSent)
					{
						m_disconnect = gsi_false;
						ReportStats();
					}

					m_localProgress.SetPos(0);
					m_remoteProgress.SetPos(0);
				}
			}
			else
			{
				char buffer[64];
				int rcode;				
				
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

void CScRaceSampleDlg::Logout()
{
	OnLogout();
}

void CScRaceSampleDlg::Countdown()
{	
	// If report was just recently submitted, reset the submission flag
	///////////////////////////////////////////////////////////////////
	if (m_reportSent)
		m_reportSent = gsi_false;
	
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

void CScRaceSampleDlg::OnStart()
{
	// The countdown here could be thought of as a loading screen.
	// During this loading phase - the Host will create the game session and notify
	// the other players of the session ID. All players will set their report intentions.
	/////////////////////////////////////////////
	if (m_hosting)
	{
		scCreateSession(m_interface, &gPlayerData.mCertificate, &gPlayerData.mPrivateData, 
			createSessionCallback, SCRACE_TIMEOUT_MS, NULL);
		m_state = HOST_SEND_SESSID;
	}

	// Start the countdown.
	///////////////////////
	m_countdown = COUNTDOWN_START;
	SetTimer(TIMER_COUNTDOWN, 1000, NULL);
	Countdown();
}

BOOL CScRaceSampleDlg::PreTranslateMessage(MSG* pMsg) 
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

void CScRaceSampleDlg::StartRace()
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

	// initialize stats markers
	/////////////////////////////////////////////
	m_win = gsi_false;
	m_tie = gsi_false;
	m_disconnect = gsi_false;


	UpdateData();
	m_info.Format("GO!");
	UpdateData(FALSE);
}

void CScRaceSampleDlg::ReportStats()
{
	SCReportPtr     aReport = NULL;
	SCResult        aResult = SCResult_NO_ERROR;
	SCGameResult	myGameResult, opponentGameResult;
	int				myTeam, opponentTeam;
	char			myTeamName[64], opponentTeamName[64];
	int				numPlayers = SCRACE_NUM_PLAYERS;
	int				numTeams = SCRACE_NUM_TEAMS;

	
	// Determine winners and losers
	/////////////////////////////////////////////
	if (!m_disconnect)
	{
		if (m_win)
		{
			myGameResult = SCGameResult_WIN;
			opponentGameResult = SCGameResult_LOSS;
		}
		else if (!m_tie)
		{
			myGameResult = SCGameResult_LOSS;
			opponentGameResult = SCGameResult_WIN;
		} 
		else
		{
			myGameResult = SCGameResult_DRAW;
			opponentGameResult = SCGameResult_DRAW;
		}
	}
	else
	{
		//report disconnected game - don't report any keys
		myGameResult = SCGameResult_DISCONNECT;
		opponentGameResult = SCGameResult_DISCONNECT;
	}


	// Determine teams, and who is on which
	/////////////////////////////////////////////
	if (m_hosting)
	{
		myTeam = SCRACE_HOST_TEAM;
		opponentTeam = SCRACE_CLIENT_TEAM;
	}
	else
	{
		myTeam = SCRACE_CLIENT_TEAM;
		opponentTeam = SCRACE_HOST_TEAM;
	}

	sprintf(myTeamName, "%s's Team", m_loginDlg.m_nick);
	sprintf(opponentTeamName, "%s's Team", m_remoteCertificate.mProfileNick);


	// Create the report and begin building it
	/////////////////////////////////////////////
	aResult = scCreateReport(m_interface, ATLAS_RULE_SET_VERSION, numPlayers, numTeams, &aReport);
	if (aResult != SCResult_NO_ERROR)
	{
		MessageBox("Failed to Create Report - Out of memory");
		return;
	}

	// Non-player data
	/////////////////////////////////////////////
	scReportBeginGlobalData(aReport);
	// no global data reported

	// Player data
	/////////////////////////////////////////////
	scReportBeginPlayerData(aReport);

	// Report your data
	////////////////////
	scReportBeginNewPlayer(aReport);
	scReportSetPlayerData(aReport, 0, gPlayerData.mConnectionId, myTeam, 
		myGameResult, gPlayerData.mProfileId, &gPlayerData.mCertificate, gPlayerData.mStatsAuthdata);
	if (!m_disconnect)
		scReportAddIntValue(aReport, RACE_TIME, m_localTime);

	// Report opponent data
	////////////////////
	scReportBeginNewPlayer(aReport);
	scReportSetPlayerData(aReport, 1, m_remoteConnId, opponentTeam, 
		opponentGameResult, m_remoteProfile, &m_remoteCertificate, gPlayerData.mStatsAuthdata);
	if (!m_disconnect)
		scReportAddIntValue(aReport, RACE_TIME, m_remoteTime);	

	
	// Team data
	/////////////////////////////////////////////
	scReportBeginTeamData(aReport);

	// Report your team data
	////////////////////////
	scReportBeginNewTeam(aReport); 
		scReportSetTeamData(aReport, myTeam, myGameResult);

	// Report opponent team data
	////////////////////////
	scReportBeginNewTeam(aReport);
		scReportSetTeamData(aReport, opponentTeam, opponentGameResult);	
	

	// End the report and set GameStatus
	if (!m_disconnect)
		scReportEnd(aReport, SCRACE_AUTHORITATIVE, SCGameStatus_COMPLETE);
	else
		scReportEnd(aReport, SCRACE_AUTHORITATIVE, SCGameStatus_BROKEN);

	// Submit the Report
	/////////////////////////////////////////////
	if (SCResult_NO_ERROR != scSubmitReport(m_interface, aReport, SCRACE_AUTHORITATIVE, &gPlayerData.mCertificate, 
		&gPlayerData.mPrivateData, submitReportCallback, SCRACE_TIMEOUT_MS, NULL))
	{
		MessageBox("Failed to submit Stats Report - Out of memory");
		return;
	}
	
	// To keep the logic clean, wait for submission to finish and then cleanup the report buffer
	/////////////////////////////////////////////
	waitForScCallbacks(m_interface, 1);

	m_reportSent = gsi_true; //mark that we've submitted a report for this session

	// Cleanup
	/////////////////////////////////////////////
	scDestroyReport(aReport);
	aReport = NULL;
}
