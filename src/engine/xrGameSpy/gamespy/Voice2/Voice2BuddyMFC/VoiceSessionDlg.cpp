// VoiceSessionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Voice2BuddyMFC.h"
#include "VoiceSessionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define THINK_TIMER_ID      100+2
#define THINK_TIMER_DELAY   20


/////////////////////////////////////////////////////////////////////////////
// CVoiceSessionDlg dialog
struct VoicePacket
{
	GVFrameStamp  mFrameStamp;
	int           mDataLength;	
	GVByte        mBuffer[1024];  // most likely get < 256 bytes
};
const int gVoicePacketHeaderSize = sizeof(GVFrameStamp)+sizeof(int);


CVoiceSessionDlg::CVoiceSessionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVoiceSessionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVoiceSessionDlg)
	m_DisplayText = _T("");
	//}}AFX_DATA_INIT
}


void CVoiceSessionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVoiceSessionDlg)
	DDX_Control(pDX, IDC_SPEAKING2, m_RemoteSpeaking);
	DDX_Control(pDX, IDC_SPEAKING1, m_LocalSpeaking);
	DDX_Text(pDX, IDC_DISPLAYTEXT, m_DisplayText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVoiceSessionDlg, CDialog)
	//{{AFX_MSG_MAP(CVoiceSessionDlg)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVoiceSessionDlg message handlers
void CVoiceSessionDlg::OnTimer(UINT nIDEvent) 
{
	// Win32 timers can be called concurrently by the OS.
	//    (you can get a second timer callback before the first finishes!)
	// CRITICAL_SECTION will not prevent this as Win32 allows concurrent access
	static bool inTimer = false;
	if (inTimer)
		return;
	
	// Prevent windows from entering the timer again
	inTimer = true;

	// Process voice data
	gvThink();

	// Process NN
	NNThink();

	// Process network traffic
	if (m_Socket)
		gt2Think(m_Socket);

	// If we're not connected we don't need to do the rest
	if (!m_Connection || (gt2GetConnectionState(m_Connection) != GT2Connected))
	{
		inTimer = false;
		return;
	}

	// Show a bitmap if the sources are speaking
	static bool wasLocalSpeaking = false;
	static bool wasRemoteSpeaking = false;

	bool isLocalSpeaking  = (GVTrue == gvIsSourceTalking(m_SetupInfo.m_PlaybackDevice, 0));
	bool isRemoteSpeaking = (GVTrue == gvIsSourceTalking(m_SetupInfo.m_PlaybackDevice, m_RemoteAddr));
	
	if (isLocalSpeaking != wasLocalSpeaking)
	{
		wasLocalSpeaking = isLocalSpeaking;
		m_LocalSpeaking.ShowWindow(isLocalSpeaking ? SW_SHOW:SW_HIDE);
	}
	if (isRemoteSpeaking != wasRemoteSpeaking)
	{
		wasRemoteSpeaking = isRemoteSpeaking;
		m_RemoteSpeaking.ShowWindow(isRemoteSpeaking ? SW_SHOW:SW_HIDE);
	}

	// Check for microphone activity
	int aBytesAvailable = gvGetAvailableCaptureBytes(m_SetupInfo.m_CaptureDevice);
	if (aBytesAvailable > 0)
	{
		// Voice packet struct, so we don't have to do a buffer copy
		// We just record the data directly into the packet
		// (the actual packet may be smaller

		VoicePacket aVoicePacket;
		memset(&aVoicePacket, 0, sizeof(aVoicePacket));
		aVoicePacket.mDataLength = sizeof(aVoicePacket.mBuffer);

		GVScalar    aVolume; // this doesn't need to be sent

		// Read in a voice packet
		GVBool gotPacket = gvCapturePacket(m_SetupInfo.m_CaptureDevice, aVoicePacket.mBuffer, &aVoicePacket.mDataLength, &aVoicePacket.mFrameStamp, &aVolume);
		if (gotPacket == GVTrue)
		{
			// Playback for local echo
			//gvPlayPacket(m_SetupInfo.m_PlaybackDevice, aVoicePacket.mBuffer, aVoicePacket.mDataLength, 0, aVoicePacket.mFrameStamp);

			// Contruct a message for the buddy
			int aTotalPacketSize = gVoicePacketHeaderSize + aVoicePacket.mDataLength;
			gt2Send(m_Connection, (GT2Byte*)&aVoicePacket, aTotalPacketSize, GT2False);
		}
	}

	// Allow new timers to be triggered
	inTimer = false;
	
	CDialog::OnTimer(nIDEvent);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Callbacks for GT2
void GT2SocketErrorCallback(GT2Socket theSocket)
{
	CVoiceSessionDlg* aDialog = (CVoiceSessionDlg*)gt2GetSocketData(theSocket);
	aDialog->m_DisplayText = "GT2SocketErrorCallback.  Connection Closed";
	aDialog->UpdateData(FALSE);
}

void GT2ConnectedCallback(GT2Connection theConnection, GT2Result theResult, unsigned char* theMessage, int theLength)
{
	GT2Socket aSocket = gt2GetConnectionSocket(theConnection);
	CVoiceSessionDlg* aDialog = (CVoiceSessionDlg*)gt2GetSocketData(aSocket);

	// Was there an error?
	if (theResult != GT2Success)
	{
		aDialog->MessageBox("GT2ConnectedCallback returned failure!");
		aDialog->PostMessage(WM_CLOSE, 0, 0);
	}
	else
	{
		// Remember our connection
		aDialog->m_Connection = theConnection;
		aDialog->m_DisplayText = "Session established.\r\n(You may begin speaking)";
		aDialog->UpdateData(FALSE);

		// Start the voice devices
		if (aDialog->m_SetupInfo.m_CaptureDevice != NULL)
			gvStartDevice(aDialog->m_SetupInfo.m_CaptureDevice, GV_CAPTURE);
		if (aDialog->m_SetupInfo.m_PlaybackDevice != NULL)
			gvStartDevice(aDialog->m_SetupInfo.m_PlaybackDevice, GV_PLAYBACK);
	}
	GSI_UNUSED(theMessage);
	GSI_UNUSED(theLength);
	GSI_UNUSED(theResult);
}

void GT2ReceivedCallback(GT2Connection theConnection, unsigned char* theBuffer, int theLength, int wasReliable)
{
	// Get the ptr to the dialog
	CVoiceSessionDlg* aDlg = (CVoiceSessionDlg*)gt2GetSocketData(gt2GetConnectionSocket(theConnection));

	// Cast to a voice packet so we can extract data
	VoicePacket* aVoicePacket = (VoicePacket*)theBuffer;

	// Sanity check the packet
	assert(theLength >= gVoicePacketHeaderSize);
	assert(theLength == (gVoicePacketHeaderSize + aVoicePacket->mDataLength));

	// Play the voice data
	GVSource aSource = gt2GetRemoteIP(theConnection);
	gvPlayPacket(aDlg->m_SetupInfo.m_PlaybackDevice, aVoicePacket->mBuffer, aVoicePacket->mDataLength, aSource, aVoicePacket->mFrameStamp, 0);
	OutputDebugString("Playing voice packet\r\n");
	GSI_UNUSED(wasReliable);
	GSI_UNUSED(theLength);
}

GT2Bool GT2UnrecognizedMessageCallback(GT2Socket theSocket, unsigned int theIp, unsigned short thePort, GT2Byte* theData, int theLength)
{
	static unsigned char aNNHeader[NATNEG_MAGIC_LEN] = 
				{ NN_MAGIC_0, NN_MAGIC_1, NN_MAGIC_2,
				NN_MAGIC_3, NN_MAGIC_4, NN_MAGIC_5 };

	// Bail out if it's too short
	if (theLength < NATNEG_MAGIC_LEN)
	{
		OutputDebugString("\tDiscarded message (too small)\r\n");
		return GT2False;
	}

	// Check if it matches the NN Magic bytes
	if (0==memcmp(theData, aNNHeader,NATNEG_MAGIC_LEN))
	{
		// Hand off to the NN SDK
		sockaddr_in anAddr;
		anAddr.sin_family      = AF_INET;
		anAddr.sin_port        = htons(thePort);
		anAddr.sin_addr.s_addr = theIp;
		NNProcessData((char*)theData, theLength, &anAddr);
		OutputDebugString("\tProcessed NN message\r\n");
		return GT2True;
	}

	// Not handled by us
	OutputDebugString("\tDiscarded message (not recognized)\r\n");
	GSI_UNUSED(theSocket);
	return GT2False;
}

void GT2ClosedCallback(GT2Connection theConnection, GT2CloseReason theReason)
{
	GT2Socket aSocket = gt2GetConnectionSocket(theConnection);
	CVoiceSessionDlg* aDialog = (CVoiceSessionDlg*)gt2GetSocketData(aSocket);

	aDialog->m_DisplayText = "Connection closed. (GT2ClosedCallback)";
	aDialog->UpdateData(FALSE);
	GSI_UNUSED(theReason);
}

void GT2ConnectAttemptCallback(GT2Socket theSocket,  GT2Connection theConnection,  unsigned int theIp,  unsigned short thePort,  int theLatency,  GT2Byte * theMessage,  int theLength)
{
	CVoiceSessionDlg* aDialog = (CVoiceSessionDlg*)gt2GetSocketData(theSocket);

	// Set the callbacks so we can receive data
	GT2ConnectionCallbacks aCallbackList;
	aCallbackList.received  = GT2ReceivedCallback;
	aCallbackList.connected = GT2ConnectedCallback;  // we're connected
	aCallbackList.closed    = GT2ClosedCallback;

	// Accept the connection
	aDialog->m_Connection = theConnection;
	gt2Accept(theConnection, &aCallbackList);

	// Store off the remote addr
	aDialog->m_RemoteAddr = gt2GetRemoteIP(theConnection);

	// Start the voice devices
	if (aDialog->m_SetupInfo.m_CaptureDevice != NULL)
		gvStartDevice(aDialog->m_SetupInfo.m_CaptureDevice, GV_CAPTURE);
	if (aDialog->m_SetupInfo.m_PlaybackDevice != NULL)
		gvStartDevice(aDialog->m_SetupInfo.m_PlaybackDevice, GV_PLAYBACK);

	aDialog->m_DisplayText = "Session established.\r\n(You may begin speaking)";
	aDialog->UpdateData(FALSE);
	GSI_UNUSED(theMessage);
	GSI_UNUSED(theLength);
	GSI_UNUSED(theIp);
	GSI_UNUSED(thePort);
	GSI_UNUSED(theLatency);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Callbacks for the NN SDK
void NNProgressCallback(NegotiateState theState, void* theParam)
{
	CVoiceSessionDlg* aDialog = (CVoiceSessionDlg*)theParam;
	aDialog->m_DisplayText.Format("NNProgressCallback: %d\r\n", theState);
	aDialog->UpdateData(FALSE);
}

void NNCompletedCallback(NegotiateResult theResult, SOCKET theSocket, sockaddr_in* theAddr, void* theParam)
{
	CVoiceSessionDlg* aDialog = (CVoiceSessionDlg*)theParam;
	
	// Check the result, are we connected at the socket layer?
	if (theResult == nr_success)
	{
		// Convert sockaddr_in to a string
		CString aAddrString;
		aAddrString.Format("%s:%d", inet_ntoa(theAddr->sin_addr), ntohs(theAddr->sin_port));
		aDialog->m_DisplayText = "NN Successful";

		// If hosting, listen for the client...
		if (aDialog->m_IsHost)
		{
			gt2Listen(aDialog->m_Socket, GT2ConnectAttemptCallback);
			aDialog->m_DisplayText = "Listening for GT2 connection";
		}
		// ...otherwise connect to the host
		else
		{
			// Store off the remote addr
			aDialog->m_RemoteAddr = theAddr->sin_addr.s_addr;

			// Set our gt2 callbacks, so we can receive messages
			GT2ConnectionCallbacks aCallbackList;
			aCallbackList.connected = GT2ConnectedCallback;  // we're connected
			aCallbackList.received  = GT2ReceivedCallback;   // we've received data
			aCallbackList.closed    = GT2ClosedCallback;

			int aTimeout = 0;
			GT2Result aResult = gt2Connect(aDialog->m_Socket, &aDialog->m_Connection, aAddrString, NULL, 0, aTimeout, &aCallbackList, GT2False);
			if (aResult != GT2Success)
			{
				// Error, bail out
				aDialog->m_DisplayText = "Failed on gt2Connect";
				//aDialog->PostMessage(WM_CLOSE, 0, 0);
			}
			aDialog->m_DisplayText = "Connecting with GT2";
		}
	}
	else
	{
		aDialog->m_DisplayText = "Failed to negotiate a connection.";
	}

	// Draw the new display text
	aDialog->UpdateData(FALSE);
	GSI_UNUSED(theSocket);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MFC Event handlers
BOOL CVoiceSessionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Init some variables
	m_Socket     = NULL;
	m_Connection = NULL;

	// Set the gt2Callbacks
	// Create our gt2 socket (we'll need it for NN)
	//   We use NULL for the local addr because we don't care which
	//   port we're bound to.  We could specify one if we wanted by
	//   using ":5000" as the addr, for example
	GT2Result aResult = gt2CreateSocket(&m_Socket, NULL, 0, 0, GT2SocketErrorCallback);
	if (aResult != GT2Success)
	{
		// We're hosed, bail out by closing the dialog
		MessageBox("Failed on gt2CreateSocket");
		PostMessage(WM_CLOSE, 0, 0);
		return TRUE;
	}

	// Set the socket data (the user param) to our dialog
	gt2SetSocketData(m_Socket, this);
	gt2SetUnrecognizedMessageCallback(m_Socket, GT2UnrecognizedMessageCallback);

	// Begin nat negotiation (use the gt2 socket)
	//    m_NNCookie and m_IsHost are set before the dialog is run
	SOCKET aWinSocket = gt2GetSocketSOCKET(m_Socket);
	NNBeginNegotiationWithSocket(aWinSocket, m_NNCookie, m_IsHost, NNProgressCallback, NNCompletedCallback, this);

	m_DisplayText = "Establishing connection...";
	UpdateData(FALSE);

	// Set the think timer
	SetTimer(THINK_TIMER_ID, THINK_TIMER_DELAY, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CVoiceSessionDlg::DestroyWindow() 
{
	// Clean up NN SDK
	NNFreeNegotiateList();

	// Close the gt2 socket (will close the connection if connected)
	if (m_Socket)
		gt2CloseSocket(m_Socket);

	// Stop the voice devices
	if (m_SetupInfo.m_CaptureDevice != NULL)
		gvStopDevice(m_SetupInfo.m_CaptureDevice, GV_CAPTURE);
	if (m_SetupInfo.m_PlaybackDevice != NULL)
		gvStopDevice(m_SetupInfo.m_PlaybackDevice, GV_PLAYBACK);

	// Kill the timer
	KillTimer(THINK_TIMER_ID);


	return CDialog::DestroyWindow();
}
