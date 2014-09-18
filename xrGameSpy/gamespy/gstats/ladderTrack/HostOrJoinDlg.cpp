// HostOrJoinDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ladderTrack.h"
#include "HostOrJoinDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHostOrJoinDlg dialog


CHostOrJoinDlg::CHostOrJoinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHostOrJoinDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHostOrJoinDlg)
	m_joinAddress = _T("");
	m_hostOrJoin = 0;
	//}}AFX_DATA_INIT
}


void CHostOrJoinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHostOrJoinDlg)
	DDX_Text(pDX, IDC_JOIN_ADDRESS, m_joinAddress);
	DDX_Radio(pDX, IDC_HOST, m_hostOrJoin);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHostOrJoinDlg, CDialog)
	//{{AFX_MSG_MAP(CHostOrJoinDlg)
	ON_EN_SETFOCUS(IDC_JOIN_ADDRESS, OnSetfocusJoinAddress)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHostOrJoinDlg message handlers

void CHostOrJoinDlg::OnOK() 
{
	UpdateData();

	// Check for no join address.
	/////////////////////////////
	if((m_hostOrJoin == HOSTORJOIN_JOIN) && m_joinAddress.IsEmpty())
	{
		MessageBox("Please enter the address of the host to connect to");
		return;
	}

	CDialog::OnOK();
}

void CHostOrJoinDlg::OnSetfocusJoinAddress() 
{
	// Make sure its on join.
	/////////////////////////
	UpdateData();
	m_hostOrJoin = HOSTORJOIN_JOIN;
	UpdateData(FALSE);
}
