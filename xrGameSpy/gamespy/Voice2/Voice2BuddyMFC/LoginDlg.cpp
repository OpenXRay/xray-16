// LoginDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Voice2BuddyMFC.h"
#include "LoginDlg.h"

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
	m_Email = _T("");
	m_Nickname = _T("");
	m_Password = _T("");
	//}}AFX_DATA_INIT
}


void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Text(pDX, IDC_EMAIL, m_Email);
	DDX_Text(pDX, IDC_NICKNAME, m_Nickname);
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg message handlers

void CLoginDlg::OnOK() 
{
	// Retrieve settings from the view
	UpdateData(TRUE);

	// Save settings to the registry
	HKEY aRegKey = NULL;
	LONG aResult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\GameSpy\\Voice2BuddyMFC", 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &aRegKey, NULL);
	if (aResult == ERROR_SUCCESS)
	{
		RegSetValue(aRegKey, "LastLoginNickname", REG_SZ, m_Nickname, m_Nickname.GetLength());
		RegSetValue(aRegKey, "LastLoginEmail",    REG_SZ, m_Email,    m_Email.GetLength());
		RegCloseKey(aRegKey);
	}

	
	CDialog::OnOK();
}

void CLoginDlg::OnCancel() 
{
	
	CDialog::OnCancel();
}

BOOL CLoginDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Load settings from the registry
	char aStringBuf[255];
	LONG aLongBuf;

	HKEY aRegKey = NULL;
	LONG aResult = 	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\GameSpy\\Voice2BuddyMFC", 0, NULL, 
                                   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &aRegKey, NULL);
	if (aResult == ERROR_SUCCESS)
	{
		// Load last session username
		aLongBuf = 255;
		aResult = RegQueryValue(aRegKey, "LastLoginNickname", aStringBuf, &aLongBuf);
		if (aResult == ERROR_SUCCESS)
			m_Nickname = aStringBuf;

		// Load last session email
		aLongBuf = 255;
		aResult = RegQueryValue(aRegKey, "LastLoginEmail", aStringBuf, &aLongBuf);
		if (aResult == ERROR_SUCCESS)
			m_Email = aStringBuf;

		RegCloseKey(aRegKey);
	}

	// Reflect new settings into the view
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
