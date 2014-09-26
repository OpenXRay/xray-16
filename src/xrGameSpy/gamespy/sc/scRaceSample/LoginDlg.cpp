// LoginDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ScRaceSample.h"
#include "LoginDlg.h"
#include "../../common/gsAvailable.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog


CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginDlg)
	m_email = _T("");
	m_nick = _T("");
	m_password = _T("");
	//}}AFX_DATA_INIT
}


void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Text(pDX, IDC_EMAIL, m_email);
	DDX_Text(pDX, IDC_NICK, m_nick);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg message handlers

GPResult checkResult;
int checkProfile;
gsi_char loginResponse[1024];
gsi_bool loginSuccess;
gsi_bool waitForLogin;


void CheckUserCallback(GPConnection * connection, void * _arg, void * param)
{
	GPCheckResponseArg * arg = (GPCheckResponseArg *)_arg;

	checkResult = arg->result;
	checkProfile = arg->profile;
}

void LoginCallback(GHTTPResult httpResult, WSLoginResponse * theResponse, void * theUserData)
{
	CString debugStr;

	if (httpResult != GHTTPSuccess)
	{
		loginSuccess = gsi_false;

		debugStr.Format("[LoginCallback] HTTPError: %d", httpResult);
		OutputDebugString(debugStr);	
		
		sprintf(loginResponse, "HTTP Error: Player Login Failed.");
	}
	else if (theResponse->mLoginResult != WSLogin_Success)
	{
		loginSuccess = gsi_false;

		debugStr.Format("[LoginCallback] Login Result Error: %d", theResponse->mLoginResult);
		OutputDebugString(debugStr);	

		sprintf(loginResponse, "Server Error: Player Login Failed.");
	}
	else
	{
		SamplePlayerData * newPlayer = &gPlayerData;

		loginSuccess = gsi_true;

		
		// Store the profile ID for the player
		//////////////////////////////////////
		newPlayer->mProfileId = theResponse->mCertificate.mProfileId;

		// Store the Login Certificate & Private Data for later use w/ Competition
		//////////////////////////////////////////////////////////////////////////
		memcpy(&newPlayer->mCertificate, &theResponse->mCertificate, sizeof(GSLoginCertificate));
		memcpy(&newPlayer->mPrivateData, &theResponse->mPrivateData, sizeof(GSLoginPrivateData));
	}
	waitForLogin = gsi_true;
}


void CLoginDlg::OnOK() 
{
	GPConnection connection;
	HCURSOR hourglass;
	HCURSOR lastCursor;
	GPResult result;
	int loginResult;


	UpdateData();

	// Check for no account info
	////////////////////////////
	if(m_email.IsEmpty() || m_nick.IsEmpty() || m_password.IsEmpty())
	{
		MessageBox("Please fill in all the account information.");
		return;
	}

	// Initialize GP
	////////////////
	if(gpInitialize(&connection, SCRACE_PRODUCTID, WSLogin_NAMESPACE_SHARED_NONUNIQUE, GP_PARTNERID_GAMESPY) != GP_NO_ERROR)
	{
		MessageBox("Error initializing the login system.");
		return;
	}

	// Wait cursor on
	/////////////////
	hourglass = LoadCursor(NULL, IDC_WAIT);
	if(hourglass)
		lastCursor = SetCursor(hourglass);

	// Check for the account specified
	//////////////////////////////////
	result = gpCheckUser(&connection, m_nick, m_email, m_password, GP_BLOCKING, CheckUserCallback, NULL);

	// Wait cursor off
	//////////////////
	if(hourglass)
		SetCursor(lastCursor);

	// Destroy the GP Object
	////////////////////////
	gpDestroy(&connection);

	// Check for an error
	/////////////////////
	if(result != GP_NO_ERROR)
	{
		MessageBox("Error verifying the account.");
		return;
	}

	// Check the result
	///////////////////
	if(checkResult != GP_NO_ERROR)
	{
		if(checkResult == GP_CHECK_BAD_EMAIL)
			MessageBox("Invalid e-mail.");
		else if(checkResult == GP_CHECK_BAD_NICK)
			MessageBox("Invalid nick.");
		else if(checkResult == GP_CHECK_BAD_PASSWORD)
			MessageBox("Invalid password.");
		else
			MessageBox("Error verifying the account.");
		return;
	}

	// Save the login info for next time
	////////////////////////////////////
	FILE * file;
	file = fopen("login.txt", "wt");
	if(file)
	{
		fprintf(file, "%s\n%s\n%s", m_email, m_nick, m_password);
		fclose(file);
	}

	// Login to the Authentication Service
	//////////////////////////////////////
	loginResult = wsLoginProfile(WSLogin_NAMESPACE_SHARED_NONUNIQUE, WSLogin_PARTNERCODE_GAMESPY, m_nick, m_email, m_password, "", LoginCallback, NULL);
	if (loginResult != WSLogin_Success)
	{
		if (loginResult == WSLogin_InvalidParameters)
			MessageBox("Login Failed - Invalid Parameters."); // this should not be reached
		else if (loginResult == WSLogin_OutOfMemory)
			MessageBox("Login Failed - Out of Memory Exception.");
		else if (loginResult == WSLogin_NoAvailabilityCheck)
			MessageBox("Login Failed - Availability Check Not Performed");
		else
			MessageBox("Login Failed - Unknown Error.");
		return;
	}

	while (!waitForLogin)
	{
		gsCoreThink(5); //process the login and wait for callback to complete
	}

	if (!loginSuccess)
	{
		MessageBox(loginResponse);
		return;
	}

	CDialog::OnOK();
}

BOOL CLoginDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// load login info
	//////////////////
	FILE * file;
	file = fopen("login.txt", "rt");
	if(file)
	{
		char buffer[512];

		if(fgets(buffer, sizeof(buffer), file))
			m_email = buffer;
		if(fgets(buffer, sizeof(buffer), file))
			m_nick = buffer;
		if(fgets(buffer, sizeof(buffer), file))
			m_password = buffer;

		fclose(file);

		m_email.Remove('\n');
		m_nick.Remove('\n');
		m_password.Remove('\n');
	}

	UpdateData(FALSE);

	return TRUE;
}