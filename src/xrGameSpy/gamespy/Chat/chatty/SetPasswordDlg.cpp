// SetPasswordDlg.cpp : implementation file
//

#include "stdafx.h"
#include "chatty.h"
#include "SetPasswordDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetPasswordDlg dialog


CSetPasswordDlg::CSetPasswordDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSetPasswordDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSetPasswordDlg)
	m_password = _T("");
	m_enable = FALSE;
	//}}AFX_DATA_INIT
}


void CSetPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetPasswordDlg)
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDX_Check(pDX, IDC_ENABLE, m_enable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetPasswordDlg, CDialog)
	//{{AFX_MSG_MAP(CSetPasswordDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetPasswordDlg message handlers
