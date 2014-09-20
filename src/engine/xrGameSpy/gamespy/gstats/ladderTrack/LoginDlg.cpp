// LoginDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ladderTrack.h"
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

void CheckUserCallback(GPConnection * connection, void * _arg, void * param)
{
	GPCheckResponseArg * arg = (GPCheckResponseArg *)_arg;

	checkResult = arg->result;
	checkProfile = arg->profile;
}

void CLoginDlg::OnOK() 
{
	GPConnection connection;
	HCURSOR hourglass;
	HCURSOR lastCursor;
	GPResult result;

	UpdateData();

	// CHECK FOR NO ACCOUNT INFO
	////////////////////////////
	if(m_email.IsEmpty() || m_nick.IsEmpty() || m_password.IsEmpty())
	{
		MessageBox("Please fill in all the account information.");
		return;
	}

	// Make sure the backend is available
	GSIACResult aResult = GSIACWaiting; 
	GSIStartAvailableCheck(gcd_gamename);
	while(aResult == GSIACWaiting)
	{
		aResult = GSIAvailableCheckThink();
		msleep(5);
	}

	if (aResult == GSIACUnavailable)
	{
		MessageBox("GameSpy backend services are not available.");
		return;
	}

	// INITIALIZE GP
	////////////////
	if(gpInitialize(&connection, 535, 0, GP_PARTNERID_GAMESPY) != GP_NO_ERROR)
	{
		MessageBox("Error initializing the login system.");
		return;
	}

	// wait cursor on
	/////////////////
	hourglass = LoadCursor(NULL, IDC_WAIT);
	if(hourglass)
		lastCursor = SetCursor(hourglass);

	// CHECK FOR THE ACCOUNT SPECIFIED
	//////////////////////////////////
	result = gpCheckUser(&connection, m_nick, m_email, m_password, GP_BLOCKING, CheckUserCallback, NULL);

	// wait cursor off
	//////////////////
	if(hourglass)
		SetCursor(lastCursor);

	// DESTROY THE GP OBJECT
	////////////////////////
	gpDestroy(&connection);

	// CHECK FOR AN ERROR
	/////////////////////
	if(result != GP_NO_ERROR)
	{
		MessageBox("Error verifying the account.");
		return;
	}

	// CHECK THE RESULT
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

	// save the login info for next time
	////////////////////////////////////
	FILE * file;
	file = fopen("login.txt", "wt");
	if(file)
	{
		fprintf(file, "%s\n%s\n%s", m_email, m_nick, m_password);
		fclose(file);
	}

	// STORE THE PROFILE ID
	///////////////////////
	m_profile = checkProfile;

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