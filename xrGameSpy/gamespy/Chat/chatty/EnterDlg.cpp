// EnterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "chatty.h"
#include "EnterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnterDlg dialog


CEnterDlg::CEnterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEnterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEnterDlg)
	m_channel = _T("");
	m_password = _T("");
	//}}AFX_DATA_INIT

	m_quickChannel = "";
}


void CEnterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnterDlg)
	DDX_Text(pDX, IDC_CHANNEL, m_channel);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEnterDlg, CDialog)
	//{{AFX_MSG_MAP(CEnterDlg)
	ON_BN_CLICKED(IDC_QUICK1, OnQuick1)
	ON_BN_CLICKED(IDC_QUICK2, OnQuick2)
	ON_BN_CLICKED(IDC_QUICK3, OnQuick3)
	ON_BN_CLICKED(IDC_QUICK4, OnQuick4)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnterDlg message handlers

void CEnterDlg::OnQuick1() 
{
	m_quickChannel = "#istanbul";
	OnOK();
}

void CEnterDlg::OnQuick2() 
{
	m_quickChannel = "#montreal";
	OnOK();
}

void CEnterDlg::OnQuick3() 
{
	m_quickChannel = "#pants-test";
	OnOK();
}

void CEnterDlg::OnQuick4() 
{
	m_quickChannel = "#zurna";
	OnOK();
}
