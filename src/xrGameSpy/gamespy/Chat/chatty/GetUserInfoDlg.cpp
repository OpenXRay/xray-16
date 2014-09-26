// GetUserInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "chatty.h"
#include "GetUserInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGetUserInfoDlg dialog


CGetUserInfoDlg::CGetUserInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGetUserInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGetUserInfoDlg)
	m_address = _T("");
	m_name = _T("");
	m_user = _T("");
	m_nick = _T("");
	//}}AFX_DATA_INIT
}


void CGetUserInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetUserInfoDlg)
	DDX_Control(pDX, IDC_CHANNELS, m_channels);
	DDX_Text(pDX, IDC_ADDRESS, m_address);
	DDX_Text(pDX, IDC_NAME, m_name);
	DDX_Text(pDX, IDC_USER, m_user);
	DDX_Text(pDX, IDC_NICK, m_nick);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetUserInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CGetUserInfoDlg)
	ON_BN_CLICKED(ID_GET_INFO, OnGetInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetUserInfoDlg message handlers

void GetUserInfoCallback(CHAT chat, CHATBool success, const char * nick, const char * user, const char * name, const char * address, int numChannels, const char ** channels, void * param)
{
	CGetUserInfoDlg * dlg = (CGetUserInfoDlg *)param;
	
	if(success)
	{
		dlg->m_user = user;
		dlg->m_name = name;
		dlg->m_address = address;
		dlg->UpdateData(FALSE);
		dlg->m_channels.ResetContent();
		for(int i = 0 ; i < numChannels ; i++)
			dlg->m_channels.AddString(channels[i]);
	}

	dlg->MessageBox("done");
   
	GSI_UNUSED(nick);
	GSI_UNUSED(chat);
}

void CGetUserInfoDlg::OnGetInfo() 
{
	if(theApp.m_chat != NULL)
	{
		UpdateData();
		chatGetUserInfo(theApp.m_chat, m_nick, GetUserInfoCallback, this, CHATTrue);
	}
}
