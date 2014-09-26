// Voice2BuddyMFCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Voice2BuddyMFC.h"
#include "Voice2BuddyMFCDlg.h"

#include "LoginDlg.h"
#include "SetupDlg.h"
#include "VoiceSessionDlg.h"


#include "../../common/gsAvailable.h"
#include "../../voice2/gv.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
enum V2BuddyMessage
{
	V2B_MSG_DOSETUP,
	V2B_MSG_DOLOGIN,
	V2B_MSG_DOVOICECHAT,
};

#define THINK_TIMER_ID            100
#define THINK_TIMER_DELAY         10
#define GAME_NAME                 "gmtest"
#define NAMESPACE_GAMESPY_SHARED  0
#define PRODUCTID_GMTEST          1 // can't use 0!

#define V2B_GP_LOCATION           "Voice2BuddyMFC"
#define V2B_GP_STATUS_IDLE        "Idle"      // free to invite
#define V2B_GP_STATUS_CHATTING    "Chatting"  // actively chatting
#define V2B_GP_INVITE_DECLINED    "Voice invitation declined."
#define V2B_GP_INVITE_ACCEPTED    "Voice invitation accepted."

// Utility to check buddy status and make sure they're running V2B
BOOL IsBuddyUsingV2B(GPConnection* theConnection, GPProfile theProfileId, GPBuddyStatus* theStatus);


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

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
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVoice2BuddyMFCDlg dialog

CVoice2BuddyMFCDlg::CVoice2BuddyMFCDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVoice2BuddyMFCDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVoice2BuddyMFCDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVoice2BuddyMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVoice2BuddyMFCDlg)
	DDX_Control(pDX, ID_SETUP, m_SetupButton);
	DDX_Control(pDX, ID_VOICECHAT, m_VoiceChatButton);
	DDX_Control(pDX, IDC_BUDDYLIST, m_BuddyList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVoice2BuddyMFCDlg, CDialog)
	//{{AFX_MSG_MAP(CVoice2BuddyMFCDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_SETUP, OnSetup)
	ON_BN_CLICKED(ID_VOICECHAT, OnVoiceChat)
	ON_BN_CLICKED(IDCANCEL, OnExit)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_BUDDYLIST, OnSelchangeBuddylist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVoice2BuddyMFCDlg message handlers

BOOL CVoice2BuddyMFCDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// We're not connected yet
	m_GP = NULL;
	m_Initialized   = FALSE;
	m_Connected     = FALSE;
	m_InvitedPlayer = 0;
	m_MyProfileId   = 0;

	// Start network
	SocketStartUp();

	// Init voice SDK (pre-init, do not start devices yet)
	GVBool aResult = gvStartup(m_hWnd);
	if (aResult != GVTrue)
	{
		MessageBox("Failed on gvStartup!");
		PostQuitMessage(0);
	}

	// Set the codec (this sample hard codes it)
	gvSetSampleRate(GVRate_16KHz);
	aResult = gvSetCodec(GVCodecSuperHighQuality);
	if (aResult != GVTrue)
	{
		MessageBox("Failed on gvSetCodec!");
		PostQuitMessage(0);
	}

	// Disable the "voice chat" button until a buddy is selected
	m_VoiceChatButton.EnableWindow(FALSE);

	// Clear the setup info
	memset(&m_SetupInfo, 0, sizeof(m_SetupInfo));

	// Show the setup dialog
	PostMessage(WM_USER+1, V2B_MSG_DOSETUP, 0);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVoice2BuddyMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CVoice2BuddyMFCDlg::OnPaint() 
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
HCURSOR CVoice2BuddyMFCDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CVoice2BuddyMFCDlg::OnSetup() 
{
	// Setup and calibrate the voice hardware
	CSetupDlg aSetupDlg;
	aSetupDlg.m_SetupInfo = &m_SetupInfo;
	aSetupDlg.DoModal();
}

void CVoice2BuddyMFCDlg::OnVoiceChat() 
{
	// Get the selected buddy
	int selIndex = m_BuddyList.GetCurSel();
	if (selIndex == -1)
		return;
	
	// Get the buddy profile
	GPProfile aProfile = m_BuddyList.GetItemData(selIndex);

	// Make sure the buddy is using V2B
	GPBuddyStatus aStatus;
	if (FALSE == IsBuddyUsingV2B(&m_GP, aProfile, &aStatus))
	{
		MessageBox("Buddy is not running V2B.");
		return;
	}
	
	// Make sure this buddy isn't chatting already
	if (strcmp(V2B_GP_STATUS_CHATTING, aStatus.locationString) == 0)
	{
		MessageBox("Buddy is currently involved in a voice chat.");
		return;
	}

	// Invite the buddy to voice chat 
	gpInvitePlayer(&m_GP, aProfile, PRODUCTID_GMTEST, NULL);
	m_InvitedPlayer = aProfile; // remember who we invited
}

void CVoice2BuddyMFCDlg::OnExit() 
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

LRESULT CVoice2BuddyMFCDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message == (WM_USER+1))
	{
		// Process custom message
		switch(wParam)
		{
		case V2B_MSG_DOSETUP:
			{
				// Show the setup dialog
				OnSetup();

				// Pop up the login dialog
				PostMessage(WM_USER+1, V2B_MSG_DOLOGIN, 0);

				// Start the app timer
				SetTimer(THINK_TIMER_ID, THINK_TIMER_DELAY, NULL);
				return 0;
			}
		case V2B_MSG_DOLOGIN:
			{
				// Show the login dialog
				CLoginDlg aDlg;
				int result = aDlg.DoModal();
				if (result == IDCANCEL)
					PostQuitMessage(0);
				else
					DoLogin(aDlg.m_Email, aDlg.m_Nickname, aDlg.m_Password);
				return 0;
			}
		case V2B_MSG_DOVOICECHAT:
			{
				// Create the voice session dialog
				CVoiceSessionDlg aVoiceSessionDlg;
				aVoiceSessionDlg.m_NNCookie  = lParam;
				aVoiceSessionDlg.m_SetupInfo = m_SetupInfo;
				
				// We use the profileId of the host as the NNCookie 
				//    now, check to see if we're the host
				if (m_MyProfileId == aVoiceSessionDlg.m_NNCookie)
					aVoiceSessionDlg.m_IsHost = TRUE;
				else
					aVoiceSessionDlg.m_IsHost = FALSE;
				
				// Run the dialog
				aVoiceSessionDlg.DoModal();
				return 0;
			}
		};
	} // end custom messages
	return CDialog::WindowProc(message, wParam, lParam);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Utility to check if a buddy is using this app
BOOL IsBuddyUsingV2B(GPConnection* theConnection, GPProfile theProfileId, GPBuddyStatus* theStatus)
{
	GPBuddyStatus   aTempStatus;
	GPBuddyStatus*  aStatus;
	if (theStatus != NULL)
		aStatus = theStatus;	// store location in return value
	else
		aStatus = &aTempStatus; // store location in temp

	// get the internal buddy index
	int aBuddyIndex;
	GPResult aResult = gpGetBuddyIndex(theConnection, theProfileId, &aBuddyIndex);
	if (aResult != GP_NO_ERROR)
		return FALSE; // not a buddy
	
	// Get the buddy status
	aResult = gpGetBuddyStatus(theConnection, aBuddyIndex, aStatus);
	if (aResult != GP_NO_ERROR)
		return FALSE; // couldn't get status

	// Is the buddy using V2B?
	if (strcmp(V2B_GP_LOCATION, aStatus->locationString) != 0)
		return FALSE; // no using V2B

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void OnGPError(GPConnection* theConnection, GPErrorArg* theArg, void* theParam)
{
	CVoice2BuddyMFCDlg* aDialog = (CVoice2BuddyMFCDlg*)theParam;

	char buf[1024];
	sprintf(buf, "GP ERROR: %s", theArg->errorString);
	aDialog->MessageBox(buf);

	if (theArg->fatal == GP_FATAL)
		aDialog->m_Connected = FALSE;
	GSI_UNUSED(theConnection);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void OnGPBuddyInfo(GPConnection* theConnection, GPGetInfoResponseArg* theArg, void* theParam)
{
	CVoice2BuddyMFCDlg* aDialog = (CVoice2BuddyMFCDlg*)theParam;

	// Make sure this name is in the list
	int anIndex = aDialog->m_BuddyList.FindString(0, theArg->nick);
	if (anIndex == -1)
	{
		int anIndex = aDialog->m_BuddyList.AddString(theArg->nick);
		if (anIndex != -1)
			aDialog->m_BuddyList.SetItemData(anIndex, theArg->profile);
	}
	
	GSI_UNUSED(theConnection);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void OnGPBuddyStatus(GPConnection* theConnection, GPRecvBuddyStatusArg* theArg, void* theParam)
{
	CVoice2BuddyMFCDlg* aDialog = (CVoice2BuddyMFCDlg*)theParam;
	
	// Retrieve info for the buddy (nickname etc.)
	// will return with file cache information, if available
	gpGetInfo(theConnection, theArg->profile, GP_CHECK_CACHE, GP_NON_BLOCKING, (GPCallback)OnGPBuddyInfo, theParam);
	GSI_UNUSED(aDialog);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void OnGPBuddyMessage(GPConnection* theConnection, GPRecvBuddyMessageArg* theArg, void* theParam)
{
	CVoice2BuddyMFCDlg* aDialog = (CVoice2BuddyMFCDlg*)theParam;

	// Ignore messages that are not from V2B
	if (FALSE == IsBuddyUsingV2B(theConnection, theArg->profile, NULL))
		return;

	// If this wasn't the guy we invited, ignore it
	if (theArg->profile != aDialog->m_InvitedPlayer)
		return;

	// Is this an invite accept?
	if (strcmp(V2B_GP_INVITE_ACCEPTED, theArg->message)==0)
		aDialog->PostMessage(WM_USER+1, V2B_MSG_DOVOICECHAT, aDialog->m_MyProfileId);
		
	// Is this an invite decline?
	if (strcmp(V2B_GP_INVITE_DECLINED, theArg->message)==0)
		aDialog->MessageBox(theArg->message);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void OnGPInvite(GPConnection* theConnection, GPRecvGameInviteArg* theArg, void* theParam)
{
	CVoice2BuddyMFCDlg* aDialog = (CVoice2BuddyMFCDlg*)theParam;

	// Make sure the invite is from V2B
	if (FALSE == IsBuddyUsingV2B(theConnection, theArg->profile, NULL))
		return;

	// Find the nickname of the buddy, for display 
	// (may not be the selected buddy)
	CString aBuddyName = "";
	int anIndex = 0;
	while(anIndex < aDialog->m_BuddyList.GetCount())
	{
		// If this is the buddy, store off the name
		if (aDialog->m_BuddyList.GetItemData(anIndex) == (unsigned int)theArg->profile)
		{
			aDialog->m_BuddyList.GetText(anIndex, aBuddyName);
			break;
		}
		anIndex++;
	}
	if (aBuddyName.IsEmpty())
		return; // buddy not found, ignore the invite

	// Ask the user if they want to accept
	char buf[255];
	sprintf(buf, "%s has invited you to voice chat.  Accept?", (LPCSTR)aBuddyName);
	int aDialogResult = aDialog->MessageBox(buf, "Voice Chat", MB_YESNO);
	if (aDialogResult == IDYES)
	{
		// begin voice session
		gpSendBuddyMessage(&aDialog->m_GP, theArg->profile, V2B_GP_INVITE_ACCEPTED);
		aDialog->PostMessage(WM_USER+1, V2B_MSG_DOVOICECHAT, theArg->profile);
	}
	else
	{
		// send rejection
		gpSendBuddyMessage(&aDialog->m_GP, theArg->profile, V2B_GP_INVITE_DECLINED);
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void OnGPConnect(GPConnection* theConnection, GPConnectResponseArg* theArg, void* theParam)
{
	CVoice2BuddyMFCDlg* aDialog = (CVoice2BuddyMFCDlg*)theParam;

	if (theArg->result == GP_NO_ERROR)
	{
		aDialog->m_Connected = TRUE;
		aDialog->m_SetupButton.EnableWindow(TRUE);
		aDialog->m_MyProfileId = theArg->profile;
	}
	else
	{
		aDialog->MessageBox("Failed to connect!");
		PostQuitMessage(0);
	}
	GSI_UNUSED(theConnection);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CVoice2BuddyMFCDlg::DoLogin(const CString& theEmail, const CString& theNickname, const CString& thePassword)
{
	// Begin the availability check
	GSIACResult aResult = GSIACWaiting;
	GSIStartAvailableCheck(GAME_NAME);
	while(aResult == GSIACWaiting)
	{
		aResult = GSIAvailableCheckThink();
		Sleep(20);
	}

	// Initialize the Presence SDK
	GPResult result = gpInitialize(&m_GP, PRODUCTID_GMTEST, NAMESPACE_GAMESPY_SHARED, GP_PARTNERID_GAMESPY);
	if (result != GP_NO_ERROR)
	{
		MessageBox("Failed on gpInitialize\r\n");
		PostQuitMessage(0);
	}

	gpSetCallback(&m_GP, GP_ERROR,              (GPCallback)OnGPError,       this);
	gpSetCallback(&m_GP, GP_RECV_BUDDY_STATUS,  (GPCallback)OnGPBuddyStatus, this);
	gpSetCallback(&m_GP, GP_RECV_GAME_INVITE,	(GPCallback)OnGPInvite,      this);
	gpSetCallback(&m_GP, GP_RECV_BUDDY_MESSAGE, (GPCallback)OnGPBuddyMessage,this);
	m_Initialized = TRUE;
	
	// Connect to the presence server (goes to callback even though it's blocking)
	gpConnect(&m_GP, theNickname, theEmail, thePassword, GP_NO_FIREWALL, GP_BLOCKING, (GPCallback)OnGPConnect, this);

	// Set the think timer if we connected
	// (m_Connected is set from the callback function above)
	if (m_Connected)
		SetTimer(THINK_TIMER_ID, THINK_TIMER_DELAY, NULL);

	// Set our location
	gpSetStatus(&m_GP, GP_ONLINE, V2B_GP_STATUS_IDLE, V2B_GP_LOCATION);
}

void CVoice2BuddyMFCDlg::OnTimer(UINT nIDEvent) 
{
	// Win32 timers can be called concurrently by the OS.
	//    (you can get a second timer callback before the first finishes!)
	// CRITICAL_SECTION will not prevent this as Win32 allows concurrent access
	static bool inTimer = false;
	if (inTimer)
		return;
	
	// Prevent windows from entering the timer again
	inTimer = true;

	if (m_Connected)
		gpProcess(&m_GP);

	// Allow windows to enter the timer again
	inTimer = false;

	CDialog::OnTimer(nIDEvent);
}

void CVoice2BuddyMFCDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// Kill the think timer
	KillTimer(THINK_TIMER_ID);

	// Disconnect from GP
	if (m_Connected)
		gpDisconnect(&m_GP);

	// Destruct GP
	if (m_Initialized)
		gpDestroy(&m_GP);

	gvCleanup();
}

void CVoice2BuddyMFCDlg::OnSelchangeBuddylist() 
{
	int aSel = m_BuddyList.GetCurSel();
	if (aSel == -1)
		m_VoiceChatButton.EnableWindow(FALSE);
	else
		m_VoiceChatButton.EnableWindow(TRUE);
}
