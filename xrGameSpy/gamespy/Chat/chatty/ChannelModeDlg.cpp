// ChannelModeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "chatty.h"
#include "ChannelModeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChannelModeDlg dialog


CChannelModeDlg::CChannelModeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChannelModeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChannelModeDlg)
	m_inviteOnly = FALSE;
	m_limit = 0;
	m_moderated = FALSE;
	m_noExternalMessages = FALSE;
	m_onlyOpsChangeTopic = FALSE;
	m_private = FALSE;
	m_secret = FALSE;
	//}}AFX_DATA_INIT
}


void CChannelModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChannelModeDlg)
	DDX_Check(pDX, IDC_INVITE_ONLY, m_inviteOnly);
	DDX_Text(pDX, IDC_LIMIT, m_limit);
	DDV_MinMaxInt(pDX, m_limit, 0, 9999);
	DDX_Check(pDX, IDC_MODERATED, m_moderated);
	DDX_Check(pDX, IDC_NO_EXTERNAL_MESSAGES, m_noExternalMessages);
	DDX_Check(pDX, IDC_ONLY_OPS_CHANGE_TOPIC, m_onlyOpsChangeTopic);
	DDX_Check(pDX, IDC_PRIVATE, m_private);
	DDX_Check(pDX, IDC_SECRET, m_secret);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChannelModeDlg, CDialog)
	//{{AFX_MSG_MAP(CChannelModeDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChannelModeDlg message handlers
